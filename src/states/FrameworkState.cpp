#include "FrameworkState.hpp"
#include "WorkspaceState.hpp"
#include "spdlog/spdlog.h"

namespace first {

  // ----------------------------------
  FrameworkState::FrameworkState(StateImpl::UX ux, std::filesystem::path root_path)
      :  StateImpl{ux}
        ,m_root_path{root_path} { 
    // UX creation moved to create_ux()
    // this->m_ux.push_back(m_root_path.string());

    // Moved to create_update_options()
    // this->add_cmd_option('0', WorkspaceState::cmd_option_from(m_root_path));
  }

  StateImpl::UpdateOptions FrameworkState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    
    // Convert WorkspaceState::cmd_option_from to update option
    auto caption = m_root_path.filename().string();
    if (caption.empty()) {
      caption = m_root_path.string(); // Use full path if filename is empty
    }
    
    result.add('0', {caption, [root_path = m_root_path]() -> StateUpdateResult {
      return {std::nullopt, [root_path]() -> std::optional<Msg> {
        auto workspace_ux = StateImpl::UX{"Workspace UX"};
        State new_state = std::make_shared<WorkspaceState>(workspace_ux, root_path);
        return std::make_shared<PushStateMsg>(new_state);
      }};
    }});
    
    return result;
  }
  
  StateImpl::UX FrameworkState::create_ux() const {
    UX result{};
    
    // Base UX from constructor
    if (!m_ux.empty()) {
      result = m_ux;
    }
    
    // Add root path info
    result.push_back(m_root_path.string());
    
    return result;
  }

}