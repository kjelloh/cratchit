#pragma once

#include "Archive.hpp"
#include "open_file.hpp"
#include "parse_archive.hpp"

#include <expected>
#include <filesystem>
#include <fstream>

enum class ImportArchiveError {
   Undefined
  ,NotYetImplemented
  ,UnsupportedArchiveSource
  ,NoFile
  ,UnsupportedOpenFileError
  ,MalformedArchiveSource
  ,ParseErrorUnsupported
  ,Unknown
};
std::string concrete_error_to_string(ImportArchiveError);
using ExpectedImportedArchive = std::expected<Archive,ImportArchiveError>;
ExpectedImportedArchive import_archive(std::filesystem::path);
