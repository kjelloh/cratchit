#pragma once

#include "DataState.hpp"
#include "Ux.hpp"
#include "ViewState.hpp"
#include "Msg.hpp"

#include "immer/box.hpp"

class RuntimeView {
public:

  // #Msg #View accept/reject
  std::tuple<Transition<ViewState>,tea::Cmd> update(app::EnterKeyMsg const&) const;
  std::tuple<Transition<ViewState>,tea::Cmd> update(app::EscapeKeyMsg const&) const;

  // with_xxx mutators
  RuntimeView with_data_state(DataState data_state) const;

  // #view
  tea::Ux view() const;

private:
  [[maybe_unused]] DataState m_data_state;
}; // RuntimeView
