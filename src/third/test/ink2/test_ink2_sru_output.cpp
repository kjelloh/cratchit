#include "core/ink2/sru_output.hpp"

#include "core/balances.hpp"
#include "core/closing.hpp"
#include "core/ink2/previous.hpp"
#include "core/sie/file.hpp"
#include "core/sru.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <fstream>

using namespace std::chrono;
using cratchit::ink2::block_name;
using cratchit::ink2::postal_of;
using cratchit::ink2::to_latin1;

namespace {

std::vector<cratchit::ink2::Line> const& mapping() {
  static auto const lines = *cratchit::ink2::parse_mapping(cratchit::ink2::mapping_text());
  return lines;
}

std::vector<cratchit::ink2::SkvField> const& skv() {
  static auto const fields = *cratchit::ink2::parse_skv_fields(cratchit::ink2::skv_fields_text());
  return fields;
}

cratchit::ink2::Filer const filer{"556782-8172", "The ITfied AB", "17141", "Solna"};
cratchit::ink2::Stamp const stamp{2026y / November / 3, "141744"};

std::string read(std::filesystem::path const& file) {
  std::ifstream in(file, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

} // namespace

TEST(Ink2SruOutput, OrgnrAndBlockName) {
  EXPECT_EQ(cratchit::ink2::sru_orgnr("556782-8172"), "165567828172");
  EXPECT_EQ(cratchit::ink2::sru_orgnr("165567828172"), "165567828172");
  EXPECT_EQ(block_name("INK2R", 2026y / April / 30), "INK2R-2026P1");
  EXPECT_EQ(block_name("INK2S", 2026y / June / 30), "INK2S-2026P2");
  EXPECT_EQ(block_name("INK2", 2026y / August / 31), "INK2-2026P3");
  EXPECT_EQ(block_name("INK2", 2026y / December / 31), "INK2-2026P4");
}

TEST(Ink2SruOutput, Postal) {
  EXPECT_EQ(postal_of("17141 Solna"), (std::pair<std::string, std::string>{"17141", "Solna"}));
  EXPECT_EQ(postal_of("176 68 Järfälla"), (std::pair<std::string, std::string>{"17668", "Järfälla"}));
  EXPECT_FALSE(postal_of("Solna"));
  EXPECT_FALSE(postal_of("1714 Solna"));
  EXPECT_FALSE(postal_of("17141"));
  EXPECT_FALSE(postal_of("171412 Solna"));
  EXPECT_EQ(cratchit::ink2::postal_of_info("#ORGNR 1\r\n#POSTNR 17141\r\n#POSTORT Solna\r\n"),
            (std::pair<std::string, std::string>{"17141", "Solna"}));
  EXPECT_FALSE(cratchit::ink2::postal_of_info("#POSTNR 17141\n"));
}

TEST(Ink2SruOutput, Latin1) {
  EXPECT_EQ(to_latin1("Järfälla Å"), std::string("J\xE4rf\xE4lla \xC5"));
  EXPECT_EQ(to_latin1("–").error(), "\"–\" is not in ISO 8859-1, which SRU files are");
}

TEST(Ink2SruOutput, Info) {
  EXPECT_EQ(cratchit::ink2::format_info(filer, stamp, "cratchit_ink2 0.1.0"),
            "#DATABESKRIVNING_START\n"
            "#PRODUKT SRU\n"
            "#SKAPAD 20261103 141744\n"
            "#PROGRAM cratchit_ink2 0.1.0\n"
            "#FILNAMN BLANKETTER.SRU\n"
            "#DATABESKRIVNING_SLUT\n"
            "#MEDIELEV_START\n"
            "#ORGNR 165567828172\n"
            "#NAMN The ITfied AB\n"
            "#POSTNR 17141\n"
            "#POSTORT Solna\n"
            "#MEDIELEV_SLUT\n");
}

TEST(Ink2SruOutput, Blanketter) {
  // A year with a loss of 300 kr, 100 of it not deductible, and last year's underskott of 1000
  cratchit::Balances const balances{{"1930", 50000}, {"2081", -80000}, {"2099", 30000},
                                    {"6110", 20000}, {"6992", 10000}, {"8999", -30000}};
  auto const form = cratchit::ink2::build_form(mapping(), balances);
  auto const tax = cratchit::ink2::build_tax_return(skv(), form, {1000, "last.sru", {"8041", "8045"}, std::nullopt});
  cratchit::sie::FiscalYear const year{0, 2025y / May / 1, 2026y / April / 30};
  EXPECT_EQ(cratchit::ink2::format_blanketter(skv(), filer, year, form, tax, stamp),
            "#BLANKETT INK2-2026P1\n"
            "#IDENTITET 165567828172 20261103 141744\n"
            "#NAMN The ITfied AB\n"
            "#UPPGIFT 7011 20250501\n"
            "#UPPGIFT 7012 20260430\n"
            "#UPPGIFT 7114 1200\n"
            "#BLANKETTSLUT\n"
            "#BLANKETT INK2R-2026P1\n"
            "#IDENTITET 165567828172 20261103 141744\n"
            "#NAMN The ITfied AB\n"
            "#UPPGIFT 7011 20250501\n"
            "#UPPGIFT 7012 20260430\n"
            "#UPPGIFT 7281 500\n"
            "#UPPGIFT 7301 800\n"
            "#UPPGIFT 7302 -300\n"
            "#UPPGIFT 7513 300\n"
            "#UPPGIFT 7550 300\n"
            "#BLANKETTSLUT\n"
            "#BLANKETT INK2S-2026P1\n"
            "#IDENTITET 165567828172 20261103 141744\n"
            "#NAMN The ITfied AB\n"
            "#UPPGIFT 7011 20250501\n"
            "#UPPGIFT 7012 20260430\n"
            "#UPPGIFT 7750 300\n"
            "#UPPGIFT 7653 100\n"
            "#UPPGIFT 7763 1000\n"
            "#UPPGIFT 7770 1200\n"
            "#UPPGIFT 8041 X\n"
            "#UPPGIFT 8045 X\n"
            "#BLANKETTSLUT\n"
            "#FIL_SLUT\n");
}

TEST(Ink2SruOutput, WriteOutputs) {
  auto const base = std::filesystem::temp_directory_path() / "cratchit_ink2_test_output";
  std::filesystem::remove_all(base);
  std::filesystem::create_directories(base);
  auto const sie = base / "books.se";
  std::ofstream(sie, std::ios::binary) << "#FLAGGA 0\r\n";
  cratchit::sie::FiscalYear const year{0, 2025y / May / 1, 2026y / April / 30};
  auto const paths = cratchit::ink2::output_paths(base, "556782-8172", year);
  EXPECT_EQ(paths.folder, base / "556782-8172_2025-05-01_2026-04-30");
  EXPECT_EQ(paths.fields.filename().u8string(), u8"556782-8172_2025-05-01_2026-04-30_ink2_fält.txt");
  EXPECT_EQ(paths.report, paths.folder / "556782-8172_2025-05-01_2026-04-30_ink2_rapport.txt");
  EXPECT_EQ(paths.blanketter, paths.folder / "BLANKETTER.SRU");
  EXPECT_EQ(paths.info, paths.folder / "INFO.SRU");
  EXPECT_EQ(paths.sie, paths.folder / "556782-8172_2025-05-01_2026-04-30.se");
  // Complete: everything
  auto const written = cratchit::ink2::write_outputs(paths, "report 1", std::string("fields 1"),
                                                     cratchit::ink2::SruFiles{"#BLANKETT X\n", "#POSTORT Järfälla\n"}, sie);
  ASSERT_TRUE(written) << written.error();
  EXPECT_TRUE(written->fields);
  EXPECT_TRUE(written->sru);
  EXPECT_TRUE(written->sie_copied);
  EXPECT_EQ(read(paths.report), "report 1");
  EXPECT_EQ(read(paths.fields), "fields 1");
  EXPECT_EQ(read(paths.blanketter), "#BLANKETT X\n");
  EXPECT_EQ(read(paths.info), "#POSTORT J\xE4rf\xE4lla\n");
  EXPECT_EQ(read(paths.sie), "#FLAGGA 0\r\n");
  EXPECT_EQ(cratchit::ink2::info_file_in(paths.folder), paths.info);
  // Not complete: the report and the fields, the earlier SRU files left and said to be
  auto const again = cratchit::ink2::write_outputs(paths, "report 2", std::string("fields 2"), std::nullopt, sie);
  ASSERT_TRUE(again);
  EXPECT_FALSE(again->sru);
  EXPECT_TRUE(again->old_sru);
  EXPECT_FALSE(again->sie_copied);
  EXPECT_EQ(read(paths.fields), "fields 2");
  EXPECT_EQ(read(paths.blanketter), "#BLANKETT X\n");
  // Errors: the report only; a changed SIE file is not copied over the one that matches the earlier return
  std::ofstream(sie, std::ios::binary) << "#FLAGGA 1\r\n";
  auto const errors = cratchit::ink2::write_outputs(paths, "report 3", std::nullopt, std::nullopt, sie);
  ASSERT_TRUE(errors);
  EXPECT_FALSE(errors->fields);
  EXPECT_FALSE(errors->sie_copied);
  EXPECT_EQ(read(paths.report), "report 3");
  EXPECT_EQ(read(paths.fields), "fields 2");
  EXPECT_EQ(read(paths.sie), "#FLAGGA 0\r\n");
  // Text not in ISO 8859-1 writes nothing
  EXPECT_EQ(cratchit::ink2::write_outputs(paths, "report 4", std::string("fields 4"),
                                          cratchit::ink2::SruFiles{"#NAMN –\n", ""}, sie)
                .error(),
            "BLANKETTER.SRU: \"–\" is not in ISO 8859-1, which SRU files are");
  EXPECT_EQ(read(paths.report), "report 3");
  std::filesystem::remove_all(base);
}

namespace {

// The return the app makes of a session SIE file (git ignored) with last year's return, as SRU text
std::optional<std::string> made_return(char const* sie_name, std::vector<cratchit::SruForm> const& last_year) {
  auto const sie = std::filesystem::path(CRATCHIT_INK2_SIE_FOLDER) / sie_name;
  if (!std::filesystem::exists(sie)) {
    return std::nullopt;
  }
  auto const document = cratchit::sie::parse_document(cratchit::sie::lex_file(sie)->lines).document;
  auto const year = *document.year(0);
  auto const previous = cratchit::ink2::previous_from_sru(last_year, "last year", document.orgnr, year.start);
  EXPECT_TRUE(previous) << previous.error();
  auto const form = cratchit::ink2::build_form(mapping(), cratchit::year_balances(document, 0),
                                               cratchit::ink2::sru_codes(document));
  auto const tax = cratchit::ink2::build_tax_return(skv(), form, *previous);
  return cratchit::ink2::format_blanketter(skv(), filer, year, form, tax, stamp);
}

std::string field(std::vector<cratchit::SruForm> const& forms, std::string_view kind, std::string const& code) {
  auto const form = std::ranges::find(forms, kind, &cratchit::SruForm::kind);
  return form == forms.end() || !form->fields.contains(code) ? "" : form->fields.at(code);
}

} // namespace

TEST(Ink2SruOutput, TheAppChainsItsOwnReturns) {
  // From the filed 2022 return, each year from the return the app made the year before
  auto const filed_2022 =
      cratchit::read_sru(std::filesystem::path(CRATCHIT_INK2_SRU_FOLDER) / "deklaration 2022/blanketter.sru");
  if (!filed_2022) {
    GTEST_SKIP() << filed_2022.error();
  }
  std::vector<cratchit::SruForm> last_year = *filed_2022;
  std::vector<std::string> deficits;
  for (char const* const sie : {"TheITfiedAB20230503_152345.se", "TheITfiedAB20240727_213539.se",
                                "TheITfiedAB20250817_172409.se"}) {
    auto const text = made_return(sie, last_year);
    if (!text) {
      GTEST_SKIP() << "No " << sie;
    }
    auto const forms = cratchit::parse_sru(*text);
    ASSERT_TRUE(forms) << forms.error();
    deficits.push_back(std::format("{}/{}", field(*forms, "INK2S", "7763"), field(*forms, "INK2S", "7770")));
    EXPECT_EQ(field(*forms, "INK2", "7114"), field(*forms, "INK2S", "7770"));
    last_year = *forms;
  }
  // 2022-23: 92519 + 84193 + 5 - 685; 2023-24: + 30547 + 15 - 625; 2024-25: + 46702 + 12 - 1217
  EXPECT_EQ(deficits, (std::vector<std::string>{"92519/176032", "176032/205969", "205969/251466"}));
}
