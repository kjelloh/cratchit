#include "core/ink2/check.hpp"
#include "core/ink2/report.hpp"

#include <gtest/gtest.h>

#include <algorithm>

using cratchit::Balances;
using cratchit::ink2::check_return;
using cratchit::ink2::Previous;

namespace {

std::vector<cratchit::ink2::Line> const& mapping() {
  static auto const lines = *cratchit::ink2::parse_mapping(cratchit::ink2::mapping_text());
  return lines;
}

Previous const from_file{1000, "last.sru", {"8041", "8045"}, std::nullopt};

// Each item's number and whether its title starts as expected, e.g. "2 4.6a"
std::vector<std::string> titles(std::vector<cratchit::ink2::CheckItem> const& items,
                                std::vector<std::string> const& starts) {
  std::vector<std::string> out;
  for (std::size_t index = 0; index < items.size(); ++index) {
    auto const& title = items[index].title;
    out.push_back(std::format("{} {}", items[index].number,
                              index < starts.size() && title.starts_with(starts[index]) ? starts[index] : title));
  }
  return out;
}

} // namespace

TEST(Ink2Check, CompleteWhenTheBooksTellAll) {
  Balances const balances{{"1930", 10000}, {"2081", -10000}, {"6110", 500}, {"8999", -500}};
  auto const check = check_return(cratchit::ink2::build_form(mapping(), balances), {}, balances, from_file, false,
                                  std::nullopt, true);
  EXPECT_TRUE(check.missing.empty());
  EXPECT_TRUE(check.markers().empty());
  EXPECT_EQ(titles(check.to_confirm, {"4.4b", "4.13", "4.14b-c", "4.20"}),
            (std::vector<std::string>{"0 4.4b", "0 4.13", "0 4.14b-c", "0 4.20"}));
}

TEST(Ink2Check, WhatTheBooksShowIsMissing) {
  // Books not closed, a periodiseringsfond at the start, a nedskrivning on 3.17, pension costs
  Balances const opening{{"2125", -500000}};
  Balances const balances{{"2125", -500000}, {"8072", 10000}, {"7411", 20000}};
  auto const form = cratchit::ink2::build_form(mapping(), balances);
  auto const check = check_return(form, opening, balances, Previous{}, true, -30000, false);
  EXPECT_EQ(titles(check.missing, {"Bokslut", "4.6a", "4.3b", "4.21", "Uppdragstagare", "INFO.SRU"}),
            (std::vector<std::string>{"1 Bokslut", "2 4.6a", "3 4.3b", "4 4.21", "5 Uppdragstagare", "6 INFO.SRU"}));
  auto const markers = check.markers();
  EXPECT_EQ(markers.at("7550"), "TBD [1]");
  EXPECT_EQ(markers.at("7654"), "TBD [2]");
  EXPECT_EQ(markers.at("7652"), "TBD [3]");
  EXPECT_EQ(markers.at("8022"), "TBD [4]");
  EXPECT_EQ(markers.at("8041"), "TBD [5]");
  EXPECT_EQ(check.missing[1].found, "periodiseringsfonder (2110-2139) of 5000.00 at the start of the year");
  // A deficit typed is to confirm
  EXPECT_EQ(check.to_confirm.back().title, "4.14a Outnyttjat underskott från föregående år");
  EXPECT_EQ(check.to_confirm.back().found, "0 kr, given by --previous-deficit");
}

TEST(Ink2Check, Text) {
  Balances const balances{{"1930", 100}, {"2081", -100}};
  auto const check = check_return(cratchit::ink2::build_form(mapping(), balances), {}, balances, Previous{}, false,
                                  std::nullopt, true);
  auto const text = cratchit::ink2::format_check(check);
  EXPECT_TRUE(text.starts_with("INK2 check\n==========\n\nNot complete: 1 item is missing, marked TBD [n] in the "
                               "return. While any is missing the SRU files are not written.\n\n[1] Uppdragstagare"))
      << text;
  EXPECT_NE(text.find("(fält 8040, 8041, 8044, 8045)"), std::string::npos);
  EXPECT_NE(text.find("\nTo confirm: these may apply, and the books cannot tell.\n\n- 4.4b"), std::string::npos);
}

TEST(Ink2Report, Text) {
  cratchit::ink2::ReportHeader const header{"x_ink2_fält.txt", "2026-11-03 18:04", "cratchit_ink2 0.1.0", "books.se",
                                            "x.se", "#PROGRAM Fortnox 3.57", "4.16 Underskott 0 kr of last.sru",
                                            "a.csv and b.csv", "complete", "BLANKETTER.SRU and INFO.SRU written"};
  EXPECT_EQ(cratchit::ink2::format_report(header, {}, {"Closing: a note"}, "INK2 check\n"),
            "Report for x_ink2_fält.txt\n"
            "==========================\n"
            "\n"
            "Generated: 2026-11-03 18:04 by cratchit_ink2 0.1.0\n"
            "SIE file:  books.se, copied as x.se\n"
            "           #PROGRAM Fortnox 3.57\n"
            "Last year: 4.16 Underskott 0 kr of last.sru\n"
            "Mapping:   a.csv and b.csv\n"
            "Status:    complete\n"
            "SRU files: BLANKETTER.SRU and INFO.SRU written\n"
            "\n"
            "Errors\n"
            "------\n"
            "None.\n"
            "\n"
            "Notes\n"
            "-----\n"
            "Closing: a note\n"
            "\n"
            "INK2 check\n");
}
