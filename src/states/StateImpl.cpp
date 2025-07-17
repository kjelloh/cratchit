#include "StateImpl.hpp"
#include "to_type_name.hpp"
#include <spdlog/spdlog.h>
#include <unicode/uchar.h>  // for u_isprint
#include "msgs/msg.hpp"  // for NCursesKeyMsg

namespace first {

  // State
  // ----------------------------------
  StateImpl::StateImpl(std::optional<std::string> caption) 
    :  m_caption{caption}
      ,m_transient_maybe_update_options{}
      ,m_transient_maybe_ux{} {

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
    // Lazy UX generation similar to update_options pattern
    if (!m_transient_maybe_ux) {
      m_transient_maybe_ux = create_ux();
    }
    
    if (!m_transient_maybe_ux) {
      spdlog::error("DESIGN_INSUFFICIENCY: StateImpl::ux() requires this->create_ux to return non-empty UX!");
      static UX dummy{};
      return dummy;
    }
    
    return *m_transient_maybe_ux;
  }

  // ----------------------------------
  std::string StateImpl::caption() const {
    return m_caption.value_or("??");
  }

  StateImpl::UpdateOptions const& StateImpl::update_options() const {

    // Note: m_transient_maybe_update_options must be synchronised with actual state instance.
    //       Then option lambdas may safelly capture 'this' (See UpdateOptions: key -> (caption,lambda))

    // Design: Mutate State -> nullopt options (StateImpl constructor)
    //         Client call + empty cache -> ask concrete state to create_update_options()
    //         Mutate cache with created instance
    //         Whine if cache still does not contain an instance
    //         Return cache    
    if (!this->m_transient_maybe_update_options) {
      // mutable = Allow mutation of transient data although we are const
      this->m_transient_maybe_update_options = this->create_update_options();
    }

    // Whine?
    if (!this->m_transient_maybe_update_options) {
      spdlog::error("DESIGN_INSUFFICIENCY: StateImpl::update_options() requires this->create_update_options to return non std::nullopt value!");
      static StateImpl::UpdateOptions dummy{};
      return dummy; // Prohibit crach!
    }

    return this->m_transient_maybe_update_options.value();
  }

  // ----------------------------------
  // StateUpdateResult StateImpl::dispatch(Msg const& msg) const {
  //   spdlog::info("StateImpl::dispatch(msg) - BEGIN");
  //   return this->update(msg);

    // all-state scope key handling moved out to cratchit::update::try_state_update     
    // Try virtual update first
    // if (auto result = this->update(msg)) {
    //   // Derived state handled the message
    //   spdlog::info("StateImpl::dispatch(msg) - Handled by derived state");
    //   return result;
    // }
    // else if (auto key_msg_ptr = std::dynamic_pointer_cast<NCursesKeyMsg>(msg); key_msg_ptr != nullptr) {
    //   spdlog::info("StateImpl::dispatch(msg) - NCursesKeyMsg");
    //   auto ch = key_msg_ptr->key;
    //   if (auto update_result = this->update_options().apply(ch)) {
    //     return update_result;
    //   }
    //   else if (ch == 'q') {
    //     return {std::nullopt,DO_QUIT};
    //   }
    //   else if (ch == '-') {
    //     // Default pop-state (no payload child -> parent state)
    //     return {std::nullopt,[]() -> std::optional<Msg>{
    //       return std::make_shared<PopStateMsg>();
    //     }};
    //   }
    //   spdlog::info("StateImpl::update(NCursesKeyMsg) - ignored message");
    // }

    // // Derived didn't handle it - use base default logic
    // spdlog::info("StateImpl::dispatch(msg) - Using default_update fallback");
    // return this->default_update(msg);

    // return StateUpdateResult{}; // not handled    
  // }

  // ----------------------------------
  StateUpdateResult StateImpl::update(Msg const& msg) const {
    spdlog::info("StateImpl::update(msg) - Base implementation - didn't handle");
    return {std::nullopt, Cmd{}}; // Base: "didn't handle"
  }

  // // TODO: Refactor get_cargo() -> get_on_destruct_msg mechanism
  // std::optional<Msg> StateImpl::get_on_destruct_msg() const {
  //   return std::nullopt;
  // }

  // ----------------------------------
  // Helper to convert StateFactory to PushStateMsg Cmd (Phase 1)
  Cmd StateImpl::cmd_from_state_factory(StateFactory factory) {
    return [factory]() -> std::optional<Msg> {
      State new_state = factory();
      return std::make_shared<PushStateMsg>(new_state);
    };
  }

  // private:

  StateImpl::UpdateOptions StateImpl::create_update_options() const {
    StateImpl::UpdateOptions result{};
    // Add a dummy entry to show that state has to override this approprietly
    result.add('?',UpdateOption{"StateImpl::create_update_options"
      ,[]() -> StateUpdateResult {
          return StateUpdateResult{}; // 'null'
        }
      }
    );
    return result;
  }
  
  StateImpl::UX StateImpl::create_ux() const {
    // Default implementation - states should override this
    UX result;
    result.push_back(caption()); // Use the caption getter which handles nullopt
    result.push_back("StateImpl::create_ux - override in concrete state");
    return result;
  }

  // ----------------------------------
  // StateUpdateResult StateImpl::default_update(Msg const& msg) const {
  //   spdlog::info("StateImpl::default_update(msg) - BEGIN");

  //   // Handle NCursesKeyMsg messages by delegating to update(char)
  //   if (auto key_msg_ptr = std::dynamic_pointer_cast<NCursesKeyMsg>(msg); key_msg_ptr != nullptr) {
  //     spdlog::info("StateImpl::default_update(msg) - NCursesKeyMsg");
  //     return this->default_update(key_msg_ptr->key);
  //   }

  //   return {std::nullopt,Cmd{}}; // fallback - no StateImpl mutation
  // }

  // ----------------------------------
  // StateUpdateResult StateImpl::default_update(char ch) const {
  //   spdlog::info("StateImpl::default_update(key) - BEGIN");

  //   Cmd cmd{}; // null
  //   std::optional<State> mutated_state{}; // None
    
  //   if (ch == 'q') {
  //     cmd = DO_QUIT;
  //   }
  //   else if (auto update_result = this->update_options().apply(ch)) {
  //     auto const& [new_state, new_cmd] = update_result;
  //     mutated_state = new_state;
  //     cmd = new_cmd;
  //   }
  //   else if (ch == '-') {
  //     cmd = []() -> std::optional<Msg> {
  //       return std::make_shared<PopStateMsg>();
  //     };
  //   }
  //   else {
  //     spdlog::info("StateImpl::update(ch) - ignored message");
  //   }
    
  //   return {mutated_state, cmd};
  // }

} // namespace first