#include "to_inferred_encoding.hpp"
// #include <deque>
// #include <iomanip>
#include <fstream>
// #include <filesystem>
// #include <vector>
// #include <algorithm>
#include <unicode/ucsdet.h>  // ICU Character Set Detection
// #include <unicode/ucnv.h>    // ICU Converter API
// #include <unicode/ustring.h> // ICU String utilities
#include "logger/log.hpp"

namespace text {
  namespace encoding {

    namespace inferred {

      EncodingID canonical_name_to_enum(CanonicalEncodingName const& canonical_name) {
        if (canonical_name == "Undefined") return EncodingID::Undefined;
        if (canonical_name == "UTF-8") return EncodingID::UTF8;
        if (canonical_name == "UTF-16BE") return EncodingID::UTF16BE;
        if (canonical_name == "UTF-16LE") return EncodingID::UTF16LE;
        if (canonical_name == "UTF-32BE") return EncodingID::UTF32BE;
        if (canonical_name == "UTF-32LE") return EncodingID::UTF32LE;
        if (canonical_name == "ISO-8859-1") return EncodingID::ISO_8859_1;
        if (canonical_name == "ISO-8859-15") return EncodingID::ISO_8859_15;
        if (canonical_name == "windows-1252") return EncodingID::WINDOWS_1252;
        if (canonical_name == "IBM437") return EncodingID::CP437;
        return EncodingID::Unknown;
      }

      namespace maybe {

        std::optional<EncodingDetectionResult> to_content_encoding(
          char const* data
          ,size_t length
          ,int32_t confidence_threshold) {

          auto maybe_icu_result = text::encoding::icu_facade::maybe::to_content_encoding(data,length,confidence_threshold);

          if (maybe_icu_result) {
            // NOTE: We trust ICU to return the expected canocical encoding name.
            //       In fact, we reversed engineered our canonical name from what ICU uses.
            //       But this means we are bnot free to invent our own names down the line!
            auto encoding = canonical_name_to_enum(maybe_icu_result->canonical_name);
            return EncodingDetectionResult{
              .meta = {
                 .canonical_name = maybe_icu_result->canonical_name
                ,.display_name = text::encoding::enum_to_display_name(encoding)
                ,.confidence = maybe_icu_result->confidence
                ,.language = "??"
                ,.detection_method = "ICU"
              }
              ,.defacto = encoding
            };
          }

          return {}; // TODO: Refactor from icu_facade_deprecated in encoding unit
        }

      } // maybe

    } // inferred

    namespace icu_facade {

      std::string to_string(UErrorCode status) {
        return std::format(
            "{} ()"
          ,static_cast<int>(status)
          ,u_errorName(status));
      }

      // ICU-based Encoding Detection Implementation

      namespace maybe {

        std::optional<EncodingDetectionResult> to_content_encoding(
          char const* data
          ,size_t length
          ,int32_t confidence_threshold) {

          UErrorCode status = U_ZERO_ERROR;
          
          // Create ICU character set detector
          UCharsetDetector* detector = ucsdet_open(&status);
          if (U_FAILURE(status) || !detector) {
            logger::development_trace("to_content_encoding: ICU detector creation failed. status:{}" 
              ,to_string(status));
            return {};
          }
          
          // Set the input data
          ucsdet_setText(detector, data, static_cast<int32_t>(length), &status);
          if (U_FAILURE(status)) {
            ucsdet_close(detector);
            logger::development_trace("ICU setText failed. status:{}"
              ,to_string(status));
            return {};
          }
          
          // Detect the character set
          const UCharsetMatch* match = ucsdet_detect(detector, &status);
          if (U_FAILURE(status) || !match) {
            ucsdet_close(detector);
            logger::development_trace("ICU failed to detect the character set. status:{}"
              ,to_string(status));
            return {};
          }
                    
          if (U_FAILURE(status)) {
            ucsdet_close(detector);
            logger::development_trace(
              "ICU result extraction of canonical_name,confidence or language failed. status:{}"
              ,to_string(status));
            return {};
          }
          
          // Extract results
          const char* canonical_name = ucsdet_getName(match, &status);
          int32_t confidence = ucsdet_getConfidence(match, &status);

          // // Convert to our types
          // std::string canonical_str = canonical_name ? canonical_name : "UTF-8";
          // std::string language_str = language ? language : "";
          // EncodingID encoding = canonical_name_to_enum(canonical_str);
          // std::string display_name = enum_to_display_name(encoding);
          
          ucsdet_close(detector);
          
          return EncodingDetectionResult{
               canonical_name
              ,confidence
          };
          return {}; // TODO: Refactor from icu_facade_deprecated in encoding unit
        }

      } // maybe


    } // icu_facade

  } // namespace encoding
} // text