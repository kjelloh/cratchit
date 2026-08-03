#include "SubHandler.hpp"
#include "log.hpp"

#include <optional>
#include <algorithm> // std::set_difference,

namespace detail {

  // TODO: Move to cpp-file when fully moved to SubHandler and subscriptions mechanism
  class MetronomeEventEmitter : public EmitterIfc {
  public:
    MetronomeEventEmitter(MetronomeEventDescriptor const& descriptor);
    std::optional<tea::Msg> poll();
  private:
    const MetronomeEventDescriptor m_descriptor{};
    using Clock = std::chrono::steady_clock;
    std::chrono::milliseconds m_interval_in_ms{};
    Clock::time_point m_next_click_time_point{};
    bool expired();
  }; // PolledTimer

} // detail

namespace detail {

  // #TEA::events: Concrete event emitter
  class TestEventEmitter : public EmitterIfc {
  public:
    TestEventEmitter(TestEventDescriptor const&);
    std::optional<tea::Msg> poll();
  private:
  }; // TestEventEmitter

  // #TEA::events: Concrete event emitter construct from descriptor
  TestEventEmitter::TestEventEmitter(TestEventDescriptor const&) {}

  // #TEA::events: Poll concrete emitter
  std::optional<tea::Msg> TestEventEmitter::poll() {
    static size_t call_counter{0};
    if (call_counter++ % 60 == 0) {
      return to_event_msg(TestEventDescriptor{}, TestEventDescriptor::payload_type{42});
    }
    return std::nullopt;
  } // TestEventEmitter::poll()


} // detail


namespace detail {

  MetronomeEventEmitter::MetronomeEventEmitter(MetronomeEventDescriptor const& descriptor) 
    : m_descriptor{descriptor} {
    m_interval_in_ms = std::chrono::milliseconds(m_descriptor.interval_in_ms);
    m_next_click_time_point = Clock::now() + m_interval_in_ms;
  }

  bool MetronomeEventEmitter::expired() {

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

std::unique_ptr<EmitterIfc> to_emitter(MetronomeEventDescriptor const& d) {
  return std::make_unique<detail::MetronomeEventEmitter>(d);
}

// #TEA::events: Free fatcory function for an emitter as required by descriptor type and value
std::unique_ptr<EmitterIfc> to_emitter(TestEventDescriptor const& d) {
  return std::make_unique<detail::TestEventEmitter>(d);
}

// #TEA::events: Subscriptions handler poll of all active emitters
std::vector<tea::Msg> SubHandler::poll() {
  std::vector<tea::Msg> result{};
  for (auto const& [descriptor,emitter] : this->m_active_subscriptions) {
    if (auto maybe_msg = emitter->poll()) result.push_back(*maybe_msg);
  }
  return result;
} // SubHandler::poll()

// #TEA::events: Subscriptions handler update of event emitters required to be active
void SubHandler::update(Sub const& sub) {
  // sub contains the descriptors of the desired active subscibable events
  Sub active = std::accumulate(m_active_subscriptions.begin(),m_active_subscriptions.end(),Sub{},[](
     Sub acc
    ,auto const& entry) {
    acc.push_back(entry.first);
    return acc;
  });

  // active - sub = in active but not in sub
  Sub to_remove{};
  std::set_difference(
     active.begin(),active.end()
    ,sub.begin(),sub.end()
    ,std::back_inserter(to_remove)
  );
  for (auto const& d : to_remove) {
    auto iter = m_active_subscriptions.find(d);
    if (iter != m_active_subscriptions.end()) {
      m_active_subscriptions.erase(iter);
    }
  }

  // sub - active = in sub but not in active
  Sub to_add{};
  std::set_difference(
     sub.begin(),sub.end()
    ,active.begin(),active.end()
    ,std::back_inserter(to_add)
  );

  for (auto const& d : to_add) {
    std::visit(
      [this](auto const& concrete_descriptor){
        this->m_active_subscriptions[concrete_descriptor] = to_emitter(concrete_descriptor);
      }
      ,d
    );
    log_development_trace(
       "SubHandler::update() ok for descriptor variant ix:{}. now {} active"
      ,d.index()
      ,this->m_active_subscriptions.size()
    );
  }

} // SubHandler::update()
