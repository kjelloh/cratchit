#pragma once

#include "StateImpl.hpp"
#include "mod10view.hpp"

namespace first {
  struct RBDsState : public StateImpl {
    using RBDs = std::vector<std::string>;
    RBDsState::RBDs m_all_rbds;
    first::Mod10View m_mod10_view;
    RBDsState(RBDs all_rbds, Mod10View mod10_view);
    RBDsState(RBDs all_rbds);

  }; // struct RBDsState
}