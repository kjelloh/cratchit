#pragma once

#include "Msg.hpp"
#include "Ux.hpp"
#include "DataState.hpp"
#include "Transition.hpp"
#include "Cmd.hpp"


#include <immer/vector.hpp>
#include <variant>
#include <tuple>

// Forwards for variant
class RootView; 
class ProjectsView;
// Variant of possible views
using ViewState = std::variant<RootView,ProjectsView>;

ViewState double_dispatch_accept(ViewState const& target, ViewState const& source);

// Concrete view

class RootView {
public:
  DataState update(DataState const& data_state) const;
  RootView accept(ViewState const& source) const;

  // update returns transition between view states (for state stack mutation)
  std::tuple<Transition<ViewState>,tea::Cmd> update(tea::UnicodeKeyMsg const& unicode_msg) const;
  std::tuple<Transition<ViewState>,tea::Cmd> update(tea::BackspaceKeyMsg const&) const;
  std::tuple<Transition<ViewState>,tea::Cmd> update(tea::CursorBlinkMsg const&) const;

  using CodePointBuffer = immer::vector<char32_t>;

  // with modifiers operates to and from concrete type (allows chaining with_().with()...)
  RootView with_pushed_unicode(char32_t cp) const;
  RootView with_popped_unicode() const;
  RootView with_data_state(DataState data_state) const;
  RootView with_cursor_visible(bool cursor_visible) const;

  // view returns a user interface representation that the tea runtime can render
  tea::Ux view() const;

  private:
    DataState m_data_state{};
    CodePointBuffer m_code_point_buffer{};
    bool m_cursor_visible{false};
};

class ProjectsView {
public:
  using This = ProjectsView;
  DataState const& update(DataState const& data_state) const;


  std::tuple<Transition<ViewState>,tea::Cmd> update(tea::UnicodeKeyMsg const& unicode_msg) const;
  std::tuple<Transition<ViewState>,tea::Cmd> update(tea::EnterKeyMsg const&) const;
  std::tuple<Transition<ViewState>,tea::Cmd> update(tea::EscapeKeyMsg const&) const;

  // view returns a user interface representation that the tea runtime can render
  tea::Ux view() const;

private:
  DataState m_data_state{};
};