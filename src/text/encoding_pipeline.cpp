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
             .meta = encoding_result->defacto
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
            .meta = DetectedEncoding::UTF8
            ,.defacto = std::move(wt_buffer.defacto)
          };
          result.push_message(
            std::format(
              "Encoding detection failed (confidence below threshold {}), defaulting to UTF-8"
              ,confidence_threshold));
        }

        return result;

      } // to_with_detected_encoding_step

    } // monadic
  } // encoding
} // text
