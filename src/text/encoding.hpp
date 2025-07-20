#pragma once

#include "std_overload.hpp"
#include "charset.hpp"
#include <array>
#include <iostream>
#include <sstream> // ostringstream,
#include <iomanip> // std::quoted,
#include <optional>
#include <deque>

namespace encoding {

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

		encoding::UTF8::ostream& operator<<(encoding::UTF8::ostream& os,char32_t cp);
		std::string unicode_to_utf8(char16_t_string const& s);

		// UTF-8 to Unicode
		class ToUnicodeBuffer {
		public:
			using OptionalUnicode = std::optional<uint16_t>;
			OptionalUnicode push(uint8_t b);
		private:
			std::deque<uint8_t> m_utf_8_buffer{};
			OptionalUnicode to_unicode();
		};

    char16_t_string utf8ToUnicode(std::string const& s_utf8);

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
          auto unicode_s = encoding::UTF8::utf8ToUnicode(raw_entry);
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
      // getline: Transcodes input from ISO8859-1 encoding to Unicode and then applies F to decode it to target encoding (std::nullopt on failure)
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
    // while (auto entry = in.getline(encoding::unicode::to_utf8{}))
    struct to_utf8 {
      using value_type = std::string;
      value_type operator()(char16_t_string const& unicode_s) const;

    };
  } // namespace unicode

} // namespace encoding
