#pragma once

#include "cross_dependent.hpp" // MsgImpl

namespace first {
  // ----------------------------------
  struct NCursesKey : public MsgImpl {
    int key;
    NCursesKey(int key);
  };

  // ----------------------------------
  struct Quit : public MsgImpl {};

  // ----------------------------------
  struct PushStateMsg : public MsgImpl {
    State m_parent{};
    State m_state{};
    PushStateMsg(State const &parent, State const &state);
  };

  // ----------------------------------
  struct PoppedStateCargoMsg : public MsgImpl {
    State m_top{};
    std::string m_cargo{};
    PoppedStateCargoMsg(State const &top, std::string cargo);
  };

  // ----------------------------------
  extern Msg const QUIT_MSG;

} // namespace first