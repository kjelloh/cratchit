#pragma once

#include <cstdio>

namespace test {

// A temporary file, removed when closed. MSVC marks std::tmpfile as unsafe
// (C4996) and wants tmpfile_s, which only it has.
inline std::FILE* temporary_file() {
#ifdef _MSC_VER
  std::FILE* file = nullptr;
  if (tmpfile_s(&file) != 0) return nullptr;
  return file;
#else
  return std::tmpfile();
#endif
}

} // namespace test
