#include "State.hpp"
#include "log.hpp"
#include "msg_to_string.hpp"
#include "utf8.hpp"

RootState RootState::with_pushed_unicode(char32_t cp) const {
  RootState result{*this};
  result.m_code_point_buffer = this->m_code_point_buffer.push_back(cp);
  return result;
}

RootState RootState::with_popped_unicode() const {
  RootState result{*this};
  if (this->m_code_point_buffer.size()>0) {
    result.m_code_point_buffer = this->m_code_point_buffer.take(m_code_point_buffer.size()-1);
  }
  return result;
}

RootState RootState::operator()(tea::UnicodeKeyMsg const& m) const {
  log_development_trace("RootState on {}",msg_to_string(m));
  return *this; // Nop
}

tea::Ux RootState::view() const {
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

  auto to_bottom_rows = [this](size_t row_count) -> std::vector<std::string> {
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
        case 0: result.push_back(to_buffer_hex_value_string("Unicode",this->m_code_point_buffer)); break;
        case 1: {

          std::vector<uint8_t> utf8_byte_buffer{};

          for (auto const& code_point : this->m_code_point_buffer) {
            auto utf8_bytes = unicode_to_utf8(static_cast<uint32_t>(code_point));
            for (auto utf8_byte : utf8_bytes) utf8_byte_buffer.push_back(utf8_byte);
          }
          result.push_back(to_buffer_hex_value_string("UTF8",utf8_byte_buffer));

        } break;
        case 2: {
          std::string utf8_string{">"};

          for (auto const& code_point : this->m_code_point_buffer) {
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

  return tea::Ux{
      to_test_rows(TOP_PANE_ROW_COUNT)
    ,to_test_rows(MIDDLE_PANE_ROW_COUNT)
    ,to_bottom_rows(BOTTOM_PANE_ROW_COUNT)      
  };
}
