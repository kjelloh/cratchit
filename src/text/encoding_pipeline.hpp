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

namespace text {

  namespace encoding {

    using ByteBuffer = persistent::in::ByteBuffer;

    namespace monadic {

      // Monadic AnnotatedMaybe #3: byte buffer -> (threshold,byte buffer) pair
      using WithThresholdByteBuffer = MetaDefacto<int32_t,ByteBuffer>;
      inline AnnotatedMaybe<WithThresholdByteBuffer> to_with_threshold_step(int32_t confidence_threshold, ByteBuffer byte_buffer) {
        return WithThresholdByteBuffer{confidence_threshold,std::move(byte_buffer)};
      }

      // Monadic AnnotatedMaybe #4: (threshold,byte buffer) pair -> (detected encoding,byte buffer) pair
      using WithDetectedEncodingByteBuffer = MetaDefacto<DetectedEncoding,ByteBuffer>;
      inline AnnotatedMaybe<WithDetectedEncodingByteBuffer> to_with_detected_encoding_step(WithThresholdByteBuffer wt_buffer) {

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

      // Monadic AnnotatedMaybe #5: (detected encoding,byte buffer) pair -> string (platform encoding)
      inline AnnotatedMaybe<std::string> to_platform_encoded_string_step(
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

    }

    // Monadic AnnotatedMaybe #1 ... #5 shortcut
    inline AnnotatedMaybe<std::string> path_to_platform_encoded_string_shortcut(
      std::filesystem::path const& file_path,
      int32_t confidence_threshold = icu::DEFAULT_CONFIDENCE_THERSHOLD) {

      return persistent::in::path_to_byte_buffer_shortcut(file_path) // #1 + #2
        .and_then([confidence_threshold](ByteBuffer buffer) {
          return monadic::to_with_threshold_step(confidence_threshold, std::move(buffer)); // #3
        })
        .and_then(monadic::to_with_detected_encoding_step) // #4
        .and_then(monadic::to_platform_encoded_string_step); // #5
    }

  } // encoding

} // text
