#include "StateImpl.hpp"
#include <spdlog/spdlog.h>

namespace first {

  // State
  // ----------------------------------
  StateImpl::StateImpl(UX const& ux) : m_ux{ux},m_options{} {
      if (true) {
        spdlog::info("StateImpl constructor called for {}", static_cast<const void*>(this));
        spdlog::default_logger()->flush();
      }
  }

  StateImpl::~StateImpl() {
      if (true) {
        spdlog::info("StateImpl destructor called for {}", static_cast<const void*>(this));
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

  Cargo StateImpl::get_cargo() const {
    return to_cargo(NoCargo{});
  }


} // namespace first