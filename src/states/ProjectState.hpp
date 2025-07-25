#pragma once

#include "StateImpl.hpp"
#include "PersistentFile.hpp"
#include "environment.hpp"
#include <filesystem>

namespace first {
  // ----------------------------------
  class ProjectState : public StateImpl {
  private:
    std::filesystem::path m_root_path;
    PersistentFile<Environment> m_persistent_environment_file; 
    Environment m_environment;
    bool m_has_null_environment = false;
    std::string m_init_error;

  public:
    ProjectState(ProjectState const&) = delete; // Force create from fresh data (file)
    ProjectState(std::filesystem::path root_path); // 'Initial' state
    ProjectState(
       std::optional<std::string> caption
      ,PersistentFile<Environment> persistent_environment_file
      ,Environment environment);
    virtual ~ProjectState() override;
    virtual StateUpdateResult update(Msg const& msg) const override;
    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual StateImpl::UX create_ux() const override;
    virtual std::string caption() const override;    
  };
} // namespace first