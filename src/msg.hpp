#pragma once

#include "cross_dependent.hpp" // MsgImpl
#include "cargo/CargoBase.hpp"

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
    State m_state{};
    PushStateMsg(State const &state);
  };

  // ----------------------------------
  struct PoppedStateCargoMsg : public MsgImpl {
    State m_top{};
    Cargo m_cargo{};
    PoppedStateCargoMsg(State const &top, Cargo&& cargo);
  };

  struct UserEntryMsg : public MsgImpl {
    std::string m_entry;
    UserEntryMsg(std::string entry);
  };

  // ----------------------------------
  extern Msg const QUIT_MSG;

} // namespace first