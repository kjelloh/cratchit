#pragma once

#include "encoding.hpp"
#include "transcoding_views.hpp"
#include "persistent/in/file_reader.hpp"
#include "MetaDefacto.hpp"
#include <filesystem>
#include <string>
#include <format>
#include <ranges>
#include <algorithm>

namespace text::encoding {

  using ByteBuffer = persistent::in::ByteBuffer;

  using WithThresholdByteBuffer = MetaDefacto<int32_t,ByteBuffer>;
  inline AnnotatedMaybe<WithThresholdByteBuffer> with_threshold(int32_t confidence_threshold, ByteBuffer byte_buffer) {
    return WithThresholdByteBuffer{confidence_threshold,std::move(byte_buffer)};
  }

  using WithDetectedEncodingByteBuffer = MetaDefacto<DetectedEncoding,ByteBuffer>;
  inline AnnotatedMaybe<WithDetectedEncodingByteBuffer> with_detetced_encoding(WithThresholdByteBuffer wt_buffer) {

    AnnotatedMaybe<WithDetectedEncodingByteBuffer> result{};

    auto const& [confidence_threshold,buffer] = wt_buffer;
    auto encoding_result = icu::to_detetced_encoding(buffer, confidence_threshold);

    if (encoding_result) {      
      result = WithDetectedEncodingByteBuffer{
         .meta = encoding_result->encoding
        ,.defacto = std::move(wt_buffer.defacto)
      };
      result.push_message(
        std::format("Detected encoding: {} (confidence: {}, method: {})",
                   encoding_result->display_name,
                   encoding_result->confidence,
                   encoding_result->detection_method)
      );
    } else {
      // Default to UTF-8 on detection failure (permissive strategy)
      result = WithDetectedEncodingByteBuffer{
         .meta = DetectedEncoding::UTF8
        ,.defacto = std::move(wt_buffer.defacto)
      };
      result.push_message(
        std::format(
           "Encoding detection failed (confidence below threshold {}), defaulting to UTF-8"
          ,confidence_threshold));
    }

    return result;

  }

  // WithDetectedEncodingByteBuffer -> std::string (materialized to platform encoding)
  // Takes by value to own the buffer during materialization
  inline AnnotatedMaybe<std::string> materialize_platform_string(
      WithDetectedEncodingByteBuffer wd_buffer) {

    AnnotatedMaybe<std::string> result{};

    auto const& [detected_encoding, buffer] = wd_buffer;

    if (buffer.empty()) {
      result.push_message("Empty buffer - no content to transcode");
      return result;  // Return failure (nullopt)
    }

    // Create lazy transcoding pipeline: bytes → Unicode → Platform encoding
    auto unicode_view = views::bytes_to_unicode(buffer, detected_encoding);
    auto platform_view = views::unicode_to_runtime_encoding(unicode_view);

    // Materialize the lazy view into a string (while buffer is still alive)
    std::string transcoded;
    transcoded.reserve(buffer.size()); // Reasonable initial capacity

    for (char byte : platform_view) {
      transcoded.push_back(byte);
    }

    result = std::move(transcoded);
    result.push_message(
      std::format("Successfully transcoded {} bytes to {} platform encoding bytes",
                  buffer.size(), result.value().size())
    );

    return result;
  }

  // Fully monadic composition: file path → byte buffer → threshold → detect encoding → materialize
  inline AnnotatedMaybe<std::string> read_file_with_encoding_detection(
    std::filesystem::path const& file_path,
    int32_t confidence_threshold = icu::DEFAULT_CONFIDENCE_THERSHOLD) {

    return persistent::in::path_to_byte_buffer(file_path)
      .and_then([confidence_threshold](ByteBuffer buffer) {
        return with_threshold(confidence_threshold, std::move(buffer));
      })
      .and_then(with_detetced_encoding)
      .and_then(materialize_platform_string);
  }

} // namespace text::encoding
