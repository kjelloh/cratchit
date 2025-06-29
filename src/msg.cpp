#include "msg.hpp"

namespace first {
  // ----------------------------------
  NCursesKey::NCursesKey(int key) : key{key}, MsgImpl{} {}

  // ----------------------------------
  PushStateMsg::PushStateMsg(State const &state)
      : m_state{state}, MsgImpl{} {}

  // ----------------------------------
  PoppedStateCargoMsg::PoppedStateCargoMsg(State const &top, Cargo&& cargo)
      : m_top{top}, m_cargo{std::move(cargo)}, MsgImpl{} {}


  UserEntryMsg::UserEntryMsg(std::string entry)
    : m_entry{entry} {}

  // ----------------------------------
  Msg const QUIT_MSG{std::make_shared<Quit>()};

} // namespace first