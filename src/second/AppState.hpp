#pragma once
#include "ViewState.hpp"

enum class TransitionKind {
   Undefined
  ,Push
  ,Mutate
  ,Ignore
  ,Accept
  ,Reject
  ,Unknown
}; // TransitionKind

template<class S>
class Transition {
public:
  Transition(TransitionKind kind, S const& next_state)
  : m_kind(kind)
  , m_next_state(next_state) {}

  TransitionKind kind() const { return m_kind; }
  S const& next_state() const { return m_next_state; }
private:
  TransitionKind m_kind{TransitionKind::Undefined};
  S m_next_state{};
}; // Transition

class AppState {
public:
  AppState with_view_state(ViewState view_state) const;
  Transition<AppState> update(tea::Msg const& msg) const;
private:
  ViewState m_view_state{};
}; // AppState