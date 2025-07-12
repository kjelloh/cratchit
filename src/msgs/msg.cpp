#include "msg.hpp"

namespace first {
  // ----------------------------------
  NCursesKeyMsg::NCursesKeyMsg(int key) : key{key}, MsgImpl{} {}

  // ----------------------------------
  PushStateMsg::PushStateMsg(State const &state)
      :  MsgImpl{}
        ,m_state{state} {}

  // ----------------------------------
  PoppedStateCargoMsg::PoppedStateCargoMsg(Cargo const& cargo)
      :  MsgImpl{}
        ,m_cargo{cargo} {}

  UserEntryMsg::UserEntryMsg(std::string entry)
    : m_entry{entry} {}

  PopStateMsg::PopStateMsg(Msg const& cargo_msg)
    :  MsgImpl{}
      ,m_maybe_null_cargo_msg{cargo_msg} {}


  // ----------------------------------
  Msg const QUIT_MSG{std::make_shared<Quit>()};

} // namespace first