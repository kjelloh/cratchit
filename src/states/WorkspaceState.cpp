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

    auto project_factory_factory = [root_path = m_root_path](std::string name) {
      auto folder_name = to_underscored_spaces(name);
      auto project_path = root_path / folder_name;
      return [project_path]() {
        auto project_ux = StateImpl::UX{project_path.string()};

        return std::make_shared<ProjectState>(project_ux,project_path);
      };
    };

    this->add_option('0',{".",project_factory_factory(".")});
    this->add_option('1',{"ITfied AB",project_factory_factory("ITfied AB")});
    this->add_option('2',{"Org x",project_factory_factory("Org x")});        
  }

  // ----------------------------------
  WorkspaceState::~WorkspaceState() {
    spdlog::info("WorkspaceState destructor executed");
  }
}