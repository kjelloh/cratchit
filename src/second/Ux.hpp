/**
 * This is the The Elm Architecture (TEA) user interface representation
 * It is part of the TEA framework.
 * The TEA runtime calls the client view: Model -> Ux
 * And the TEA runtime render knows how to interpret it into actual window output
 */
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

