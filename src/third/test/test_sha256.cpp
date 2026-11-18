// The test vectors of NIST (FIPS 180-4 examples, and the CAVP short and long messages)
#include "core/sha256.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace {

std::string sha256_of(std::string_view text) {
  cratchit::Sha256 hash;
  hash.add({reinterpret_cast<std::uint8_t const*>(text.data()), text.size()});
  return hash.hex();
}

} // namespace

TEST(Sha256, GivesNistDigests) {
  EXPECT_EQ(sha256_of(""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  EXPECT_EQ(sha256_of("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  EXPECT_EQ(sha256_of("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
            "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
  EXPECT_EQ(sha256_of(std::string(1000000, 'a')), "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

TEST(Sha256, IsTheSameWhenAddedInPieces) {
  cratchit::Sha256 hash;
  std::string_view const text = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
  for (auto const c : text) hash.add({reinterpret_cast<std::uint8_t const*>(&c), 1});
  EXPECT_EQ(hash.hex(), "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

TEST(Sha256, OfAFile) {
  auto const file = std::filesystem::temp_directory_path() / "cratchit_sha256_test.txt";
  std::ofstream(file, std::ios::binary) << "abc";
  EXPECT_EQ(cratchit::sha256_of_file(file), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  std::filesystem::remove(file);
  EXPECT_FALSE(cratchit::sha256_of_file(file));
}
