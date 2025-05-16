#pragma once

#include "StateImpl.hpp"
#include "RBDsState.hpp"

namespace first {
  struct May2AprilState : public StateImpl {
    StateFactory RBDs_factory = []() {
      auto all_rbds = RBDsState::RBDs{
          "RBD #0",  "RBD #1",  "RBD #2",  "RBD #3",  "RBD #4",  "RBD #5",
          "RBD #6",  "RBD #7",  "RBD #8",  "RBD #9",  "RBD #10", "RBD #11",
          "RBD #12", "RBD #13", "RBD #14", "RBD #15", "RBD #16", "RBD #17",
          "RBD #18", "RBD #19", "RBD #20", "RBD #21", "RBD #22", "RBD #23"};
      return std::make_shared<RBDsState>(all_rbds);
    };
    May2AprilState(StateImpl::UX ux);
  }; // struct May2AprilState
}