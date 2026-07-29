#pragma once

#include "Msg.hpp"
#include "Ux.hpp"
#include "DataState.hpp"
#include "Transition.hpp"


#include <immer/vector.hpp>
#include <variant>

// Forwards for variant
class RootView; 
class ProjectsView;
// Variant of possible views
using ViewState = std::variant<RootView,ProjectsView>;

// helper to dispatch data_state to concrete view in ViewState variant
DataState const& update(DataState const& data_state, ViewState const& view_state);

// Concrete view

class RootView {
public:
  DataState const& update(DataState const& data_state) const;

  // update returns transition between view states (for state stack mutation)
  Transition<ViewState> update(tea::UnicodeKeyMsg const& unicode_msg) const;
  Transition<ViewState> update(tea::BackspaceKeyMsg const& backspace_key_msg) const;

  using CodePointBuffer = immer::vector<char32_t>;

  // with modifiers operates to and from concrete type (allows chaining with_().with()...)
  RootView with_pushed_unicode(char32_t cp) const;
  RootView with_popped_unicode() const;

  // view returns a user interface representation that the tea runtime can render
  tea::Ux view() const;

  private:
    DataState m_data_state{};
    CodePointBuffer m_code_point_buffer{};
};

class ProjectsView {
public:
  using This = ProjectsView;
  DataState const& update(DataState const& data_state) const;
  Transition<ViewState> update(tea::UnicodeKeyMsg const& unicode_msg) const;
  // view returns a user interface representation that the tea runtime can render
  tea::Ux view() const;

private:
  DataState m_data_state{};
};