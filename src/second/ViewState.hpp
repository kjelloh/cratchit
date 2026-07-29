#pragma once

#include "Msg.hpp"
#include "Ux.hpp"
#include "DataState.hpp"

#include <immer/vector.hpp>
#include <variant>

class RootView;

using ViewState = std::variant<RootView>;

DataState const& update(DataState const& data_state, ViewState const& view_state);

class RootView {
public:
  using This = RootView;

  DataState const& update(DataState const& data_state) const;

  // update
  This update(tea::UnicodeKeyMsg const& unicode_msg) const;
  This update(tea::BackspaceKeyMsg const& backspace_key_msg) const;

  tea::Ux view() const;
  using CodePointBuffer = immer::vector<char32_t>;

  // code point buffer handling
  This with_pushed_unicode(char32_t cp) const;
  This with_popped_unicode() const;

  private:
    DataState m_data_state{};
    CodePointBuffer m_code_point_buffer{};
};

class ProjectsView {
public:
  using This = ProjectsView;
  DataState const& update(DataState const& data_state) const;
  This update(tea::UnicodeKeyMsg const& unicode_msg) const;
private:
  DataState m_data_state{};
};