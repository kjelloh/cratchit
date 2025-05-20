#pragma once

#include "StateImpl.hpp"
#include <filesystem>

namespace first {
  // ----------------------------------
  class ProjectState : public StateImpl {
  private:
    std::filesystem::path m_root_path;

  public:
    ProjectState(StateImpl::UX ux, std::filesystem::path root_path);
  };
} // namespace first