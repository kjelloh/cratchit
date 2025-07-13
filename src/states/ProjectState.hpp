#pragma once

#include "StateImpl.hpp"
#include "cargo/EnvironmentCargo.hpp"
#include "PersistentFile.hpp"
#include <filesystem>

namespace first {
  // ----------------------------------
  class ProjectState : public StateImpl {
  private:
    std::filesystem::path m_root_path;
    PersistentFile<Environment> m_persistent_environment_file; 
    Environment m_environment;
    void update_options();
    void update_ux();

  public:
    ProjectState(ProjectState const&) = delete; // Force create from fresh data (file)
    ProjectState(StateImpl::UX ux, std::filesystem::path root_path); // 'Initial' state
    ProjectState(
       StateImpl::UX ux
      ,PersistentFile<Environment> persistent_environment_file
      ,Environment environment);
    virtual ~ProjectState() override;
    virtual StateUpdateResult update(Msg const& msg) const override;
    virtual StateUpdateResult apply(cargo::EnvironmentCargo const& cargo) const override;
    virtual Cargo get_cargo() const override;
    virtual StateImpl::UpdateOptions create_update_options() const override;
    
    static StateFactory factory_from(std::filesystem::path project_path);
    static StateImpl::CmdOption cmd_option_from(std::string org_name, std::filesystem::path root_path);
  };
} // namespace first