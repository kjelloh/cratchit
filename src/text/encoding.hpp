#pragma once

#include "std_overload.hpp"
#include "charset.hpp"
#include "MetaDefacto.hpp"
#include "functional/memory.hpp" // OwningMaybeRef,...
#include <array>
#include <iostream>
#include <sstream> // ostringstream,
#include <iomanip> // std::quoted,
#include <optional>
#include <deque>
#include <filesystem>
#include <vector>
#include <variant>
#include <unicode/errorcode.h> // ICU library UErrorCode,...
#include <span>

namespace text {
  namespace encoding {

    using ByteBuffer = std::vector<std::byte>;

    enum class EncodingID {
      Undefined,
      UTF8,
      UTF16BE,
      UTF16LE, 
      UTF32BE,
      UTF32LE,
      ISO_8859_1,
      ISO_8859_15,
      WINDOWS_1252,
      CP437,
      Unknown
    };

    std::string enum_to_display_name(EncodingID encoding);

    struct BOM {
      using value_type = std::array<unsigned char,3>; // Works for 3-byte BOM like UTF8
      value_type value;

      // TODO: Refactor to handle BOM of length other than 3 *sigh*
      // NOTE: It seems no BOM is more than 4 bytes (So 4 bytes with checking the third and fourth only if the first two and three match?)
      /*
      Encoding	              BOM
      UTF-8	                  EF BB BF
      UTF-16 (Big endian)	    FE FF
      UTF-16 (Little endian)	FF FE
      UTF-32 (Big endian)	    00 00 FE FF
      UTF-32 (Little endian)  FF FE 00 00
      UTF-7	                  2B 2F 76 followed by 38, 39, 2B, 2F, or 38 2D
      UTF-1	                  F7 64 4C
      UTF-EBCDIC	            DD 73 66 73
      SCSU	                  0E FE FF
      BOCU-1	                FB EE 28
      GB-18030	              84 31 95 33
      */    

      static constexpr BOM::value_type UTF8_VALUE{0xEF,0xBB,0xBF};
    };

    std::istream& operator>>(std::istream& is,BOM& bom);
    std::ostream& operator<<(std::ostream& os,BOM const& bom);

    namespace ISO_8859_1  {

    }

    namespace UTF8 {
      // Unicode constants
      // Leading (high) surrogates: 0xd800 - 0xdbff
      // Trailing (low) surrogates: 0xdc00 - 0xdfff
      const char32_t LEAD_SURROGATE_MIN  = 0x0000d800;
      const char32_t LEAD_SURROGATE_MAX  = 0x0000dbff;
      const char32_t TRAIL_SURROGATE_MIN = 0x0000dc00;
      const char32_t TRAIL_SURROGATE_MAX = 0x0000dfff;

      // Maximum valid value for a Unicode code point
      const char32_t CODE_POINT_MAX      = 0x0010ffff;

      inline bool is_surrogate(char32_t cp) {
            return (cp >= LEAD_SURROGATE_MIN && cp <= TRAIL_SURROGATE_MAX);
      }

      constexpr const char* REPLACEMENT_CHARACTER = "\uFFFD";

      bool is_valid_unicode(char32_t cp);

      std::string unicode_to_utf8(cratchit_unicode_string const& s);

      // UTF-8 to Unicode
      class ToUnicodeBuffer {
      public:
        using OptionalUnicode = std::optional<uint16_t>;
        OptionalUnicode push(uint8_t b);
      private:
        std::deque<uint8_t> m_utf_8_buffer{};
        OptionalUnicode to_unicode();
      };

      cratchit_unicode_string utf8ToUnicode(std::string const& s_utf8);


    } // namespace UTF8

    namespace CP437 {
    } // namespace CP437
    
    namespace unicode {
      
      struct to_utf8 {
        using value_type = std::string;
        value_type operator()(cratchit_unicode_string const& unicode_s) const;

      };

    } // namespace unicode

    namespace icu_facade_deprecated {

      // A facade to the ICU Unicode C++ library

      std::string to_string(UErrorCode status);

      using CanonicalEncodingName = std::string;

      // ICU encoding name -> cratchit EncodingID
      EncodingID canonical_name_to_enum(CanonicalEncodingName const& canonical_name);

      struct EncodingDetectionMeta {
        CanonicalEncodingName canonical_name; // ICU canonical name (e.g., "UTF-8")
        std::string display_name;             // Human readable name
        int32_t confidence;                   // ICU confidence (0-100)
        std::string language;                 // Detected language (e.g., "sv" for Swedish)
        std::string detection_method;         // "ICU", "BOM", "Extension", "Default"
      };

      using EncodingDetectionResult = MetaDefacto<EncodingDetectionMeta,EncodingID>;

      const int32_t DEFAULT_CONFIDENCE_THERSHOLD = 90;

      namespace maybe {

      } // maybe

    } // icu_facade_deprecated

  } // namespace encoding
} // text