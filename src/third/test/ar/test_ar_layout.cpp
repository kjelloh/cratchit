#include "core/ar/layout.hpp"

#include <gtest/gtest.h>

#include <algorithm>

#include <format>
#include <string>
#include <vector>

using cratchit::ar::LineKind;
using cratchit::ar::parse_account_set;
using cratchit::ar::parse_layout;

namespace {

// The accounts 1000-9999 in a set, written for comparing
std::string members(std::string_view text) {
  auto const set = parse_account_set(text);
  EXPECT_TRUE(set.has_value()) << (set ? "" : set.error());
  std::string out;
  for (int number = 1000; set && number <= 9999; ++number) {
    if (set->contains(std::to_string(number))) {
      out += std::to_string(number) + ' ';
    }
  }
  return out;
}

} // namespace

TEST(ArAccountSet, SingleAccount) {
  EXPECT_EQ(members("2099"), "2099 ");
}

TEST(ArAccountSet, Range) {
  EXPECT_EQ(members("1336-1337"), "1336 1337 ");
}

TEST(ArAccountSet, CommasAndEller) {
  EXPECT_EQ(members("2081, 2083 eller 2084"), "2081 2083 2084 ");
  EXPECT_EQ(members("1410-1411, 1430, 1431 eller 1438"), "1410 1411 1430 1431 1438 ");
}

TEST(ArAccountSet, ExclusionList) {
  EXPECT_EQ(members("8100-8105 (förutom 8101, 8102 och 8104-8105)"), "8100 8103 ");
}

TEST(ArAccountSet, ExclusionThenMore) {
  EXPECT_EQ(members("1570-1574 (förutom 1573) eller 1670-1674 (förutom 1673)"),
            "1570 1571 1572 1574 1670 1671 1672 1674 ");
  EXPECT_EQ(members("2490-2493 (förutom 2492), 2600-2601 eller 2880"), "2490 2491 2493 2600 2601 2880 ");
}

TEST(ArAccountSet, OnlyFourDigitAccounts) {
  auto const set = parse_account_set("1000-1999");
  ASSERT_TRUE(set);
  EXPECT_TRUE(set->contains("1930"));
  EXPECT_FALSE(set->contains("193"));
  EXPECT_FALSE(set->contains("19300"));
  EXPECT_FALSE(set->contains("19a0"));
}

TEST(ArAccountSet, Unreadable) {
  EXPECT_FALSE(parse_account_set(""));
  EXPECT_FALSE(parse_account_set("1000-"));
  EXPECT_FALSE(parse_account_set("1999-1000"));
  EXPECT_FALSE(parse_account_set("1000 1001"));
  EXPECT_FALSE(parse_account_set("1000 (utom 1001)"));
  EXPECT_FALSE(parse_account_set("1000-1010 (förutom 1001"));
  EXPECT_FALSE(parse_account_set("100"));
  EXPECT_FALSE(parse_account_set("2650 vid debetsaldo"));
}

TEST(ArLayout, Rows) {
  auto const lines = parse_layout("Nivå;Typ;BAS Range;ÅR\n1;Rubrik;;Resultaträkning\n3;Post;3000-3799;Nettoomsättning\n"
                                  "1;Summa;;Årets resultat\n;Utanför;8999;Bokslutspost\n");
  ASSERT_TRUE(lines) << lines.error();
  ASSERT_EQ(lines->size(), 4u);
  EXPECT_EQ((*lines)[0].kind, LineKind::heading);
  EXPECT_EQ((*lines)[1].kind, LineKind::item);
  EXPECT_EQ((*lines)[1].level, 3);
  EXPECT_TRUE((*lines)[1].accounts.contains("3308"));
  EXPECT_EQ((*lines)[2].kind, LineKind::sum);
  EXPECT_EQ((*lines)[3].kind, LineKind::outside);
  EXPECT_EQ((*lines)[3].level, 0);
}

TEST(ArLayout, Errors) {
  EXPECT_FALSE(parse_layout("BAS Range;ÅR\n3000;Nettoomsättning\n"));
  EXPECT_FALSE(parse_layout("Nivå;Typ;BAS Range;ÅR\n1;Okänd;;X\n"));
  EXPECT_FALSE(parse_layout("Nivå;Typ;BAS Range;ÅR\n1;Rubrik;3000;X\n"));
  EXPECT_FALSE(parse_layout("Nivå;Typ;BAS Range;ÅR\n;Post;3000;X\n"));
  EXPECT_FALSE(parse_layout("Nivå;Typ;BAS Range;ÅR\n3;Post;30x0;X\n"));
  EXPECT_FALSE(parse_layout("Nivå;Typ;BAS Range;ÅR\n1;Utanför;8999;X\n"));
}

namespace {

// Each built-in mapping, by its BAS year
class ArBuiltInLayout : public testing::TestWithParam<int> {
protected:
  std::string_view text() const { return cratchit::ar::layout_for(GetParam())->text; }
};

} // namespace

TEST_P(ArBuiltInLayout, Reads) {
  auto const lines = parse_layout(text());
  ASSERT_TRUE(lines) << lines.error();
  EXPECT_GT(lines->size(), 100u);
}

TEST_P(ArBuiltInLayout, PutsNoAccountOnTwoLines) {
  // At most one line in the resultaträkning and balansräkning, and at most one in the notes
  auto const lines = parse_layout(text());
  ASSERT_TRUE(lines);
  for (int number = 1000; number <= 9999; ++number) {
    auto const account = std::to_string(number);
    std::vector<std::string> texts;
    std::vector<std::string> note_texts;
    bool notes = false;
    for (auto const& line : *lines) {
      if (line.kind == LineKind::heading && line.level == 1) {
        notes = line.text == "Noter";
      }
      if (line.accounts.contains(account)) {
        (notes && line.kind != LineKind::outside ? note_texts : texts).push_back(line.text);
      }
    }
    EXPECT_LE(texts.size(), 1u) << account << " is on " << texts.front() << " and " << texts.back();
    EXPECT_LE(note_texts.size(), 1u) << account << " is on " << note_texts.front() << " and " << note_texts.back();
  }
}

TEST_P(ArBuiltInLayout, Note3SplitsItsPostExactly) {
  // The sections of Not 3 hold exactly the accounts of the post Inventarier, verktyg och installationer
  auto const lines = parse_layout(text());
  ASSERT_TRUE(lines);
  auto const post = std::ranges::find_if(*lines, [](auto const& line) {
    return line.kind == LineKind::item && line.text == "Inventarier, verktyg och installationer";
  });
  auto const note = std::ranges::find(*lines, "Not 3 Inventarier, verktyg och installationer", &cratchit::ar::Line::text);
  ASSERT_NE(post, lines->end());
  ASSERT_NE(note, lines->end());
  std::vector<cratchit::ar::Line const*> sections;
  for (auto at = note + 1; at != lines->end() && at->kind == LineKind::item; ++at) {
    sections.push_back(&*at);
  }
  ASSERT_EQ(sections.size(), 3u);
  for (int number = 1000; number <= 9999; ++number) {
    auto const account = std::to_string(number);
    auto const in_sections = std::ranges::count_if(sections, [&account](auto const* line) { return line->accounts.contains(account); });
    EXPECT_EQ(in_sections, post->accounts.contains(account) ? 1 : 0) << account;
  }
}

INSTANTIATE_TEST_SUITE_P(Years, ArBuiltInLayout, testing::Values(2024, 2026));

namespace {

// The line of the resultaträkning or balansräkning (not the notes) that the mapping for bas_year puts account on
std::string line_of(int bas_year, std::string_view account) {
  auto const lines = parse_layout(cratchit::ar::layout_for(bas_year)->text);
  EXPECT_TRUE(lines);
  bool notes = false;
  for (auto const& line : *lines) {
    if (line.kind == LineKind::heading && line.level == 1) {
      notes = line.text == "Noter";
    }
    bool const in_notes = notes && line.kind != LineKind::outside;
    if (!in_notes && line.accounts.contains(account)) {
      return line.text;
    }
  }
  return {};
}

} // namespace

TEST(ArLayout, BuiltInMappingsAreForBas2024And2026) {
  auto const& layouts = cratchit::ar::built_in_layouts();
  ASSERT_EQ(layouts.size(), 2u);
  EXPECT_EQ(layouts[0].year, 2024);
  EXPECT_EQ(layouts[0].file, "resources/ar/bas_to_ar_2024.csv");
  EXPECT_EQ(layouts[1].year, 2026);
  EXPECT_EQ(layouts[1].file, "resources/ar/bas_to_ar_2026.csv");
  EXPECT_EQ(cratchit::ar::default_bas_year, 2024);
  EXPECT_EQ(cratchit::ar::layout_text(), layouts[0].text);
}

TEST(ArLayout, ChosenByTheBasYearOfTheBooks) {
  EXPECT_FALSE(cratchit::ar::layout_for(2023));
  EXPECT_EQ(cratchit::ar::layout_for(2024)->year, 2024);
  EXPECT_EQ(cratchit::ar::layout_for(2025)->year, 2024);
  EXPECT_EQ(cratchit::ar::layout_for(2026)->year, 2026);
  EXPECT_EQ(cratchit::ar::layout_for(2030)->year, 2026);
  EXPECT_EQ(cratchit::ar::bas_years_of(*cratchit::ar::layout_for(2024)), "BAS 2024-2025");
  EXPECT_EQ(cratchit::ar::bas_years_of(*cratchit::ar::layout_for(2026)), "BAS 2026 and later");
}

TEST(ArLayout, BuiltInMappingsDifferOnlyByBas2026) {
  // The same headings, posts and sums in the same order, but for the BAS 2026 post Övriga immateriella
  // anläggningstillgångar and the text of the Utanför row for 8990-8999
  auto const shape = [](int bas_year) {
    auto const lines = parse_layout(cratchit::ar::layout_for(bas_year)->text);
    EXPECT_TRUE(lines);
    std::vector<std::string> out;
    for (auto const& line : *lines) {
      if (line.text == "Övriga immateriella anläggningstillgångar") {
        continue;
      }
      out.push_back(std::format("{} {} {}", line.level, static_cast<int>(line.kind),
                                line.kind == LineKind::outside ? "" : line.text));
    }
    return out;
  };
  EXPECT_EQ(shape(2024), shape(2026));
}

TEST(ArLayout, BuiltInMappingFollowsBas2024) {
  // The 2023 web mapping, BAS before 2026
  EXPECT_EQ(line_of(2024, "4010"), "Råvaror och förnödenheter");
  EXPECT_EQ(line_of(2024, "4960"), "Handelsvaror");
  EXPECT_EQ(line_of(2024, "1230"), "Inventarier, verktyg och installationer");
  EXPECT_EQ(line_of(2024, "1270"), "Inventarier, verktyg och installationer");
  EXPECT_EQ(line_of(2024, "1260"), "");
  EXPECT_EQ(line_of(2024, "1092"), "");
  EXPECT_EQ(line_of(2024, "8990"), "");
  EXPECT_EQ(line_of(2024, "8999"), "Årets resultat (bokslutspost, redan med i resultaträkningen)");
  // The K2 post names (BFNAR 2016:10 p. 3.5, 4.7) are the same in both
  EXPECT_EQ(line_of(2024, "1455"), "Färdiga varor");
  EXPECT_EQ(line_of(2024, "1465"), "Handelsvaror");
  EXPECT_EQ(line_of(2024, "2330"), "Kontokredit");
  EXPECT_EQ(line_of(2024, "1690"), "Tecknat men ej inbetalt kapital");
}

TEST(ArLayout, BuiltInMappingFollowsBas2026) {
  // Where BAS 2026 (BAS-kontogruppen, Bokföringsboken 2026) puts accounts the 2023 web mapping placed elsewhere
  // Groups 40-42 are Handelsvaror, 43-48 Råvaror och förnödenheter
  EXPECT_EQ(line_of(2026, "4010"), "Handelsvaror");
  EXPECT_EQ(line_of(2026, "4210"), "Handelsvaror");
  EXPECT_EQ(line_of(2026, "4960"), "Handelsvaror");
  EXPECT_EQ(line_of(2026, "4310"), "Råvaror och förnödenheter");
  EXPECT_EQ(line_of(2026, "4810"), "Råvaror och förnödenheter");
  EXPECT_EQ(line_of(2026, "4910"), "Råvaror och förnödenheter");
  EXPECT_EQ(line_of(2026, "4940"), "Förändring av lager av produkter i arbete, färdiga varor och pågående arbete för annans räkning");
  // Övriga immateriella anläggningstillgångar, e.g. tomträtter
  EXPECT_EQ(line_of(2026, "1092"), "Övriga immateriella anläggningstillgångar");
  // 1230 and 1240 are free accounts for maskiner, 1250 and 1260 for inventarier; 127x is gone
  EXPECT_EQ(line_of(2026, "1230"), "Maskiner och andra tekniska anläggningar");
  EXPECT_EQ(line_of(2026, "1249"), "Maskiner och andra tekniska anläggningar");
  EXPECT_EQ(line_of(2026, "1260"), "Inventarier, verktyg och installationer");
  EXPECT_EQ(line_of(2026, "1269"), "Inventarier, verktyg och installationer");
  EXPECT_EQ(line_of(2026, "1270"), "");
  // The Resultat group is outside the årsredovisning
  EXPECT_EQ(line_of(2026, "8990"), "Resultat (bokslutspost, redan med i resultaträkningen)");
  // Not allowed under K2, so on no line
  EXPECT_EQ(line_of(2026, "1010"), "");
  EXPECT_EQ(line_of(2026, "2240"), "");
}
