#pragma once

#include "Archive.hpp"
#include "OwningIStreamPtr.hpp"

#include <expected>

enum class ParseArchiveError {
   Undefined
  ,NotYetImplemented
  ,NoFile
  ,Unknown
}; // ParseArchiveError
using ExpectedParsedArchive = std::expected<Archive,ParseArchiveError>;
ExpectedParsedArchive parse_archive(OwningIStreamPtr);
