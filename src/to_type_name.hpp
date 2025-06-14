#pragma once

#include <string>
#if defined(__GNUC__) || defined(__clang__)
  #include <cxxabi.h> // GCC/Clang name demangling
#endif

inline std::string to_unmangled(const char *name) {
  std::string result{"??"};
#if defined(__GNUC__) || defined(__clang__)
  int status = 0;
  std::unique_ptr<char[], void (*)(void *)> res{
      abi::__cxa_demangle(name, nullptr, nullptr, &status),
      std::free};
  result = (status == 0) ? res.get() : name;
#elif defined(_MSC_VER)
  result = name;
#endif
  return result;
}

inline std::string to_type_name(std::type_info const& ti) {
  return to_unmangled(ti.name());
}
