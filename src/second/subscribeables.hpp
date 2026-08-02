#pragma once

#include <cstddef> // size_t
#include <compare> // for operator<=>

struct MetronomeEventDescriptor {
  auto operator<=>(MetronomeEventDescriptor const&) const = default;
  const size_t interval_in_ms;
  struct payload_type {};
}; // MetronomeEventDescriptor

struct TestEventDescriptor {
  auto operator<=>(TestEventDescriptor const&) const = default;
  struct payload_type {int value;};
}; // TestEventDescriptor
