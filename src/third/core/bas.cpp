#include "core/bas.hpp"

namespace cratchit {

AccountKind account_kind(std::string_view number, std::string_view type) {
  if (type == "T" || type == "S") {
    return AccountKind::balance;
  }
  if (type == "K" || type == "I") {
    return AccountKind::result;
  }
  if (number.empty()) {
    return AccountKind::unknown;
  }
  auto const digit = number.front();
  if (digit == '1' || digit == '2') {
    return AccountKind::balance;
  }
  if (digit >= '3' && digit <= '8') {
    return AccountKind::result;
  }
  return AccountKind::unknown;
}

} // namespace cratchit
