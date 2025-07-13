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
    
    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual StateImpl::UX create_ux() const override;
    
    // static StateImpl::CmdOption cmd_option_from(std::filesystem::path workspace_path);
  }; // Workspace StateImpl
}