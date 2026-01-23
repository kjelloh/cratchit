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
    } // maybe

    namespace monadic {

      AnnotatedMaybe<WithThresholdByteBuffer> ToWithThresholdF::operator()(ByteBuffer byte_buffer) const {
        auto lifted = cratchit::functional::to_annotated_maybe_f(
           text::encoding::maybe::to_with_threshold_step_f(confidence_threshold)
          ,"Failed to pair confidence_threshold with byte buffer"
        );
        return lifted(byte_buffer);
      }
      ToWithThresholdF to_with_threshold_step_f(int32_t confidence_threshold) {
        return ToWithThresholdF{confidence_threshold};
      }

    } // monadic
  } // encoding
} // text
