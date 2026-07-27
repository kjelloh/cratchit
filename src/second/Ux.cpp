#include "Ux.hpp"

namespace tea {
  Ux::Ux(std::vector<std::string> top_pane_rows
    ,std::vector<std::string> middle_pane_rows
    ,std::vector<std::string> bottom_pane_rows)
    :  m_top_pane_rows{top_pane_rows}
      ,m_middle_pane_rows{middle_pane_rows}
      ,m_bottom_pane_rows{bottom_pane_rows} {}

  std::vector<std::string> Ux::top_pane_rows() const {return m_top_pane_rows;}
  std::vector<std::string> Ux::middle_pane_rows() const {return m_middle_pane_rows;}
  std::vector<std::string> Ux::bottom_pane_rows() const {return m_bottom_pane_rows;}

} // tea