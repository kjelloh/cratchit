#include "open_file.hpp"

#include <fstream>

ExpectedOpenFile open_file(std::filesystem::path) {
  return std::unexpected(OpenFileError::NotYetImplemented);
}
