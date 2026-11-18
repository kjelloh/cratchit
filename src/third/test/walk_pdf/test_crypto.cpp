#include "crypto/md5.hpp"
#include "crypto/rc4.hpp"

#include <gtest/gtest.h>

#include <format>
#include <string>

namespace {

std::string hex(std::string_view bytes) {
  std::string result;
  for (auto const c : bytes) {
    result += std::format("{:02x}", static_cast<unsigned char>(c));
  }
  return result;
}

std::string hex(crypto::Md5Digest const& digest) {
  return hex(std::string_view{reinterpret_cast<char const*>(digest.data()), digest.size()});
}

} // namespace

TEST(Md5, Rfc1321TestSuite) {
  EXPECT_EQ(hex(crypto::md5("")), "d41d8cd98f00b204e9800998ecf8427e");
  EXPECT_EQ(hex(crypto::md5("a")), "0cc175b9c0f1b6a831c399e269772661");
  EXPECT_EQ(hex(crypto::md5("abc")), "900150983cd24fb0d6963f7d28e17f72");
  EXPECT_EQ(hex(crypto::md5("message digest")), "f96b697d7cb7938d525a2f31aaf161d0");
  EXPECT_EQ(hex(crypto::md5("abcdefghijklmnopqrstuvwxyz")), "c3fcd3d76192e4007dfb496cca67e13b");
  EXPECT_EQ(hex(crypto::md5("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789")),
            "d174ab98d277d9f5a5611c2c9f419d9f");
  EXPECT_EQ(hex(crypto::md5("1234567890123456789012345678901234567890"
                            "1234567890123456789012345678901234567890")),
            "57edf4a22be3c955ac49da2e2107b67a");
}

TEST(Md5, InPiecesIsTheSameAsInOneGo) {
  std::string const message(200, 'x');
  crypto::Md5 md5;
  md5.update(message.substr(0, 55)).update(message.substr(55, 9)).update(message.substr(64));
  EXPECT_EQ(md5.finish(), crypto::md5(message));
}

TEST(Rc4, KnownVectors) {
  EXPECT_EQ(hex(crypto::rc4("Key", "Plaintext")), "bbf316e8d940af0ad3");
  EXPECT_EQ(hex(crypto::rc4("Wiki", "pedia")), "1021bf0420");
  EXPECT_EQ(hex(crypto::rc4("Secret", "Attack at dawn")), "45a01f645fc35b383552544b9bf5");
}

TEST(Rc4, DecryptingIsEncryptingAgain) {
  EXPECT_EQ(crypto::rc4("Key", crypto::rc4("Key", "Plaintext")), "Plaintext");
}

TEST(Rc4, InPiecesIsTheSameAsInOneGo) {
  crypto::Rc4 rc4{"Secret"};
  // One statement each: the order the operands of + are evaluated in is not given (GCC takes the right first)
  auto const first = rc4.apply("Attack");
  auto const second = rc4.apply(" at dawn");
  EXPECT_EQ(first + second, crypto::rc4("Secret", "Attack at dawn"));
}
