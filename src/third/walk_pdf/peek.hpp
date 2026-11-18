#pragma once

#include "document.hpp"

#include <cstddef>
#include <string>

namespace walk_pdf {

// A one-line look at what an element holds, at most width characters long
// (a stream's description may make an object's peek a little longer).
// The keyword that opens the element ("12 0 obj", "trailer", ...) is left out,
// since the path already says what the element is. Whitespace runs become one
// space, text that does not fit ends with "...", and bytes that are not text
// are described in <...> rather than shown.
std::string peek(Content const& content, std::size_t width = 60);

} // namespace walk_pdf
