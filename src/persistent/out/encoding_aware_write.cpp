#include "encoding_aware_write.hpp"
#include "text/encoding.hpp"

namespace persistent {
  namespace out {

    namespace UTF8 {

      // TODO: Base on bom_ostream if/when appropriate or required
      //       For now UTF8 outputs to a raw ostream
      //       Further see bom_istream design

      persistent::out::UTF8::ostream& operator<<(persistent::out::UTF8::ostream& os,char32_t cp) {
        // Thanks to https://sourceforge.net/p/utfcpp/code/HEAD/tree/v3_0/src/utf8.h
        if (!::text::encoding::UTF8::is_valid_unicode(cp)) {
          os.os << ::text::encoding::UTF8::REPLACEMENT_CHARACTER;
        }
        else if (cp < 0x80)                   // one octet
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

    } // UTF8

  } // in
} // persistent
