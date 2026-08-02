#pragma once

#include "subscriptions.hpp" // SubDescriptor,Sub,...
#include <map>
#include <memory> // std::unique_ptr

// Use overload to dispatch to concrete emitter
class SubscibeableIfc {
public:
  virtual ~SubscibeableIfc() = default;
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual std::optional<tea::Msg> poll() = 0;
};

namespace detail {

  // TODO: Move to cpp-file when fully moved to SubHandler and subscriptions mechanism
  class MetronomeEventEmitter : public SubscibeableIfc {
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

} // detail

class SubHandler {
  public:
    std::vector<tea::Msg> poll();
    void update(Sub const& sub);

  private:
    std::map<SubDescriptor,std::unique_ptr<SubscibeableIfc>> m_active_subscriptions{};
  }; // SubHandler
