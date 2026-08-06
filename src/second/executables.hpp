#pragma once

#include <variant>

class TestCmdDescriptor {
public:
  // make us work as 'key' (comparable)
  auto operator<=>(TestCmdDescriptor const&) const = default;
  size_t arg;
  struct payload_type  {
    size_t progress_ix;
  }; // result_type
private:
};

using Executable = std::variant<
  TestCmdDescriptor
>;