#include "FrameworkState.hpp"
#include "WorkspaceState.hpp"
#include "spdlog/spdlog.h"

namespace first {

  // ----------------------------------
  FrameworkState::FrameworkState(StateImpl::UX ux, std::filesystem::path root_path)
      :  StateImpl{ux}
        ,m_root_path{root_path} { 
    this->m_ux.push_back(m_root_path.string());

    this->add_cmd_option('0', WorkspaceState::cmd_option_from(m_root_path));
  }

  StateImpl::UpdateOptions FrameworkState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    // TODO: Refactor add_update_option in constructor to update options here
    // TODO: Refactor add_cmd_option in constructor to update options here
    return result;
  }

}