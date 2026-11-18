#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace walk_pdf {

// The bytes a string token holds (PDF 32000-1:2008, 7.3.4 String objects):
// a literal string "(...)" with its escapes undone, or a hexadecimal string
// "<...>" with its digits paired into bytes
std::string string_bytes(std::string_view token);

// A text string's bytes as UTF-8, when they are marked as UTF-16BE (FE FF)
// or UTF-8 (EF BB BF); the mark itself is left out. nullopt for other text
// strings, which are PDFDocEncoding (not decoded yet). Bytes that do not
// decode become U+FFFD (replacement character).
std::optional<std::string> text_to_utf8(std::string_view bytes);

} // namespace walk_pdf
