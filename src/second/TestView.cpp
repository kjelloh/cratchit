#include "TestView.hpp"
#include "ViewState.hpp" // Complete type
#include "msg_to_string.hpp"
#include "log.hpp"

DataState TestView::update(DataState const&) const {
  return this->m_data_state;
}

std::tuple<Transition<ViewState>,tea::Cmd> TestView::update(app::UnicodeKeyMsg const& unicode_msg) const {
  log_development_trace("TestView::update(m:{})",msg_to_string(unicode_msg));
  TestView result{*this};
  return {
    {TransitionKind::Mutate, result}
    ,tea::Cmd{}
  };
}

std::tuple<Transition<ViewState>,tea::Cmd> TestView::update(app::EnterKeyMsg const& concrete_msg) const {
  log_development_trace("TestView::update(m:{})",msg_to_string(concrete_msg));
  TestView result{*this};
  return {
    {TransitionKind::Accept, result}
    ,tea::Cmd{}
  };
}
std::tuple<Transition<ViewState>,tea::Cmd> TestView::update(app::EscapeKeyMsg const& concrete_msg) const {
  log_development_trace("TestView::update(m:{})",msg_to_string(concrete_msg));
  TestView result{*this};
  return {
    {TransitionKind::Reject, result}
    ,tea::Cmd{}
  };
}

std::tuple<Transition<ViewState>,tea::Cmd> TestView::update(app::TestCmdResultMsg const& m) const {
  log_development_trace("TestView::update(m:{})",msg_to_string(m));
  std::string option_text{"TestCmdResultMsg"};

  // #TEA::tea::Cmd
  if (m.payload.response_type == tea::CmdResponseType::Done) {
    option_text += " Done";
  }
  else {
    option_text += " In Progress";
  }

  return {
    {TransitionKind::Mutate, this->with_option_entry(9,option_text)}
    ,tea::Cmd{}
  };
} // TestView::update

TestView TestView::with_option_entry(uint8_t ix,std::string option_text) const {
  TestView result(*this);
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

tea::Ux TestView::view() const {

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

  return tea::Ux{
     to_test_rows(TOP_PANE_ROW_COUNT)
    ,to_options_rows(MIDDLE_PANE_ROW_COUNT)
    ,to_test_rows(BOTTOM_PANE_ROW_COUNT)      
  };
}




