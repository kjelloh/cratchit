#include "parse_archive.hpp"

#include "NameValuePairArchiveCodec.tpp"
#include "create_archive_codec.tpp"
#include "expected_to_string.tpp"

#include "log.hpp"

ExpectedParsedArchive parse_archive(OwningIStreamPtr in_ptr) {
  auto archive_codec = create_archive_codec<NameValuePairCodecDescriptor>();
  auto parse_result = archive_codec.parse(*in_ptr);
  if (true) {
    log_development_trace(
      "parse_archive -> {}"
      ,expected_to_string(parse_result)
    );
  } // if log
  return parse_result;
}
