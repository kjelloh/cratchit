#include "ui/design_insufficiency.hpp"

#include "core/log.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

TEST(DesignInsufficiencyState, IsInteAnForTheOptionAndSaysWhatIsMissing) {
  auto state = cratchit::ui::make_design_insufficiency_state("Ny bokföring", "att skapa en ny bokföring");
  EXPECT_EQ(state->name(), "Inte än (Ny bokföring)");
  EXPECT_EQ(state->view(),
            (pyroceph::tui::View{"Utvecklaren av cratchit har inte infört att skapa en ny bokföring ännu."}));
  EXPECT_TRUE(state->options().empty());
  EXPECT_EQ(state->on_key(U'1'), nullptr);
  EXPECT_EQ(state->on_key(U'-'), nullptr);
  EXPECT_FALSE(state->on_input(U"x"));
}

TEST(DesignInsufficiencyState, LogsADesignInsufficiencyWithTheCallersPlace) {
  auto const file = fs::temp_directory_path() / "cratchit_test_design_insufficiency" / "log.txt";
  fs::remove_all(file.parent_path());
  fs::create_directories(file.parent_path());
  ASSERT_TRUE(cratchit::log::open(file));
  auto const line = std::source_location::current().line() + 1;
  auto const state = cratchit::ui::make_design_insufficiency_state("Val", "att göra något");
  cratchit::log::close();
  std::ostringstream text;
  {
    std::ifstream in(file);
    text << in.rdbuf();
  } // closed before the folder is removed: Windows does not remove an open file
  EXPECT_NE(text.str().find(std::format(" DESIGN_INSUFFICIENCY test_design_insufficiency.cpp:{} Val: att göra något\n", line)),
            std::string::npos)
      << text.str();
  fs::remove_all(file.parent_path());
}
