#pragma once

#include "StateImpl.hpp"
#include "RBDState.hpp"
#include "../mod10view.hpp"

namespace first {
  struct RBDsState : public StateImpl {
    using RBDs = std::vector<std::string>;
    RBDsState::RBDs m_all_rbds;
    first::Mod10View m_mod10_view;

    struct RBDs_subrange_factory {
      // RBD subrange StateImpl factory
      RBDsState::RBDs m_all_rbds{};
      Mod10View m_mod10_view;

      auto operator()() {
        return std::make_shared<RBDsState>(m_all_rbds, m_mod10_view);
      }

      RBDs_subrange_factory(RBDsState::RBDs all_rbds, Mod10View mod10_view)
          : m_mod10_view{mod10_view}, m_all_rbds{all_rbds} {}
    };

    RBDsState(RBDs all_rbds, Mod10View mod10_view);
    RBDsState(RBDs all_rbds);

  }; // struct RBDsState
}