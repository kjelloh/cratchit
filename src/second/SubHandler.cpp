#include "SubHandler.hpp"


namespace detail {
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
} // detail

std::vector<tea::Msg> SubHandler::poll() {
  std::vector<tea::Msg> result{};
  return result;
} // SubHandler::poll()

void SubHandler::update(Sub const&) {
} // SubHandler::update()
