#pragma once

#include "ViewState.zpp" // incomplete type (break circual dependance)
#include "DataState.hpp"
#include "Transition.tpp"
#include "Cmd.hpp"
#include "Msg.hpp"
#include "Ux.hpp"

#include <map>

class TestView {
public:

  TestView();

  DataState update(DataState const& data_state) const;

  std::tuple<Transition<ViewState>,tea::Cmd> update(app::UnicodeKeyMsg const& unicode_msg) const;
  std::tuple<Transition<ViewState>,tea::Cmd> update(app::EnterKeyMsg const&) const;
  std::tuple<Transition<ViewState>,tea::Cmd> update(app::EscapeKeyMsg const&) const;
  // #TEA::tea::Cmd
  std::tuple<Transition<ViewState>,tea::Cmd> update(app::TestCmdResultMsg const&) const;

  TestView with_option_entry(uint8_t ix,std::string option_text) const;

  // view returns a user interface representation that the tea runtime can render
  tea::Ux view() const;

private:
  DataState m_data_state{};
  std::map<uint8_t,std::string> m_option_entries{};

};