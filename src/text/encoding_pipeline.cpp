#include "encoding_pipeline.hpp"
#include "logger/log.hpp"

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
        logger::development_trace("to_with_detected_encoding_step");
        auto& [confidence_threshold,buffer] = wt_buffer;
        return text::encoding::inferred::maybe::to_inferred_encoding(buffer, confidence_threshold)
          .transform([buffer = std::move(buffer)](auto&& meta) mutable {

            auto [maybe_bom,stripped_buffer] = text::encoding::to_bom_and_buffer(buffer);

            // TODO: Consider to pass the BOM on?
            //       Or at least ensure the BOM matches the 'meta' that is the detected encoding?
            //       For now we are satisfied to have a detected enocing and have removed the BOM.
            return WithDetectedEncodingByteBuffer{
              .meta = std::forward<decltype(meta)>(meta)
              ,.defacto = std::move(stripped_buffer)
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

      AnnotatedMaybe<WithDetectedEncodingByteBuffer> to_with_detected_encoding_step(WithThresholdByteBuffer wt_buffer) {

        auto const& [confidence_threshold,_] = wt_buffer;

        auto to_msg = [](WithDetectedEncodingByteBuffer const& result) -> std::string {
          return std::format("Detected: {}",result.meta.meta.display_name);
        };

        auto lifted = cratchit::functional::_to_annotated_maybe_f(
          text::encoding::maybe::to_with_detected_encoding_step
          ,"with encoding"
          ,to_msg
        );

        auto result = lifted(wt_buffer);

        if (!result) {
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

        auto to_msg = [](std::string const& result) -> std::string {
          return std::format("{} bytes",result.size());
        };

        auto lifted = cratchit::functional::_to_annotated_maybe_f(
          text::encoding::maybe::to_platform_encoded_string_step
          ,"platform encoded"
          ,to_msg
        );

        result =  lifted(wd_buffer);

        return result;
        
      } // to_platform_encoded_string_step

    } // monadic

    AnnotatedMaybe<std::string> path_to_platform_encoded_string_shortcut(
      std::filesystem::path const& file_path,
      int32_t confidence_threshold) {

      return persistent::in::text::path_to_byte_buffer_shortcut(file_path) // #1 + #2
        .and_then(monadic::to_with_threshold_step_f(confidence_threshold))
        .and_then(monadic::to_with_detected_encoding_step) // #4
        .and_then(monadic::to_platform_encoded_string_step); // #5
    }

  } // encoding
} // text
