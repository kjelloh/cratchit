#pragma once

#include "OwningIStreamPtr.hpp"

#include <memory> // std::unique_ptr

template <typename T>
OwningIStreamPtr make_owning_istream(auto&&... args) {
  OwningIStreamPtr result = std::make_unique<T>(std::forward<decltype(args)>(args)...);
  return result;
}