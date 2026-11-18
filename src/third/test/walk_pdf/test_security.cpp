#include "document.hpp"
#include "peek.hpp"
#include "security.hpp"

#include <gtest/gtest.h>
#include <zlib.h>

#include "crypto/rc4.hpp"

#include <format>
#include <string>

using walk_pdf::Document;
using walk_pdf::Path;

namespace {

// Expected values computed independently, with Python's hashlib MD5 and its
// own RC4, for /O = bytes 00..1F, /P -572 and /ID[0] 00112233445566778899AABBCCDDEEFF.
// Object 1 holds /Title (UTF-16 "Malmö") and /Plain ("Hello"), encrypted;
// object 2 a stream "BT /F1 12 Tf (Hi) Tj ET", encrypted.
struct Vectors {
  std::string_view dictionary; // /V /R /Length
  std::string_view key;
  std::string_view user;
  std::string_view title;
  std::string_view plain;
  std::string_view stream;
};

constexpr Vectors rc4_128{"/V 2 /R 3 /Length 128", "4d4904736fef1d095ebebe0bb8b2aa3c",
                          "1027fa8a57cfcd498f1678b74ecf23f800000000000000000000000000000000",
                          "92b48e336f1892974249e7ad", "242ee21200",
                          "0a79fc24ce97545aebf5b3e0e0163966891f1d8b1b0ead"};

constexpr Vectors rc4_40{"/V 1 /R 2 /Length 40", "97e6874e61",
                         "31591e94aa9ab889a150af88b5e538b11a87e47e97b7a33871604397a3ece64b",
                         "c895be3414de532df992b328", "7e0fd2157b",
                         "ffde360c1c4099cba9a654e66b2629ee69ebd770585207"};

std::string bytes_of(std::string_view hex) {
  std::string result;
  for (std::size_t i = 0; i + 1 < hex.size(); i += 2) {
    result += static_cast<char>(std::stoi(std::string{hex.substr(i, 2)}, nullptr, 16));
  }
  return result;
}

std::string hex_of(std::string_view bytes) {
  std::string result;
  for (auto const c : bytes) {
    result += std::format("{:02x}", static_cast<unsigned char>(c));
  }
  return result;
}

std::string const owner = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";

std::string encrypted_pdf(Vectors const& v, std::string_view user, std::string_view extra = "") {
  auto const stream = bytes_of(v.stream);
  return std::format("%PDF-1.4\n"
                     "1 0 obj\n<< /Title <{}> /Plain <{}> >>\nendobj\n"
                     "2 0 obj\n<< /Length {} >>\nstream\n{}\nendstream\nendobj\n"
                     "3 0 obj\n<< /Filter /Standard {} /P -572 /O <{}> /U <{}> {}>>\nendobj\n"
                     "trailer\n<< /Size 4 /Root 1 0 R /Encrypt 3 0 R"
                     " /ID [<00112233445566778899AABBCCDDEEFF> <00112233445566778899AABBCCDDEEFF>] >>\n"
                     "%%EOF\n",
                     v.title, v.plain, stream.size(), stream, v.dictionary, owner, user, extra);
}

std::string peek_at(Document const& document, std::string_view path) {
  auto const content = document.find(*Path::parse(path));
  EXPECT_TRUE(content) << path;
  return content ? walk_pdf::peek(*content) : std::string{};
}

} // namespace

TEST(Security, Rc4With128BitKey) {
  Document const document{encrypted_pdf(rc4_128, rc4_128.user)};
  auto const& security = document.security();
  ASSERT_TRUE(security);
  ASSERT_TRUE(security->key);
  EXPECT_EQ(hex_of(*security->key), rc4_128.key);
  EXPECT_EQ(security->status, "RC4 128-bit, no password needed");
  EXPECT_EQ(security->dictionary_object, 3U);
}

TEST(Security, Rc4With40BitKey) {
  Document const document{encrypted_pdf(rc4_40, rc4_40.user)};
  auto const& security = document.security();
  ASSERT_TRUE(security);
  ASSERT_TRUE(security->key);
  EXPECT_EQ(hex_of(*security->key), rc4_40.key);
  EXPECT_EQ(security->status, "RC4 40-bit, no password needed");
}

TEST(Security, StringsAndStreamsAreDecrypted) {
  for (auto const& v : {rc4_128, rc4_40}) {
    Document const document{encrypted_pdf(v, v.user)};
    EXPECT_EQ(peek_at(document, "/rev[0]/obj[1]/Title"), "\"Malm\xC3\xB6\"");
    EXPECT_EQ(peek_at(document, "/rev[0]/obj[1]/Plain"), "(Hello)");
    EXPECT_EQ(peek_at(document, "/rev[0]/obj[1]"), "<< /Title \"Malm\xC3\xB6\" /Plain (Hello) >>");
    EXPECT_EQ(peek_at(document, "/rev[0]/obj[2]/stream"), "BT /F1 12 Tf (Hi) Tj ET");
  }
}

TEST(Security, EncryptionDictionaryAndTrailerAreNotDecrypted) {
  Document const document{encrypted_pdf(rc4_128, rc4_128.user)};
  EXPECT_FALSE(document.decrypt(*Path::parse("/rev[0]/obj[3]/O"), "x"));
  EXPECT_FALSE(document.decrypt(*Path::parse("/rev[0]/trailer[0]/ID/[0]"), "x"));
  EXPECT_TRUE(document.decrypt(*Path::parse("/rev[0]/obj[1]/Plain"), "x"));
}

TEST(Security, PermissionsInTheDocumentPeek) {
  Document const document{encrypted_pdf(rc4_128, rc4_128.user)};
  EXPECT_EQ(walk_pdf::peek(document.root()),
            "1 revision; encrypted: RC4 128-bit, no password needed; not permitted: modify, "
            "copy/extract, annotate, extract for accessibility");
  EXPECT_FALSE(document.security()->allowed(5)); // copy/extract
  EXPECT_TRUE(document.security()->allowed(3));  // print
}

TEST(Security, NeedsAPassword) {
  // A /U that the empty password does not give
  Document const document{encrypted_pdf(rc4_128, std::string(64, '0'))};
  ASSERT_TRUE(document.security());
  EXPECT_FALSE(document.security()->key);
  EXPECT_EQ(document.security()->status, "RC4 128-bit, needs a password");
  EXPECT_EQ(peek_at(document, "/rev[0]/obj[1]/Plain"), "<242ee21200>"); // left as it is
}

TEST(Security, AesIsNotSupportedYet) {
  auto text = encrypted_pdf(rc4_128, rc4_128.user, "/CF << /StdCF << /CFM /AESV2 >> >> /StmF /StdCF ");
  text.replace(text.find("/V 2 /R 3"), 9, "/V 4 /R 4");
  Document const document{text};
  ASSERT_TRUE(document.security());
  EXPECT_FALSE(document.security()->key);
  EXPECT_EQ(document.security()->status, "AES-128: not supported yet");
}

TEST(Security, StreamIsDecryptedThenDecoded) {
  // An encrypted FlateDecode stream: compress, then encrypt with object 2's key
  std::string const page = "BT /F1 12 Tf (Hi) Tj ET";
  auto size = compressBound(static_cast<uLong>(page.size()));
  std::string compressed(size, '\0');
  compress(reinterpret_cast<Bytef*>(compressed.data()), &size, reinterpret_cast<Bytef const*>(page.data()),
           static_cast<uLong>(page.size()));
  compressed.resize(size);
  auto const encrypted = crypto::rc4(walk_pdf::object_key(bytes_of(rc4_128.key), 2, 0), compressed);
  auto text = encrypted_pdf(rc4_128, rc4_128.user);
  auto const old_stream = bytes_of(rc4_128.stream);
  text.replace(text.find(old_stream), old_stream.size(), encrypted);
  text.replace(text.find("2 0 obj\n<< /Length 23 >>"), 24,
               "2 0 obj\n<< /Length " + std::to_string(encrypted.size()) + " /Filter /FlateDecode >>");
  Document const document{text};
  auto const decoded = document.find(*Path::parse("/rev[0]/obj[2]/stream/decoded"));
  ASSERT_TRUE(decoded);
  EXPECT_EQ(decoded->bytes(), page);
  EXPECT_FALSE(document.decrypt(decoded->path(), "x")); // decrypted as a whole already
}

TEST(Security, UnencryptedFile) {
  Document const document{"%PDF-1.4\n1 0 obj\n(Hello)\nendobj\ntrailer\n<< /Size 2 >>\n%%EOF\n"};
  EXPECT_FALSE(document.security());
  EXPECT_FALSE(document.decrypt(*Path::parse("/rev[0]/obj[1]"), "x"));
  EXPECT_EQ(walk_pdf::peek(document.root()), "1 revision");
}
