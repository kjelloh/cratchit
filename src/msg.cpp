#include "msg.hpp"

namespace first {
  // ----------------------------------
  NCursesKey::NCursesKey(int key) : key{key}, MsgImpl{} {}

  // ----------------------------------
  PushStateMsg::PushStateMsg(State const &parent, State const &state)
      : m_parent{parent}, m_state{state}, MsgImpl{} {}

  // ----------------------------------
  PoppedStateCargoMsg::PoppedStateCargoMsg(State const &top, std::string cargo)
      : m_top{top}, m_cargo{cargo}, MsgImpl{} {}

  UserEntryMsg::UserEntryMsg(std::string entry)
    : m_entry{entry} {}

  // ----------------------------------
  Msg const QUIT_MSG{std::make_shared<Quit>()};

} // namespace first