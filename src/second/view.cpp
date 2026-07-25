#include "view.hpp"
#include "utf8.hpp"
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

    auto to_bottom_rows = [](Model const& model,size_t row_count) -> std::vector<std::string> {
      std::vector<std::string> result{};

      auto to_buffer_hex_value_string = [](std::string const& caption,auto buffer) -> std::string {
        std::string result{std::format("{}:",caption)};
        for (auto const& value : buffer) {
        }
        return result;
      };

      for (size_t i=0;i<row_count;++i) {
        switch (i) {
          case 0: result.push_back(to_buffer_hex_value_string("Unicode",model.code_point_buffer())); break;
          case 1: {
            std::string utf8_hex_message{};
            for (auto const& code_point : model.code_point_buffer()) {
              auto utf8_bytes = unicode_to_utf8(static_cast<uint32_t>(code_point));
              for (auto const& utf8_byte : utf8_bytes) {
                utf8_hex_message += std::format("<{:X}>",utf8_byte);
              }
            }
            result.push_back(to_buffer_hex_value_string("UTF8",utf8_hex_message));

          } break;
          case 2: {
            std::string utf8_string{">"};
            for (auto const& code_point : model.code_point_buffer()) {
              auto utf8_bytes = unicode_to_utf8(static_cast<uint32_t>(code_point));
              for (auto utf8_byte : utf8_bytes) utf8_string += static_cast<char>(utf8_byte);
            }
            result.push_back(utf8_string);
          } break;
        }
      }
      return result;
    };

    return Ux{
       to_test_rows(TOP_PANE_ROW_COUNT)
      ,to_test_rows(MIDDLE_PANE_ROW_COUNT)
      ,to_bottom_rows(model,BOTTOM_PANE_ROW_COUNT)      
    };
  } // view
} // tea

