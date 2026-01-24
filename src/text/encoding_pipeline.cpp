#include "encoding_pipeline.hpp"

namespace text {
  namespace encoding {

    namespace maybe {
      std::optional<WithThresholdByteBuffer> ToWithThresholdF::operator()(ByteBuffer byte_buffer) const {
        return WithThresholdByteBuffer{confidence_threshold,std::move(byte_buffer)};
      }
      ToWithThresholdF to_with_threshold_step_f(int32_t confidence_threshold) {
        return ToWithThresholdF{confidence_threshold};
      }

      std::optional<WithDetectedEncodingByteBuffer> to_with_detected_encoding_step(WithThresholdByteBuffer wt_buffer) {
        return {};
      }

    } // maybe

    namespace monadic {

      ToWithThresholdF to_with_threshold_step_f(int32_t confidence_threshold) {   
        auto lifted = cratchit::functional::to_annotated_maybe_f(
           text::encoding::maybe::to_with_threshold_step_f(confidence_threshold)
          ,"Failed to pair confidence_threshold with byte buffer"
        );
        return lifted;
      }

      AnnotatedMaybe<WithDetectedEncodingByteBuffer> to_with_detected_encoding_step(WithThresholdByteBuffer wt_buffer) {

        AnnotatedMaybe<WithDetectedEncodingByteBuffer> result{};

        auto const& [confidence_threshold,buffer] = wt_buffer;
        auto encoding_result = icu_facade::maybe::to_detetced_encoding(buffer, confidence_threshold);

        if (encoding_result) {      
          result = WithDetectedEncodingByteBuffer{
             .meta = encoding_result.value()
            ,.defacto = std::move(wt_buffer.defacto)
          };
          result.push_message(
            std::format("Detected encoding: {} (confidence: {}, method: {})",
                      encoding_result->meta.display_name,
                      encoding_result->meta.confidence,
                      encoding_result->meta.detection_method)
          );
        } 
        else {
          // Default to UTF-8 on detection failure (permissive strategy)
          result = WithDetectedEncodingByteBuffer{
            .meta = text::encoding::icu_facade::EncodingDetectionResult{
              .meta = {
                "UTF-8"
                ,"UTF-8"
                ,100
                ,""
                ,"Assumed"
              }
              ,.defacto = DetectedEncoding::UTF8
            }
            ,.defacto = std::move(wt_buffer.defacto)
          };
          result.push_message(
            std::format(
              "Encoding detection failed (confidence below threshold {}), defaulting to UTF-8"
              ,confidence_threshold));
        }

        return result;

      } // to_with_detected_encoding_step

      AnnotatedMaybe<std::string> to_platform_encoded_string_step(
          WithDetectedEncodingByteBuffer wd_buffer) {

        AnnotatedMaybe<std::string> result{};

        auto const& [detected_encoding, buffer] = wd_buffer;

        if (buffer.empty()) {
          result.push_message("Empty buffer - no content to transcode");
          return result;  // Return failure (nullopt)
        }

        // Create lazy transcoding pipeline: bytes → Unicode → Platform encoding
        auto unicode_view = views::bytes_to_unicode(buffer, detected_encoding.defacto);
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
      } // to_platform_encoded_string_step

    } // monadic
  } // encoding
} // text
