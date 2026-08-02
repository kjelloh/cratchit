#pragma once

#include "subscriptions.hpp" // SubDescriptor,Sub,...
#include <map>
#include <memory> // std::unique_ptr

// Use overload to dispatch to concrete emitter
class EmitterIfc {
public:
  virtual ~EmitterIfc() = default;
  virtual std::optional<tea::Msg> poll() = 0;
};

class SubHandler {
  public:
    std::vector<tea::Msg> poll();
    void update(Sub const& sub);

  private:
    std::map<SubDescriptor,std::unique_ptr<EmitterIfc>> m_active_subscriptions{};
  }; // SubHandler
