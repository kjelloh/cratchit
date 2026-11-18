#include "crypto/rc4.hpp"

#include <utility>

namespace crypto {

Rc4::Rc4(std::string_view key) {
  for (std::size_t i = 0; i < state_.size(); ++i) {
    state_[i] = static_cast<std::uint8_t>(i);
  }
  if (key.empty()) {
    return;
  }
  std::uint8_t j = 0;
  for (std::size_t i = 0; i < state_.size(); ++i) {
    j = static_cast<std::uint8_t>(j + state_[i] + static_cast<std::uint8_t>(key[i % key.size()]));
    std::swap(state_[i], state_[j]);
  }
}

std::string Rc4::apply(std::string_view bytes) {
  std::string result(bytes.size(), '\0');
  for (std::size_t n = 0; n < bytes.size(); ++n) {
    i_ = static_cast<std::uint8_t>(i_ + 1);
    j_ = static_cast<std::uint8_t>(j_ + state_[i_]);
    std::swap(state_[i_], state_[j_]);
    auto const k = state_[static_cast<std::uint8_t>(state_[i_] + state_[j_])];
    result[n] = static_cast<char>(static_cast<std::uint8_t>(bytes[n]) ^ k);
  }
  return result;
}

std::string rc4(std::string_view key, std::string_view bytes) { return Rc4{key}.apply(bytes); }

} // namespace crypto
