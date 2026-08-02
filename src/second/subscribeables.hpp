#pragma once

#include <cstddef> // size_t

struct MetronomeEventDescriptor {
  const size_t interval_in_ms;
  struct payload_type {};
}; // MetronomeEventDescriptor

struct TestEventDescriptor {
  struct payload_type {int value;};
}; // TestEventDescriptor
