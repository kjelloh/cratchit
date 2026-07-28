#include "AppState.hpp"

Transition<AppState> AppState::update(tea::Msg const&) const {
  return Transition<AppState>(TransitionKind::Ignore,*this);
}
