#pragma once

#include "encoding.hpp"
#include "transcoding_views.hpp"
#include "io/file_reader.hpp"
#include <filesystem>
#include <string>
#include <format>
#include <ranges>
#include <algorithm>

namespace text::encoding {

  /**
   * Integrated encoding pipeline: file path → runtime-encoded text (UTF-8 fallback)
   *
   * This function composes Steps 1-4 of the CSV import pipeline:
   *   1. File I/O: Read file to byte buffer
   *   2. Encoding detection: Detect source encoding using ICU
   *   3. Lazy transcoding: Bytes → Unicode code points
   *   4. Lazy encoding: Unicode → runtime encoding
   *
   * Error Handling Strategy:
   *   - File I/O failures: Propagate through IOResult
   *   - Encoding detection failures: Default / fallback to UTF-8 and proceed (most permissive)
   *   - This strategy prioritizes availability over correctness for ambiguous cases
   *
   * Performance Characteristics:
   *   - Lazy evaluation: Transcoding happens on-demand when iterating the result
   *   - Memory efficiency: Large files don't cause excessive memory usage
   *   - The byte buffer is eagerly loaded, but transcoding is lazy
   *
   * @param file_path Path to the file to read and transcode
   * @param confidence_threshold ICU confidence threshold for encoding detection (default: 90)
   * @return IOResult<std::string> containing Platform (runtime) encoded text or error messages
   */
  cratchit::io::IOResult<std::string> read_file_with_encoding_detection(
    std::filesystem::path const& file_path,
    int32_t confidence_threshold = icu::DEFAULT_CONFIDENCE_THERSHOLD
  ) {
    cratchit::io::IOResult<std::string> result{};

    // Step 1: Read file to byte buffer
    auto buffer_result = cratchit::io::read_file_to_buffer(file_path);

    if (!buffer_result) {
      // Propagate file I/O error
      result.m_messages = std::move(buffer_result.m_messages);
      return result;
    }

    // Copy messages from file read
    result.m_messages = buffer_result.m_messages;
    auto const& buffer = buffer_result.value();

    // Handle empty file case
    if (buffer.empty()) {
      result.m_value = "";
      result.push_message("File is empty - returning empty string");
      return result;
    }

    // Step 2: Detect encoding
    auto encoding_result = icu::detect_buffer_encoding(buffer, confidence_threshold);

    DetectedEncoding detected_encoding;
    if (encoding_result) {
      detected_encoding = encoding_result->encoding;
      result.push_message(
        std::format("Detected encoding: {} (confidence: {}, method: {})",
                   encoding_result->display_name,
                   encoding_result->confidence,
                   encoding_result->detection_method)
      );
    } else {
      // Default to UTF-8 on detection failure (permissive strategy)
      detected_encoding = DetectedEncoding::UTF8;
      result.push_message(
        std::format("Encoding detection failed (confidence below threshold {}), defaulting to UTF-8",
                   confidence_threshold)
      );
    }

    // Step 3 & 4: Lazy transcoding pipeline: bytes → Unicode → Platform encoding
    // This creates a lazy view - no actual transcoding happens yet
    auto unicode_view = views::bytes_to_unicode(buffer, detected_encoding);
    auto platform_text_view = views::unicode_to_runtime_encoding(unicode_view);

    // Materialize the lazy view into a string
    // This is where the actual transcoding happens
    std::string transcoded_text;
    transcoded_text.reserve(buffer.size()); // Reasonable initial capacity

    for (char byte : platform_text_view) {
      transcoded_text.push_back(byte);
    }

    result.m_value = std::move(transcoded_text);
    result.push_message(
      std::format("Successfully transcoded {} bytes to {} Platform encoding bytes",
                 buffer.size(),
                 result.value().size())
    );

    return result;
  }

  /**
   * Variant that returns a lazy view instead of materializing the string.
   *
   * For very large files where you want to process content on-demand
   * without loading the entire transcoded text into memory.
   *
   * Note: The returned view holds references to the buffer, so ensure
   * the buffer's lifetime extends beyond the view's usage.
   *
   * @param buffer Byte buffer to transcode (must outlive the returned view)
   * @param confidence_threshold ICU confidence threshold for encoding detection
   * @return Lazy view producing Platform encoding bytes, or empty optional on detection failure
   */
  template<typename ByteBuffer>
  auto create_lazy_encoding_view(
    ByteBuffer const& buffer,
    int32_t confidence_threshold = icu::DEFAULT_CONFIDENCE_THERSHOLD
  ) -> std::optional<decltype(
      views::unicode_to_runtime_encoding(
        views::bytes_to_unicode(buffer, DetectedEncoding::UTF8)
      )
    )>
  {
    if (buffer.empty()) {
      return std::nullopt;
    }

    // Detect encoding
    auto encoding_result = icu::detect_buffer_encoding(buffer, confidence_threshold);

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

} // namespace text::encoding
