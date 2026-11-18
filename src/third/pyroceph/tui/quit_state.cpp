#include "pyroceph/tui/quit_state.hpp"

#include "pyroceph/tui/prompt_loop.hpp"

#include <string>

namespace pyroceph::tui {

namespace {

class QuitState : public State {
public:
  explicit QuitState(State const* before) : before_(before) {}
  std::string name() const override { return std::string(quit_state_name); }
  View view() const override { return {"Vill du avsluta programmet?"}; }
  OptionsList options() const override { return {{quit_key, "Avsluta"}, {stay_key, "Nej"}}; }
  // quit_key is left to the prompt loop, which ends here; stay_key goes back (back_to)
  OnKeyResult on_key(CodePoint key) override {
    if (key == stay_key) staying_ = true;
    return nullptr;
  }
  State const* back_to() const override { return staying_ ? before_ : nullptr; }
  std::string help_topic() const override { return std::string(quit_help_topic); }

private:
  State const* before_;
  bool staying_ = false;
};

} // namespace

std::unique_ptr<State> make_quit_state(State const* before) { return std::make_unique<QuitState>(before); }

bool is_quit_state(State const& state) { return dynamic_cast<QuitState const*>(&state) != nullptr; }

} // namespace pyroceph::tui
