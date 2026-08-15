#pragma once

#include "ArchiveCodec.tpp"
#include "parse_archive.hpp"
#include "DesignInsufficiencyException.hpp"

template <>
class ArchiveCodec<NameValuePairCodecDescriptor> {
public:
  ExpectedParsedArchive parse(std::istream& in) {
    log_development_trace("ArchiveCodec<NameValuePairCodecDescriptor>::import()");
    if (!in) {
      throw DesignInsufficiencyException{"ArchiveCodec<NameValuePairCodecDescriptor>::import: Expected healthy in-stream"};
    }

    return std::unexpected(ParseArchiveError::NotYetImplemented);
  }
private:
}; // ArchiveCodec

using NameValuePairArchiveCodec = ArchiveCodec<NameValuePairCodecDescriptor>;
