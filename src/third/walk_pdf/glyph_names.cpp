#include "glyph_names.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <utility>

namespace walk_pdf {

namespace {

constexpr std::pair<std::string_view, charset::unicode::CodePoint> names[] = {
#include "glyph_names_table.inc"
};

std::optional<charset::unicode::CodePoint> hex(std::string_view digits) {
  // std::from_chars takes integer types only, not char32_t (libstdc++ refuses it; libc++ let it through)
  std::uint32_t value = 0;
  auto const [ptr, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), value, 16);
  if (digits.empty() || ec != std::errc{} || ptr != digits.data() + digits.size()) {
    return std::nullopt;
  }
  return static_cast<charset::unicode::CodePoint>(value);
}

} // namespace

std::optional<charset::unicode::CodePoint> glyph_unicode(std::string_view name) {
  name = name.substr(0, name.find('.'));
  auto const found = std::ranges::lower_bound(names, name, {}, &std::pair<std::string_view, charset::unicode::CodePoint>::first);
  if (found != std::end(names) && found->first == name) {
    return found->second;
  }
  if (name.starts_with("uni") && name.size() >= 7) {
    return hex(name.substr(3, 4)); // the first of possibly several
  }
  if (name.starts_with('u') && name.size() >= 5 && name.size() <= 7) {
    return hex(name.substr(1));
  }
  return std::nullopt;
}

} // namespace walk_pdf
