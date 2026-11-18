#include "filters.hpp"

#include <gtest/gtest.h>
#include <zlib.h>

#include <format>
#include <string>
#include <string_view>

using walk_pdf::decode;
using walk_pdf::Filter;
using namespace std::string_view_literals;

namespace {

std::string deflate(std::string_view text) {
  auto size = compressBound(static_cast<uLong>(text.size()));
  std::string result(size, '\0');
  compress(reinterpret_cast<Bytef*>(result.data()), &size, reinterpret_cast<Bytef const*>(text.data()),
           static_cast<uLong>(text.size()));
  result.resize(size);
  return result;
}

Filter flate(int predictor = 1, int columns = 1, int colors = 1) {
  return Filter{"FlateDecode", predictor, colors, 8, columns};
}

} // namespace

TEST(Filters, NoFiltersKeepsTheData) {
  auto const result = decode("BT (Hi) Tj ET", {});
  EXPECT_EQ(result.bytes, "BT (Hi) Tj ET");
  EXPECT_FALSE(result.problem);
}

TEST(Filters, Flate) {
  std::string const page = "BT /F1 12 Tf 72 712 Td (Hello) Tj ET\n";
  auto const result = decode(deflate(page), {flate()});
  EXPECT_EQ(result.bytes, page);
  EXPECT_FALSE(result.problem);
}

TEST(Filters, FlateLargerThanOneChunk) {
  std::string page;
  for (int i = 0; i < 20000; ++i) {
    page += "BT (" + std::to_string(i) + ") Tj ET\n";
  }
  EXPECT_EQ(decode(deflate(page), {flate()}).bytes, page);
}

TEST(Filters, FlateCutOffKeepsWhatWasDecoded) {
  std::string page(10000, 'x');
  for (std::size_t i = 0; i < page.size(); i += 7) {
    page[i] = static_cast<char>('a' + i % 26);
  }
  auto compressed = deflate(page);
  compressed.resize(compressed.size() / 2);
  auto const result = decode(compressed, {flate()});
  ASSERT_TRUE(result.problem);
  EXPECT_TRUE(result.problem->starts_with("FlateDecode: data ends early")) << *result.problem;
  EXPECT_TRUE(page.starts_with(result.bytes));
}

TEST(Filters, FlateDamaged) {
  auto const result = decode("this is not zlib data", {flate()});
  ASSERT_TRUE(result.problem);
  EXPECT_TRUE(result.problem->starts_with("FlateDecode: damaged")) << *result.problem;
}

TEST(Filters, PngPredictors) {
  // Rows of 3 bytes, each starting with its PNG filter type, as in xref streams
  // (/Predictor 12 /Columns 3). The decoded rows are 01 02 03, 04 05 06, ...
  std::string const predicted{
      "\x00\x01\x02\x03"   // None:    01 02 03
      "\x02\x03\x03\x03"   // Up:      04 05 06 (each up + 03)
      "\x01\x07\x01\x01"   // Sub:     07 08 09 (each left + 01)
      "\x03\x07\x02\x02"   // Average: 0A 0B 0C (each (left + up) / 2 + raw)
      "\x04\x03\x01\x01"sv}; // Paeth:   0D 0E 0F
  auto const result = decode(deflate(predicted), {flate(12, 3)});
  EXPECT_FALSE(result.problem);
  EXPECT_EQ(result.bytes, "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"sv);
}

TEST(Filters, TiffPredictor) {
  auto const result = decode(deflate("\x01\x01\x01\x05\x01\x01"sv), {flate(2, 3)});
  EXPECT_EQ(result.bytes, "\x01\x02\x03\x05\x06\x07"sv);
}

TEST(Filters, AsciiHex) {
  EXPECT_EQ(decode("48 65 6C\n6c 6F>", {Filter{"ASCIIHexDecode"}}).bytes, "Hello");
  EXPECT_EQ(decode("901FA>", {Filter{"AHx"}}).bytes, "\x90\x1F\xA0");
  EXPECT_TRUE(decode("4G>", {Filter{"ASCIIHexDecode"}}).problem);
}

TEST(Filters, Ascii85) {
  // Expected values from Python's base64.a85encode
  EXPECT_EQ(decode("9jqo^~>", {Filter{"ASCII85Decode"}}).bytes, "Man ");
  EXPECT_EQ(decode("9jn~>", {Filter{"ASCII85Decode"}}).bytes, "Ma");
  EXPECT_EQ(decode("87cURD_*#TDfTZ)+T~>", {Filter{"A85"}}).bytes, "Hello, world!");
  EXPECT_EQ(decode("z~>", {Filter{"ASCII85Decode"}}).bytes, std::string(4, '\0'));
  EXPECT_EQ(decode("9jq o^\n~>", {Filter{"ASCII85Decode"}}).bytes, "Man ");
}

TEST(Filters, InOrder) {
  // ASCIIHex of Flate: hex digits first, then inflate
  std::string hex;
  for (auto const c : deflate("BT (Hi) Tj ET")) {
    hex += std::format("{:02X}", static_cast<unsigned char>(c));
  }
  auto const result = decode(hex + ">", {Filter{"ASCIIHexDecode"}, flate()});
  EXPECT_EQ(result.bytes, "BT (Hi) Tj ET");
}

TEST(Filters, ImagesAreNotDecoded) {
  EXPECT_FALSE(walk_pdf::can_decode("DCTDecode"));
  EXPECT_TRUE(walk_pdf::can_decode("FlateDecode"));
  auto const result = decode("jpeg bytes", {Filter{"DCTDecode"}});
  EXPECT_EQ(result.problem, "DCTDecode: not decoded");
}
