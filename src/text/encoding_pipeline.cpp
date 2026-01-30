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
        auto& [confidence_threshold,buffer] = wt_buffer;
        return text::encoding::inferred::maybe::to_inferred_encoding(buffer, confidence_threshold)
          .transform([buffer = std::move(buffer)](auto&& meta){
            return WithDetectedEncodingByteBuffer{
              .meta = std::forward<decltype(meta)>(meta)
              ,.defacto = std::move(buffer)
            };
          });
      } // to_with_detected_encoding_step

      std::optional<std::string> to_platform_encoded_string_step(WithDetectedEncodingByteBuffer wd_buffer) {
        auto const& [detected_encoding_result, buffer] = wd_buffer;
        auto const& [_,deteced_encoding] = detected_encoding_result;

        // buffer -> unicode -> runtime encoding
        auto unicode_view = views::bytes_to_unicode(buffer, deteced_encoding);
        auto platform_encoding_view = views::unicode_to_runtime_encoding(unicode_view);

        std::string transcoded;
        for (auto const code_point : platform_encoding_view) {
          transcoded.push_back(code_point);
        }

        return transcoded;
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

        auto const& [confidence_threshold,_] = wt_buffer;
        auto lifted = cratchit::functional::to_annotated_maybe_f(
           text::encoding::maybe::to_with_detected_encoding_step
          ,std::format(
              "Encoding detection failed (confidence below threshold {}), defaulting to UTF-8"
              ,confidence_threshold)
        );

        auto result = lifted(wt_buffer);

        if (result) {
          // TODO: Consider what we can do better for stacked MetaDefacto?
          //       Ideally we would like to 'flatten' metas so we don't
          //       get meta.meta... as below?
          //       I suppose we have to manually define new meta-types that
          //       aggregates stacked metas? (Is it worth it?)
          result.push_message(
            std::format("Detected encoding: {} (confidence: {}, method: {})",
                      result.value().meta.meta.display_name,
                      result.value().meta.meta.confidence,
                      result.value().meta.meta.detection_method)
          );
        }
        else {
          // Default to UTF-8 on detection failure (permissive strategy)
          // TODO 20260124 - Consider to remove this else?
          //                 It seems no test even triggers this code?
          //                 Or, the detection logic already defaults to UTF-8 (never nullopt:s)?
          result = WithDetectedEncodingByteBuffer{
            .meta = text::encoding::inferred::EncodingDetectionResult{
              .meta = {
                "UTF-8"
                ,"UTF-8"
                ,100
                ,""
                ,"Assumed"
              }
              ,.defacto = EncodingID::UTF8
            }
            ,.defacto = std::move(wt_buffer.defacto)
          };
        }

        return result;

      } // to_with_detected_encoding_step

      AnnotatedMaybe<std::string> to_platform_encoded_string_step(WithDetectedEncodingByteBuffer wd_buffer) {

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
