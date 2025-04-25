#include "StateImpl.hpp"

namespace first {

  // State
  // ----------------------------------
  StateImpl::StateImpl(UX const& ux) : m_ux{ux},m_options{} {}

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

} // namespace first