#include "FrameworkState.hpp"
#include "WorkspaceState.hpp"
#include "logger/log.hpp"

namespace first {

  // ----------------------------------
  FrameworkState::FrameworkState(std::string caption, std::filesystem::path root_path)
      :  StateImpl{caption}
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
        State new_state = make_state<WorkspaceState>("Workspace", root_path);
        return std::make_shared<PushStateMsg>(new_state);
      }};
    }});
    
    // Add '-' key option last - back with root path as payload
    result.add('-', {"Back", [this]() -> StateUpdateResult {
      using PathMsg = CargoMsgT<std::filesystem::path>;
      return {std::nullopt, [root_path = this->m_root_path]() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>(
          std::make_shared<PathMsg>(root_path)
        );
      }};
    }});
    
    return result;
  }
  
  StateImpl::UX FrameworkState::create_ux() const {
    UX result{};
    
    // Base UX from constructor
    result.push_back(caption());
    
    // Add root path info
    result.push_back(m_root_path.string());
    
    return result;
  }

}