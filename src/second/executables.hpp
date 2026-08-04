#pragma once

#include <variant>

class TestCmdDescriptor {
public:
  // mak us work as 'key' (comparable)
  auto operator<=>(TestCmdDescriptor const&) const = default;
  struct result_type  {
    int value;
  }; // result_type
private:
};

using Executable = std::variant<
  TestCmdDescriptor
>;