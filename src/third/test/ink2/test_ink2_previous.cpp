#include "core/ink2/previous.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <fstream>

using cratchit::ink2::orgnr_digits;
using cratchit::ink2::previous_from_sru;
using namespace std::chrono;

namespace {

constexpr cratchit::Date start = 2025y / May / 1;

// Last year's return as Fortnox/edeklarera writes it
std::vector<cratchit::SruForm> last_year(std::string_view deficit = "#UPPGIFT 7770 205963\n") {
  auto const text = std::format("#BLANKETT INK2R-2025P1\n"
                                "#IDENTITET 556782-8172 20250830 120000\n"
                                "#UPPGIFT 7011 20240501\n"
                                "#UPPGIFT 7012 20250430\n"
                                "#UPPGIFT 7281 124896\n"
                                "#UPPGIFT 7369 270\n"
                                "#BLANKETTSLUT\n"
                                "#BLANKETT INK2S-2025P1\n"
                                "#IDENTITET 556782-8172 20250830 120000\n"
                                "#UPPGIFT 7011 20240501\n"
                                "#UPPGIFT 7012 20250430\n"
                                "{}"
                                "#UPPGIFT 8041 X\n"
                                "#UPPGIFT 8045 X\n"
                                "#BLANKETTSLUT\n"
                                "#FIL_SLUT\n",
                                deficit);
  return *cratchit::parse_sru(text);
}

} // namespace

TEST(Ink2Previous, OrgnrDigits) {
  EXPECT_EQ(orgnr_digits("556782-8172"), "5567828172");
  EXPECT_EQ(orgnr_digits("165567828172"), "5567828172");
  EXPECT_EQ(orgnr_digits("5567828172"), "5567828172");
}

TEST(Ink2Previous, FromLastYearsReturn) {
  auto const previous = previous_from_sru(last_year(), "last.sru", "556782-8172", start);
  ASSERT_TRUE(previous) << previous.error();
  EXPECT_EQ(previous->deficit, 205963);
  EXPECT_EQ(previous->source, "last.sru");
  EXPECT_EQ(previous->marks, (std::vector<std::string>{"8041", "8045"}));
  ASSERT_TRUE(previous->ink2r);
  EXPECT_EQ(previous->ink2r->fields.at("7281"), "124896");
  // The orgnr as older SRU files write it
  EXPECT_TRUE(previous_from_sru(last_year(), "last.sru", "165567828172", start));
}

TEST(Ink2Previous, NoDeficitAfterAnOverskott) {
  auto const previous = previous_from_sru(last_year("#UPPGIFT 7670 1000\n"), "last.sru", "556782-8172", start);
  ASSERT_TRUE(previous);
  EXPECT_EQ(previous->deficit, 0);
}

TEST(Ink2Previous, Wrong) {
  EXPECT_EQ(previous_from_sru(last_year(), "last.sru", "556000-0001", start).error(),
            "Last year's return last.sru: orgnr 556782-8172, not 556000-0001");
  EXPECT_EQ(previous_from_sru(last_year(), "last.sru", "556782-8172", 2026y / May / 1).error(),
            "Last year's return last.sru: its fiscal year ends 20250430, not 20260430 (the day before this one "
            "starts)");
  EXPECT_EQ(previous_from_sru({}, "last.sru", "556782-8172", start).error(), "Last year's return last.sru: no INK2S");
  EXPECT_EQ(previous_from_sru(last_year("#UPPGIFT 7770 12x\n"), "last.sru", "556782-8172", start).error(),
            "Last year's return last.sru: 4.16 (7770) \"12x\" is not an amount");
}

TEST(Ink2Previous, SruFileInAFolder) {
  auto const folder = std::filesystem::temp_directory_path() / "cratchit_ink2_test_previous";
  std::filesystem::remove_all(folder);
  std::filesystem::create_directories(folder);
  EXPECT_EQ(cratchit::ink2::sru_file_in(folder).error(), std::format("No BLANKETTER.SRU in {}", folder.string()));
  std::ofstream(folder / "Blanketter.sru") << "#FIL_SLUT\n";
  EXPECT_EQ(cratchit::ink2::sru_file_in(folder), folder / "Blanketter.sru");
  EXPECT_EQ(cratchit::ink2::sru_file_in(folder / "Blanketter.sru"), folder / "Blanketter.sru");
  EXPECT_FALSE(cratchit::ink2::sru_file_in(folder / "missing"));
  std::filesystem::remove_all(folder);
}

TEST(Ink2Previous, OpeningAgainstLastYearsINK2R) {
  cratchit::ink2::Form opening;
  opening.fields["7281"] = {true, 12489600, 124896, {}};
  opening.fields["7369"] = {true, -2704000, 271, {}};
  opening.fields["7302"] = {false, 0, 0, {}};
  EXPECT_EQ(cratchit::ink2::check_opening(opening, last_year().front()),
            (std::vector<std::string>{"Opening balance: 7369 is 271 kr by this year's #IB but 270 kr in last year's INK2R"}));
}
