#include "mod10view.hpp"

namespace first {

    // ----------------------------------
    Mod10View::Mod10View(Mod10View::Range const& range)
    :  m_range{range}
      ,m_subrange_size{static_cast<size_t>(std::pow(10,std::ceil(std::log10(range.second-range.first))-1))} {}
    
    // ----------------------------------
    std::vector<Mod10View::Range> Mod10View::subranges() {
      std::vector<std::pair<size_t,size_t>> result{};
      for (size_t i=m_range.first;i<m_range.second;i += m_subrange_size) {
        result.push_back(std::make_pair(i,std::min(i+m_subrange_size,m_range.second)));
      }
      return result;
    }
  
}