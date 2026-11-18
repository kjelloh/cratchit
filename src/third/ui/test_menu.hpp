#pragma once

#include "pyroceph/tui/state.hpp"

#include <memory>

namespace cratchit::ui {

// The test menu: the first small UI, kept to remember how State and navigation
// are meant to work. Projekt > Öppna projekt, Projekt > Öppna (misslyckas),
// which leads to an error state, and Bokslut, which takes a year of four
// digits as input. Reached by 9 from the root.
std::unique_ptr<pyroceph::tui::State> make_test_menu();

} // namespace cratchit::ui
