#include "SubHandler.hpp"

#include <optional>


namespace detail {

  MetronomeEventEmitter::MetronomeEventEmitter(MetronomeEventDescriptor const& descriptor) 
    : m_descriptor{descriptor} {
  }

  void MetronomeEventEmitter::start() {
    m_interval_in_ms = std::chrono::milliseconds(m_descriptor.interval_in_ms);
    m_next_click_time_point = Clock::now() + m_interval_in_ms;
    m_enabled = true;
  } // MetronomeEventEmitter::start()

  void MetronomeEventEmitter::stop() {
    m_enabled = false;
  } // MetronomeEventEmitter::stop()

  bool MetronomeEventEmitter::expired() {
    if (!m_enabled) return false;

    auto now = Clock::now();

    if (now > m_next_click_time_point) {
      m_next_click_time_point += m_interval_in_ms;
      return true;
    }
    return false;
  } // MetronomeEventEmitter::expired()

  std::optional<tea::Msg> MetronomeEventEmitter::poll() {
    if (expired()) {
      return to_event_msg(m_descriptor, MetronomeEventDescriptor::payload_type{});
    }
    return std::nullopt;
  }

} // detail

std::vector<tea::Msg> SubHandler::poll() {
  std::vector<tea::Msg> result{};
  return result;
} // SubHandler::poll()

void SubHandler::update(Sub const&) {
  // What changed?
  std::map<SubDescriptor,Subscibeable> already_active_subscriptions{};
  std::map<SubDescriptor,Subscibeable> now_deactivated_subscriptions{};
  std::map<SubDescriptor,Subscibeable> new_to_activate_subscriptions{};
} // SubHandler::update()
