#pragma once

#include "cross_dependent.hpp" // MsgImpl
#include "cargo/CargoBase.hpp"

namespace first {
  // ----------------------------------
  struct NCursesKeyMsg : public MsgImpl {
    // int key;
    // NCursesKeyMsg(int key);

    // cratchit processes key-input as unicode code points.
    // Also see NCursesHead::get_input() for UTF-8 -> unicode_int_code_point
    int unicode_int_code_point;
    NCursesKeyMsg(int unicode_int_code_point);
  };

  // ----------------------------------
  struct Quit : public MsgImpl {};

  // ----------------------------------
  template<class Payload>
  struct CargoMsgT : public MsgImpl {
    Payload payload;
    CargoMsgT(Payload const& payload) : payload(payload) {}
  };
  
  // ----------------------------------
  // struct PoppedStateCargoMsg : public MsgImpl {
  //   Cargo m_cargo{};
  //   PoppedStateCargoMsg(Cargo const& cargo);
  // };

  // ----------------------------------
  struct UserEntryMsg : public MsgImpl {
    std::string m_entry;
    UserEntryMsg(std::string entry);
  };

  // ----------------------------------
  struct PushStateMsg : public MsgImpl {
    State m_state{};
    PushStateMsg(State const &state);
  };

  // ----------------------------------
  struct PopStateMsg : public MsgImpl {
    Msg m_maybe_null_cargo_msg{};
    PopStateMsg() = default;
    PopStateMsg(Msg const& cargo_msg);
  };


  // ----------------------------------
  extern Msg const QUIT_MSG;

} // namespace first