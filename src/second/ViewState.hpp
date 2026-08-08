#pragma once

#include "ViewState.zpp" // incomplete types header

// Make ViewState complete by inclusing ALL concrete view declarations!
#include "RootView.hpp"
#include "TestView.hpp"

// Include TEA mechanism types
#include "Msg.hpp"
#include "Ux.hpp"
#include "DataState.hpp"
#include "Transition.tpp"
#include "Cmd.hpp"

// incldue external dependancies
#include <immer/vector.hpp>
#include <variant>
#include <tuple>
#include <map>

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