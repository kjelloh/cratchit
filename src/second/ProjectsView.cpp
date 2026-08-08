#include "ProjectsView.hpp"
#include "ViewState.hpp" // Complete type

#include "log.hpp"
#include "msg_to_string.hpp"

DataState ProjectsView::update(DataState const&) const {
  return m_data_state;
}

std::tuple<Transition<ViewState>,tea::Cmd> ProjectsView::update(app::UnicodeKeyMsg const& unicode_msg) const {
  log_development_trace("ProjectsView::update(m:{})",msg_to_string(unicode_msg));
  ProjectsView result{*this};
  return {
    {TransitionKind::Mutate, result}
    ,tea::Cmd{}
  };
}

std::tuple<Transition<ViewState>,tea::Cmd> ProjectsView::update(app::EnterKeyMsg const& concrete_msg) const {
  log_development_trace("ProjectsView::update(m:{})",msg_to_string(concrete_msg));
  ProjectsView result{*this};
  return {
    {TransitionKind::Accept, result}
    ,tea::Cmd{}
  };
}
std::tuple<Transition<ViewState>,tea::Cmd> ProjectsView::update(app::EscapeKeyMsg const& concrete_msg) const {
  log_development_trace("ProjectsView::update(m:{})",msg_to_string(concrete_msg));
  ProjectsView result{*this};
  return {
    {TransitionKind::Reject, result}
    ,tea::Cmd{}
  };
}

// view returns a user interface representation that the tea runtime can render
tea::Ux ProjectsView::view() const {
  return tea::Ux{
      {"ProjectsView: top pane"}
    ,{"ProjectsView: middle pane"}
    ,{"ProjectsView: bottom pane"}      
  };
}
