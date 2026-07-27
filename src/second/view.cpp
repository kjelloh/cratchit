#include "view.hpp"
#include "utf8.hpp"
#include <format>

namespace tea {

  Ux view(Model const& model) {

    if (model.state_stack().size() > 0) {
      auto ux = std::visit(
        [](auto const& s){
          return s.view();
        }
        ,model.state_stack().back()
      );
      // return ux;
    }

    static size_t m_frames_counter = 0;

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
          result += std::format("<{:X}>",static_cast<uint32_t>(value));
        }
        return result;
      };

      for (size_t i=0;i<row_count;++i) {
        switch (i) {
          case 0: result.push_back(to_buffer_hex_value_string("Unicode",model.code_point_buffer())); break;
          case 1: {

            std::vector<uint8_t> utf8_byte_buffer{};

            for (auto const& code_point : model.code_point_buffer()) {
              auto utf8_bytes = unicode_to_utf8(static_cast<uint32_t>(code_point));
              for (auto utf8_byte : utf8_bytes) utf8_byte_buffer.push_back(utf8_byte);
            }
            result.push_back(to_buffer_hex_value_string("UTF8",utf8_byte_buffer));

          } break;
          case 2: {
            std::string utf8_string{">"};

            for (auto const& code_point : model.code_point_buffer()) {
              auto utf8_bytes = unicode_to_utf8(static_cast<uint32_t>(code_point));
              for (auto utf8_byte : utf8_bytes) utf8_string += static_cast<char>(utf8_byte);
            }

            // handle (hard code) 'cursor'
            if (((m_frames_counter++/20)%2) == 0) {
              // Assume 60 fps
              // frame    m_frames_counter/20    %2    
              // 0-19	    0	                      0	  visible
              // 20-39	  1	                      1	  hidden
              // 40-59	  2	                      0	  visible
              // 60-79	  3	                      1	  hidden              
              utf8_string += '_';
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

