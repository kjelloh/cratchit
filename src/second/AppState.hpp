#pragma once
#include "ViewState.hpp"
#include "Transition.hpp"

class AppState {
public:
  AppState with_view_state(ViewState view_state) const;
  Transition<AppState> update(tea::Msg const& msg) const;
  tea::Ux view() const;
private:
  ViewState m_view_state{};
}; // AppState