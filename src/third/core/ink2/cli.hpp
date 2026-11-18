#pragma once

#include <cstdio>
#include <span>
#include <string_view>

namespace cratchit::ink2 {

// cratchit_ink2's command line (its main.cpp, session 7d4475fb), run by
// cratchit as cratchit --sie-to-ink2 <arguments>. arguments are those after
// --sie-to-ink2:
//   [--trace] <file.se> <last year's BLANKETTER.SRU or its folder>
//   [--trace] <file.se> --previous-deficit <kronor>
//       makes INK2R, INK2S and INK2 page 1 of the inkomstdeklaration 2 for fiscal
//       year 0 of the SIE file, written to the folder <orgnr>_<start>_<end> of the
//       working directory as text with a report, and as SRU files when nothing
//       known is missing, with a copy of the SIE file; with --trace it prints how
//       each amount is made instead, and writes no files
//   --help or -h  the manual (docs/sie_to_ink2.md)
// Output goes to out, errors and notes to err, in English as cratchit_ink2 has
// them. version is cratchit's, for the report and INFO.SRU. Gives the exit
// code: 0 success, 1 errors in the books or files, 2 invalid arguments.
int run_cli(std::span<std::string_view const> arguments, std::FILE* out, std::FILE* err, std::string_view version);

} // namespace cratchit::ink2
