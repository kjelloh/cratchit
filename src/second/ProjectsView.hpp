#pragma once

#include "ViewState.zpp" // incomplete type
#include "DataState.hpp"
#include "Transition.tpp"

#include "Cmd.hpp"
#include "Msg.hpp"
#include "Ux.hpp"

class ProjectsView {
public:
  using This = ProjectsView;
  DataState update(DataState const& data_state) const;


  std::tuple<Transition<ViewState>,tea::Cmd> update(app::UnicodeKeyMsg const& unicode_msg) const;
  std::tuple<Transition<ViewState>,tea::Cmd> update(app::EnterKeyMsg const&) const;
  std::tuple<Transition<ViewState>,tea::Cmd> update(app::EscapeKeyMsg const&) const;

  // view returns a user interface representation that the tea runtime can render
  tea::Ux view() const;

private:
  DataState m_data_state{};
};