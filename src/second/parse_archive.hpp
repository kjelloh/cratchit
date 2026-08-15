#pragma once

#include "Archive.hpp"
#include "StreamSource.hpp"

#include <expected>

enum class ParseArchiveError {
   Undefined
  ,NotYetImplemented
  ,UnsupportedStreamSource
  ,NoFile
  ,NoMagicValue
  ,Unknown
}; // ParseArchiveError
std::string concrete_error_to_string(ParseArchiveError);
using ExpectedParsedArchive = std::expected<Archive,ParseArchiveError>;
ExpectedParsedArchive parse_archive(StreamSource);
