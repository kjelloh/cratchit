#include "ViewState.hpp"
#include "log.hpp"
#include "msg_to_string.hpp"
#include "utf8.hpp"

namespace detail {

  template<typename S>
  concept ProvidesDataStateUpdate = requires(S s) {
    { s.data_state() } -> std::same_as<DataState const&>;
  };

  template<typename S>
  DataState const& update(DataState const& data_state, S const& s) {
    if constexpr (ProvidesDataStateUpdate<S>) {
      return s.data_state();
    }
    else {
      return data_state; // fallback
    }
  }

} // detail

DataState const& update(DataState const& data_state, ViewState const& view_state) {
  return std::visit(
    [&data_state](auto const& concrete_view) -> DataState const& {
      return detail::update(data_state, concrete_view);
    }
    ,view_state
  );
}

DataState const& RootView::update(DataState const&) const {
  return m_data_state;
}

RootView RootView::with_pushed_unicode(char32_t cp) const {
  log_development_trace("RootView::with_pushed_unicode:{}",static_cast<uint32_t>(cp));

  RootView result{*this};
  result.m_code_point_buffer = this->m_code_point_buffer.push_back(cp);
  log_development_trace("with_pushed_unicode m_code_point_buffer:{}",result.m_code_point_buffer.size());
  return result;
}

RootView RootView::with_popped_unicode() const {
  RootView result{*this};
  if (this->m_code_point_buffer.size()>0) {
    result.m_code_point_buffer = this->m_code_point_buffer.take(m_code_point_buffer.size()-1);
  }
  return result;
}

Transition<ViewState> RootView::update(tea::UnicodeKeyMsg const& m) const {
  log_development_trace("RootView::update(m:{})",msg_to_string(m));
  return {TransitionKind::Mutate, this->with_pushed_unicode(m.code_point)};
}

Transition<ViewState> RootView::update(tea::BackspaceKeyMsg const& m) const {
  log_development_trace("RootView::update(m:{})",msg_to_string(m));
  return {TransitionKind::Mutate, this->with_popped_unicode()};
}

tea::Ux RootView::view() const {
  log_development_trace("RootView::view() m_code_point_buffer:{}",m_code_point_buffer.size());
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
  }; // to_bottom_rows

  return tea::Ux{
      to_test_rows(TOP_PANE_ROW_COUNT)
    ,to_test_rows(MIDDLE_PANE_ROW_COUNT)
    ,to_bottom_rows(BOTTOM_PANE_ROW_COUNT)      
  };
}

// ProjectsView
DataState const& ProjectsView::update(DataState const&) const {
  return m_data_state;
}
Transition<ViewState> ProjectsView::update(tea::UnicodeKeyMsg const& unicode_msg) const {
  log_development_trace("ProjectsView::update(m:{})",msg_to_string(unicode_msg));
  ProjectsView result{*this};
  return {TransitionKind::Mutate, result};
}

// view returns a user interface representation that the tea runtime can render
tea::Ux ProjectsView::view() const {
  return tea::Ux{
      {"ProjectsView: top pane"}
    ,{"ProjectsView: middle pane"}
    ,{"ProjectsView: bottom pane"}      
  };
}

