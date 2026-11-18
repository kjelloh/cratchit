// The common closing module (core/closing, see test_closing.cpp) as cratchit_ar
// uses it: the årsredovisning of books not yet closed
#include "core/ar/layout.hpp"
#include "core/ar/notes.hpp"
#include "core/ar/overview.hpp"
#include "core/ar/requirements.hpp"
#include "core/ar/statement.hpp"
#include "core/balances.hpp"
#include "core/closing.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

using namespace std::chrono;

namespace {

// Opening: 100 kr in the bank against share capital. In the year: 30 kr of costs paid from the bank.
cratchit::sie::Document unclosed_books() {
  cratchit::sie::Document document;
  document.years = {{0, 2025y / May / 1, 2026y / April / 30}};
  document.opening = {{0, "1930", 10000, ""}, {0, "2081", -10000, ""}};
  document.closing = {{0, "1930", 7000, ""}, {0, "2081", -10000, ""}};
  document.results = {{0, "6110", 3000, ""}};
  return document;
}

// The notes for years 0 and -1 of one of the user's SIE files (not in the repository), or nothing when missing
std::optional<std::vector<std::string>> notes_of_file(char const* name) {
  auto const sie = std::filesystem::path(CRATCHIT_AR_SIE_FOLDER) / name;
  if (!std::filesystem::exists(sie)) {
    return std::nullopt;
  }
  auto const lexed = cratchit::sie::lex_file(sie);
  EXPECT_TRUE(lexed);
  auto const document = cratchit::sie::parse_document(lexed->lines).document;
  std::vector<std::string> notes;
  for (int index = 0; index >= -1; --index) {
    for (auto const& note : cratchit::check_closing(cratchit::year_balances(document, index), document.year(index)->end)) {
      notes.push_back(note.substr(0, note.find(';')));
    }
  }
  return notes;
}

} // namespace

TEST(ArClosing, SimulatedClosingEntryBalancesTheStatement) {
  auto const document = cratchit::with_closing_entry(unclosed_books(), 3000);
  auto const layout = cratchit::ar::parse_layout(cratchit::ar::layout_text());
  ASSERT_TRUE(layout);
  auto const statement = cratchit::ar::build_statement(*layout, {cratchit::year_balances(document, 0)});
  EXPECT_TRUE(statement.errors.empty());
  EXPECT_TRUE(cratchit::ar::check_statement(statement).empty());
}

TEST(ArClosing, UnclosedFile) {
  // The user's books for 2025-26, exported before closing (not in the repository)
  auto const sie = std::filesystem::path(CRATCHIT_AR_SIE_FOLDER) / "sie_2025-26_all.se";
  if (!std::filesystem::exists(sie)) {
    GTEST_SKIP() << "No " << sie;
  }
  auto const lexed = cratchit::sie::lex_file(sie);
  ASSERT_TRUE(lexed);
  auto const original = cratchit::sie::parse_document(lexed->lines).document;
  auto const unbooked = cratchit::unbooked_result(original);
  ASSERT_TRUE(unbooked);
  EXPECT_EQ(*unbooked, 2498949);
  auto const document = cratchit::with_closing_entry(original, *unbooked);
  auto const layout = cratchit::ar::parse_layout(cratchit::ar::layout_text());
  ASSERT_TRUE(layout);
  auto const statement = cratchit::ar::build_statement(
      *layout, {cratchit::year_balances(document, 0), cratchit::year_balances(document, -1)});
  EXPECT_TRUE(statement.errors.empty());
  EXPECT_TRUE(cratchit::ar::check_statement(statement).empty());
  auto const check = cratchit::ar::check_requirements(statement, cratchit::ar::build_overview(statement),
                                                      cratchit::ar::build_notes(statement, document), unbooked);
  EXPECT_EQ(check.marker("bokslut"), "TBD [1]");
  auto const notes = notes_of_file("sie_2025-26_all.se");
  ASSERT_TRUE(notes);
  EXPECT_NE(std::ranges::find(*notes, "No depreciation (7800-7899) in the year ending 2026-04-30, although 1229 "
                                      "(accumulated depreciation) is -278114.96"),
            notes->end());
  EXPECT_NE(std::ranges::find(*notes, "2098 Vinst eller förlust från föregående år is 46701.78 at 2026-04-30"),
            notes->end());
}
