#include "encoding_pipeline.hpp"

namespace text {
  namespace encoding {
    namespace monadic {

      AnnotatedMaybe<WithThresholdByteBuffer> ToWithThresholdF::operator()(ByteBuffer byte_buffer) const {
        return WithThresholdByteBuffer{confidence_threshold,std::move(byte_buffer)};
      }
      ToWithThresholdF to_with_threshold_step(int32_t confidence_threshold) {
        return ToWithThresholdF{confidence_threshold};
      }

    } // monadic
  } // encoding
} // text
