#include "StateImpl.hpp"
#include "to_type_name.hpp"
#include <spdlog/spdlog.h>
#include <unicode/uchar.h>  // for u_isprint
#include "msg.hpp"  // for NCursesKeyMsg

namespace first {

  // State
  // ----------------------------------
  StateImpl::StateImpl(UX const& ux) 
    :  m_ux{ux}
      ,m_cmd_options{{},{}}
      ,m_update_options{{},{}} {

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
  StateImpl::UX const& StateImpl::ux() const {
    return m_ux;
  }

  // ----------------------------------
  StateImpl::UX& StateImpl::ux() {
    return m_ux;
  }


  // ----------------------------------
  StateUpdateResult StateImpl::dispatch(Msg const& msg) {
    spdlog::info("StateImpl::dispatch(msg) - BEGIN");
    
    // Try virtual update first
    auto result = this->update(msg);
    if (result.first or result.second) {
      // Derived state handled the message
      spdlog::info("StateImpl::dispatch(msg) - Handled by derived state");
      return result;
    }
    
    // Derived didn't handle it - use base default logic
    spdlog::info("StateImpl::dispatch(msg) - Using default_update fallback");
    return this->default_update(msg);
  }

  // ----------------------------------
  StateUpdateResult StateImpl::update(Msg const& msg) {
    spdlog::info("StateImpl::update(msg) - Base implementation - didn't handle");
    return {std::nullopt, Cmd{}}; // Base: "didn't handle"
  }

  // ----------------------------------
  StateUpdateResult StateImpl::apply(cargo::DummyCargo const& cargo) const {
    return {std::nullopt,Nop}; // Default - no StateImpl mutation
  }

  // ----------------------------------
  StateUpdateResult StateImpl::apply(cargo::HADsCargo const& cargo) const {
    return {std::nullopt,Nop}; // Default - no StateImpl mutation
  }
  // ----------------------------------
  StateUpdateResult StateImpl::apply(cargo::EnvironmentCargo const& cargo) const {
    return {std::nullopt,Nop}; // Default - no StateImpl mutation
  }

  // ----------------------------------
  StateUpdateResult StateImpl::apply(cargo::EditedItemCargo<HeadingAmountDateTransEntry> const& cargo) const {
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
  void StateImpl::add_update_option(char ch, UpdateOption const &option) {
    if (auto iter = std::ranges::find(m_update_options.first,ch); iter == m_update_options.first.end()) {
      // Allow insertion of same option only once
      m_update_options.first.push_back(ch);
      m_update_options.second[ch] = option;
    }
    else {
      spdlog::warn("StateImpl::add_update_option: Ignored add of existing update option '{}'", ch);
    }
  }

  // ----------------------------------
  StateImpl::CmdOptions const& StateImpl::cmd_options() const {
    return m_cmd_options;
  }

  // ----------------------------------
  StateImpl::UpdateOptions const& StateImpl::update_options() const {
    return m_update_options;
  }

  // ----------------------------------
  StateUpdateResult StateImpl::default_update(Msg const& msg) {
    spdlog::info("StateImpl::default_update(msg) - BEGIN");

    // Handle NCursesKeyMsg messages by delegating to update(char)
    if (auto key_msg_ptr = std::dynamic_pointer_cast<NCursesKeyMsg>(msg); key_msg_ptr != nullptr) {
      spdlog::info("StateImpl::default_update(msg) - NCursesKeyMsg");
      return this->default_update(key_msg_ptr->key);
    }

    return {std::nullopt,Cmd{}}; // fallback - no StateImpl mutation
  }

  // ----------------------------------
  StateUpdateResult StateImpl::default_update(char ch) const {
    spdlog::info("StateImpl::default_update(key) - BEGIN");

    Cmd cmd{}; // null
    std::optional<State> mutated_state{}; // None
    
    if (ch == 'q') {
      cmd = DO_QUIT;
    }
    else if (ch == '-') {
      cmd = []() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>();
      };
    }
    else if (this->m_update_options.second.contains(ch)) {
      // Execute UpdateOption function and extract result
      auto update_option = this->m_update_options.second.at(ch);
      auto [new_state, new_cmd] = update_option.second();
      mutated_state = new_state;
      cmd = new_cmd;
    }
    else if (this->cmd_options().second.contains(ch)) {
      cmd = this->cmd_options().second.at(ch).second;
    }
    // else if (not m_input_buffer.empty() and ch == 127) { // Backspace
    //   auto new_state = std::make_shared<StateImpl>(*this);
    //   new_state->m_input_buffer.pop_back();
    //   mutated_state = new_state;
    // }
    // else if (not m_input_buffer.empty() and ch == '\n') { // Enter - submit input
    //   cmd = [entry = m_input_buffer]() -> std::optional<Msg> {
    //     return std::make_shared<UserEntryMsg>(entry);
    //   };
    //   // Clear input buffer
    //   auto new_state = std::make_shared<StateImpl>(*this);
    //   new_state->m_input_buffer.clear();
    //   mutated_state = new_state;
    // }
    // else if (u_isprint(static_cast<UChar32>(static_cast<unsigned char>(ch)))) {
    //   // Add character to input buffer (works for both empty and non-empty buffer)
    //   auto new_state = std::make_shared<StateImpl>(*this);
    //   new_state->m_input_buffer.push_back(ch);
    //   mutated_state = new_state;
    // }
    else {
      spdlog::info("StateImpl::update(ch) - ignored message");
    }
    
    return {mutated_state, cmd};
  }

  // ----------------------------------
  // Helper to convert StateFactory to PushStateMsg Cmd (Phase 1)
  Cmd StateImpl::cmd_from_state_factory(StateFactory factory) {
    return [factory]() -> std::optional<Msg> {
      State new_state = factory();
      return std::make_shared<PushStateMsg>(new_state);
    };
  }

} // namespace first