#include "State.hpp"
#include "log.hpp"
#include "msg_to_string.hpp"

State RootState::operator()(tea::UnicodeKeyMsg const& m) const {
  log_development_trace("RootState on {}",msg_to_string(m));
  return *this; // Nop
}
