/**
 * Defines The Elm Architecture (TEA) subscribebale events
 * It is shared between the app (client) and the TEA runtime
 * This file contains the descriptors.
 * The app provides the subscriptions: Model -> Subs
 * The runtime calls subscriptions() on current model and activates requested event emitters
 * The runtime then polls event emitters and generates app-defined message on acquired events
 */

#pragma once

#include <cstddef> // size_t
#include <compare> // for operator<=>
#include <variant>

struct MetronomeEventDescriptor {
  auto operator<=>(MetronomeEventDescriptor const&) const = default;
  const size_t interval_in_ms;
  struct payload_type {};
}; // MetronomeEventDescriptor

// #TEA::events: Concrete test event descriptor
struct TestEventDescriptor {
  auto operator<=>(TestEventDescriptor const&) const = default;
  struct payload_type {int value;};
}; // TestEventDescriptor

// #TEA::events: Possible event descriptor variants
using Subscribable = std::variant<
   MetronomeEventDescriptor
  ,TestEventDescriptor
>;
