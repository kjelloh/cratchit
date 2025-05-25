#include "ProjectState.hpp"
#include "May2AprilState.hpp"
#include "Q1State.hpp"
#include <format>

namespace first {

  // ----------------------------------
  ProjectState::ProjectState(StateImpl::UX ux,std::filesystem::path root_path) 
    :  StateImpl{ux}
      ,m_root_path{root_path}
      ,m_environment{} {

    // Integrate Zeroth: Environment environment_from_file(...) here.
    // It is ok as this constructor is run as a command by the tea runtime (side effects allowed)
    try {
      m_environment = environment_from_file(m_root_path / "cratchit.env");
      m_ux.push_back(std::format("Environment size is: {}", m_environment.size()));
    }
    catch (const std::exception& e) {
      m_ux.push_back("Error initializing environment: " + std::string(e.what()));
    }

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