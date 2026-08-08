#pragma once

#include "Msg.hpp"
#include "Ux.hpp"
#include "DataState.hpp"
#include "Transition.tpp"
#include "Cmd.hpp"

#include <immer/vector.hpp>
#include <variant>
#include <tuple>
#include <map>

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
  std::tuple<Transition<ViewState>,tea::Cmd> update(app::UnicodeKeyMsg const& unicode_msg) const;
  std::tuple<Transition<ViewState>,tea::Cmd> update(app::BackspaceKeyMsg const&) const;
  std::tuple<Transition<ViewState>,tea::Cmd> update(app::CursorBlinkMsg const&) const;
  // #TEA::tea::Cmd
  std::tuple<Transition<ViewState>,tea::Cmd> update(app::TestCmdResultMsg const&) const;

  using CodePointBuffer = immer::vector<char32_t>;

  // with modifiers operates to and from concrete type (allows chaining with_().with()...)
  RootView with_pushed_unicode(char32_t cp) const;
  RootView with_popped_unicode() const;
  RootView with_data_state(DataState data_state) const;
  RootView with_cursor_visible(bool cursor_visible) const;
  RootView with_option_entry(uint8_t ix,std::string option_text) const;

  // view returns a user interface representation that the tea runtime can render
  tea::Ux view() const;

  private:
    DataState m_data_state{};
    CodePointBuffer m_code_point_buffer{};
    bool m_cursor_visible{false};
    std::map<uint8_t,std::string> m_option_entries{};
};

class ProjectsView {
public:
  using This = ProjectsView;
  DataState const& update(DataState const& data_state) const;


  std::tuple<Transition<ViewState>,tea::Cmd> update(app::UnicodeKeyMsg const& unicode_msg) const;
  std::tuple<Transition<ViewState>,tea::Cmd> update(app::EnterKeyMsg const&) const;
  std::tuple<Transition<ViewState>,tea::Cmd> update(app::EscapeKeyMsg const&) const;

  // view returns a user interface representation that the tea runtime can render
  tea::Ux view() const;

private:
  DataState m_data_state{};
};