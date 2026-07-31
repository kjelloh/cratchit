#include "subscriptions.hpp"

// TODO: Make this part of the runtime
//       It is here as a POC until we figure out 
//       how subscription() can produce a Sub that tells the runtime 
//       how to activate this timer
void PolledMetronome::start(size_t interval_in_ms) {
  m_interval_in_ms = std::chrono::milliseconds(interval_in_ms);
  m_next_click_time_point = Clock::now() + m_interval_in_ms;
  m_enabled = true;
}
void PolledMetronome::stop() {
  m_enabled = false;
}
bool PolledMetronome::expired() {
  if (!m_enabled) return false;

  auto now = Clock::now();

  if (now > m_next_click_time_point) {
    m_next_click_time_point += m_interval_in_ms;
    return true;
  }
  return false;
}

Sub subscriptions(tea::Model const&) {
  return Sub{}; // dummy for now
}