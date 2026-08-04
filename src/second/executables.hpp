#pragma once

#include <variant>

class TestCommandDescriptor {
public:
  // mak us work as 'key' (comparable)
  auto operator<=>(TestCommandDescriptor const&) const = default;
  struct result_type  {
    int value;
  }; // result_type
private:
};

using Executable = std::variant<
  TestCommandDescriptor
>;