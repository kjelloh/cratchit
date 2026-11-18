#include "pyroceph/tui/state.hpp"

#include <algorithm>

namespace pyroceph::tui {

InputResult State::on_input(std::u32string_view) { return std::unexpected(std::string(no_input_error)); }

Completion State::complete(std::u32string_view) const { return {}; }

Navigator::Navigator(std::unique_ptr<State> root) { path_.push_back(std::move(root)); }

bool Navigator::press(CodePoint key) {
  if (current().on_key_in_place(key)) return true;
  auto next = current().on_key(key);
  if (!next) return follow_back_to();
  enter(std::move(next));
  return true;
}

bool Navigator::follow_back_to() {
  auto const* target = current().back_to();
  if (target == nullptr || target == &current()) return false;
  auto const on_path = std::ranges::find_if(path_, [target](auto const& state) { return state.get() == target; });
  if (on_path == path_.end()) return false;
  auto next = current().after_back();
  while (path_.back().get() != target) path_.pop_back();
  current().on_return();
  if (next) enter(std::move(next));
  return true;
}

void Navigator::enter(std::unique_ptr<State> next) { path_.push_back(std::move(next)); }

bool Navigator::back() {
  if (!can_go_back()) return false;
  path_.pop_back();
  current().on_return();
  return true;
}

} // namespace pyroceph::tui
