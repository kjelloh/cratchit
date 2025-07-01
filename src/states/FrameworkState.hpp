#pragma once

#include "StateImpl.hpp"
#include <filesystem>

namespace first {
  // ----------------------------------
  class FrameworkState : public StateImpl {
  private:
    std::filesystem::path m_root_path;
  public:
    FrameworkState(StateImpl::UX ux,std::filesystem::path root_path);
  }; // struct FrameworkState
} // namespace first