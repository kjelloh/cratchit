#pragma once

#include <cstdio>
#include <span>
#include <string_view>

namespace cratchit::skv::vat_return {

// cratchit --sie-to-vat <arguments>: the VAT return (momsrapport) of take 3
// (session 60cae9fe) from SIE files, without cratchit's user interface.
// arguments are those after --sie-to-vat:
//   <file.se> [<last year's file.se>]
//       lists the VAT quarters ending in the fiscal year of the file, each with its
//       VAT in three steps (period, cleared, charged)
//   <file.se> [<last year's file.se>] --period <YYYYQn>
//       writes the folder <orgnr>_moms_<first>_<last> of the working directory: the
//       VAT return report with its checks (_momsrapport.txt), the suggested clearing
//       voucher (_verifikation.txt), the eSKD file for SKV (moms_<YYYYMM>.eskd) and a
//       copy of the SIE file; for a quarter not yet ended, the report only
//   --help or -h  the manual (docs/sie_to_vat.md)
// Last year's file is needed for a quarter that starts in the fiscal year before
// (a broken fiscal year): an SIE file holds the vouchers of its year 0 only. The
// files are read into books in memory as an SIE import would (books::plan_import),
// the older year first. Output on the terminal goes to out and err in English; the
// files written are in Swedish. version is cratchit's. Gives the exit code: 0
// success, 1 a file that cannot be read or imported, 2 invalid arguments.
int run_cli(std::span<std::string_view const> arguments, std::FILE* out, std::FILE* err, std::string_view version);

} // namespace cratchit::skv::vat_return
