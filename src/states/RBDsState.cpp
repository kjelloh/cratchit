#include "RBDsState.hpp"

namespace first {
  // ----------------------------------
  RBDsState::RBDsState(RBDs all_rbds,Mod10View mod10_view)
    :  m_mod10_view{mod10_view}
      ,m_all_rbds{all_rbds}
      ,StateImpl({}) {

    auto subranges = m_mod10_view.subranges();
    for (size_t i=0;i<subranges.size();++i) {
      auto const subrange = subranges[i];
      auto const& [begin,end] = subrange;
      auto caption = std::to_string(begin);
      if (end-begin==1) {
        // Single RBD in range option
        this->add_option(static_cast<char>('0'+i),{caption,[rbd=m_all_rbds[begin]](){
          // Single RBT factory
          auto RBD_ux = StateImpl::UX{
            "RBD UX goes here"
          };
          return std::make_shared<RBDState>(rbd);
        }});
      }
      else {
        caption += " .. ";
        caption += std::to_string(end-1);
        this->add_option(static_cast<char>('0'+i),{caption,RBDs_subrange_factory(m_all_rbds,subrange)});
      }
    }

    // Initiate view UX
    for (size_t i=m_mod10_view.m_range.first;i<m_mod10_view.m_range.second;++i) {
      auto entry = std::to_string(i);
      entry += ". ";
      entry += m_all_rbds[i];
      this->ux().push_back(entry);
    }
  }

  // ----------------------------------
  RBDsState::RBDsState(RBDs all_rbds) : RBDsState(all_rbds,Mod10View(all_rbds)) {}

}