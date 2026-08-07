/**
 * This is The Elm Architecture (TEA) runtime event subscription handler
 * It knows how to ascociate event descriptors with event emitters
 */
#pragma once

#include "Sub.hpp" // Sub,Subs,...
#include "Msg.hpp"
#include <map>
#include <memory> // std::unique_ptr

namespace tea {

  // #TEA::events: All event emitters interface
  class EmitterIfc {
  public:
    virtual ~EmitterIfc() = default;
    virtual std::optional<app::Msg> poll() = 0;
  };

  // #TEA::events: The subscriptions (Subs) handler
  class SubHandler {
    public:
      std::vector<app::Msg> poll();
      void update(Subs const& subs);

    private:
      std::map<Sub,std::unique_ptr<EmitterIfc>> m_active_subscriptions{};
    }; // SubHandler

} // tea

