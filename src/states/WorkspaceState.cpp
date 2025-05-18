#include "WorkspaceState.hpp"
#include "ProjectState.hpp"
#include <spdlog/spdlog.h> 

namespace first {
  // ----------------------------------
  WorkspaceState::WorkspaceState(StateImpl::UX ux,std::filesystem::path root_path) 
    :  StateImpl{ux}
      ,m_root_path{root_path} {

    this->m_ux.push_back(m_root_path.string());

    StateFactory itfied_factory = []() {
      auto itfied_ux = StateImpl::UX{"ITfied UX"};
      return std::make_shared<ProjectState>(itfied_ux);
    };

    StateFactory orx_x_factory = []() {
      auto org_x_ux = StateImpl::UX{"Other Organisation UX"};
      return std::make_shared<ProjectState>(org_x_ux);
    };

    this->add_option('0',{"ITfied AB",itfied_factory});        
    this->add_option('1',{"Org x",orx_x_factory});        
  }

  // ----------------------------------
  WorkspaceState::~WorkspaceState() {
    spdlog::info("WorkspaceState destructor executed");
  }
}