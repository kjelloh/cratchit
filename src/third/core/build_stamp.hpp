#pragma once

#include <string_view>

namespace cratchit {

// When and how the running program was built. Defined in a file that CMake
// writes on every build (cmake/build_stamp.cmake) and links into the program
// only, so the libraries and the tests do not have them.

// Local time and date of the build, "HH:MM:SS YYYY-MM-DD"
std::string_view build_time();

// The build type, e.g. "Debug" or "Release"
std::string_view build_type();

} // namespace cratchit
