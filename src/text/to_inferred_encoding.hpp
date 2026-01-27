#pragma once

#include "encoding.hpp"

namespace text {
  namespace encoding {

    namespace inferred {

      const int32_t DEFAULT_CONFIDENCE_THERSHOLD = 90;

      using CanonicalEncodingName = std::string;

      EncodingID canonical_name_to_enum(CanonicalEncodingName const& canonical_name);

      struct EncodingDetectionMeta {
        CanonicalEncodingName canonical_name; // ICU canonical name (e.g., "UTF-8")
        std::string display_name;             // Human readable name
        int32_t confidence;                   // ICU confidence (0-100)
        std::string language;                 // Detected language (e.g., "sv" for Swedish)
        std::string detection_method;         // "ICU", "BOM", "Extension", "Default"
      };

      using EncodingDetectionResult = MetaDefacto<EncodingDetectionMeta,EncodingID>;

      namespace maybe {
        std::optional<EncodingDetectionResult> to_content_encoding(
          char const* data
          ,size_t length
          ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD);

        template<typename ByteBuffer>
        std::optional<EncodingDetectionResult> to_detetced_encoding(
            ByteBuffer const& buffer
          ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD) {
          if (buffer.empty()) {
            return std::nullopt;
          }
          return maybe::to_content_encoding(
            reinterpret_cast<char const*>(buffer.data())
            ,buffer.size()
            ,confidence_threshold);
        } // to_detetced_encoding

      } // maybe

      std::vector<EncodingDetectionResult> to_encoding_options(
         char const* data
        ,size_t length
        ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD);

      EncodingDetectionResult to_bom_encoding(std::istream& is);
      EncodingDetectionResult to_bom_encoding(std::filesystem::path const& file_path);

    } // inferred

    namespace icu_facade {

      // A facade to the ICU Unicode C++ library

      std::string to_string(UErrorCode status);

      using CanonicalEncodingName = std::string;

      struct EncodingDetectionResult {
        CanonicalEncodingName canonical_name; // ICU canonical name (e.g., "UTF-8")
        int32_t confidence;                   // ICU confidence (0-100)
      }; // EncodingDetectionResult

      namespace maybe {

        std::optional<EncodingDetectionResult> to_content_encoding(
          char const* data
          ,size_t length
          ,int32_t confidence_threshold);

      } // maybe
        
    } // icu_facade

  } // namespace encoding
} // text