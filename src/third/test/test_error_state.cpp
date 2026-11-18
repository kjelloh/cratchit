#include "pyroceph/tui/error_state.hpp"

#include "menu_state.hpp"

#include <gtest/gtest.h>

using pyroceph::tui::make_error_state;
using pyroceph::tui::Navigator;
using pyroceph::tui::View;
using test::menu;

TEST(ErrorState, ShowsTheMessageAsItsViewAndHasNoOptions) {
  auto error = make_error_state("Kunde inte öppna samlingsmappen.");
  EXPECT_EQ(error->name(), "Fel");
  EXPECT_EQ(error->view(), (View{"Kunde inte öppna samlingsmappen."}));
  EXPECT_TRUE(error->options().empty());
}

TEST(ErrorState, MapsNoKeysAndTakesNoInput) {
  auto error = make_error_state("x");
  EXPECT_EQ(error->on_key(U'1'), nullptr);
  EXPECT_EQ(error->on_key(pyroceph::tui::back_key), nullptr);
  auto const input = error->on_input(U"x");
  ASSERT_FALSE(input);
  EXPECT_EQ(input.error(), pyroceph::tui::no_input_error);
}

TEST(ErrorState, BackLeadsToTheStateBefore) {
  Navigator navigator(menu("Rot", {{U'1', "Öppna", [] { return make_error_state("Gick inte."); }}}));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Fel");
  EXPECT_FALSE(navigator.press(U'-'));
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Rot");
}
