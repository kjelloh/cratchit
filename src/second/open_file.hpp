#pragma once

#include "OwningIStreamPtr.hpp"
#include "StreamSource.hpp"

#include <filesystem>
#include <expected>

enum class OpenFileError{
   Undefined
  ,NotYetImplemented
  ,NoFile
  ,Unknown
};
using ExpectedOpenFile = std::expected<StreamSource,OpenFileError>;
ExpectedOpenFile open_file(std::filesystem::path);
