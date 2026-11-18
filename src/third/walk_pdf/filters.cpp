#include "filters.hpp"

#include <zlib.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <format>

namespace walk_pdf {

namespace {

using Result = std::expected<std::string, std::string>;

// zlib's inflate. A damaged or cut-off stream keeps what was decoded before
// the damage, and says so.
std::pair<std::string, std::optional<std::string>> flate(std::string_view data) {
  z_stream stream{};
  if (inflateInit(&stream) != Z_OK) {
    return {{}, "zlib could not start"};
  }
  stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
  stream.avail_in = static_cast<uInt>(data.size());
  std::string result;
  std::array<char, 65536> chunk{};
  int status = Z_OK;
  while (status == Z_OK) {
    stream.next_out = reinterpret_cast<Bytef*>(chunk.data());
    stream.avail_out = static_cast<uInt>(chunk.size());
    status = inflate(&stream, Z_NO_FLUSH);
    result.append(chunk.data(), chunk.size() - stream.avail_out);
    if (status == Z_BUF_ERROR && stream.avail_in == 0) {
      break; // The data ended before the end of the compressed stream
    }
  }
  inflateEnd(&stream);
  if (status == Z_STREAM_END) {
    return {result, std::nullopt};
  }
  if (status == Z_BUF_ERROR) {
    return {result, std::format("FlateDecode: data ends early, after {} bytes decoded", result.size())};
  }
  return {result, std::format("FlateDecode: damaged after {} bytes decoded ({})", result.size(),
                              stream.msg != nullptr ? stream.msg : "zlib error")};
}

std::optional<int> hex_value(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return std::nullopt;
}

bool is_space(char c) {
  return c == '\0' || c == '\t' || c == '\n' || c == '\f' || c == '\r' || c == ' ';
}

// "48 65 6C>" is "Hel"; an odd last digit is followed by 0
Result ascii_hex(std::string_view data) {
  std::string result;
  std::optional<int> high;
  for (auto const c : data) {
    if (c == '>') {
      break;
    }
    if (is_space(c)) {
      continue;
    }
    auto const value = hex_value(c);
    if (!value) {
      return std::unexpected(std::format("ASCIIHexDecode: '{}' is not a hex digit", c));
    }
    if (high) {
      result += static_cast<char>(*high * 16 + *value);
      high.reset();
    } else {
      high = value;
    }
  }
  if (high) {
    result += static_cast<char>(*high * 16);
  }
  return result;
}

// Base-85: groups of five characters '!'..'u' make four bytes; 'z' is four
// zero bytes; "~>" ends the data. A last group of n characters makes n-1 bytes.
Result ascii85(std::string_view data) {
  std::string result;
  std::uint32_t value = 0;
  int count = 0;
  auto const flush = [&](int characters) {
    for (int i = characters; i < 5; ++i) {
      value = value * 85 + 84; // pad with 'u'
    }
    for (int i = 0; i < characters - 1; ++i) {
      result += static_cast<char>((value >> (24 - 8 * i)) & 0xFF);
    }
    value = 0;
  };
  for (std::size_t i = 0; i < data.size(); ++i) {
    auto const c = data[i];
    if (c == '~') {
      break;
    }
    if (is_space(c)) {
      continue;
    }
    if (c == 'z' && count == 0) {
      result.append(4, '\0');
      continue;
    }
    if (c < '!' || c > 'u') {
      return std::unexpected(std::format("ASCII85Decode: '{}' is not base-85", c));
    }
    value = value * 85 + static_cast<std::uint32_t>(c - '!');
    if (++count == 5) {
      flush(5);
      count = 0;
    }
  }
  if (count == 1) {
    return std::unexpected(std::string{"ASCII85Decode: a last group of one character"});
  }
  if (count > 0) {
    flush(count);
  }
  return result;
}

// PNG predictors (Predictor 10 and up): each row starts with its own filter
// type byte. TIFF predictor 2 is supported for 8 bits per component.
Result unpredict(std::string data, Filter const& filter) {
  if (filter.predictor == 1) {
    return data;
  }
  auto const bits_per_pixel = filter.colors * filter.bits_per_component;
  auto const bytes_per_pixel = std::max(1, bits_per_pixel / 8);
  auto const row_size = static_cast<std::size_t>((filter.columns * bits_per_pixel + 7) / 8);
  if (row_size == 0) {
    return std::unexpected(std::string{"predictor: no columns"});
  }
  auto const bpp = static_cast<std::size_t>(bytes_per_pixel);

  if (filter.predictor == 2) {
    if (filter.bits_per_component != 8) {
      return std::unexpected(std::format("TIFF predictor with {} bits per component: not supported",
                                         filter.bits_per_component));
    }
    for (std::size_t row = 0; row + row_size <= data.size(); row += row_size) {
      for (std::size_t i = bpp; i < row_size; ++i) {
        data[row + i] = static_cast<char>(data[row + i] + data[row + i - bpp]);
      }
    }
    return data;
  }
  if (filter.predictor < 10) {
    return std::unexpected(std::format("predictor {}: not supported", filter.predictor));
  }

  std::string result;
  std::string previous(row_size, '\0');
  for (std::size_t at = 0; at < data.size(); at += row_size + 1) {
    auto const type = static_cast<unsigned char>(data[at]);
    std::string row = data.substr(at + 1, row_size);
    row.resize(row_size, '\0'); // A short last row
    for (std::size_t i = 0; i < row_size; ++i) {
      auto const a = i >= bpp ? static_cast<unsigned char>(row[i - bpp]) : 0; // left
      auto const b = static_cast<unsigned char>(previous[i]);                 // up
      auto const c = i >= bpp ? static_cast<unsigned char>(previous[i - bpp]) : 0; // up left
      int add = 0;
      switch (type) {
      case 0: break;
      case 1: add = a; break;
      case 2: add = b; break;
      case 3: add = (a + b) / 2; break;
      case 4: {
        auto const p = a + b - c;
        auto const pa = std::abs(p - a);
        auto const pb = std::abs(p - b);
        auto const pc = std::abs(p - c);
        add = pa <= pb && pa <= pc ? a : (pb <= pc ? b : c);
        break;
      }
      default: return std::unexpected(std::format("PNG predictor: unknown row type {}", type));
      }
      row[i] = static_cast<char>(static_cast<unsigned char>(row[i]) + add);
    }
    result += row;
    previous = row;
  }
  return result;
}

} // namespace

bool can_decode(std::string_view filter) {
  return filter == "FlateDecode" || filter == "Fl" || filter == "ASCIIHexDecode" || filter == "AHx" ||
         filter == "ASCII85Decode" || filter == "A85";
}

Decoded decode(std::string data, std::vector<Filter> const& filters) {
  for (auto const& filter : filters) {
    Result result;
    if (filter.name == "FlateDecode" || filter.name == "Fl") {
      auto [bytes, problem] = flate(data);
      if (problem) {
        return {std::move(bytes), problem};
      }
      result = unpredict(std::move(bytes), filter);
    } else if (filter.name == "ASCIIHexDecode" || filter.name == "AHx") {
      result = ascii_hex(data);
    } else if (filter.name == "ASCII85Decode" || filter.name == "A85") {
      result = ascii85(data);
    } else {
      return {std::move(data), filter.name + ": not decoded"};
    }
    if (!result) {
      return {std::move(data), result.error()};
    }
    data = std::move(*result);
  }
  return {std::move(data), std::nullopt};
}

} // namespace walk_pdf
