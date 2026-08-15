#include "parse_archive.hpp"

#include "NameValuePairArchiveCodec.tpp"
#include "create_archive_codec.tpp"
#include "expected_to_string.tpp"

#include "log.hpp"

std::string concrete_error_to_string(ParseArchiveError error) {

  switch (error) {
    case ParseArchiveError::Undefined: return "ParseArchiveError::Undefined";
    case ParseArchiveError::NotYetImplemented: return "ParseArchiveError::NotYetImplemented";
    case ParseArchiveError::UnsupportedStreamSource: return "ParseArchiveError::UnsupportedStreamSource";
    case ParseArchiveError::NoFile: return "ParseArchiveError::NoFile";
    case ParseArchiveError::NoMagicValue: return "ParseArchiveError::NoMagicValue";
    case ParseArchiveError::Unknown: return "ParseArchiveError::Unknown";
  } // switch

  return "??ParseArchiveError??";

} // concrete_error_to_string

ExpectedParsedArchive parse_archive(StreamSource in_source) {
  if (is_local_file_url(in_source.url())) {
    auto archive_codec = create_archive_codec<NameValuePairCodecDescriptor>();
    auto parse_result = archive_codec.parse(in_source.stream());
    if (true) {
      log_development_trace(
        "parse_archive -> {}"
        ,expected_to_string(parse_result)
      );
    } // if log
    return parse_result;
  }

  return std::unexpected(ParseArchiveError::UnsupportedStreamSource);
} // parse_archive
