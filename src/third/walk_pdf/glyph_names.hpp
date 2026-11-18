#pragma once

#include "charset/unicode.hpp"

#include <optional>
#include <string_view>

namespace walk_pdf {

// The Unicode code point an Adobe glyph name stands for, as in a font's
// /Differences: names of ASCII, Latin-1, Windows-1252 and Mac OS Roman
// characters ("adieresis" is ä), "uniXXXX" and "uXXXX" to "uXXXXXX". A suffix
// after a period is ignored ("a.sc" is a). A subset of the Adobe Glyph List,
// enough for Latin text; other names give nullopt.
std::optional<charset::unicode::CodePoint> glyph_unicode(std::string_view name);

} // namespace walk_pdf
