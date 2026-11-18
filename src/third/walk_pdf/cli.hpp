#pragma once

#include <cstdio>
#include <span>
#include <string_view>

namespace walk_pdf {

// walk_pdf's command line (its main.cpp, session 9fe36d28), run by cratchit as
// cratchit --walk-pdf <arguments>. arguments are those after --walk-pdf:
//   <file> [<path>] [--depth N]  lists the element at path (default the whole
//                                file) and what is inside it, depth levels down (default 3)
//   --text <file>                the document's text, page by page
//   -h or --help                 the manual (docs/walk_pdf.md)
//   nothing                      walk_pdf's name and version, and version (cratchit's)
// Output goes to out, warnings and errors to err, in English as walk_pdf has
// them. Gives the exit code: 0 success, 1 the file could not be read or has
// no element at the path (with --text: also no text to give), 2 invalid
// arguments or path.
int run_cli(std::span<std::string_view const> arguments, std::FILE* out, std::FILE* err, std::string_view version);

} // namespace walk_pdf
