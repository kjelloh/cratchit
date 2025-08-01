#include "WorkspaceState.hpp"
#include "ProjectState.hpp"
#include "logger/log.hpp"
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
  WorkspaceState::WorkspaceState(std::string caption,std::filesystem::path root_path) 
    :  StateImpl{caption}
      ,m_root_path{root_path} {

    // UX creation moved to create_ux()
    // this->m_ux.push_back(m_root_path.string());

    // Moved to create_update_options()
    // this->add_cmd_option('0', ProjectState::cmd_option_from(".", m_root_path));
    // this->add_cmd_option('1', ProjectState::cmd_option_from("ITfied AB", m_root_path));
    // this->add_cmd_option('2', ProjectState::cmd_option_from("Org x", m_root_path));        
  }

  StateImpl::UpdateOptions WorkspaceState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    
    result.add('0', {".", [root_path = m_root_path]() -> StateUpdateResult {
      return {std::nullopt, [root_path]() -> std::optional<Msg> {
        auto folder_name = to_underscored_spaces(".");
        auto project_path = root_path / folder_name;
        State new_state = make_state<ProjectState>(project_path);
        return std::make_shared<PushStateMsg>(new_state);
      }};
    }});
    
    result.add('1', {"ITfied AB", [root_path = m_root_path]() -> StateUpdateResult {
      return {std::nullopt, [root_path]() -> std::optional<Msg> {
        auto folder_name = to_underscored_spaces("ITfied AB");
        auto project_path = root_path / folder_name;
        State new_state = make_state<ProjectState>(project_path);
        return std::make_shared<PushStateMsg>(new_state);
      }};
    }});
    
    result.add('2', {"Org x", [root_path = m_root_path]() -> StateUpdateResult {
      return {std::nullopt, [root_path]() -> std::optional<Msg> {
        auto folder_name = to_underscored_spaces("Org x");
        auto project_path = root_path / folder_name;
        State new_state = make_state<ProjectState>(project_path);
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

  StateImpl::UX WorkspaceState::create_ux() const {
    UX result{};
    
    // Base UX from constructor
    result.push_back(caption());
    
    // Add root path info
    result.push_back(m_root_path.string());
    
    return result;
  }

}