#pragma once
#include "Model.hpp"

#include <chrono>

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

using Sub = int; // placeholder type for now

Sub subscriptions(tea::Model const& model);