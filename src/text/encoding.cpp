#include "encoding.hpp"
#include "persistent/out/encoding_aware_write.hpp"
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

    BOM::BOM(ByteBuffer::const_iterator begin,ByteBuffer::const_iterator end) {
      value.fill(0);
      auto len = std::min<size_t>(value.size(),std::distance(begin,end));
      std::transform(
         begin
        ,std::next(begin, len)
        ,value.begin()
        ,[](std::byte b) {
          return static_cast<unsigned char>(b);
        });    
    }

    std::istream& operator>>(std::istream& is,BOM& bom) {
      using std_overload::operator>>; // to 'see' the defined overload for std::array
      using std_overload::operator<<; // to 'see' the defined overload for std::array
      auto original_pos = is.tellg();
      // TODO: Beware that this always consumes value.size() chars
      //       Future variable length BOM needs another mechanism
      //       For now only UTF8 BOM is supported
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

    MetaDefacto<std::optional<BOM>,ByteBuffer> to_bom_and_buffer(ByteBuffer buffer) {

      // TODO: Consider a cleaner and more flexible BOM detection and removal
      //       This works for UTF8 BOM
      //       Note that we do not check if BOM and 'inferred encoding' matches (ICU should get this right?)
      //       Also, it seems ICU have no way of doing this for us (so whe have to apply out own removal?) 
      //       What we have is a mutual lambda that mutates the buffer copy we have moved from our
      //       in-buffer passed by value OK (it is ours and not a ref)

      if (    buffer.size() >= 3 
            && buffer[0] == std::byte{0xEF}
            && buffer[1] == std::byte{0xBB}
            && buffer[2] == std::byte{0xBF}) {
          BOM bom(buffer.begin(),buffer.begin()+3);
          logger::development_trace("to_bom_and_buffer - BOM detected: buffer.size:{}",buffer.size());
          buffer.erase(buffer.begin(), buffer.begin() + 3);
          logger::development_trace("to_bom_and_buffer - BOM STRIPPED, new buffer.size:{}",buffer.size());
          return {bom,std::move(buffer)};
      }

      return {std::nullopt,std::move(buffer)};

    }

    namespace ISO_8859_1 {


    } // namespace ISO_8859_1

    namespace UTF8 {

      bool is_valid_unicode(char32_t cp) {
        return (cp <= CODE_POINT_MAX && !is_surrogate(cp));
      }

      std::string unicode_to_utf8(cratchit_unicode_string const& s) {
        std::ostringstream os{};
        persistent::out::UTF8::ostream utf8_os{os};
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

    namespace CP437 {


    } // namespace CP437

    namespace unicode {

      std::string to_utf8::operator()(cratchit_unicode_string const& unicode_s) const {
        return text::encoding::UTF8::unicode_to_utf8(unicode_s);
      }

    } // unicode
  } // namespace encoding
} // text