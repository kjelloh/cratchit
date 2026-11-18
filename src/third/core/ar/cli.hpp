#pragma once

#include <cstdio>
#include <span>
#include <string_view>

namespace cratchit::ar {

// cratchit_ar's command line (its main.cpp, session 5b35b2b9), run by cratchit
// as cratchit --sie-to-ar <arguments>. arguments are those after --sie-to-ar:
//   [--trace] [--bas-year <year>] <file.se>  writes the årsredovisning of an SIE 4
//       file, its report and a copy of the SIE file into the folder
//       <orgnr>_<start>_<end> of the working directory; with --trace prints the BAS
//       accounts and vouchers behind each amount instead, and writes no files
//   --help or -h  the manual (docs/sie_to_ar.md)
// Output goes to out, errors and notes to err, in English as cratchit_ar has
// them. version is cratchit's, for the report. Gives the exit code: 0 success,
// 1 errors in the books or files, 2 invalid arguments.
int run_cli(std::span<std::string_view const> arguments, std::FILE* out, std::FILE* err, std::string_view version);

} // namespace cratchit::ar
