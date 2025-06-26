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
  public:
    ProjectState(StateImpl::UX ux, std::filesystem::path root_path);
    virtual ~ProjectState() override;
    virtual std::pair<std::optional<State>, Cmd> update(Msg const &msg) override;
    virtual std::pair<std::optional<State>, Cmd> apply(cargo::EnvironmentCargo const& cargo) const override;
    virtual Cargo get_cargo() const override;
  };
} // namespace first