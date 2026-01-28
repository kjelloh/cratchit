#include "encoding.hpp"
#include <deque>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <unicode/ucsdet.h>  // ICU Character Set Detection
#include <unicode/ucnv.h>    // ICU Converter API
#include <unicode/ustring.h> // ICU String utilities
#include "logger/log.hpp"

namespace text {
  namespace encoding {

    std::string enum_to_display_name(EncodingID encoding) {
      switch (encoding) {
        case EncodingID::Undefined: return "Undefined";
        case EncodingID::UTF8: return "UTF-8";
        case EncodingID::UTF16BE: return "UTF-16 Big Endian";
        case EncodingID::UTF16LE: return "UTF-16 Little Endian";
        case EncodingID::UTF32BE: return "UTF-32 Big Endian";
        case EncodingID::UTF32LE: return "UTF-32 Little Endian";
        case EncodingID::ISO_8859_1: return "ISO-8859-1 (Latin-1)";
        case EncodingID::ISO_8859_15: return "ISO-8859-15 (Latin-9)";
        case EncodingID::WINDOWS_1252: return "Windows-1252";
        case EncodingID::CP437: return "CP437 (DOS)";
        case EncodingID::Unknown: return "Unknown";
      }
      return "Unknown";
    }


    std::istream& operator>>(std::istream& is,BOM& bom) {
      using std_overload::operator>>; // to 'see' the defined overload for std::array
      using std_overload::operator<<; // to 'see' the defined overload for std::array
      auto original_pos = is.tellg();
      if (is >> bom.value) {
        if (bom.value == BOM::UTF8_VALUE) {      
          // std::cout << "\noperator>>(BOM) consumed from read file. bom:" << bom.value;
        }
        else {
          // std::cout << "\noperator>>(BOM) Not a valid BOM - file position is reset";
          // NOTE 240613 - Most vexing semantics: seekg does nothing if the failbit is set!!
          //               So it is important we do not set the failbit until after setting the file position *sigh*
          is.seekg(original_pos); // Restore to before checking for BOM in case there was none
          is.setstate(std::ios_base::failbit); // Signal the failure
        }
      }
      else {
        // failed to even read the bytes
      }
      return is;
    }

    std::ostream& operator<<(std::ostream& os,BOM const& bom) {
      using std_overload::operator<<; // to 'see' the defined overload for std::array
      os << bom.value;
      return os;
    }

    bom_istream::bom_istream(std::istream& in) : raw_in{in} {
      // Check for BOM in fresh input stream
      BOM candidate{};

      if (raw_in >> candidate) {
        logger::cout_proxy << "\nConsumed BOM:" << candidate;
        this->bom = candidate;
      }
      else {
        // std::cout << "\nNo BOM detected in stream";
        raw_in.clear(); // clear the signalled failure to allow the stream to be read for non-BOM content
      }
      // std::cout << "\nbom_istream{} raw_in is at position:" << raw_in.tellg();
    }

    bom_istream::operator bool() {
      return static_cast<bool>(raw_in);
    }

    namespace UTF8 {

      bool is_valid_unicode(char32_t cp) {
        return (cp <= CODE_POINT_MAX && !is_surrogate(cp));
      }

      text::encoding::UTF8::ostream& operator<<(text::encoding::UTF8::ostream& os,char32_t cp) {
        // Thanks to https://sourceforge.net/p/utfcpp/code/HEAD/tree/v3_0/src/utf8.h
        if (!is_valid_unicode(cp)) {
          os.os << '?';
        }
        else if (cp < 0x80)                        // one octet
            os.os << static_cast<char>(cp);
        else if (cp < 0x800) {                // two octets
            os.os << static_cast<char>((cp >> 6)            | 0xc0);
            os.os << static_cast<char>((cp & 0x3f)          | 0x80);
        }
        else if (cp < 0x10000) {              // three octets
            os.os << static_cast<char>((cp >> 12)           | 0xe0);
            os.os << static_cast<char>(((cp >> 6) & 0x3f)   | 0x80);
            os.os << static_cast<char>((cp & 0x3f)          | 0x80);
        }
        else {                                // four octets
            os.os << static_cast<char>((cp >> 18)           | 0xf0);
            os.os << static_cast<char>(((cp >> 12) & 0x3f)  | 0x80);
            os.os << static_cast<char>(((cp >> 6) & 0x3f)   | 0x80);
            os.os << static_cast<char>((cp & 0x3f)          | 0x80);
        }
        return os;
      }

      std::string unicode_to_utf8(cratchit_unicode_string const& s) {
        std::ostringstream os{};
        text::encoding::UTF8::ostream utf8_os{os};
        for (auto cp : s) utf8_os << cp;
        if (false) {
          logger::cout_proxy << "\nunicode_to_utf8(";
          for (auto ch : s) logger::cout_proxy << " " << std::hex << static_cast<unsigned int>(ch) << std::dec;
          logger::cout_proxy << ") --> " << std::quoted(os.str());
        }
        return os.str();
      }

      ToUnicodeBuffer::OptionalUnicode ToUnicodeBuffer::push(uint8_t b) {
        OptionalUnicode result{};
        // See https://en.wikipedia.org/wiki/UTF-8#Encoding
        // Code point <-> UTF-8 conversion
        // First code point	Last code point	Byte 1		Byte 2    Byte 3    Byte 4
        // U+0000	U+007F										0xxxxxxx	
        // U+0080	U+07FF										110xxxxx	10xxxxxx	
        // U+0800	U+FFFF										1110xxxx	10xxxxxx	10xxxxxx	
        // U+10000	[nb 2]U+10FFFF					11110xxx	10xxxxxx	10xxxxxx	10xxxxxx
        this->m_utf_8_buffer.push_back(b);
        if (false) {
          logger::cout_proxy << "\nToUnicodeBuffer::push(" << std::hex << static_cast<unsigned int>(b) << ")" << std::dec;
          logger::cout_proxy << ":size:" << std::dec << m_utf_8_buffer.size() << " ";
          for (auto ch : m_utf_8_buffer) logger::cout_proxy << "[" << std::hex << static_cast<unsigned int>(ch) << "]" << std::dec;
        }
        return this->to_unicode();
      }

      ToUnicodeBuffer::OptionalUnicode ToUnicodeBuffer::to_unicode() {
        OptionalUnicode result{};
        switch (m_utf_8_buffer.size()) {
          case 1: {
            if (m_utf_8_buffer[0] <= 0x7F) {
              result = m_utf_8_buffer[0];
              m_utf_8_buffer.clear();
            }
          } break;
          case 2: {
            if (m_utf_8_buffer[0]>>5 == 0b110) {
              uint16_t wch = (m_utf_8_buffer[0] & 0b00011111) << 6;
              wch += (m_utf_8_buffer[1] & 0b00111111);
              result = wch;
              m_utf_8_buffer.clear();
            }
          } break;
          case 3: {
            if (m_utf_8_buffer[0]>>4 == 0b1110) {
              uint16_t wch = (m_utf_8_buffer[0] & 0b00001111) << 6;
              wch += (m_utf_8_buffer[1] & 0b00111111);
              wch = (wch << 6) + (m_utf_8_buffer[2] & 0b00111111);
              result = wch;
              m_utf_8_buffer.clear();
            }
          } break;
          default: {
            // We don't support Unicodes above the range U+0800	U+FFFF
            m_utf_8_buffer.clear(); // reset
          }
        }
        return result;
      }

      cratchit_unicode_string utf8ToUnicode(std::string const& s_utf8) {
        cratchit_unicode_string result{};
        ToUnicodeBuffer to_unicode_buffer{};
        for (auto ch : s_utf8) {
          if (auto unicode = to_unicode_buffer.push(static_cast<std::uint8_t>(ch))) {
            result += *unicode;
          }
        }
        if (false) {
          logger::cout_proxy << "\nutf8ToUnicode(";
          for (auto ch : s_utf8) logger::cout_proxy << " " << std::hex << static_cast<unsigned int>(ch) << std::dec;
          logger::cout_proxy << ") --> ";
          for (auto ch : result) logger::cout_proxy << " " << std::hex << static_cast<unsigned int>(ch) << std::dec;
        }

        return result;
      }

    } // namespace UTF8

    namespace ISO_8859_1 {

      istream::istream(std::istream& in) : bom_istream{in} {
        if (this->bom) {
          // We expect the input stream to be without BOM
          logger::cout_proxy << "\nSorry, Expected ISO8859-1 stream but found bom:" << *(this->bom);

          this->raw_in.setstate(std::ios_base::failbit); // disable reading this stream
        }
      }

    } // namespace ISO_8859_1

    namespace CP437 {

      istream::istream(std::istream& in) : bom_istream{in} {
        if (this->bom) {
          // We expect the input stream to be without BOM
          logger::cout_proxy << "\nSorry, Expected CP437 (SIE-file) stream but found bom:" << *(this->bom);

          this->raw_in.setstate(std::ios_base::failbit); // disable reading this stream
        }
      }

    } // namespace CP437

    namespace UTF8 {

      istream::istream(std::istream& in) : bom_istream{in} {
        if (this->bom) {
          if (this->bom->value != BOM::UTF8_VALUE) {
            // We expect the input stream to be in UTF8 and thus any BOM must confirm this
            logger::cout_proxy << "\nSorry, Expected an UTF8 input stream but found contradicting BOM:" << *(this->bom);

            this->raw_in.setstate(std::ios_base::failbit); // disable reading this stream
          }
        }
      }

    } // namespace UTF8

    namespace unicode {

      std::string to_utf8::operator()(cratchit_unicode_string const& unicode_s) const {
        return text::encoding::UTF8::unicode_to_utf8(unicode_s);
      }

    } // unicode

    namespace icu_facade_deprecated{

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
          
          // Extract results
          const char* canonical_name = ucsdet_getName(match, &status);
          int32_t confidence = ucsdet_getConfidence(match, &status);
          const char* language = ucsdet_getLanguage(match, &status);
          
          if (U_FAILURE(status)) {
            ucsdet_close(detector);
            logger::development_trace(
              "ICU result extraction of canonical_name,confidence or language failed. status:{}"
              ,to_string(status));
            return {};
          }
          
          // Convert to our types
          std::string canonical_str = canonical_name ? canonical_name : "UTF-8";
          std::string language_str = language ? language : "";
          EncodingID encoding = canonical_name_to_enum(canonical_str);
          std::string display_name = enum_to_display_name(encoding);
          
          ucsdet_close(detector);
          
          return EncodingDetectionResult{
            .meta = {
               canonical_str
              ,display_name
              ,confidence
              ,language_str
              ,"ICU"            
            }
            ,.defacto = encoding
          };
        }

      } // maybe

      std::vector<EncodingDetectionResult> to_encoding_options(
         char const* data
        ,size_t length
        ,int32_t confidence_threshold) {

        std::vector<EncodingDetectionResult> results;
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

        
        // Get all possible matches
        int32_t match_count;
        const UCharsetMatch** matches = ucsdet_detectAll(detector, &match_count, &status);
        if (U_FAILURE(status) || !matches) {
          ucsdet_close(detector);
          logger::development_trace("ICU 'detect all' failed. status:{}"
            ,to_string(status));
          return {};
        }
        
        // Convert all matches to EncodingDetectionResult
        for (int32_t i = 0; i < match_count; ++i) {
          const char* canonical_name = ucsdet_getName(matches[i], &status);
          int32_t confidence = ucsdet_getConfidence(matches[i], &status);
          const char* language = ucsdet_getLanguage(matches[i], &status);
          
          if (U_SUCCESS(status) && canonical_name) {
            std::string canonical_str = canonical_name;
            std::string language_str = language ? language : "";
            EncodingID encoding = canonical_name_to_enum(canonical_str);
            std::string display_name = enum_to_display_name(encoding);

            if (confidence >= confidence_threshold) {
              results.push_back(EncodingDetectionResult{
                .meta = {
                   canonical_str
                  ,display_name
                  ,confidence
                  ,language_str
                  ,"ICU"
                }
                ,.defacto = encoding
              });
            }
            else {
              logger::development_trace(
                "Skipped ICU match - Confidence:{} < provided threshold:{} for match:{}"
                ,confidence
                ,confidence_threshold
                ,i);
            }
          }
          else {
          logger::development_trace(
             "ICU extract canonical_name, confidence or language failed for match:{} . status:{}"
            ,i
            ,to_string(status));
          }
        }
        
        ucsdet_close(detector);
        return results;
      }

      EncodingDetectionResult to_bom_encoding(std::filesystem::path const& file_path) {
        std::ifstream file(file_path, std::ios::binary);
        return to_bom_encoding(file);
      }

      EncodingDetectionResult to_bom_encoding(std::istream& file) {
        if (!file) {
          return EncodingDetectionResult{
            .meta = {
              ""
              ,"Unknown"
              ,0
              ,""
              ,"Cannot open file"
            }
            ,.defacto = EncodingID::Unknown
          };
        }
        
        // Read first few bytes for BOM detection
        std::array<unsigned char, 4> bom_bytes{};
        file.read(reinterpret_cast<char*>(bom_bytes.data()), bom_bytes.size());
        size_t bytes_read = file.gcount();
        
        if (bytes_read >= 3) {
          // UTF-8 BOM: EF BB BF
          if (bom_bytes[0] == 0xEF && bom_bytes[1] == 0xBB && bom_bytes[2] == 0xBF) {
            return EncodingDetectionResult{
              .meta = {
                 "UTF-8"
                ,"UTF-8"
                ,100
                ,""
                ,"BOM"
              }
              ,.defacto = EncodingID::UTF8
            };
          }
        }
        
        if (bytes_read >= 2) {
          // UTF-16 BE BOM: FE FF
          if (bom_bytes[0] == 0xFE && bom_bytes[1] == 0xFF) {
            return EncodingDetectionResult{
              .meta = {
                 "UTF-16BE"
                ,"UTF-16 Big Endian"
                ,100
                ,""
                ,"BOM"
              }
              ,.defacto = EncodingID::UTF16BE
            };
          }
          // UTF-16 LE BOM: FF FE
          if (bom_bytes[0] == 0xFF && bom_bytes[1] == 0xFE) {
            return EncodingDetectionResult{
              .meta = {
                 "UTF-16LE"
                ,"UTF-16 Little Endian"
                ,100
                ,""
                ,"BOM"
              }
              ,.defacto = EncodingID::UTF16LE
            };
          }
        }
        
        return EncodingDetectionResult{
          .meta = {
              ""
              ,"Unknown"
              ,0
              ,""
              ,"No BOM"
          }
          ,.defacto = EncodingID::Unknown
        };

      }

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

    } // icu_facade_deprecated

  } // namespace encoding
} // text