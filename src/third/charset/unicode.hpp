#pragma once

namespace charset::unicode {

// One Unicode code point
using CodePoint = char32_t;

// U+FFFD, stands in for a character that could not be decoded or encoded
constexpr CodePoint replacement_character = 0xFFFD;

} // namespace charset::unicode
