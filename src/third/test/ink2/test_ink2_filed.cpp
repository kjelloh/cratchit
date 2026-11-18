// INK2R, INK2S and INK2 page 1 made from the session's SIE files (chained from the previous year's
// filed return) against what the company filed for the same fiscal year (both git ignored, in
// _Resources; the tests skip when they are missing).
#include "core/balances.hpp"
#include "core/closing.hpp"
#include "core/date.hpp"
#include "core/ink2/form.hpp"
#include "core/ink2/mapping.hpp"
#include "core/ink2/previous.hpp"
#include "core/ink2/skv.hpp"
#include "core/ink2/tax.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"
#include "core/sru.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <format>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace {

std::filesystem::path const sie_folder = CRATCHIT_INK2_SIE_FOLDER;
std::filesystem::path const sru_folder = CRATCHIT_INK2_SRU_FOLDER;

// The filed returns used as last year's and as this year's reference. Of the three 2024 filings the
// corrected one of 2024-11-30: its 4.14a carries 2022-23's 4.16 (176026), and 2025's return carries
// its 4.16 (205963) on. The latest (2025-08-15) carries 84193 (2022-23's loss only). Their INK2R are the same.
char const* const filed_2022 = "deklaration 2022/blanketter.sru";
char const* const filed_2023 = "deklaration 2023/blanketter.sru";
char const* const filed_2024 = "deklaration 2024/Korrigerad 20241130/blanketter.sru";
char const* const filed_2025 = "deklaration 2025/BLANKETTER.SRU";

struct Made {
  cratchit::sie::FiscalYear year;
  cratchit::ink2::Form form;
  std::optional<cratchit::ink2::TaxReturn> tax; // when made with last year's return
};

Made make(std::filesystem::path const& sie, std::optional<std::filesystem::path> const& last_year) {
  auto const lexed = cratchit::sie::lex_file(sie);
  EXPECT_TRUE(lexed) << lexed.error();
  auto const parsed = cratchit::sie::parse_document(lexed->lines);
  EXPECT_TRUE(parsed.errors.empty());
  auto const unbooked = cratchit::unbooked_result(parsed.document);
  auto const document = unbooked ? cratchit::with_closing_entry(parsed.document, *unbooked) : parsed.document;
  auto const lines = cratchit::ink2::parse_mapping(cratchit::ink2::mapping_text());
  EXPECT_TRUE(lines);
  Made made{*document.year(0),
            cratchit::ink2::build_form(*lines, cratchit::year_balances(document, 0), cratchit::ink2::sru_codes(document)),
            std::nullopt};
  if (last_year) {
    auto const forms = cratchit::read_sru(*last_year);
    EXPECT_TRUE(forms) << forms.error();
    auto const previous =
        cratchit::ink2::previous_from_sru(*forms, last_year->string(), document.orgnr, made.year.start);
    EXPECT_TRUE(previous) << previous.error();
    auto const skv = cratchit::ink2::parse_skv_fields(cratchit::ink2::skv_fields_text());
    made.tax = cratchit::ink2::build_tax_return(*skv, made.form, *previous);
  }
  return made;
}

// Where made differs from filed, "<code>: filed <amount>, made <amount>", a field not given counting
// as 0; marks as "<code>: filed X, made " and the other way; the fiscal year when it differs
std::vector<std::string> differences(cratchit::sie::FiscalYear const& year, std::map<std::string, cratchit::Kronor> const& made,
                                     std::set<std::string> const& made_marks, cratchit::SruForm const& filed) {
  std::vector<std::string> out;
  auto const start = cratchit::format_compact_date(year.start);
  auto const end = cratchit::format_compact_date(year.end);
  if (filed.fields.at("7011") != start || filed.fields.at("7012") != end) {
    out.push_back(std::format("fiscal year: filed {}-{}, made {}-{}", filed.fields.at("7011"), filed.fields.at("7012"),
                              start, end));
  }
  std::set<std::string> codes;
  for (auto const& [code, value] : filed.fields) {
    if (code != "7011" && code != "7012") {
      codes.insert(code);
    }
  }
  for (auto const& [code, amount] : made) {
    codes.insert(code);
  }
  codes.insert(made_marks.begin(), made_marks.end());
  for (auto const& code : codes) {
    auto const filed_value = filed.fields.find(code);
    if ((filed_value != filed.fields.end() && filed_value->second == "X") || made_marks.contains(code)) {
      auto const filed_mark = filed_value != filed.fields.end() ? filed_value->second : "";
      auto const made_mark = made_marks.contains(code) ? "X" : "";
      if (filed_mark != made_mark) {
        out.push_back(std::format("{}: filed {}, made {}", code, filed_mark, made_mark));
      }
      continue;
    }
    long long filed_amount = 0;
    if (filed_value != filed.fields.end()) {
      auto const& text = filed_value->second;
      auto const [end_of_number, error] = std::from_chars(text.data(), text.data() + text.size(), filed_amount);
      EXPECT_TRUE(error == std::errc{} && end_of_number == text.data() + text.size()) << code << " " << text;
    }
    auto const made_value = made.find(code);
    long long const made_amount = made_value == made.end() ? 0 : made_value->second;
    if (filed_amount != made_amount) {
      out.push_back(std::format("{}: filed {}, made {}", code, filed_amount, made_amount));
    }
  }
  return out;
}

// The INK2R fields made from accounts
std::map<std::string, cratchit::Kronor> ink2r_of(cratchit::ink2::Form const& form) {
  std::map<std::string, cratchit::Kronor> out;
  for (auto const& [code, field] : form.fields) {
    if (field.used && cratchit::ink2::form_of(code) == "INK2R") {
      out[code] = field.kronor;
    }
  }
  return out;
}

cratchit::SruForm const& form_named(std::vector<cratchit::SruForm> const& forms, std::string_view kind) {
  auto const found = std::ranges::find(forms, kind, &cratchit::SruForm::kind);
  EXPECT_NE(found, forms.end()) << kind;
  return *found;
}

struct Filed {
  char const* sie;
  char const* sru;       // this year's filed return, under sru_folder
  char const* last_year; // last year's filed return, under sru_folder; none for INK2R only
  std::vector<std::string> ink2r;
  std::vector<std::string> ink2s;
  std::vector<std::string> ink2;
  std::vector<std::string> errors;
  std::vector<std::string> sru_notes; // the notes on the books' #SRU
};

void check(Filed const& filed) {
  auto const sie = sie_folder / filed.sie;
  auto const sru = sru_folder / filed.sru;
  auto const last_year =
      filed.last_year ? std::optional<std::filesystem::path>(sru_folder / filed.last_year) : std::nullopt;
  if (!std::filesystem::exists(sie) || !std::filesystem::exists(sru) ||
      (last_year && !std::filesystem::exists(*last_year))) {
    GTEST_SKIP() << "No " << sie << " or " << sru;
  }
  auto const made = make(sie, last_year);
  auto const forms = cratchit::read_sru(sru);
  ASSERT_TRUE(forms) << forms.error();
  EXPECT_EQ(differences(made.year, ink2r_of(made.form), {}, form_named(*forms, "INK2R")), filed.ink2r);
  if (made.tax) {
    EXPECT_EQ(differences(made.year, made.tax->ink2s, made.tax->ink2s_marks, form_named(*forms, "INK2S")), filed.ink2s);
    EXPECT_EQ(differences(made.year, made.tax->ink2, {}, form_named(*forms, "INK2")), filed.ink2);
    EXPECT_TRUE(made.tax->errors.empty());
  }
  EXPECT_EQ(made.form.errors, filed.errors);
  std::vector<std::string> sru_notes;
  std::ranges::copy_if(made.form.notes, std::back_inserter(sru_notes),
                       [](std::string const& note) { return note.starts_with("#SRU"); });
  EXPECT_EQ(sru_notes, filed.sru_notes);
}

// Fortnox's #SRU agree with BAS's table but for these own accounts of the company
std::string const own_accounts = "#SRU: no #SRU in the books for 1226, 1227; the mapping decides";

} // namespace

TEST(Ink2Filed, Year2021_22) {
  // No return of 2020-21 in _Resources: INK2R only
  check({"TheITfiedAB20221124_192100.se",
         filed_2022,
         nullptr,
         {
             // Filed: moms 2650 with a debit balance (428.09) moved to övriga fordringar, as BAS asks at
             // closing (1650 Momsfordran); the books leave it on 2650, a debt account
             "7261: filed 1179, made 751",
             // Filed: 464825.51 and 195.50 rounded down, so that the filed balansräkning (with 2650 as a
             // receivable) adds up in whole kronor: 504840 on both sides, where rounding each field gives
             // 504841 against 504840
             "7281: filed 464825, made 464826",
             "7365: filed 195, made 196",
             "7369: filed 0, made -428",
         },
         {},
         {},
         {},
         {own_accounts}});
}

TEST(Ink2Filed, Year2022_23) {
  check({"TheITfiedAB20230503_152345.se",
         filed_2023,
         filed_2022,
         {
             // Filed without övriga skulder (2641, 2893, 2898): the filed balansräkning does not add up
             "7369: filed 0, made 6270",
             // Filed: 8710 (195.50, not in BAS 2023's table) in övriga rörelseintäkter with 3999
             "7413: filed 67, made -129",
             // Filed: 84192.50 rounded down so that the resultaträkning adds up in whole kronor
             "7550: filed 84192, made 84193",
         },
         {
             "7750: filed 84192, made 84193", // 3.27, as above
             "7754: filed 0, made 5",         // Filed without 8314 Skattefria ränteintäkter (5.00)
             "7770: filed 176026, made 176032",
         },
         {"7114: filed 176026, made 176032"},
         {"Account 8710 has an amount -195.50 but is in no field of the mapping, nor is its #SRU 7418"},
         {own_accounts}});
}

TEST(Ink2Filed, Year2023_24) {
  check({"TheITfiedAB20240727_213539.se",
         filed_2024,
         filed_2023,
         {
             // Filed: 25024.84 rounded down so that the balansräkning adds up in whole kronor
             "7369: filed 25024, made 25025",
         },
         {},
         {},
         {},
         {own_accounts}});
}

TEST(Ink2Filed, Year2024_25) {
  check({"TheITfiedAB20250817_172409.se",
         filed_2025,
         filed_2024,
         {},
         {
             "7653: filed 0, made 1217", // Filed without 6992 Övriga externa kostnader, ej avdragsgilla (1217.00)
             "7754: filed 0, made 12",   // Filed without 8314 Skattefria ränteintäkter (12.00)
             "7770: filed 252665, made 251460",
         },
         {"7114: filed 252665, made 251460"},
         {},
         {own_accounts}});
}

TEST(Ink2Filed, TheLatest2024FilingCarriesLessDeficit) {
  // The 2024 filing of 2025-08-15, against the one made from the books and the 2022-23 return
  check({"TheITfiedAB20240727_213539.se",
         "deklaration 2024/The ITfied AB 556782-8172/blanketter.sru",
         filed_2023,
         {"7369: filed 25024, made 25025"},
         {"7763: filed 84193, made 176026", "7770: filed 114130, made 205963"},
         {"7114: filed 114130, made 205963"},
         {},
         {own_accounts}});
}
