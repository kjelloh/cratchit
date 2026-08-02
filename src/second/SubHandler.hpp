#pragma once

#include "subscriptions.hpp" // SubDescriptor,Sub,...
#include <map>

namespace detail {
  // TODO: Move to cpp-file when fully moved to SubHandler and subscriptions mechanism
  class MetronomeEventEmitter {
  public:
    MetronomeEventEmitter(MetronomeEventDescriptor const& descriptor);
    void start();
    void stop();
    bool expired();
    std::optional<tea::Msg> poll();
  private:
    const MetronomeEventDescriptor m_descriptor{};
    using Clock = std::chrono::steady_clock;
    bool m_enabled{false};
    std::chrono::milliseconds m_interval_in_ms{};
    Clock::time_point m_next_click_time_point{};
  }; // PolledTimer

  class TestEventEmitter {
  public:
    void start() {};
    void stop() {};
    std::optional<tea::Msg> poll() {
      static size_t call_counter{0};
      if (call_counter++ % 60 == 0) {
        return to_event_msg(TestEventDescriptor{}, TestEventDescriptor::payload_type{42});
      }
      return std::nullopt;
    } // TestEventEmitter::poll()
  private:
  }; // TestEventEmitter

} // detail

using Subscibeable = std::variant<detail::MetronomeEventEmitter>; // Subscibeable

class SubHandler {
  public:
    std::vector<tea::Msg> poll();
    void update(Sub const& sub);

  private:
    std::map<SubDescriptor,Subscibeable> m_active_subscriptions{};
  }; // SubHandler
