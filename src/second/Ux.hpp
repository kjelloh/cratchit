#pragma once

#include <vector>
#include <string>

namespace tea {
  class Ux {
  public:
    Ux() = default;
    Ux(std::vector<std::string> top_pane_rows
      ,std::vector<std::string> middle_pane_rows
      ,std::vector<std::string> bottom_pane_rows);
    std::vector<std::string> top_pane_rows() const;
    std::vector<std::string> middle_pane_rows() const;
    std::vector<std::string> bottom_pane_rows() const;
  private:
    std::vector<std::string> m_top_pane_rows{};
    std::vector<std::string> m_middle_pane_rows{};
    std::vector<std::string> m_bottom_pane_rows{};
  }; // Ux
} // tea

