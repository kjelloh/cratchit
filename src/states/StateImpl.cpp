#include "StateImpl.hpp"
#include "to_type_name.hpp"
#include <spdlog/spdlog.h>
#include <unicode/uchar.h>  // for u_isprint

namespace first {

  // State
  // ----------------------------------
  StateImpl::StateImpl(UX const& ux) 
    :  m_ux{ux}
      ,m_options{}
      ,m_input_buffer{}
      ,m_cmd_options{{},{}} {

      if (true) {
        spdlog::info("StateImpl constructor called for {}", static_cast<void*>(this));
        spdlog::default_logger()->flush();
      }

  }

  StateImpl::~StateImpl() {
      if (true) {
        spdlog::info("StateImpl destructor called for {}", static_cast<void*>(this));
        spdlog::default_logger()->flush();
      }
  }

  // ----------------------------------
  void StateImpl::add_option(char ch,StateImpl::Option const& option) {
    m_options[ch] = option;
  }

  // ----------------------------------
  StateImpl::UX const& StateImpl::ux() const {
    return m_ux;
  }

  // ----------------------------------
  StateImpl::UX& StateImpl::ux() {
    return m_ux;
  }

  // ----------------------------------
  StateImpl::Options const& StateImpl::options() const {
    return m_options;
  }

  // ----------------------------------
  std::pair<std::optional<State>,Cmd> StateImpl::update(Msg const& msg) {
    return {std::nullopt,Nop}; // Default - no StateImpl mutation
  }

  // ----------------------------------
  std::pair<std::optional<State>,Cmd> StateImpl::apply(cargo::DummyCargo const& cargo) const {
    return {std::nullopt,Nop}; // Default - no StateImpl mutation
  }

  // ----------------------------------
  std::pair<std::optional<State>,Cmd> StateImpl::apply(cargo::HADsCargo const& cargo) const {
    return {std::nullopt,Nop}; // Default - no StateImpl mutation
  }
  // ----------------------------------
  std::pair<std::optional<State>,Cmd> StateImpl::apply(cargo::EnvironmentCargo const& cargo) const {
    return {std::nullopt,Nop}; // Default - no StateImpl mutation
  }

  Cargo StateImpl::get_cargo() const {
    return {};
  }

  // ----------------------------------
  // Refactoring into CmdOptions

  // ----------------------------------
  void StateImpl::add_cmd_option(char ch, CmdOption const &option) {
    if (auto iter = std::ranges::find(m_cmd_options.first,ch); iter == m_cmd_options.first.end()) {
      // Allow insertion of same option only once
      m_cmd_options.first.push_back(ch);
      m_cmd_options.second[ch] = option;
    }
    else {
      spdlog::warn("StateImpl::add_cmd_option: Ignored add of existing command option '{}'", ch);
    }
  }

  // ----------------------------------
  StateImpl::CmdOptions const& StateImpl::cmd_options() const {
    return m_cmd_options;
  }

  // ----------------------------------
  std::string const& StateImpl::input_buffer() const {
    return m_input_buffer;
  }

  // ----------------------------------
  // TODO: Refactor key procesing into this method, step-by-step
  //       When done, move into update above (and remove this refactoring step)
  std::pair<std::optional<State>, Cmd> StateImpl::update(char ch) const {
    Cmd result{};
    std::optional<State> mutated_state{};
    
    if (m_input_buffer.empty() and ch == 'q') {
      result = DO_QUIT;
    }
    else if (m_input_buffer.empty() and ch == '-') {
      result = []() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>();
      };
    }
    else if (m_input_buffer.empty() and this->options().contains(ch)) {
      result = [state_factory = this->options().at(ch).second]() -> std::optional<Msg> {
        State new_state = state_factory();
        auto msg = std::make_shared<PushStateMsg>(new_state);
        return msg;
      };
    }
    else if (m_input_buffer.empty() and this->cmd_options().second.contains(ch)) {
      result = this->cmd_options().second.at(ch).second;
    }
    else if (not m_input_buffer.empty() and ch == 127) { // Backspace
      auto new_state = std::make_shared<StateImpl>(*this);
      new_state->m_input_buffer.pop_back();
      mutated_state = new_state;
    }
    else if (not m_input_buffer.empty() and ch == '\n') { // Enter - submit input
      result = [entry = m_input_buffer]() -> std::optional<Msg> {
        return std::make_shared<UserEntryMsg>(entry);
      };
      // Clear input buffer
      auto new_state = std::make_shared<StateImpl>(*this);
      new_state->m_input_buffer.clear();
      mutated_state = new_state;
    }
    else if (u_isprint(static_cast<UChar32>(static_cast<unsigned char>(ch)))) {
      // Add character to input buffer (works for both empty and non-empty buffer)
      auto new_state = std::make_shared<StateImpl>(*this);
      new_state->m_input_buffer.push_back(ch);
      mutated_state = new_state;
    }
    
    return {mutated_state, result};
  }

} // namespace first