#include "WorkspaceState.hpp"
#include "ProjectState.hpp"
#include <spdlog/spdlog.h>
#include <ranges>
#include <algorithm>

namespace first {

  std::string to_underscored_spaces(const std::string &name) {
    std::string result;
    result.reserve(name.size()); // Reserve to avoid multiple reallocations

    std::ranges::transform(name, std::back_inserter(result),
                           [](char c) { return c == ' ' ? '_' : c; });

    return result;
  }
  // ----------------------------------
  WorkspaceState::WorkspaceState(StateImpl::UX ux,std::filesystem::path root_path) 
    :  StateImpl{ux}
      ,m_root_path{root_path} {

    this->m_ux.push_back(m_root_path.string());

    this->add_cmd_option('0', ProjectState::cmd_option_from(".", m_root_path));
    this->add_cmd_option('1', ProjectState::cmd_option_from("ITfied AB", m_root_path));
    this->add_cmd_option('2', ProjectState::cmd_option_from("Org x", m_root_path));        
  }

  StateImpl::CmdOption WorkspaceState::cmd_option_from(std::filesystem::path workspace_path) {
    auto caption = workspace_path.filename().string();
    if (caption.empty()) {
      caption = workspace_path.string(); // Use full path if filename is empty
    }
    
    auto factory = [workspace_path]() {
      auto workspace_ux = StateImpl::UX{"Workspace UX"};
      return std::make_shared<WorkspaceState>(workspace_ux, workspace_path);
    };
    
    return {caption, cmd_from_state_factory(factory)};
  }
}