#include "CmdHandler.hpp"
#include "cmd_to_msg.hpp" // client provided binding descriptor -> result_type -> Msg

class CmdHandler::Impl {
public:
  Impl();
  std::vector<tea::Msg> poll();
private:
}; // CmdHandler::Impl

CmdHandler::Impl::Impl() {

}

std::vector<tea::Msg> CmdHandler::Impl::poll() {
  return {};
}

CmdHandler::CmdHandler() {}
std::vector<tea::Msg> CmdHandler::poll() {
  return this->m_pimpl->poll();
}
