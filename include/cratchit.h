#pragma once

#include <vector>
#include <string>


#ifdef _WIN32
  #define CRATCHIT_EXPORT __declspec(dllexport)
#else
  #define CRATCHIT_EXPORT
#endif

CRATCHIT_EXPORT void cratchit();
CRATCHIT_EXPORT void cratchit_print_vector(const std::vector<std::string> &strings);
