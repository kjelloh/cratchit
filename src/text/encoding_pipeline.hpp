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

  template<typename ByteBuffer>
  auto buffer_and_threshold_to_encoding_view(
     ByteBuffer const& buffer
    ,int32_t confidence_threshold = icu::DEFAULT_CONFIDENCE_THERSHOLD)
    -> std::optional<
        decltype(views::unicode_to_runtime_encoding(
          views::bytes_to_unicode(buffer, DetectedEncoding::UTF8)))> {

    if (buffer.empty()) {
      return std::nullopt;
    }

    // Detect encoding
    auto encoding_result = icu::to_detetced_encoding(buffer, confidence_threshold);

    DetectedEncoding detected_encoding;
    if (encoding_result) {
      detected_encoding = encoding_result->encoding;
    } else {
      // Default to UTF-8 on detection failure
      detected_encoding = DetectedEncoding::UTF8;
    }

    // Create lazy transcoding pipeline
    auto unicode_view = views::bytes_to_unicode(buffer, detected_encoding);
    return views::unicode_to_runtime_encoding(unicode_view);
  }

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
      result = std::string("");
      result.push_message("Empty buffer - returning empty string");
      return result;
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

  // Monadic AnnotatedMaybe 
  inline AnnotatedMaybe<std::string> read_file_with_encoding_detection(
    std::filesystem::path const& file_path,
    int32_t confidence_threshold = icu::DEFAULT_CONFIDENCE_THERSHOLD) {

    AnnotatedMaybe<std::string> result{};

    // Step: Read file to byte buffer
    auto buffer_result = persistent::in::path_to_byte_buffer(file_path);

    if (!buffer_result) {
      // Propagate file I/O error
      result.m_messages = std::move(buffer_result.m_messages);
      return result;
    }

    // Copy messages from file read
    result.m_messages = buffer_result.m_messages;

    // auto const& buffer = buffer_result.value();

    // Handle empty file case
    if (buffer_result.value().empty()) {
      result.m_value = "";
      result.push_message("File is empty - returning empty string");
      return result;
    }

    // Build complete monadic chain: threshold → detect encoding → materialize
    auto wt_buffer_result = with_threshold(confidence_threshold, std::move(buffer_result).value());
    auto final_result = std::move(wt_buffer_result)
      .and_then(with_detetced_encoding)
      .and_then(materialize_platform_string);

    // Copy accumulated messages from the entire monadic chain
    result.m_messages.insert(
       result.m_messages.end(),
       final_result.m_messages.begin(),
       final_result.m_messages.end());

    if (!final_result) {
      return result;  // Failure propagated with messages
    }

    // Extract the final materialized string
    result.m_value = std::move(final_result).value();
    return result;
  }

} // namespace text::encoding
