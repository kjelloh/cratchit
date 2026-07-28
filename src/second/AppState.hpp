#pragma once
#include "Msg.hpp"

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
private:
  TransitionKind m_kind{TransitionKind::Undefined};
  S m_next_state{};
}; // Transition

class AppState {
public:
  Transition<AppState> update(tea::Msg const& msg) const;
private:
}; // AppState