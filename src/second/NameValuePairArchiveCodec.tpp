#pragma once

#include "ArchiveCodec.tpp"
#include "parse_archive.hpp"

template <>
class ArchiveCodec<NameValuePairCodecDescriptor> {
public:
  ExpectedParsedArchive parse(std::istream& in) {
    log_development_trace("ArchiveCodec<NameValuePairCodecDescriptor>::import()");
    if (!in) {
      log_development_trace("No in-stream");
      return std::unexpected(ParseArchiveError::NoFile);
    }
    return std::unexpected(ParseArchiveError::NotYetImplemented);
  }
private:
}; // ArchiveCodec

using NameValuePairArchiveCodec = ArchiveCodec<NameValuePairCodecDescriptor>;
