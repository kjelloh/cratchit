#include "core/version.hpp"

#include <gtest/gtest.h>

using Lines = std::vector<std::string>;

TEST(StartupLines, ShowBuildTimeProgramAndVersionOnePerLine) {
  auto const lines = cratchit::startup_lines("0.1.0", "Debug", "14:32:05 2026-11-15",
                                             std::filesystem::path("/opt/bin/cratchit"));
  EXPECT_EQ(lines, (Lines{"Byggd: 14:32:05 2026-11-15 (Debug)", "Från: /opt/bin/cratchit", "Version: 0.1.0"}));
}

TEST(StartupLines, LeaveOutTheProgramWhenUnknown) {
  auto const lines = cratchit::startup_lines("0.1.0", "Release", "14:32:05 2026-11-15", std::nullopt);
  EXPECT_EQ(lines, (Lines{"Byggd: 14:32:05 2026-11-15 (Release)", "Version: 0.1.0"}));
}

TEST(Version, IsTheProjectVersion) {
  EXPECT_FALSE(cratchit::version().empty());
}
