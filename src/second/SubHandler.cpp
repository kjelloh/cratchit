#include "SubHandler.hpp"
#include "log.hpp"

#include <optional>
#include <algorithm> // std::set_difference,

namespace detail {

  class TestEventEmitter : public SubscibeableIfc {
  public:
    TestEventEmitter(TestEventDescriptor const&) {}
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

std::unique_ptr<SubscibeableIfc> to_emitter(MetronomeEventDescriptor const& d) {
  return std::make_unique<detail::MetronomeEventEmitter>(d);
}

std::unique_ptr<SubscibeableIfc> to_emitter(TestEventDescriptor const& d) {
  return std::make_unique<detail::TestEventEmitter>(d);
}

std::vector<tea::Msg> SubHandler::poll() {
  std::vector<tea::Msg> result{};
  return result;
} // SubHandler::poll()

void SubHandler::update(Sub const& sub) {
  // sub contains the descriptors of the desired active subscibable events
  Sub active = std::accumulate(m_active_subscriptions.begin(),m_active_subscriptions.end(),Sub{},[](
     Sub acc
    ,auto const& entry) {
    acc.push_back(entry.first);
    return acc;
  });

  // active - sub = in active but not in sub
  std::vector<Sub::value_type> to_remove{};
  std::set_difference(
     active.begin(),active.end()
    ,sub.begin(),sub.end()
    ,std::back_inserter(to_remove)
  );
  for (auto const& d : to_remove) {
    auto iter = m_active_subscriptions.find(d);
    if (iter != m_active_subscriptions.end()) {
      iter->second->stop();
      m_active_subscriptions.erase(iter);
    }
  }

  // sub - active = in sub but not in active
  std::vector<Sub::value_type> to_add{};
  std::set_difference(
     sub.begin(),sub.end()
    ,active.begin(),active.end()
    ,std::back_inserter(to_add)
  );
  
  for (auto const& d : to_add) {
    std::visit(
      [this](auto const& concrete_descriptor){
        this->m_active_subscriptions[concrete_descriptor] = to_emitter(concrete_descriptor);
        this->m_active_subscriptions[concrete_descriptor]->start();
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
