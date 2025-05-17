#include "FrameworkState.hpp"
#include "spdlog/spdlog.h"

namespace first {
  // ----------------------------------
  FrameworkState::FrameworkState(StateImpl::UX ux) : StateImpl{ux} {
    this->add_option('0',{"Workspace x",workspace_0_factory});        
  }

  // ----------------------------------
  FrameworkState::~FrameworkState() {
    spdlog::info("FrameworkState destructor executed");
  }

  // ----------------------------------
  std::pair<std::optional<State>,Cmd> FrameworkState::update(Msg const& msg) {
    std::optional<State> new_state{};
    auto key_msg_ptr = std::dynamic_pointer_cast<NCursesKey>(msg);
    if (key_msg_ptr != nullptr) {
      auto ch = key_msg_ptr->key;
      if (ch == '+') {
        this->m_ux.back().push_back('+');
        new_state = std::make_shared<FrameworkState>(*this);          
      }
    }
    return {new_state,Nop};
  }
}