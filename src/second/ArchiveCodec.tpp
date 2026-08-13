#pragma once

#include "archive_codecs.hpp"
#include "Archive.hpp"
#include "log.hpp"

#include <optional>
#include <istream>

template <typename ArchiveCodecDescriptor>
class ArchiveCodec {
public:
private:
}; // ArchiveCodec

template <>
class ArchiveCodec<NameValuePairCodecDescriptor> {
public:
  std::optional<Archive> import(std::istream& in) {
    log_development_trace("ArchiveCodec<NameValuePairCodecDescriptor>::import()");
    if (!in) {
      log_development_trace("No in-stream");
      return std::nullopt;
    }
    return std::nullopt;
  }
private:
}; // ArchiveCodec

template <typename T>
auto create_archive_codec() {
  return ArchiveCodec<T>{};
} // create_archive_codec