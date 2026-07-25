#include "view.hpp"
#include <format>

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

  Ux view(Model const& model) {

    auto to_test_rows = [](size_t row_count) -> std::vector<std::string> {
      std::vector<std::string> result{};
      for (size_t i=0;i<row_count;++i) {
        result.push_back(std::format("{}",i));
      }
      return result;
    };

    // Generate test content
    const size_t TOP_PANE_ROW_COUNT = 20;
    const size_t MIDDLE_PANE_ROW_COUNT = 20;
    const size_t BOTTOM_PANE_ROW_COUNT = 3;

    return Ux{
       to_test_rows(TOP_PANE_ROW_COUNT)
      ,to_test_rows(MIDDLE_PANE_ROW_COUNT)
      ,to_test_rows(BOTTOM_PANE_ROW_COUNT)      
    };
  } // view
} // tea

