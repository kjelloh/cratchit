#include "pyroceph/tui/error_state.hpp"

#include <utility>

namespace pyroceph::tui {

namespace {

class ErrorState : public State {
public:
  explicit ErrorState(std::string message) : message_(std::move(message)) {}

  std::string name() const override { return std::string(error_state_name); }
  View view() const override { return {message_}; }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::string help_topic() const override { return std::string(error_help_topic); }

private:
  std::string message_;
};

} // namespace

std::unique_ptr<State> make_error_state(std::string message) {
  return std::make_unique<ErrorState>(std::move(message));
}

} // namespace pyroceph::tui
