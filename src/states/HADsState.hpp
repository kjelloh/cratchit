#pragma once

#include "StateImpl.hpp"
#include "mod10view.hpp"

namespace first {
  struct HADsState : public StateImpl {
    using HADs = std::vector<std::string>;
    HADsState::HADs m_all_hads;
    first::Mod10View m_mod10_view;
    HADsState(HADs all_hads, Mod10View mod10_view);
    HADsState(HADs all_hads);

  }; // struct HADsState
}