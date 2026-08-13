#include "RuntimeView.hpp"

#include "log.hpp"

std::tuple<Transition<ViewState>,tea::Cmd> RuntimeView::update(app::EnterKeyMsg const&) const {
  RuntimeView result{*this};
  return {
    {TransitionKind::Accept, result}
    ,tea::Cmd{}
  };
}
std::tuple<Transition<ViewState>,tea::Cmd> RuntimeView::update(app::EscapeKeyMsg const&) const {
  RuntimeView result{*this};
  return {
    {TransitionKind::Reject, result}
    ,tea::Cmd{}
  };
}

// with_xxx mutators
RuntimeView RuntimeView::with_data_state(DataState data_state) const {
  RuntimeView result{*this};
  result.m_data_state = data_state;
  return result;
}

tea::Ux RuntimeView::view() const {
  log_development_trace("RuntimeView::view()");
  std::vector<std::string> top_pane_rows{};
  std::vector<std::string> middle_pane_rows{};
  std::vector<std::string> bottom_pane_rows{};
  top_pane_rows.push_back("RuntimeView null content");
  middle_pane_rows.push_back("RuntimeView Nop");
  bottom_pane_rows.push_back("RuntimeView no prompt");
  return tea::Ux{
    top_pane_rows
    ,middle_pane_rows
    ,bottom_pane_rows
  };
}
