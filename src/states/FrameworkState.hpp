#pragma once

#include "StateImpl.hpp"
#include <filesystem>

namespace first {
  // ----------------------------------
  class FrameworkState : public StateImpl {
  private:
    std::filesystem::path m_root_path;
  public:
    FrameworkState(std::string caption,std::filesystem::path root_path);
    virtual UpdateOptions create_update_options() const override;
    virtual UX create_ux() const override;

  }; // struct FrameworkState
} // namespace first