#pragma once

#include "pyroceph/tui/state.hpp"

#include <memory>
#include <string_view>

namespace pyroceph::tui {

// The name of the quit state, shown in the path
inline constexpr std::string_view quit_state_name = "Avsluta";

// The help topic of the quit state (see State::help_topic)
inline constexpr std::string_view quit_help_topic = "avsluta";

// The key of the quit state that goes back without quitting
inline constexpr CodePoint stay_key = U'N';

// The safety net before the prompt loop ends: "Vill du avsluta programmet?"
// with the options quit_key "Avsluta" and stay_key "Nej". The prompt loop
// enters it on quit_key and ends when quit_key is pressed in it, so quit_key
// twice quits. stay_key, like back_key, goes back to before, the state the
// user came from (State::back_to).
std::unique_ptr<State> make_quit_state(State const* before);

// True when state is a quit state (make_quit_state)
bool is_quit_state(State const& state);

} // namespace pyroceph::tui
