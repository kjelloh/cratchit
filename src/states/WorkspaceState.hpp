#pragma once

#include "StateImpl.hpp"
#include <filesystem>

namespace first {
  // ----------------------------------
  class WorkspaceState : public StateImpl {
  private:
    std::filesystem::path m_root_path;    
  public:
    WorkspaceState(StateImpl::UX ux,std::filesystem::path root_path);
  }; // Workspace StateImpl
}