#pragma once

#include "subscriptions.hpp" // SubDescriptor,Sub,...
#include <map>

namespace detail {
  class PolledMetronome {
  public:
    void start(size_t interval_in_ms);
    void stop();
    bool expired();
  private:
    using Clock = std::chrono::steady_clock;
    bool m_enabled{false};
    std::chrono::milliseconds m_interval_in_ms{};
    Clock::time_point m_next_click_time_point{};
  }; // PolledTimer
} // detail

using Subscibeable = std::variant<detail::PolledMetronome>; // Subscibeable

class SubHandler {
  public:
    std::vector<tea::Msg> poll();
    void update(Sub const& sub);

  private:
    std::map<SubDescriptor,Subscibeable> m_active_subscriptions{};
  }; // SubHandler
