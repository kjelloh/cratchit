#include "core/terminal_encoding.hpp"

#include <gtest/gtest.h>

using cratchit::Charset;
using cratchit::choose_terminal_encoding;
using cratchit::Reported;
using cratchit::ReportedTerminal;

TEST(TerminalEncoding, NamesFromLocales) {
  EXPECT_EQ(cratchit::charset_from_name("UTF-8"), Charset::Utf8);
  EXPECT_EQ(cratchit::charset_from_name("utf8"), Charset::Utf8);
  EXPECT_EQ(cratchit::charset_from_name("ANSI_X3.4-1968"), Charset::Ascii);
  EXPECT_EQ(cratchit::charset_from_name("US-ASCII"), Charset::Ascii);
  EXPECT_EQ(cratchit::charset_from_name("ISO-8859-1"), Charset::Iso8859_1);
  EXPECT_EQ(cratchit::charset_from_name("ISO8859-15"), Charset::Iso8859_15);
  EXPECT_EQ(cratchit::charset_from_name("GB18030"), Charset::Gb18030);
  EXPECT_EQ(cratchit::charset_from_name("gb2312"), Charset::Gbk);
  EXPECT_EQ(cratchit::charset_from_name("Big5"), Charset::Big5);
  EXPECT_EQ(cratchit::charset_from_name("Shift_JIS"), Charset::ShiftJis);
  EXPECT_EQ(cratchit::charset_from_name("eucJP"), Charset::EucJp);
  EXPECT_EQ(cratchit::charset_from_name("KOI8-R"), Charset::Unknown);
  EXPECT_EQ(cratchit::charset_from_name(""), Charset::Unknown);
}

TEST(TerminalEncoding, WindowsCodePages) {
  EXPECT_EQ(cratchit::charset_from_code_page(65001), Charset::Utf8);
  EXPECT_EQ(cratchit::charset_from_code_page(437), Charset::Cp437);
  EXPECT_EQ(cratchit::charset_from_code_page(850), Charset::Cp850);
  EXPECT_EQ(cratchit::charset_from_code_page(1252), Charset::Cp1252);
  EXPECT_EQ(cratchit::charset_from_code_page(936), Charset::Gbk);
  EXPECT_EQ(cratchit::charset_from_code_page(54936), Charset::Gb18030);
  EXPECT_EQ(cratchit::charset_from_code_page(932), Charset::ShiftJis);
  EXPECT_EQ(cratchit::charset_from_code_page(866), Charset::Unknown);
}

TEST(TerminalEncoding, Utf8IsUsedWithoutWarning) {
  Reported const utf8{"UTF-8", Charset::Utf8};
  auto const chosen = choose_terminal_encoding({utf8, utf8});
  EXPECT_EQ(chosen.in, Charset::Utf8);
  EXPECT_EQ(chosen.out, Charset::Utf8);
  EXPECT_TRUE(chosen.warnings.empty());
}

TEST(TerminalEncoding, AdaptsInEachDirectionOnItsOwn) {
  // CP437 can be read but not yet written
  auto const chosen = choose_terminal_encoding({{"code page 437", Charset::Cp437}, {"code page 437", Charset::Cp437}});
  EXPECT_EQ(chosen.in, Charset::Cp437);
  EXPECT_EQ(chosen.out, Charset::Utf8);
  ASSERT_EQ(chosen.warnings.size(), 1u);
  EXPECT_EQ(chosen.warnings[0], "Varning: cratchit stöder inte terminalens teckenkodning för utmatning, code page 437 (CP437), än. cratchit använder UTF-8.");
}

TEST(TerminalEncoding, UnsupportedFallsBackToUtf8WithWarning) {
  Reported const gb18030{"GB18030", Charset::Gb18030};
  auto const chosen = choose_terminal_encoding({gb18030, gb18030});
  EXPECT_EQ(chosen.in, Charset::Utf8);
  EXPECT_EQ(chosen.out, Charset::Utf8);
  ASSERT_EQ(chosen.warnings.size(), 2u);
  EXPECT_EQ(chosen.warnings[0], "Varning: cratchit stöder inte terminalens teckenkodning för inmatning, GB18030 (GB18030), än. cratchit använder UTF-8.");
  EXPECT_EQ(chosen.warnings[1], "Varning: cratchit stöder inte terminalens teckenkodning för utmatning, GB18030 (GB18030), än. cratchit använder UTF-8.");
}

TEST(TerminalEncoding, UnknownFallsBackToUtf8WithWarning) {
  Reported const koi8{"KOI8-R", Charset::Unknown};
  auto const chosen = choose_terminal_encoding({koi8, koi8});
  EXPECT_EQ(chosen.in, Charset::Utf8);
  ASSERT_EQ(chosen.warnings.size(), 2u);
  EXPECT_EQ(chosen.warnings[0], "Varning: cratchit känner inte igen terminalens teckenkodning för inmatning, KOI8-R. cratchit använder UTF-8.");
}

TEST(TerminalEncoding, ThisPlatformReportsSomething) {
  auto const reported = cratchit::report_terminal_encoding();
  EXPECT_FALSE(reported.in.as_reported.empty());
  EXPECT_FALSE(reported.out.as_reported.empty());
}
