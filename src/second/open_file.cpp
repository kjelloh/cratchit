#include "open_file.hpp"

#include "make_owning_istream.tpp"

#include <fstream>

ExpectedOpenFile open_file(std::filesystem::path path) {
  auto istream_ptr = make_owning_istream<std::ifstream>(path);
  if (*istream_ptr) {
    return istream_ptr;
  }
  return std::unexpected(OpenFileError::NoFile);
}
