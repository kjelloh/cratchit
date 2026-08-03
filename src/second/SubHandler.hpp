/**
 * This is The Elm Architecture (TEA) runtime event subscription handler
 * It knows how to ascociate event descriptors with event emitters
 */
#pragma once

#include "subscriptions.hpp" // SubDescriptor,Sub,...
#include <map>
#include <memory> // std::unique_ptr

// #TEA::events: All event emitters interface
class EmitterIfc {
public:
  virtual ~EmitterIfc() = default;
  virtual std::optional<tea::Msg> poll() = 0;
};

// #TEA::events: The subscriptions (Sub) handler
class SubHandler {
  public:
    std::vector<tea::Msg> poll();
    void update(Sub const& sub);

  private:
    std::map<SubDescriptor,std::unique_ptr<EmitterIfc>> m_active_subscriptions{};
  }; // SubHandler
