#include "encoding.hpp"
#include <deque>
#include <iostream>
#include <iomanip>

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
      std::cout << "\nConsumed BOM:" << candidate;
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

    std::string unicode_to_utf8(char16_t_string const& s) {
      std::ostringstream os{};
      encoding::UTF8::ostream utf8_os{os};
      for (auto cp : s) utf8_os << cp;
      if (false) {
        std::cout << "\nunicode_to_utf8(";
        for (auto ch : s) std::cout << " " << std::hex << static_cast<unsigned int>(ch) << std::dec;
        std::cout << ") --> " << std::quoted(os.str());
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
        std::cout << "\nToUnicodeBuffer::push(" << std::hex << static_cast<unsigned int>(b) << ")" << std::dec;
        std::cout << ":size:" << std::dec << m_utf_8_buffer.size() << " ";
        for (auto ch : m_utf_8_buffer) std::cout << "[" << std::hex << static_cast<unsigned int>(ch) << "]" << std::dec;
      }
      return this->to_unicode();
    }

    ToUnicodeBuffer::OptionalUnicode ToUnicodeBuffer::to_unicode() {
      OptionalUnicode result{};
      switch (m_utf_8_buffer.size()) {
        case 1: {
          if (m_utf_8_buffer[0] < 0x7F) {
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

    char16_t_string utf8ToUnicode(std::string const& s_utf8) {
      char16_t_string result{};
      ToUnicodeBuffer to_unicode_buffer{};
      for (auto ch : s_utf8) {
        if (auto unicode = to_unicode_buffer.push(static_cast<std::uint8_t>(ch))) {
          result += *unicode;
        }
      }
      if (false) {
        std::cout << "\nutf8ToUnicode(";
        for (auto ch : s_utf8) std::cout << " " << std::hex << static_cast<unsigned int>(ch) << std::dec;
        std::cout << ") --> ";
        for (auto ch : result) std::cout << " " << std::hex << static_cast<unsigned int>(ch) << std::dec;
      }

      return result;
    }

  } // namespace UTF8

  namespace ISO_8859_1 {

    istream::istream(std::istream& in) : bom_istream{in} {
      if (this->bom) {
        // We expect the input stream to be without BOM
        std::cout << "\nSorry, Expected ISO8859-1 stream but found bom:" << *(this->bom);
        this->raw_in.setstate(std::ios_base::failbit); // disable reading this stream
      }
    }

  } // namespace ISO_8859_1

  namespace CP437 {

    istream::istream(std::istream& in) : bom_istream{in} {
      if (this->bom) {
        // We expect the input stream to be without BOM
        std::cout << "\nSorry, Expected CP437 (SIE-file) stream but found bom:" << *(this->bom);
        this->raw_in.setstate(std::ios_base::failbit); // disable reading this stream
      }
    }

  } // namespace CP437

  namespace UTF8 {

    istream::istream(std::istream& in) : bom_istream{in} {
      if (this->bom) {
        if (this->bom->value != BOM::UTF8_VALUE) {
          // We expect the input stream to be in UTF8 and thus any BOM must confirm this
          std::cout << "\nSorry, Expected an UTF8 input stream but found contradicting BOM:" << *(this->bom);
          this->raw_in.setstate(std::ios_base::failbit); // disable reading this stream
        }
      }
    }

  } // namespace UTF8

  namespace unicode {

    std::string to_utf8::operator()(char16_t_string const& unicode_s) const {
      return encoding::UTF8::unicode_to_utf8(unicode_s);
    }

  } // namespace unicode

} // namespace encoding