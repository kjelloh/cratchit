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
#include "spdlog/spdlog.h"

namespace encoding {

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
      std::ostringstream oss{};
      oss << "\nConsumed BOM:" << candidate;
      spdlog::info(oss.str());
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

    encoding::UTF8::ostream& operator<<(encoding::UTF8::ostream& os,char32_t cp) {
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
      encoding::UTF8::ostream utf8_os{os};
      for (auto cp : s) utf8_os << cp;
      if (false) {
        std::ostringstream oss{};
        oss << "\nunicode_to_utf8(";
        for (auto ch : s) oss << " " << std::hex << static_cast<unsigned int>(ch) << std::dec;
        oss << ") --> " << std::quoted(os.str());
        spdlog::info(oss.str());
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
      if (true) {
        std::ostringstream oss{};
        oss << "ToUnicodeBuffer::push(" << std::hex << static_cast<unsigned int>(b) << ")" << std::dec;
        oss << ":size:" << std::dec << m_utf_8_buffer.size() << " ";
        for (auto ch : m_utf_8_buffer) oss << "[" << std::hex << static_cast<unsigned int>(ch) << "]" << std::dec;
        spdlog::info(oss.str());
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
        std::ostringstream oss{};
        oss << "\nutf8ToUnicode(";
        for (auto ch : s_utf8) oss << " " << std::hex << static_cast<unsigned int>(ch) << std::dec;
        oss << ") --> ";
        for (auto ch : result) oss << " " << std::hex << static_cast<unsigned int>(ch) << std::dec;
        spdlog::info(oss.str());
      }

      return result;
    }

  } // namespace UTF8

  namespace ISO_8859_1 {

    istream::istream(std::istream& in) : bom_istream{in} {
      if (this->bom) {
        // We expect the input stream to be without BOM
        std::ostringstream oss{};
        oss << "\nSorry, Expected ISO8859-1 stream but found bom:" << *(this->bom);
        spdlog::info(oss.str());

        this->raw_in.setstate(std::ios_base::failbit); // disable reading this stream
      }
    }

  } // namespace ISO_8859_1

  namespace CP437 {

    istream::istream(std::istream& in) : bom_istream{in} {
      if (this->bom) {
        // We expect the input stream to be without BOM
        std::ostringstream oss{};
        oss << "\nSorry, Expected CP437 (SIE-file) stream but found bom:" << *(this->bom);
        spdlog::info(oss.str());

        this->raw_in.setstate(std::ios_base::failbit); // disable reading this stream
      }
    }

  } // namespace CP437

  namespace UTF8 {

    istream::istream(std::istream& in) : bom_istream{in} {
      if (this->bom) {
        if (this->bom->value != BOM::UTF8_VALUE) {
          // We expect the input stream to be in UTF8 and thus any BOM must confirm this
          std::ostringstream oss{};
          oss << "\nSorry, Expected an UTF8 input stream but found contradicting BOM:" << *(this->bom);
          spdlog::info(oss.str());

          this->raw_in.setstate(std::ios_base::failbit); // disable reading this stream
        }
      }
    }

  } // namespace UTF8

  namespace unicode {

    std::string to_utf8::operator()(cratchit_unicode_string const& unicode_s) const {
      return encoding::UTF8::unicode_to_utf8(unicode_s);
    }

  } // unicode

  namespace icu {
    // ICU-based Encoding Detection Implementation
    EncodingDetectionResult EncodingDetector::detect_file_encoding(std::filesystem::path const& file_path) {
      // First try BOM detection for quick wins
      auto bom_result = detect_by_bom(file_path);
      if (bom_result.confidence >= 95) {
        return bom_result;
      }
      
      // Read file sample for ICU analysis
      std::ifstream file(file_path, std::ios::binary);
      if (!file.is_open()) {
        return {DetectedEncoding::Unknown, "", "Unknown", 0, "", "Error: Cannot open file"};
      }
      
      // Read up to 8KB sample (ICU recommendation)
      std::vector<char> buffer(8192);
      file.read(buffer.data(), buffer.size());
      size_t bytes_read = file.gcount();
      
      if (bytes_read == 0) {
        return {DetectedEncoding::UTF8, "UTF-8", "UTF-8", 100, "", "Empty file"};
      }
      
      auto icu_result = detect_buffer_encoding(buffer.data(), bytes_read);
      
      // If ICU is uncertain, combine with extension heuristics
      if (icu_result.confidence < 50) {
        auto ext_result = detect_by_extension_heuristics(file_path);
        if (ext_result.confidence > icu_result.confidence) {
          return ext_result;
        }
      }
      
      return icu_result;
    }

    EncodingDetectionResult EncodingDetector::detect_buffer_encoding(char const* data, size_t length) {
      UErrorCode status = U_ZERO_ERROR;
      
      // Create ICU character set detector
      UCharsetDetector* detector = ucsdet_open(&status);
      if (U_FAILURE(status) || !detector) {
        return {DetectedEncoding::Unknown, "", "Unknown", 0, "", "ICU detector creation failed"};
      }
      
      // Set the input data
      ucsdet_setText(detector, data, static_cast<int32_t>(length), &status);
      if (U_FAILURE(status)) {
        ucsdet_close(detector);
        return {DetectedEncoding::Unknown, "", "Unknown", 0, "", "ICU setText failed"};
      }
      
      // Detect the character set
      const UCharsetMatch* match = ucsdet_detect(detector, &status);
      if (U_FAILURE(status) || !match) {
        ucsdet_close(detector);
        return {DetectedEncoding::UTF8, "UTF-8", "UTF-8 (fallback)", 30, "", "ICU detection failed, fallback"};
      }
      
      // Extract results
      const char* canonical_name = ucsdet_getName(match, &status);
      int32_t confidence = ucsdet_getConfidence(match, &status);
      const char* language = ucsdet_getLanguage(match, &status);
      
      if (U_FAILURE(status)) {
        ucsdet_close(detector);
        return {DetectedEncoding::UTF8, "UTF-8", "UTF-8 (fallback)", 30, "", "ICU result extraction failed"};
      }
      
      // Convert to our types
      std::string canonical_str = canonical_name ? canonical_name : "UTF-8";
      std::string language_str = language ? language : "";
      DetectedEncoding encoding = canonical_name_to_enum(canonical_str);
      std::string display_name = enum_to_display_name(encoding);
      
      ucsdet_close(detector);
      
      return {encoding, canonical_str, display_name, confidence, language_str, "ICU"};
    }

    std::vector<EncodingDetectionResult> EncodingDetector::detect_all_possible_encodings(char const* data, size_t length) {
      std::vector<EncodingDetectionResult> results;
      UErrorCode status = U_ZERO_ERROR;
      
      UCharsetDetector* detector = ucsdet_open(&status);
      if (U_FAILURE(status) || !detector) {
        return results;
      }
      
      ucsdet_setText(detector, data, static_cast<int32_t>(length), &status);
      if (U_FAILURE(status)) {
        ucsdet_close(detector);
        return results;
      }
      
      // Get all possible matches
      int32_t match_count;
      const UCharsetMatch** matches = ucsdet_detectAll(detector, &match_count, &status);
      if (U_FAILURE(status) || !matches) {
        ucsdet_close(detector);
        return results;
      }
      
      // Convert all matches to our format
      for (int32_t i = 0; i < match_count; ++i) {
        const char* canonical_name = ucsdet_getName(matches[i], &status);
        int32_t confidence = ucsdet_getConfidence(matches[i], &status);
        const char* language = ucsdet_getLanguage(matches[i], &status);
        
        if (U_SUCCESS(status) && canonical_name) {
          std::string canonical_str = canonical_name;
          std::string language_str = language ? language : "";
          DetectedEncoding encoding = canonical_name_to_enum(canonical_str);
          std::string display_name = enum_to_display_name(encoding);
          
          results.push_back({encoding, canonical_str, display_name, confidence, language_str, "ICU"});
        }
      }
      
      ucsdet_close(detector);
      return results;
    }

    EncodingDetectionResult EncodingDetector::detect_by_bom(std::filesystem::path const& file_path) {
      std::ifstream file(file_path, std::ios::binary);
      if (!file.is_open()) {
        return {DetectedEncoding::Unknown, "", "Unknown", 0, "", "Cannot open file"};
      }
      
      // Read first few bytes for BOM detection
      std::array<unsigned char, 4> bom_bytes{};
      file.read(reinterpret_cast<char*>(bom_bytes.data()), bom_bytes.size());
      size_t bytes_read = file.gcount();
      
      if (bytes_read >= 3) {
        // UTF-8 BOM: EF BB BF
        if (bom_bytes[0] == 0xEF && bom_bytes[1] == 0xBB && bom_bytes[2] == 0xBF) {
          return {DetectedEncoding::UTF8, "UTF-8", "UTF-8", 100, "", "BOM"};
        }
      }
      
      if (bytes_read >= 2) {
        // UTF-16 BE BOM: FE FF
        if (bom_bytes[0] == 0xFE && bom_bytes[1] == 0xFF) {
          return {DetectedEncoding::UTF16BE, "UTF-16BE", "UTF-16 Big Endian", 100, "", "BOM"};
        }
        // UTF-16 LE BOM: FF FE
        if (bom_bytes[0] == 0xFF && bom_bytes[1] == 0xFE) {
          return {DetectedEncoding::UTF16LE, "UTF-16LE", "UTF-16 Little Endian", 100, "", "BOM"};
        }
      }
      
      return {DetectedEncoding::Unknown, "", "Unknown", 0, "", "No BOM"};
    }

    EncodingDetectionResult EncodingDetector::detect_by_extension_heuristics(std::filesystem::path const& file_path) {
      auto ext = file_path.extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
      
      if (ext == ".csv") {
        return {DetectedEncoding::UTF8, "UTF-8", "UTF-8", 60, "", "Extension (.csv)"};
      } else if (ext == ".skv") {
        return {DetectedEncoding::ISO_8859_1, "ISO-8859-1", "ISO-8859-1", 60, "sv", "Extension (.skv)"};
      }
      
      return {DetectedEncoding::UTF8, "UTF-8", "UTF-8", 30, "", "Extension (default)"};
    }

    DetectedEncoding EncodingDetector::canonical_name_to_enum(std::string const& canonical_name) {
      if (canonical_name == "Undefined") return DetectedEncoding::Undefined;
      if (canonical_name == "UTF-8") return DetectedEncoding::UTF8;
      if (canonical_name == "UTF-16BE") return DetectedEncoding::UTF16BE;
      if (canonical_name == "UTF-16LE") return DetectedEncoding::UTF16LE;
      if (canonical_name == "UTF-32BE") return DetectedEncoding::UTF32BE;
      if (canonical_name == "UTF-32LE") return DetectedEncoding::UTF32LE;
      if (canonical_name == "ISO-8859-1") return DetectedEncoding::ISO_8859_1;
      if (canonical_name == "ISO-8859-15") return DetectedEncoding::ISO_8859_15;
      if (canonical_name == "windows-1252") return DetectedEncoding::WINDOWS_1252;
      if (canonical_name == "IBM437") return DetectedEncoding::CP437;
      return DetectedEncoding::Unknown;
    }

    std::string EncodingDetector::enum_to_display_name(DetectedEncoding encoding) {
      switch (encoding) {
        case DetectedEncoding::Undefined: return "Undefined";
        case DetectedEncoding::UTF8: return "UTF-8";
        case DetectedEncoding::UTF16BE: return "UTF-16 Big Endian";
        case DetectedEncoding::UTF16LE: return "UTF-16 Little Endian";
        case DetectedEncoding::UTF32BE: return "UTF-32 Big Endian";
        case DetectedEncoding::UTF32LE: return "UTF-32 Little Endian";
        case DetectedEncoding::ISO_8859_1: return "ISO-8859-1 (Latin-1)";
        case DetectedEncoding::ISO_8859_15: return "ISO-8859-15 (Latin-9)";
        case DetectedEncoding::WINDOWS_1252: return "Windows-1252";
        case DetectedEncoding::CP437: return "CP437 (DOS)";
        case DetectedEncoding::Unknown: return "Unknown";
      }
      return "Unknown";
    }

  } // icu



} // namespace encoding