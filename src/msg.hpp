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

  struct PopStateMsg : public MsgImpl {
    PopStateMsg() = default;
  };

  // ----------------------------------
  struct PoppedStateCargoMsg : public MsgImpl {
    Cargo m_cargo{};
    PoppedStateCargoMsg(Cargo const& cargo);
  };

  struct UserEntryMsg : public MsgImpl {
    std::string m_entry;
    UserEntryMsg(std::string entry);
  };

  // ----------------------------------
  extern Msg const QUIT_MSG;

} // namespace first