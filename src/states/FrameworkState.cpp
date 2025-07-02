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
}