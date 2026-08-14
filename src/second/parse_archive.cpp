#include "parse_archive.hpp"

ExpectedParsedArchive parse_archive(OwningIStreamPtr) {
  return std::unexpected(ParseArchiveError::NotYetImplemented);
}
