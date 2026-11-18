#pragma once

#include "pyroceph/tui/state.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace pyroceph::tui {

// The name of the error state, shown in the path
inline constexpr std::string_view error_state_name = "Fel";

// The help topic of the error state (see State::help_topic)
inline constexpr std::string_view error_help_topic = "fel";

// A state that only shows an error message as its view. It has no options and
// maps no keys, so only the common keys work there and - goes back as usual.
//
// A state returns it from on_key when the state the key maps to cannot be
// entered. So a key is either not mapped (nullptr) or always leads to a new
// state. message is UTF-8.
std::unique_ptr<State> make_error_state(std::string message);

} // namespace pyroceph::tui
