#pragma once
#include "Model.hpp"

#include <chrono>

// Concrete subscription descriptor for a Metronome event
struct MetronomeSub {
  const size_t interval_in_ms;
  const tea::Msg on_event_msg{};
}; // MetronomeSub

using SubDescriptor = std::variant<MetronomeSub>; // SubDescriptor

// Elm type name Sub
using Sub = std::vector<SubDescriptor>; // Sub lists requested subscriptions

Sub subscriptions(tea::Model const& model);