#include "security.hpp"

#include "crypto/md5.hpp"
#include "crypto/rc4.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace walk_pdf {

namespace {

// Algorithm 2, step a: the bytes a password is padded with to 32 bytes
constexpr std::string_view padding{"\x28\xBF\x4E\x5E\x4E\x75\x8A\x41\x64\x00\x4E\x56\xFF\xFA\x01\x08"
                                   "\x2E\x2E\x00\xB6\xD0\x68\x3E\x80\x2F\x0C\xA9\xFE\x64\x53\x69\x7A",
                                   32};

std::string as_string(crypto::Md5Digest const& digest, std::size_t size = 16) {
  return std::string{reinterpret_cast<char const*>(digest.data()), std::min<std::size_t>(size, 16)};
}

std::string little_endian(std::uint64_t value, int bytes) {
  std::string result;
  for (int i = 0; i < bytes; ++i) {
    result += static_cast<char>((value >> (8 * i)) & 0xFF);
  }
  return result;
}

} // namespace

std::vector<std::string> Security::denied() const {
  // PDF 32000-1:2008, Table 22
  constexpr std::array<std::pair<int, std::string_view>, 8> names{{
      {3, "print"},
      {4, "modify"},
      {5, "copy/extract"},
      {6, "annotate"},
      {9, "fill forms"},
      {10, "extract for accessibility"},
      {11, "assemble"},
      {12, "high quality print"},
  }};
  std::vector<std::string> result;
  for (auto const& [bit, name] : names) {
    if (!allowed(bit)) {
      result.emplace_back(name);
    }
  }
  return result;
}

std::optional<std::string> key_for_empty_password(int revision, std::size_t key_bytes,
                                                  std::string_view owner, std::string_view user,
                                                  std::uint32_t permissions,
                                                  std::string_view first_id, bool encrypt_metadata) {
  // Algorithm 2: the file key from the (empty, so all padding) password
  auto const n = revision == 2 ? std::size_t{5} : std::min<std::size_t>(key_bytes, 16);
  crypto::Md5 md5;
  md5.update(padding).update(owner.substr(0, 32)).update(little_endian(permissions, 4)).update(first_id);
  if (revision >= 4 && !encrypt_metadata) {
    md5.update("\xFF\xFF\xFF\xFF");
  }
  auto key = as_string(md5.finish(), n);
  if (revision >= 3) {
    for (int i = 0; i < 50; ++i) {
      key = as_string(crypto::md5(key), n);
    }
  }

  // Algorithm 4 (R 2) or 5 (R 3 and up): what /U holds for this key
  if (revision == 2) {
    return crypto::rc4(key, padding) == user.substr(0, 32) ? std::optional{key} : std::nullopt;
  }
  auto check = crypto::rc4(key, as_string(crypto::Md5{}.update(padding).update(first_id).finish()));
  for (int i = 1; i <= 19; ++i) {
    auto round_key = key;
    for (auto& c : round_key) {
      c = static_cast<char>(static_cast<unsigned char>(c) ^ i);
    }
    check = crypto::rc4(round_key, check);
  }
  return check == user.substr(0, 16) ? std::optional{key} : std::nullopt;
}

std::string object_key(std::string_view file_key, std::uint64_t number, std::uint32_t generation) {
  auto const digest = crypto::Md5{}
                          .update(file_key)
                          .update(little_endian(number, 3))
                          .update(little_endian(generation, 2))
                          .finish();
  return as_string(digest, file_key.size() + 5);
}

} // namespace walk_pdf
