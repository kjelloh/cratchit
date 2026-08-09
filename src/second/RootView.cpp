#include "RootView.hpp"
#include "ViewState.hpp" // Complete type
#include "double_dispatch_accept.tpp" // detail::update,

#include "log.hpp"
#include "msg_to_string.hpp"
#include "utf8.hpp"

RootView::RootView() {
  m_option_entries[0] = "Projects view";
  m_option_entries[1] = "Test view";
}

DataState RootView::update(DataState const&) const {
  return this->m_data_state;
}

RootView RootView::accept(ViewState const& source) const {
  return std::visit(
    [this](auto const& concrete_source){
       return this->with_data_state(detail::update(concrete_source,this->m_data_state));
    }
    ,source
  );
  return *this;
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

RootView RootView::with_data_state(DataState data_state) const {
  RootView result(*this);
  result.m_data_state = data_state;
  return result;
}

RootView RootView::with_cursor_visible(bool cursor_visible) const {
  RootView result(*this);
  result.m_cursor_visible = cursor_visible;
  return result;
}

RootView RootView::with_option_entry(uint8_t ix,std::string option_text) const {
  RootView result(*this);
  if (result.m_option_entries.contains(ix)) {
    log_design_insufficiency(
      "with_option_entry: ix:{} already exist with value:'{}'. Will overwrite with:'{}'"
      ,ix
      ,result.m_option_entries.at(ix)
      ,option_text
    );
  }
  result.m_option_entries[ix] = option_text;
  return result;
}

std::tuple<Transition<ViewState>,tea::Cmd> RootView::update(app::UnicodeKeyMsg const& m) const {
  log_development_trace("RootView::update(m:{})",msg_to_string(m));
  if (m.code_point == '0') {
    return {
      {TransitionKind::Push, ProjectsView{}}
      ,tea::Cmd{}
    };
  }
  if (m.code_point == '1') {
    return {
      {TransitionKind::Push, TestView{}}
      ,tea::Cmd{}
    };
  }

  return {
    {TransitionKind::Mutate, this->with_pushed_unicode(m.code_point)}
    ,tea::Cmd{}
  };
}

std::tuple<Transition<ViewState>,tea::Cmd> RootView::update(app::BackspaceKeyMsg const& m) const {
  log_development_trace("RootView::update(m:{})",msg_to_string(m));
  return {
    {TransitionKind::Mutate, this->with_popped_unicode()}
    ,tea::Cmd{}
  };
}

std::tuple<Transition<ViewState>,tea::Cmd> RootView::update(app::CursorBlinkMsg const& m) const {
  log_development_trace("RootView::update(m:{})",msg_to_string(m));
  return {
    {TransitionKind::Mutate, this->with_cursor_visible(!m_cursor_visible)}
    ,tea::Cmd{}
  };
}

tea::Ux RootView::view() const {

  auto to_options_rows = [this](size_t row_count) {
    std::vector<std::string> result{};
    for (size_t ix=0;ix<row_count;++ix) {
      if (this->m_option_entries.contains(ix)) {
        result.push_back(std::format(
          "{}: {}"
          ,ix
          ,this->m_option_entries.at(ix)));
      }
      else {
        result.push_back(std::format(
          "{}:"
          ,ix));
      }
    }
    return result;
  };

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

          // // handle (hard code) 'cursor'
          // if (((m_frames_counter++/20)%2) == 0) {
          //   // Assume 60 fps
          //   // frame    m_frames_counter/20    %2    
          //   // 0-19	    0	                      0	  visible
          //   // 20-39	  1	                      1	  hidden
          //   // 40-59	  2	                      0	  visible
          //   // 60-79	  3	                      1	  hidden              
          //   utf8_string += '_';
          // }

          if (m_cursor_visible) {
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
    ,to_options_rows(MIDDLE_PANE_ROW_COUNT)
    ,to_bottom_rows(BOTTOM_PANE_ROW_COUNT)      
  };
}
