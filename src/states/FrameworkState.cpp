#include "FrameworkState.hpp"
#include "WorkspaceState.hpp"
#include "spdlog/spdlog.h"

namespace first {

  // ----------------------------------
  FrameworkState::FrameworkState(StateImpl::UX ux, std::filesystem::path root_path)
      :  StateImpl{ux}
        ,m_root_path{root_path} { 
    StateFactory workspace_0_factory = [this]() {
      auto workspace_0_ux = StateImpl::UX{"Workspace UX"};
      std::filesystem::path workspace_0_path = this->m_root_path;;
      return std::make_shared<WorkspaceState>(workspace_0_ux,workspace_0_path);
    };

    this->m_ux.push_back(m_root_path.string());

    this->add_option('0', {"Workspace x", workspace_0_factory});
  }
}