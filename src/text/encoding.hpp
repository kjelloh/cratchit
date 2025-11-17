#pragma once

#include "std_overload.hpp"
#include "charset.hpp"
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

namespace text {
  namespace encoding {

    enum class DetectedEncoding {
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

    std::string enum_to_display_name(DetectedEncoding encoding);

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

    class bom_istream {
    public:
        std::istream& raw_in;
        std::optional<BOM> bom{};
        bom_istream(std::istream& in);
        operator bool();

      // TODO: Move base class members from derived 8859_1 and UTF-8 istream classes to here
    private:
    };

    namespace ISO_8859_1  {

      class istream : public bom_istream {
      public:
        istream(std::istream& in);
        // getline: Transcodes input from ISO8859-1 encoding to Unicode and then applies F to decode it to target encoding (std::nullopt on failure)
        // Emtpy line is allowed (returns nullopt only on failure to read)
        template <class F>
        std::optional<typename F::value_type> getline(F const& f) {
          typename std::optional<typename F::value_type> result{};
          std::string raw_entry{};
          if (std::getline(raw_in,raw_entry)) {
            auto unicode_s = charset::ISO_8859_1::iso8859ToUnicode(raw_entry);
            result = f(unicode_s);
          }
          // if (result.size() > 0) return result;
          // else return std::nullopt;
          return result;
        }
      };

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

      bool is_valid_unicode(char32_t cp);

      struct ostream {
        std::ostream& os;
      };

      text::encoding::UTF8::ostream& operator<<(text::encoding::UTF8::ostream& os,char32_t cp);
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

      class istream : public bom_istream {
      public:
        istream(std::istream& in);
        // getline: Transcodes input from UTF8 encoding to Unicode and then applies F to decode it to target encoding (std::nullopt on failure)
        // Emtpy line is allowed (returns nullopt only on failure to read)
        template <class F>
        std::optional<typename F::value_type> getline(F const& f) {
          typename std::optional<typename F::value_type> result{};
          std::string raw_entry{};
          if (std::getline(raw_in,raw_entry)) {
            auto unicode_s = text::encoding::UTF8::utf8ToUnicode(raw_entry);
            result = f(unicode_s);
          }
          // if (result.size() > 0) return result;
          // else return std::nullopt;
          return result;
        }
      };

    } // namespace UTF8

    namespace CP437 {
      class istream : public bom_istream {
      public:
        istream(std::istream& in);
        // getline: Transcodes input from CP437 encoding to Unicode and then applies F to decode it to target encoding (std::nullopt on failure)
        // Emtpy line is allowed (returns nullopt only on failure to read)
        template <class F>
        std::optional<typename F::value_type> getline(F const& f) {
          typename std::optional<typename F::value_type> result{};
          std::string raw_entry{};
          if (std::getline(raw_in,raw_entry)) {
            auto unicode_s = charset::CP437::cp437ToUnicode(raw_entry);
            result = f(unicode_s);
          }
          // if (result.size() > 0) return result;
          // else return std::nullopt;
          return result;
        }
      };    
    } // namespace CP437
    
    namespace unicode {

      using CodePoint = 

      struct to_utf8 {
        using value_type = std::string;
        value_type operator()(cratchit_unicode_string const& unicode_s) const;

      };
    } // namespace unicode

    namespace icu {

      std::string to_string(UErrorCode status);

      // Encoding Detection Service using ICU

      using CanonicalEncodingName = std::string;

      struct EncodingDetectionResult {
        DetectedEncoding encoding;
        CanonicalEncodingName canonical_name;    // ICU canonical name (e.g., "UTF-8")
        std::string display_name;      // Human readable name
        int32_t confidence;            // ICU confidence (0-100)
        std::string language;          // Detected language (e.g., "sv" for Swedish)
        std::string detection_method;  // "ICU", "BOM", "Extension", "Default"
      };

      const int32_t DEFAULT_CONFIDENCE_THERSHOLD = 90;

      std::optional<EncodingDetectionResult> detect_buffer_encoding(
         char const* data
        ,size_t length
        ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD);
      std::vector<EncodingDetectionResult> detect_all_possible_encodings(char const* data, size_t length);
      std::optional<EncodingDetectionResult> detect_istream_encoding(
        std::istream& is
        ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD);
      std::optional<EncodingDetectionResult> detect_file_encoding(
        std::filesystem::path const& file_path
        ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD);
      
      // Utility functions for encoding enum conversion
      DetectedEncoding canonical_name_to_enum(CanonicalEncodingName const& canonical_name);
      
      EncodingDetectionResult detect_by_bom(std::istream& file);

      EncodingDetectionResult detect_by_bom(std::filesystem::path const& file_path);
      EncodingDetectionResult detect_by_extension_heuristics(std::filesystem::path const& file_path);


    } // icu

    // TODO: Consider a 'better' (??) way to provide a 'in stream' that knows how o decode?
    //       Is there a way to 'hide' the actual decoder?
    using DecodingIn = std::variant<
      text::encoding::UTF8::istream
      ,text::encoding::ISO_8859_1::istream
      ,text::encoding::CP437::istream
    >;
    using MaybeDecodingIn = cratchit::functional::memory::OwningMaybeRef<DecodingIn>;
    MaybeDecodingIn to_decoding_in(
       icu::EncodingDetectionResult const& detected_source_encoding
      ,std::istream& is);

  } // namespace encoding
} // text