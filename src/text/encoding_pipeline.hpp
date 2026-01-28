#pragma once

// #include "encoding.hpp"
#include "to_inferred_encoding.hpp"
#include "transcoding_views.hpp"
#include "persistent/in/raw_text_read.hpp"
#include "MetaDefacto.hpp"
#include <filesystem>
#include <string>
#include <format>
#include <ranges>
#include <algorithm>

namespace text {

  namespace encoding {

    using ByteBuffer = persistent::in::text::ByteBuffer;
    using WithThresholdByteBuffer = MetaDefacto<int32_t,ByteBuffer>;
    // using WithDetectedEncodingByteBuffer = MetaDefacto<EncodingID,ByteBuffer>;
    using WithDetectedEncodingByteBuffer = MetaDefacto<inferred::EncodingDetectionResult,ByteBuffer>;

    namespace maybe {
      struct ToWithThresholdF {
          int32_t confidence_threshold; 

          std::optional<WithThresholdByteBuffer> operator()(ByteBuffer byte_buffer) const;
      };
      ToWithThresholdF to_with_threshold_step_f(int32_t confidence_threshold);

      std::optional<WithDetectedEncodingByteBuffer> to_with_detected_encoding_step(WithThresholdByteBuffer wt_buffer);
      std::optional<std::string> to_platform_encoded_string_step(WithDetectedEncodingByteBuffer wd_buffer);



    }

    namespace monadic {

      using ToWithThresholdF =
        cratchit::functional::annotated_maybe_f_t<
          decltype(text::encoding::maybe::to_with_threshold_step_f(0))
        >;

      ToWithThresholdF to_with_threshold_step_f(int32_t confidence_threshold);    
      AnnotatedMaybe<WithDetectedEncodingByteBuffer> to_with_detected_encoding_step(WithThresholdByteBuffer wt_buffer);
      AnnotatedMaybe<std::string> to_platform_encoded_string_step(WithDetectedEncodingByteBuffer wd_buffer);


    } // monadic

    // Monadic AnnotatedMaybe #1 ... #5 shortcut
    inline AnnotatedMaybe<std::string> path_to_platform_encoded_string_shortcut(
      std::filesystem::path const& file_path,
      int32_t confidence_threshold = inferred::DEFAULT_CONFIDENCE_THERSHOLD) {

      return persistent::in::text::path_to_byte_buffer_shortcut(file_path) // #1 + #2
        .and_then(monadic::to_with_threshold_step_f(confidence_threshold))
        .and_then(monadic::to_with_detected_encoding_step) // #4
        .and_then(monadic::to_platform_encoded_string_step); // #5
    }

  } // encoding

} // text
