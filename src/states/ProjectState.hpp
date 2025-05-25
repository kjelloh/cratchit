#pragma once

#include "StateImpl.hpp"
#include "environment.hpp"
#include <filesystem>

namespace first {
  // ----------------------------------
  class ProjectState : public StateImpl {
  private:
    std::filesystem::path m_root_path;
    Environment m_environment;
  public:
    ProjectState(StateImpl::UX ux, std::filesystem::path root_path);
  };
} // namespace first