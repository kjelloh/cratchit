#include "ProjectState.hpp"
#include "May2AprilState.hpp"
#include "Q1State.hpp"


namespace first {

  // ----------------------------------
  ProjectState::ProjectState(StateImpl::UX ux) : StateImpl{ux} {

    StateFactory may2april_factory = []() {
      auto may2april_ux = StateImpl::UX{"May to April"};
      return std::make_shared<May2AprilState>(may2april_ux);
    };

    StateFactory q1_factory = []() {
      auto q1_ux = StateImpl::UX{"Q1 UX goes here"};
      return std::make_shared<Q1State>(q1_ux);
    };

    this->add_option('0',{"May to April",may2april_factory});
    this->add_option('1',{"Q1",q1_factory});
  }
}