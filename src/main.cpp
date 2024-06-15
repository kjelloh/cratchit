float const VERSION = 0.5;

#include <iostream>
#include <locale>
#include <string>
#include <sstream>
#include <queue>
#include <variant>
#include <vector>
#include <optional>
#include <string_view>
#	include <filesystem>
#include <fstream>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <cctype>
#include <map>
#include <regex>
#include <chrono>
#include <numeric>
#include <functional>
#include <set>
#include <ranges> // requires c++ compiler with c++20 support
#include <concepts> // Requires C++23 support
#include <csignal>
#include <format>

// Define a signal handler function that does nothing
void handle_winch(int sig) {
    // Do nothing
}

#include "../lib/macos/lua-5.2.4_MacOS1011_lib/include/lua.hpp"

// Helpers to stream this pointer for logging purposes
template<typename T>
concept PointerToClassType = std::is_pointer_v<T> && std::is_class_v<std::remove_pointer_t<T>>;
template<PointerToClassType T>
std::ostream& operator<<(std::ostream& os, T ptr) {
    os << " memory[" << std::hex << std::showbase << reinterpret_cast<std::uintptr_t>(ptr) << "]";
    return os;
}

// The Cratchit should comply with the Swedish law Accouning Act
/*

The Accounting Act (1999:1078) Chapter 5, Section 7 states that each voucher must contain:

The date when the business transaction occurred.
The date when the voucher was compiled.
Voucher number or other identification.
Information about what the business transaction pertains to.
Amount and any applicable VAT.
The name of the counterparty.

From Swedish law text:

Bokföringslagen (1999:1078) kapitel 5 § 7 säger att varje verifikation ska innehålla:

Datum när affärshändelsen inträffade.
Datum när verifikationen sammanställdes.
Verifikationsnummer eller annan identifiering.
Uppgift om vad affärshändelsen avser.
Belopp och eventuell moms.
Motpartens namn.

As of 240603 I have yet to understand the exact difference between "date when the business transaction occurred" and "The date when the voucher was compiled".

* The SIE entry of a registered voucher contains only the "accounting date" which, as far as I can see, is usually the date when the event happened.
* This leads me to conclude that the electronic register of a voucher is valid with 'only' the 'date of event'?
* In this way, the BAS account saldo will reflect 'reality' based on the reported date of when the event happened.
* Especially for Bank Accounts or the Swedish Tax Agency account for company tax transactions the saldo will correctly reflect the reported saldo.

*/

// Scratch comments to "remember" what configuration for VSCode that does "work"

// tasks.json/"tasks"/label:"macOS..."/args: ... "--sysroot=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk" 
//                                               to have third party g++-11 find includes for, and link with, mac OS libraries.
//                                               Gcc will automatically search sub-directories /usr/include and /usr/lib.

//                                               macOS 13 seems to have broken previous path /usr/include and usr/local/include (https://developer.apple.com/forums/thread/655588?answerId=665804022#665804022)
/* "...This is fine if you’re using Xcode, or Apple’s command-line tools, because they know how to look inside an SDK for headers and stub libraries. 
    If you’re using third-party tools then you need to consult the support resources for those tools 
		to find out how they’ve adjusted to this new reality ... the critical point is the linker. 
		If your third-party tools use the Apple linker then you should just be able to point the tools at the 
		usr/include directory within the appropriate SDK." */

// c_cpp_properties.json/"configurations":/"includePath": ... "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include"
//                                               to have Intellisense find c++ library headers.

// c_cpp_properties.json/"configurations":/"includePath": ... "/Library/Developer/CommandLineTools/usr/include/c++/v1"
//                                               to have Intellisense find actual OS specific c++ library headers (required by macro "include_next" in c++ library headers).


using char16_t_string = std::wstring;

namespace charset {

  /*

  The main idea is to always encode through UNICODE as the intermediate encoding.

  So, Cratchit may assume the runtime environment / terminal uses UTF-8 encoding.

  Thus, to store text from the user input (in UTF-8) into an SIE-file that is encoded using the CP437 the transformations will be:
  UTF-8 --> UNICODE --> CP437

  And to read in an SIE-file the transformatons will be

  CP437 --> UNICODE --> UTF-8

  
  
  According to ChatGPT the Swedish letters åäöÅÄÖ are encoded in charachtrer sets of interest to Cratchit as,

  Here are the Unicode code points for the Swedish letters:

    å: U+00E5 (LATIN SMALL LETTER A WITH RING ABOVE)
    ä: U+00E4 (LATIN SMALL LETTER A WITH DIAERESIS)
    ö: U+00F6 (LATIN SMALL LETTER O WITH DIAERESIS)
    Å: U+00C5 (LATIN CAPITAL LETTER A WITH RING ABOVE)
    Ä: U+00C4 (LATIN CAPITAL LETTER A WITH DIAERESIS)
    Ö: U+00D6 (LATIN CAPITAL LETTER O WITH DIAERESIS)

  The CP 437 encodings for Swedish letters åäöÅÄÖ are:

    å (U+00E5): 0x86 (134 in decimal)
    ä (U+00E4): 0x84 (132 in decimal)
    ö (U+00F6): 0x94 (148 in decimal)
    Å (U+00C5): 0x8F (143 in decimal)
    Ä (U+00C4): 0x8E (142 in decimal)
    Ö (U+00D6): 0x99 (153 in decimal)  

  In UTF-8, these Unicode code points are encoded as follows:

    å (U+00E5) → 0xC3 0xA5 (2 bytes)
    ä (U+00E4) → 0xC3 0xA4 (2 bytes)
    ö (U+00F6) → 0xC3 0xB6 (2 bytes)
    Å (U+00C5) → 0xC3 0x85 (2 bytes)
    Ä (U+00C4) → 0xC3 0x84 (2 bytes)
    Ö (U+00D6) → 0xC3 0x96 (2 bytes)

  In ISO 8859-1 (Latin-1), these characters are represented by single byte values:

      å → 0xE5
      ä → 0xE4
      ö → 0xF6
      Å → 0xC5
      Ä → 0xC4
      Ö → 0xD6

  */

	namespace ISO_8859_1 {

		// Unicode code points 0x00 to 0xFF maps directly to ISO 8859-1 (ISO Latin).
		// So we have to convert UTF-8 to Unicode and then truncate the code point to the low value byte.
		// Code points above 0xFF has NO obvious representation in ISO 8859-1 (I suppose we could cherry pick to get some similar ISO 8859-1 glyph but...)

		char16_t iso8859ToUnicode(char ch8859) {
			return static_cast<uint8_t>(ch8859);
		}
	
		char16_t_string iso8859ToUnicode(std::string s8859) {
			char16_t_string result{};
			std::transform(s8859.begin(),s8859.end(),std::back_inserter(result),[](char ch){
				return iso8859ToUnicode(ch);
			});
      if (false) {
        std::cout << "\niso8859ToUnicode(";
        for (auto ch : s8859) std::cout << " " << std::hex << static_cast<unsigned int>(ch);
        std::cout << ") --> ";
        for (auto ch : result) std::cout << " " << std::hex << static_cast<unsigned int>(ch);
      }
			return result;
		}

		uint8_t UnicodeToISO8859(char16_t unicode) {
			uint8_t result{'?'};
			if (unicode<=0xFF) result = unicode;
			return result;
		} 

	}

	namespace CP437 {
		extern std::map<char,char16_t> cp437ToUnicodeMap;

		char16_t cp437ToUnicode(char ch437) {
			return cp437ToUnicodeMap[ch437];
		}
	
		uint8_t UnicodeToCP437(char16_t unicode) {
			uint8_t result{'?'};
			auto iter = std::find_if(cp437ToUnicodeMap.begin(),cp437ToUnicodeMap.end(),[&unicode](auto const& entry){
				return entry.second == unicode;
			});
			if (iter != cp437ToUnicodeMap.end()) result = iter->first;
      if (false) {
        std::cout << "\nUnicodeToCP437(unicode:" << std::hex << static_cast<unsigned int>(unicode) << ") --> " << static_cast<unsigned int>(result); 
      }
			return result;
		} 

		char16_t_string cp437ToUnicode(std::string s437) {
			char16_t_string result{};
			std::transform(s437.begin(),s437.end(),std::back_inserter(result),[](char ch){
				return charset::CP437::cp437ToUnicode(ch);
			});
      if (false) {
        std::cout << "\ncp437ToUnicode(";
        for (auto ch : s437) std::cout << " " << std::hex << static_cast<unsigned int>(ch);
        std::cout << ") --> ";
        for (auto ch : result) std::cout << " " << std::hex << static_cast<unsigned int>(ch);
      }

			return result;
		}
	} // namespace CP437

} // namespace CharSet

// Namespace that overloads on std types ()
namespace std_overload {
  // Note: We could have placed these overloads in either the global namespace or the std namespace.

  //       But placing them in the std namespace is considered bad practice as we would potentially pollute std with declarations with unknown conflicts as a result.
  //       And placing these in the global namespace renders these overloads to NOT be considered by the compiler is used in another (not global) namespace.

  //       Explanation: The compiler will apply C++ argument dependant lookup (ADL), i.e., look for overloads in the std namespace for arguments of std namespace types
  //       This means it will fail to consider this overload as it is not in the std namespace (and it will not look in teh global namespace)

  //       E.g., any 'std::istream >> array-type-variable' will fail to find this overload (fails to compile)

  //       Solution 1: Have the client use the call syntax ::operator>>(stream,array-type-variable)
  //       Solution 2: Place 'using std_overload::operator>>' in the namespace (class or function scope) where we want to use it
  //       Solution 3: Pre-create here the namespaces where we want to use this overload and inject them with 'using ::operator>>' so that the using code don't have to

  // None of the alternatives is fully satisfactory but in Cratchit we have chose alternative 2 for now.

  // Helper to  write an std::array of predefined size to a stream
  // Client namespace needs an 'using std_overload::operator<<' to 'see it'
  template <std::size_t N>
  std::ostream& operator<<(std::ostream& os, std::array<unsigned char, N> const& arr){
    for (int b : arr) os << " " << std::hex << b; // Note: We need b to be an int to apply the appropriate integer output formatting
    return os;
  }

  // Helper to read from a stream into an std::array of predefined size
  // Client namespace needs an 'using std_overload::operator>>' to 'see it'
  template<std::size_t N>
  std::istream& operator>>(std::istream& is, std::array<unsigned char, N>& arr) {
      is.read(reinterpret_cast<char*>(arr.data()), arr.size());
      if (is) {
        std::cout << "\noperator>> read array:" << arr;
      }
      return is;
  }
}


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

  std::istream& operator>>(std::istream& is,BOM& bom) {
    using std_overload::operator>>; // to 'see' the defined overload for std::array
    using std_overload::operator<<; // to 'see' the defined overload for std::array
    auto original_pos = is.tellg();
    if (is >> bom.value) {
      if (bom.value == BOM::UTF8_VALUE) {
        std::cout << "\noperator>>(BOM) consumed from read file. bom:" << bom.value;
      }
      else {
        std::cout << "\noperator>>(BOM) Not a valid BOM - file position is reset";
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

  class bom_istream {
  public:
			std::istream& raw_in;
      std::optional<BOM> bom{};
      bom_istream(std::istream& in) : raw_in{in} {
        // Check for BOM in fresh input stream
        BOM candidate{};

        if (raw_in >> candidate) {
          std::cout << "\nConsumed BOM:" << candidate;
          this->bom = candidate;
        }
        else {
          std::cout << "\nNo BOM detected in stream";
          raw_in.clear(); // clear the signalled failure to allow the stream to be read for non-BOM content
        }
        std::cout << "\nbom_istream{} raw_in is at position:" << raw_in.tellg();
      }
			operator bool() {return static_cast<bool>(raw_in);}

    // TODO: Move base class members from derived 8859_1 and UTF-8 istream classes to here
  private:
  };

  namespace ISO_8859_1  {

		class istream : public bom_istream {
    public:
      istream(std::istream& in) : bom_istream{in} {
        if (this->bom) {
          // We expect the input stream to be without BOM
          std::cout << "\nSorry, Expected ISO8859-1 stream but found bom:" << *(this->bom);
          this->raw_in.setstate(std::ios_base::failbit); // disable reading this stream
        }
      }
      // getline: Transcodes input from ISO8859-1 encoding to Unicode and then applies F to decode it to target encoding (std::nullopt on failure)
      template <class F>
      std::optional<typename F::value_type> getline(F const& f) {
        typename F::value_type result{};
        std::string raw_entry{};
        if (std::getline(raw_in,raw_entry)) {
          auto unicode_s = charset::ISO_8859_1::iso8859ToUnicode(raw_entry);
          result = f(unicode_s);
        }
        if (result.size() > 0) return result;
        else return std::nullopt;
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

		bool is_valid_unicode(char32_t cp) {
			return (cp <= CODE_POINT_MAX && !is_surrogate(cp));
		}

		struct ostream {
			std::ostream& os;
		};

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
        for (auto ch : s) std::cout << " " << std::hex << static_cast<unsigned int>(ch);
        std::cout << ") --> " << std::quoted(os.str());
      }
			return os.str();
		}

		// UTF-8 to Unicode
		class ToUnicodeBuffer {
		public:
			using OptionalUnicode = std::optional<uint16_t>;
			OptionalUnicode push(uint8_t b) {
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
          std::cout << "\nToUnicodeBuffer::push(" << std::hex << static_cast<unsigned int>(b) << ")";
          std::cout << ":size:" << std::dec << m_utf_8_buffer.size() << " ";
          for (auto ch : m_utf_8_buffer) std::cout << "[" << std::hex << static_cast<unsigned int>(ch) << "]";
        }
				return this->to_unicode();
			}
		private:
			std::deque<uint8_t> m_utf_8_buffer{};
			OptionalUnicode to_unicode() {
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
		};

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
        for (auto ch : s_utf8) std::cout << " " << std::hex << static_cast<unsigned int>(ch);
        std::cout << ") --> ";
        for (auto ch : result) std::cout << " " << std::hex << static_cast<unsigned int>(ch);
      }

      return result;
    }

		class istream : public bom_istream {
    public:
      istream(std::istream& in) : bom_istream{in} {
        if (this->bom) {
          if (this->bom->value != BOM::UTF8_VALUE) {
            // We expect the input stream to be in UTF8 and thus any BOM must confirm this
            std::cout << "\nSorry, Expected an UTF8 input stream but found contradicting BOM:" << *(this->bom);
            this->raw_in.setstate(std::ios_base::failbit); // disable reading this stream
          }
        }
      }
      // getline: Transcodes input from UTF8 encoding to Unicode and then applies F to decode it to target encoding (std::nullopt on failure)
      template <class F>
      std::optional<typename F::value_type> getline(F const& f) {
        typename F::value_type result{};
        std::string raw_entry{};
        if (std::getline(raw_in,raw_entry)) {
          auto unicode_s = encoding::UTF8::utf8ToUnicode(raw_entry);
          result = f(unicode_s);
        }
        if (result.size() > 0) return result;
        else return std::nullopt;
      }
		};

	} // namespace UTF8

  namespace CP437 {
		class istream : public bom_istream {
    public:
      istream(std::istream& in) : bom_istream{in} {
        if (this->bom) {
          // We expect the input stream to be without BOM
          std::cout << "\nSorry, Expected CP437 (SIE-file) stream but found bom:" << *(this->bom);
          this->raw_in.setstate(std::ios_base::failbit); // disable reading this stream
        }
      }
      // getline: Transcodes input from ISO8859-1 encoding to Unicode and then applies F to decode it to target encoding (std::nullopt on failure)
      template <class F>
      std::optional<typename F::value_type> getline(F const& f) {
        typename F::value_type result{};
        std::string raw_entry{};
        if (std::getline(raw_in,raw_entry)) {
          auto unicode_s = charset::CP437::cp437ToUnicode(raw_entry);
          result = f(unicode_s);
        }
        if (result.size() > 0) return result;
        else return std::nullopt;
      }
		};    
  } // namespace encoding
  
  namespace unicode {
    // while (auto entry = in.getline(encoding::unicode::to_utf8{}))
    struct to_utf8 {
      using value_type = std::string;
      value_type operator()(char16_t_string const& unicode_s) const {
        return encoding::UTF8::unicode_to_utf8(unicode_s);
      }

    };
  } // namespace unicode

} // namespace encoding

namespace tokenize {

  std::string trim(std::string const& s) {
    return std::ranges::to<std::string>(
        s 
      | std::views::drop_while(::isspace) 
      | std::views::reverse 
      | std::views::drop_while(::isspace) 
      | std::views::reverse
    );
  }

	bool contains(std::string const& key,std::string const& s) {
		return (s.find(key) != std::string::npos);
	}

	bool starts_with(std::string const& key,std::string const& s) {
		return s.starts_with(key);
	}

	std::string without_front_word(std::string_view sv) {
    sv.remove_prefix(std::min(sv.find_first_not_of(" "), sv.size()));
    sv.remove_prefix(std::min(sv.find_first_of(" "), sv.size()));
    sv.remove_prefix(std::min(sv.find_first_not_of(" "), sv.size()));
		return std::string{sv};
	}

	// returns s split into first,second on provided delimiter delim.
	// split fail returns first = "" and second = s
	std::pair<std::string,std::string> split(std::string s,char delim) {
		auto pos = s.find(delim);
		if (pos<s.size()) return {s.substr(0,pos),s.substr(pos+1)};
		else return {"",s}; // split failed (return right = the unsplit string)
	}

	enum class eAllowEmptyTokens {
		unknown
		,no
		,YES
		,undefined
	};

	std::vector<std::string> splits(std::string s,char delim,eAllowEmptyTokens allow_empty_tokens = eAllowEmptyTokens::no) {
		std::vector<std::string> result;
		try {
			// TODO: Refactor code so default is allowing for split into empty tokens and make skip whitespace a special case
			if (allow_empty_tokens == eAllowEmptyTokens::no) {
				auto head_tail = split(s,delim);
				// std::cout << "\nhead" << head_tail.first << " tail:" << head_tail.second;
				while (head_tail.first.size()>0) {
					auto const& [head,tail] = head_tail;
					result.push_back(head);
					head_tail = split(tail,delim);
					// std::cout << "\nhead" << head_tail.first << " tail:" << head_tail.second;
				}
				if (head_tail.second.size()>0) {
					result.push_back(head_tail.second);
					// std::cout << "\ntail:" << head_tail.second;
				}
			}
			else {
				size_t first{},delim_pos{};
				do {
					delim_pos = s.find(delim,first);
          if (delim_pos != std::string::npos) {
            result.push_back(s.substr(first,delim_pos-first));
            first = delim_pos+1;
          }
          else {
            result.push_back(s.substr(first)); // add the tail
          }
				} while (delim_pos<s.size());
			}
		}
		catch (std::exception const& e) {
			std::cout << "\nDESIGN INSUFFICIENCY: splits(s,delim,allow_empty_tokens) failed for s=" << std::quoted(s) << ". Expception=" << std::quoted(e.what());
		}
		return result;
	}

	std::vector<std::string> splits(std::string const& s) {
		std::vector<std::string> result{};
		try {
			std::istringstream is{s};
			std::string token{};
			while (is >> std::quoted(token)) {
				result.push_back(token);
				token.clear();
			}
		}
		catch (std::exception const& e) {
			std::cout << "\nDESIGN INSUFFICIENCY: splits(s) failed for s=" << std::quoted(s) << ". Expception=" << std::quoted(e.what());
		}
		
		// for (auto const& token : result) std::cout << "\n\tsplits token: " << std::quoted(token);

		return result; 
	}

	enum class SplitOn {
		Undefined
		,TextAndAmount
		,TextAmountAndDate
		,Unknown
	};

	enum class TokenID {
		Undefined
		,Caption
		,Amount
		,Date
		,Unknown
	};

	std::ostream& operator<<(std::ostream& os,TokenID const& id) {
		switch (id) {
			case TokenID::Undefined: os << "Undefined"; break;
			case TokenID::Caption: os << "Caption"; break;
			case TokenID::Amount: os << "Amount";  break;
			case TokenID::Date: os << "Date";  break;
			case TokenID::Unknown: os << "Unknown";  break;
		}
		return os;
	}

	TokenID token_id_of(std::string const& s) {
		// NOTE: The order of matching below matters (matches from least permissive (date) to most permissive (any text))
		// If you put the most permissive first the less permissive tokens will never be matched.
		TokenID result{TokenID::Undefined};
		// YYYYMMDD, YYYY-MM-DD
		if (const std::regex date_regex("([2-9]\\d{3})-?([0]\\d|[1][0-2])-?([0-2]\\d|[3][0-1])"); std::regex_match(s,date_regex)) result = TokenID::Date;
		// '+','-' or none followed by nnn... (any number of digits) followed by an optional ',' or '.' followed by ate least and max two digits
		else if (const std::regex amount_regex("^[+-]?\\d+([.,]\\d\\d?)?$"); std::regex_match(s,amount_regex)) result = TokenID::Amount;
		// any string of characters in the set a-z,A-Z, and åäöÅÄÖ. NOTE: Requires the runtime locale to be set to UTF-8 encoding and that this source file is also UTF-8 encoded!
		else if (const std::regex caption_regex("[a-zA-ZåäöÅÄÖ ]+"); std::regex_match(s,caption_regex)) result = TokenID::Caption;
		else result = TokenID::Unknown;

		// std::cout << "\n\ttoken_id_of " << std::quoted(s) << " = " << result;

		return result;
	}

	std::vector<std::string> splits(std::string const& s,SplitOn split_on) {
		// std::cout << "\nsplits(std::string const& s,SplitOn split_on)";
		std::vector<std::string> result{};
		auto spaced_tokens = splits(s);
		std::vector<TokenID> ids{};
		for (auto const& s : spaced_tokens) {
			ids.push_back(token_id_of(s));
		}

		for (int i=0;i<spaced_tokens.size();++i) {
			std::cout << "\n" << spaced_tokens[i] << " id:" << static_cast<int>(ids[i]) << std::flush;
		}

		switch (split_on) {
			case SplitOn::TextAmountAndDate: {
				std::vector<TokenID> expected_id{TokenID::Caption,TokenID::Amount,TokenID::Date};
				int state{0};
				std::string s{};
				for (int i=0;i<ids.size();) {
					if (ids[i] == expected_id[state]) {
						if (s.size() > 0) s += " ";
						s += spaced_tokens[i++];
					}
					else {
						if (s.size()>0) result.push_back(s);
            ++state;
						s.clear();					
					}
          if (state+1 > expected_id.size()) {
            std::cout << "\nDESIGN INSUFFICIENCY: Error, unable to process state:" << state;
            break;
          }
				}
				if (s.size()>0) result.push_back(s); // add any tail
			} break;
			default: {
				std::cout << "\nERROR - Unknown split_on value " << static_cast<int>(split_on);
			} break;
		}

		// for (auto const& token : result) std::cout << "\n\tsplits token: " << std::quoted(token);

		return result;
	}
} // namespace tokenize

namespace Key {
		class Path {
		public:
			auto begin() const {return m_path.begin();}
			auto end() const {return m_path.end();}
			Path() = default;
			Path(Path const& other) = default;
      Path(std::vector<std::string> const& v) : m_path{v} {}
			Path(std::string const& s_path,char delim = '^') : 
			  m_delim{delim}
				,m_path(tokenize::splits(s_path,delim,tokenize::eAllowEmptyTokens::YES)) {};
			auto size() const {return m_path.size();}
			Path operator+(std::string const& key) const {Path result{*this};result.m_path.push_back(key);return result;}
			operator std::string() const {
				std::ostringstream os{};
				os << *this;
				return os.str();
			}
			Path& operator+=(std::string const& key) {
				m_path.push_back(key);
				// std::cout << "\noperator+= :" << *this  << " size:" << this->size();
				return *this;
			}
			Path& operator--() {
				m_path.pop_back();
				// std::cout << "\noperator-- :" << *this << " size:" << this->size();
				return *this;
			}
			Path parent() {
				Path result{*this};
				--result;
				// std::cout << "\nparent:" << result << " size:" << result.size();
				return result;
			}
			std::string back() const {return m_path.back();}
			std::string operator[](std::size_t pos) const {return m_path[pos];}
			friend std::ostream& operator<<(std::ostream& os,Path const& key_path);
			std::string to_string() const {
				std::ostringstream os{};
				os << *this;
				return os.str();
			}
		private:
			std::vector<std::string> m_path{};
			char m_delim{'^'};
		}; // class Path

		using Paths = std::vector<Path>;

		std::ostream& operator<<(std::ostream& os,Key::Path const& key_path) {
			int key_count{0};
			for (auto const& key : key_path) {
				if (key_count++>0) os << key_path.m_delim;
        if (false) {
          // patch to filter out unprintable characters that results from incorrect character decodings
          // NOTE: This loop will break down multi-character encodings like UTF-8 and turn them into two or more '?'
          for (auto ch : key) {
          	if (std::isprint(ch)) os << ch; // Filter out non printable characters (AND characters in wrong encoding, e.g., charset::ISO_8859_1 from raw file input that erroneously end up here ...)
          	else os << "?";
          }
        }
        else {
          os << key; // Assume key is in runtime character encoding (ok to output as-is)
        }
				// std::cout << "\n\t[" << key_count-1 << "]:" << std::quoted(key);
			}
			return os;
		}
    std::string to_string(Key::Path const& key_path) {
      std::ostringstream os{};
      os << key_path;
      return os.str();
    }
} // namespace Key

std::string filtered(std::string const& s,auto filter) {
	std::string result{};;
	std::copy_if(s.begin(),s.end(),std::back_inserter(result),filter);
	return result;
}

namespace WrappedCentsAmount {

  // A drop-in type for an integer amount in cents.
  // This implies any conversion between CentsAmount and any built in arithmetic type keeps the value as a value in cents.  
  class CentsAmount {
  public:
    using cents_value_type = int;
    CentsAmount() = default;
    explicit CentsAmount(CentsAmount::cents_value_type cents_value) : m_in_cents_value{cents_value} {}

    CentsAmount& operator+=(CentsAmount const& other) {
      this->m_in_cents_value += other.m_in_cents_value;
      return *this;
    }

    CentsAmount operator+(CentsAmount const& other) {
      CentsAmount result{*this};
      result += other;
      return result;
    }

    auto operator<=>(CentsAmount const& other) const = default;

    bool operator==(CentsAmount const& other) const {return this->m_in_cents_value == other.m_in_cents_value;}
    bool operator!=(CentsAmount const& other) const {return not (*this==other);}
    bool operator!=(CentsAmount::cents_value_type const& cents_value) const {return not (this->m_in_cents_value == cents_value);}

    friend CentsAmount::cents_value_type to_amount_in_cents_integer(CentsAmount const& cents_amount);


  private:
    cents_value_type m_in_cents_value;
  };

  CentsAmount::cents_value_type to_amount_in_cents_integer(CentsAmount const& cents_amount) {
    return cents_amount.m_in_cents_value;
  }

  CentsAmount abs(CentsAmount const& cents_amount) {
    return CentsAmount{std::abs(to_amount_in_cents_integer(cents_amount))};
  }

  CentsAmount::cents_value_type to_whole_part_integer(CentsAmount const& cents_amount) {
    return to_amount_in_cents_integer(cents_amount) / 100;
  }

  CentsAmount::cents_value_type to_cents_part_integer(CentsAmount const& cents_amount) {
    return to_amount_in_cents_integer(cents_amount) % 100;
  }

  std::ostream& operator<<(std::ostream& os,CentsAmount const& cents_amount) {
    os << to_amount_in_cents_integer(cents_amount); // keep value in integer cents
    return os;
  }

  std::string to_string(CentsAmount const& cents_amount) {
    std::ostringstream oss{};
    oss << cents_amount;
    return oss.str();
  }
}

namespace std {
  template<>
  struct hash<WrappedCentsAmount::CentsAmount> {
      std::size_t operator()(WrappedCentsAmount::CentsAmount const& cents_amount) const noexcept {
          return std::hash<WrappedCentsAmount::CentsAmount::cents_value_type>{}(to_amount_in_cents_integer(cents_amount));
      }
  };
}

namespace IntCentsAmount {
  // Cents Amount represents e.g., 117.17 as the integer 11717
  using CentsAmount = int;

  CentsAmount to_whole_part_integer(CentsAmount const& cents_amount) {
    return cents_amount / 100;
  }

  CentsAmount to_cents_part_integer(CentsAmount const& cents_amount) {
    return cents_amount % 100;
  }

}

// using CentsAmount = IntCentsAmount::CentsAmount;
// using IntCentsAmount::to_whole_part_integer;
// using IntCentsAmount::to_cents_part_integer;

using CentsAmount = WrappedCentsAmount::CentsAmount;

using OptionalCentsAmount = std::optional<CentsAmount>;

// The namespace for a class Amount that wraps a double as representation
namespace WrappedDoubleAmount {
  // A C++ double typically represents a floating-point number using 64 bits, 
  // following the IEEE 754 floating-point standard. 
  // This allows for approximately 15-16 decimal digits of precision or nnnnnnnnnnnnn.nnn amounts if we allow for three decimal to allow for cents rounding?

  // Amount aims to be a drop-in-replacement class for a 'using Amount = double'.
  // To enable the same code to compile and run with any of the two representations.
  // The class Amount enables more control over currency amount expressions restricted to the limits of a two decimals (cents) currency amount
  class Amount
  {
  public:
    Amount() = default;
    Amount(double value)  {
      // Convert to integer representation of cents
      long long cents = static_cast<long long>(std::round(value * 100));
      double converted_back = cents / 100.0;
      auto error = std::fabs(value - converted_back);

      // If the converted back value does not match the original, it had more than two decimal places
      // Note 240601 - The value 0.01 comes from practical testing. I still fail to understand
      //               how *100 followed by /100 can introduce such a large error?
      //               This code seems to work for now.
      //               TODO: refactor Cratchit to represent Currency values as whole integer cents to avoid the floating point precision problems!
      if (error > 0.01) {
        std::cout << "\nDESIGN_INSUFFICIENCY: Amount(" << value << ") has more than two decimal places. Error:" << error << ". Rounded it to " << converted_back;
        this->m_double_value = converted_back;
      } else {
        this->m_double_value = value; // ok    
      }
        
    }

    /*

    Allow for Currency Math
    Amount + Amount = Amount
    Amount - Amount = Amount
    Amount * Amount = Not defined (NOT an amount)
    double * Amount = Amount
    Amount * Double = Amount
    Amount / double = Amount
    double / Amount = Not defined (NOT an amount)
    Amount / Amount = double ok (and also NOT an Amount)

    */

    // Amount += Amount
    Amount operator+=(const Amount& other) { 
      this->m_double_value += other.m_double_value;
      return *this;
    }

    // Amount + Amount
    Amount operator+(const Amount& other) const {
      Amount result{*this};
      result += other;
      return result;
    }

    // Amount - Amount
    Amount operator-(const Amount& other) const { return Amount(m_double_value - other.m_double_value); }
    Amount operator-() const { return Amount(-m_double_value); }
    // Amount * double
    Amount operator*(double scalar) const { return Amount(m_double_value * scalar); }
    double operator/(const Amount& other) const { return m_double_value / other.m_double_value; }

    // Amount / double
    Amount operator/(double divisor) const { return Amount(m_double_value / divisor); }

    bool operator==(Amount const& other) const {return this->m_double_value == other.m_double_value;}
    auto operator<=>(Amount const& other) const {
      return this->m_double_value <=> other.m_double_value;
    }

    friend double to_double(Amount const& amount);
    friend Amount operator*(double a, Amount const &b);
    
  private:
    double m_double_value;
  };

  // double + Amount
  Amount operator+(double a, Amount const &b) {
    return Amount{a} + b; // Do Amount + Amount
  }

  // double - Amount
  Amount operator-(double a, Amount const &b) {
    return Amount{a} - b; // Do Amount - Amount
  }

  // double * Amount
  Amount operator*(double a, Amount const &b) {
    return Amount{a} * b.m_double_value; // Do Amount * double
  }

  double to_double(Amount const& amount) {
    return amount.m_double_value; 
  }

  // Return Amount rounded to whole value
  Amount round(Amount const& amount) {
    return Amount{std::round(to_double(amount))};
  }

  // Return positive amount value (remove negative sign)
  Amount abs(Amount const& amount) {
    return Amount{std::abs(to_double(amount))};
  }

  // Returns Amount truncated to whole value (ignore decimal cents)
  Amount trunc(Amount const& amount) {
    return Amount{trunc(to_double(amount))};
  }

  std::istream& operator>>(std::istream& is, Amount& amount) {
      double double_value;
      if (is >> double_value) {
        amount = Amount{double_value};
      }
      return is;
  }

  std::ostream& operator<<(std::ostream& os, Amount const& amount) {
      os << std::fixed << std::setprecision(2) << to_double(amount);
      return os;
  }

}

// namespace for Amount as a plain double (the 'original' approach)
namespace DoubleAmount {

  // using Amount= float;
  using Amount= double;

  double to_double(Amount const& amount) {
    return amount; 
  }
}

// Choose the Amount type to use
// using Amount = DoubleAmount::Amount;
// using DoubleAmount::to_double;
using Amount = WrappedDoubleAmount::Amount;


using OptionalAmount = std::optional<Amount>;

OptionalAmount to_amount(std::string const& sAmount) {
	// std::cout << "\nto_amount " << std::quoted(sAmount);
	OptionalAmount result{};
	Amount amount{};
	std::istringstream is{sAmount};
	if (auto pos = sAmount.find(','); pos != std::string::npos) {
		// Handle 123,45 ==> 123.45
		result = to_amount(std::accumulate(sAmount.begin(),sAmount.end(),std::string{},[](auto acc,char ch){
			acc += (ch==',')?'.':ch;
			return acc;
		}));
	}
	else if (is >> amount) {
		result = amount;
	}
	else {
		// handle integer (no decimal point)
		try {
			auto int_amount = std::stoi(sAmount);
			result = static_cast<Amount>(int_amount);
		}
		catch (std::exception const& e) { /* failed - do nothing */}
	}
	// if (result) std::cout << "\nresult " << *result;
	return result;
}

std::string to_string(Amount const& amount) {
  std::ostringstream oss{};
  oss << std::fixed << std::setprecision(2) << to_double(amount);
  return oss.str();
}

OptionalCentsAmount to_cents_amount(std::string const& s) {
	OptionalCentsAmount result{};
	try {
		result = CentsAmount{std::stoi(s)};
	}
	catch (...) {
		// Whine about input and failure
		std::cout << "\nThe string " << std::quoted(s) << " is not a valid cents amount (to_cents_amount returns nullopt)";
	}
	return result;
}

CentsAmount to_cents_amount(Amount const& amount) {
  // Compiler will cast double to CentsAmount::cents_value_type and use CentsAmount{cents_value_type} constructor
	return static_cast<CentsAmount>(std::round(to_double(amount)*100));
}

using UnitsAndCentsValueType = int;
using UnitsAndCents = std::pair<UnitsAndCentsValueType,UnitsAndCentsValueType>;

UnitsAndCents to_units_and_cents(CentsAmount const& cents_amount) {
	UnitsAndCents result{to_whole_part_integer(cents_amount),to_cents_part_integer(cents_amount)};
	return result;
}

Amount to_amount(UnitsAndCents const& units_and_cents) {
  return static_cast<Amount>(units_and_cents.first) + static_cast<Amount>(units_and_cents.second) / 100;
}

std::ostream& operator<<(std::ostream& os,UnitsAndCents const& units_and_cents) {
  bool is_negative = (units_and_cents.first<0) or (units_and_cents.second < 0);
  if (is_negative) os << "-";
	os << abs(units_and_cents.first) << ',' << std::setfill('0') << std::setw(2) << abs(units_and_cents.second);
	return os;
}

std::string to_string(UnitsAndCents const& units_and_cents) {
	std::ostringstream os{};
	os << units_and_cents;
	return os.str();
}

using Date = std::chrono::year_month_day;
using OptionalDate = std::optional<Date>;

std::ostream& operator<<(std::ostream& os, Date const& yyyymmdd) {
	// TODO: Remove when support for ostream << chrono::year_month_day (g++11 stdlib seems to lack support?)
	os << std::setfill('0') << std::setw(4) << static_cast<int>(yyyymmdd.year());
	os << std::setfill('0') << std::setw(2) << static_cast<unsigned>(yyyymmdd.month());
	os << std::setfill('0') << std::setw(2) << static_cast<unsigned>(yyyymmdd.day());
	return os;
}
std::string to_string(Date const& yyyymmdd) {
		std::ostringstream os{};
		os << yyyymmdd;
		return os.str();
}

Date to_date(int year,unsigned month,unsigned day) {
	return Date {
		std::chrono::year{year}
		,std::chrono::month{month}
		,std::chrono::day{day}
	};
}

OptionalDate to_date(std::string const& sYYYYMMDD) {
	// std::cout << "\nto_date(" << sYYYYMMDD << ")";
	OptionalDate result{};
	try {
		if (sYYYYMMDD.size()==8) {
			result = to_date(
				std::stoi(sYYYYMMDD.substr(0,4))
				,static_cast<unsigned>(std::stoul(sYYYYMMDD.substr(4,2)))
				,static_cast<unsigned>(std::stoul(sYYYYMMDD.substr(6,2))));
		}
		else {
			// Handle "YYYY-MM-DD" "YYYY MM DD" etc.
			std::string sDate = filtered(sYYYYMMDD,::isdigit);
			if (sDate.size()==8) result = to_date(sDate);
		}
		// if (result) std::cout << " = " << *result;
		// else std::cout << " = null";
	}
	catch (std::exception const& e) {} // swallow silently
	return result;
}

Date to_today() {
	// TODO: Upgrade to correct std::chrono way when C++20 compiler support is there
	// std::cout << "\nto_today";
	std::ostringstream os{};
	auto now_timet = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm* now_local = localtime(&now_timet);
	return to_date(1900 + now_local->tm_year,1 + now_local->tm_mon,now_local->tm_mday);	
}

class DateRange {
public:
	DateRange(Date const& begin,Date const& end) : m_begin{begin},m_end{end} {}
	DateRange(std::string const& yyyymmdd_begin,std::string const& yyyymmdd_end) {
		OptionalDate begin{to_date(yyyymmdd_begin)};
		OptionalDate end{to_date(yyyymmdd_end)};
		if (begin and end) {
			m_valid = true;
			m_begin = *begin;
			m_end = *end;
		}
	}
	Date begin() const {return m_begin;}
	Date end() const {return m_end;}
	bool contains(Date const& date) const { return begin() <= date and date <= end();}
	operator bool() const {return m_valid;}
private:
	bool m_valid{};
	Date m_begin{};
	Date m_end{};
};
using OptionalDateRange = std::optional<DateRange>;

struct Quarter {
	unsigned ix;
};

Quarter to_quarter(Date const& a_period_date) {
	return {((static_cast<unsigned>(a_period_date.month())-1) / 3u)+ 1u}; // ((0..3) + 1
}

std::chrono::month to_quarter_begin(Quarter const& quarter) {
	unsigned begin_month_no = (quarter.ix-1u) * 3u + 1u; // [0..3]*3 = [0,3,6,9] + 1 = [1,4,7,10]
	return std::chrono::month{begin_month_no};
}

std::chrono::month to_quarter_end(Quarter const& quarter) {
	return (to_quarter_begin(quarter) + std::chrono::months{2});
}

DateRange to_quarter_range(Date const& a_period_date) {
// std::cout << "\nto_quarter_range: a_period_date:" << a_period_date;
	auto quarter = to_quarter(a_period_date);
	auto begin_month = to_quarter_begin(quarter);
	auto end_month = to_quarter_end(quarter);
	auto begin = Date{a_period_date.year()/begin_month/std::chrono::day{1u}};
	auto end = Date{a_period_date.year()/end_month/std::chrono::last}; // trust operator/ to adjust the day to the last day of end_month
  if (false) {
    std::cout << "\nto_quarter_range(" << a_period_date << ") --> " << begin << ".." << end;
  }
	return {begin,end};
}

DateRange to_three_months_earlier(DateRange const& quarter) {
	auto const quarter_duration = std::chrono::months{3};
  // get the year and month for the date range to return
  auto ballpark_end = quarter.end() - quarter_duration;
  // Adjust the end day to the correct one for the range end month
  auto end = ballpark_end.year() / ballpark_end.month() / std::chrono::last;
  // Note: We do not need to adjust the begin day as all month starts on day 1
	return {quarter.begin() - quarter_duration,end};
}

std::ostream& operator<<(std::ostream& os,DateRange const& dr) {
	os << dr.begin() << "..." << dr.end();
	return os;
}

struct IsPeriod {
	DateRange period;
	bool operator()(Date const& date) const {
		return period.contains(date);
	}
};

IsPeriod to_is_period(DateRange const& period) {
	return {period};
}

std::optional<IsPeriod> to_is_period(std::string const& yyyymmdd_begin,std::string const& yyyymmdd_end) {
	std::optional<IsPeriod> result{};
	if (DateRange date_range{yyyymmdd_begin,yyyymmdd_end}) result = to_is_period(date_range);
	else {
		std::cout << "\nERROR, to_is_period failed. Invalid period " << std::quoted(yyyymmdd_begin) << " ... " << std::quoted(yyyymmdd_begin);
	}
	return result;
}

class TaggedAmount {
public:
  friend std::ostream& operator<<(std::ostream& os, TaggedAmount const& ta);
  using OptionalTagValue = std::optional<std::string>;
  using Tags = std::map<std::string,std::string>;
  using ValueId = std::size_t;
  using OptionalValueId = std::optional<ValueId>;
  using ValueIds = std::vector<ValueId>;
  using OptionalValueIds = std::optional<ValueIds>;
  TaggedAmount(Date const& date,CentsAmount const& cents_amount,Tags&& tags = Tags{})
    : m_date{date}
      ,m_cents_amount{cents_amount}
      ,m_tags{tags} {} 

  // Getters
  Date const& date() const {return m_date;}
  CentsAmount const& cents_amount() const {return m_cents_amount;}
  Tags const& tags() const {return m_tags;}	
  Tags& tags() {return m_tags;}

  // Map key to optional value
  OptionalTagValue tag_value(std::string const& key) const {
    OptionalTagValue result{};
    if (m_tags.contains(key)) {
      result = m_tags.at(key); 
    }
    return result;
  }

  bool operator==(TaggedAmount const& other) const;

  // tagged_amount::to_string ensures it does not override std::to_string(integral type) or any local one
  static std::string to_string(TaggedAmount::ValueId value_id) {
    std::ostringstream os{};
    os << std::setw(sizeof(std::size_t) * 2) << std::setfill('0') << std::hex << value_id;
    return os.str();
  }

private:
  Date m_date;
  CentsAmount m_cents_amount;
  Tags m_tags;
}; // class TaggedAmount

using TaggedAmounts = std::vector<TaggedAmount>;
using OptionalTaggedAmount = std::optional<TaggedAmount>;
using OptionalTaggedAmounts = std::optional<TaggedAmounts>;

template <class T>
__attribute__((no_sanitize("undefined")))
inline void hash_combine(std::size_t& seed, const T& v)
{
    constexpr auto shift_left_count = 1;
    constexpr std::size_t max_size_t = std::numeric_limits<std::size_t>::max();
    constexpr std::size_t mask = max_size_t >> shift_left_count;
    std::hash<T> hasher;
    // Note: I decided to NOT use boost::hash_combine code as it will cause integer overflow and thus undefined behaviour.
    //       And I need the hash I produce to be consistent so I can use it in persistent storage (the environment text file for Cratchit)
    //       Now, maybe the risk of getting different values on different hardware or runtimes (macOS, Linus, Windows etc) is non existent in practice.
    //       But hey, better safe than sorry, right?
    // seed ^= hasher(v) + 0x9e3779b9 + ((seed & mask) <<6) + (seed>>2); // *magic* dustribution as defined by boost::hash_combine
    seed ^= (hasher(v) & mask) << shift_left_count; // Simple shift left distribution and no addition
}

namespace std {
  template<>
  struct hash<TaggedAmount> {
      std::size_t operator()(TaggedAmount const& ta) const noexcept {
        std::size_t result{};
        auto yyyymmdd = ta.date();
        hash_combine(result, static_cast<int>(yyyymmdd.year()));
        hash_combine(result, static_cast<unsigned>(yyyymmdd.month()));
        hash_combine(result, static_cast<unsigned>(yyyymmdd.day()));
        hash_combine(result, ta.cents_amount());
        for (auto const& [key,value] : ta.tags()) {
          hash_combine(result, key);
          hash_combine(result, value);
        }
        return result;
      }
  };
} // namespace std

TaggedAmount::ValueId to_value_id(TaggedAmount const& ta) {
  return std::hash<TaggedAmount>{}(ta);
}

bool TaggedAmount::operator==(TaggedAmount const& other) const {
  auto result =     this->date() == other.date() 
                and this->cents_amount() == other.cents_amount()
                and std::all_of(
                    m_tags.begin()
                  ,m_tags.end()
                  ,[&other](Tags::value_type const& entry) {
                    return (     (entry.first.starts_with("_"))
                              or (     other.tags().contains(entry.first) 
                                    and other.tags().at(entry.first) == entry.second));
                  });

  // std::cout << "\nTaggedAmountClass::operator== ";
  // if (result) std::cout << "TRUE"; else std::cout << "FALSE";
  return result;
}

std::ostream& operator<<(std::ostream& os, TaggedAmount const& ta) {
  os << TaggedAmount::to_string(to_value_id(ta));
  os << " " << ::to_string(ta.date());
  os << " " << ::to_string(to_units_and_cents(ta.cents_amount()));
  for (auto const& tag : ta.tags()) {
    os << " \"" << tag.first << "=" << tag.second << "\"";
  }
  return os;
}

TaggedAmount::OptionalValueId to_value_id(std::string const& s) {
	// std::cout << "\nto_value_id()" << std::flush;
	TaggedAmount::OptionalValueId result{};
	TaggedAmount::ValueId value_id{};
	std::istringstream is{s};
	try {
		is >> std::hex >> value_id;
		result = value_id;
	}
	catch (...) {
		std::cout << "\nto_value_id(" << std::quoted(s) << ") failed. General Exception caught." << std::flush;
	}
	return result;
}

TaggedAmount::OptionalValueIds to_value_ids(Key::Path const& sids) {
	std::cout << "\nto_value_ids()" << std::flush;
	TaggedAmount::OptionalValueIds result{};
	TaggedAmount::ValueIds value_ids{};
	for (auto const& sid : sids) {
		if (auto value_id = to_value_id(sid)) {
			std::cout << "\n\tA valid instance id sid=" << std::quoted(sid);
			value_ids.push_back(*value_id);
		}
		else {
			std::cout << "\nto_value_ids: Not a valid instance id string sid=" << std::quoted(sid) << std::flush;
		}
	}
	if (value_ids.size() == sids.size()) {
		result = value_ids;
	}
	else {
		std::cout << "\nto_value_ids(Key::Path const& " << sids.to_string() << ") Failed. Created" << value_ids.size() << " out of " << sids.size() << " possible.";
	}
	return result;
}

using TaggedAmountValueIdMap = std::map<TaggedAmount::ValueId,TaggedAmount>; 
using TaggedAmountValueIdMap = std::map<std::size_t,TaggedAmount>;

class DateOrderedTaggedAmountsContainer {
	public:
		using OptionalTagValue = TaggedAmount::OptionalTagValue;
		using Tags = TaggedAmount::Tags;
		using ValueId = TaggedAmount::ValueId;
		using OptionalValueId = TaggedAmount::OptionalValueId;
		using ValueIds = TaggedAmount::ValueIds;
		using OptionalValueIds = TaggedAmount::OptionalValueIds;

		using iterator = TaggedAmounts::iterator;
		using const_iterator = TaggedAmounts::const_iterator;
		TaggedAmounts const& tagged_amounts() {return m_date_ordered_tagged_amounts;}
		std::size_t size() const { return m_date_ordered_tagged_amounts.size();}
		iterator begin() {return m_date_ordered_tagged_amounts.begin();}
		iterator end() {return m_date_ordered_tagged_amounts.end();}
		const_iterator begin() const {return m_date_ordered_tagged_amounts.begin();}
		const_iterator end() const {return m_date_ordered_tagged_amounts.end();}
		auto in_date_range(DateRange const& date_period) {
			auto first = std::find_if(this->begin(),this->end(),[&date_period](auto const& ta){
				return (ta.date() >= date_period.begin());
			});
			auto last = std::find_if(this->begin(),this->end(),[&date_period](auto const& ta){
				return (ta.date() > date_period.end());
			});
			return std::ranges::subrange(first,last);
		}

    OptionalTaggedAmount at(ValueId const& value_id) {
			std::cout << "\nDateOrderedTaggedAmountsContainer::at(" << TaggedAmount::to_string(value_id) << ")" << std::flush;
			OptionalTaggedAmount result{};
			if (m_tagged_amount_value_id_map.contains(value_id)) {
				result = m_tagged_amount_value_id_map.at(value_id);
			}
			else {
				std::cout << "\nDateOrderedTaggedAmountsContainer::at could not find a mapping to value_id=" << TaggedAmount::to_string(value_id) << std::flush;
			}
			return result;
    }

		OptionalTaggedAmount operator[](ValueId const& value_id) {
			std::cout << "\nDateOrderedTaggedAmountsContainer::operator[](" << TaggedAmount::to_string(value_id) << ")" << std::flush;
			OptionalTaggedAmount result{};
			if (auto o_ptr = this->at(value_id)) {
				result = o_ptr;
			}
			else {
				std::cout << "\nDateOrderedTaggedAmountsContainer::operator[] could not find a mapping to value_id=" << TaggedAmount::to_string(value_id) << std::flush;
			}
			return result;
		}

		OptionalTaggedAmounts to_ta_ptrs(ValueIds const& value_ids) {
			std::cout << "\nDateOrderedTaggedAmountsContainer::to_ta_ptrs()" << std::flush;
			OptionalTaggedAmounts result{};
			TaggedAmounts tas{};
			for (auto const& value_id : value_ids) {
				if (auto o_ta = (*this)[value_id]) {
					tas.push_back(*o_ta);
				}
				else {
					std::cout << "\nDateOrderedTaggedAmountsContainer::to_ta_ptrs() failed. No instance found for value_id=" << TaggedAmount::to_string(value_id) << std::flush;
				}
			}
			if (tas.size() == value_ids.size()) {
				result = tas;
			}
			else {
				std::cout << "\nto_ta_ptrs() Failed. tas.size() = " << tas.size() << " IS NOT provided value_ids.size() = " << value_ids.size() << std::flush;
			}
			return result;
		}

		DateOrderedTaggedAmountsContainer& clear() {
			m_tagged_amount_value_id_map.clear();
			m_date_ordered_tagged_amounts.clear();
			return *this;
		}

		DateOrderedTaggedAmountsContainer& operator=(DateOrderedTaggedAmountsContainer const& other) {
			this->m_date_ordered_tagged_amounts = other.m_date_ordered_tagged_amounts;
			this->m_tagged_amount_value_id_map = other.m_tagged_amount_value_id_map;
			return *this;
		}

    // TODO 240218: Consider to provide a predicate for the caller to control what should be regarded as "same value"
    //              This could be a way to apply special treatment to SIE aggregate tagged amounts (last in wins and erases any previous occurrence of same series and sequence number of same fiscal year) 
    // TODO: 240225: NOTE that insert of an aggregate does not insert the members of the aggregate. What is a good solution for this?
		iterator insert(TaggedAmount const& ta) {
			auto result = m_date_ordered_tagged_amounts.end();
      auto value_id = to_value_id(ta);
      if (m_tagged_amount_value_id_map.contains(value_id) == false) {
        if (true) {
          std::cout << "\nthis:" << this << " Inserted new " << ta;
        }
        // Find the last element with a date less than the date of ta        
        auto end = std::upper_bound(m_date_ordered_tagged_amounts.begin(),m_date_ordered_tagged_amounts.end(),ta,[](TaggedAmount const& ta1, TaggedAmount const& ta2) {
            return ta1.date() < ta2.date();
        });

			  m_tagged_amount_value_id_map.insert({value_id,ta}); // id -> ta
				result = m_date_ordered_tagged_amounts.insert(end,ta); // place after all with date less than the one of ta
      }
      else {
        std::cout << "\nthis:" << this;
        std::cout << "\n\tDESIGN_INSUFFICIENCY: Error, Skipped new[" << TaggedAmount::to_string(value_id) << "] " << ta;
        std::cout << "\n\t                             same as old[" << TaggedAmount::TaggedAmount::to_string(to_value_id(m_tagged_amount_value_id_map.at(value_id))) << "] " << m_tagged_amount_value_id_map.at(value_id);
      }
			return result;
		}

    DateOrderedTaggedAmountsContainer erase(ValueId const& value_id) {
      if (auto o_ptr = this->at(value_id)) {
        m_tagged_amount_value_id_map.erase(value_id);
        auto iter = std::ranges::find(m_date_ordered_tagged_amounts,*o_ptr);
        if (iter != m_date_ordered_tagged_amounts.end()) {
          m_date_ordered_tagged_amounts.erase(iter);
        }
        else {
          std::cout << "\nDESIGN INSUFFICIENCY: Failed to erase tagged amount in map but not in date-ordered-vector, value_id " << value_id;
        }
      }
      else {
        std::cout << "nDateOrderedTaggedAmountsContainer::at failed to find value_id " << value_id;
      }
      return *this;
    }

		DateOrderedTaggedAmountsContainer const& for_each(auto f) const {
			for (auto const& ta : m_date_ordered_tagged_amounts) {
				f(ta);
			}
			return *this;
		}

		DateOrderedTaggedAmountsContainer& operator+=(DateOrderedTaggedAmountsContainer const& other) {
			other.for_each([this](TaggedAmount const& ta){
        // TODO 240217: Consider a way to ensure that SIE entries in SIE file has preceedence (overwrite any existing tagged amounts reflecting the same events)
        // Hm...Maybe this is NOT the convenient place to do this?
				this->insert(ta);
			});
			return *this;
		}
		DateOrderedTaggedAmountsContainer& operator+=(TaggedAmounts const& tas) {
			for (auto const& ta : tas) this->insert(ta);
			return *this;
		}

		DateOrderedTaggedAmountsContainer& operator=(TaggedAmounts const& tas) {
			this->clear();
			*this += tas;
			return *this;
		}

	private:
    // Note: Each tagged amount pointer instance is stored twice. Once in a mapping between value_id and tagged amount pointer and once in a vector ordered by date.
		TaggedAmountValueIdMap m_tagged_amount_value_id_map{}; // map <instance id> -> <tagged amount ptr>
		TaggedAmounts m_date_ordered_tagged_amounts{};  // vector of tagged amount ptrs ordered by date
}; // class DateOrderedTaggedAmountsContainer

// Forward declaration of data and members of namespaces
namespace BAS {
	using AccountNo = unsigned int;
	using OptionalAccountNo = std::optional<AccountNo>;
	using AccountNos = std::vector<AccountNo>;
	using OptionalAccountNos = std::optional<AccountNos>;

	OptionalAccountNo to_account_no(std::string const& s);
}

namespace tas {
	// namespace for processing that produces tagged amounts and tagged amounts

	// Generic for parsing a range or container of tagged amount pointers into a vector of saldo tagged amounts (tagged with 'BAS' for each accumulated bas account)
	TaggedAmounts to_bas_omslutning(auto const& tas) {
		TaggedAmounts result{};
		using BASBuckets = std::map<BAS::AccountNo,TaggedAmounts>;
		BASBuckets bas_buckets{};
		auto is_valid_bas_account_transaction = [](TaggedAmount const& ta){
			if (ta.tags().contains("BAS") and !(BAS::to_account_no(ta.tags().at("BAS")))) {
				// Whine about invalid tagging of 'BAS' tag!
				// It is vital we do NOT have any badly tagged BAS account transactions as this will screw up the saldo calculation!
				std::cout << "\nDESIGN_INSUFFICIENCY: tas::to_bas_omslutning failed to create a valid BAS account no from tag 'BAS' with value " << std::quoted(ta.tags().at("BAS"));
				return false;
			}
			else return (     (ta.tags().contains("BAS"))
										and (BAS::to_account_no(ta.tags().at("BAS")))
										and (    (     (ta.tags().contains("type") == false))
													or (     (ta.tags().contains("type") == true)
															 and (ta.tags().at("type") != "saldo"))));
		};
		for (auto const& ta : tas) {
			if (is_valid_bas_account_transaction(ta)) {
				bas_buckets[*BAS::to_account_no(ta.tags().at("BAS"))].push_back(ta);
			}
		}
		for (auto const& [bas_account_no,tas] : bas_buckets) {
			Date period_end_date{};
			std::cout << "\n" << bas_account_no;
			auto cents_saldo = std::accumulate(tas.begin(),tas.end(),CentsAmount{0},[&period_end_date](auto acc, auto const& ta){
				period_end_date = std::max(period_end_date,ta.date()); // Ensure we keep the latest date. NOTE: We expect they are in growing date order. But just in case...
				acc += ta.cents_amount();
				std::cout << "\n\t" << period_end_date << " " << to_string(to_units_and_cents(ta.cents_amount())) << " ackumulerat:" << to_string(to_units_and_cents(acc));
				return acc;
			});

			TaggedAmount saldo_ta{period_end_date,cents_saldo};
			saldo_ta.tags()["BAS"] = std::to_string(bas_account_no);
			saldo_ta.tags()["type"] = "saldo";
			result.push_back(saldo_ta);
		}
		return result;
	}
} // namespace tas

namespace SKV::SRU::INK1 {
	extern const char* ink1_csv_to_sru_template;
	extern const char* k10_csv_to_sru_template;
}
namespace SKV::SRU::INK2 {
	extern const char* INK2_csv_to_sru_template;
	extern const char* INK2S_csv_to_sru_template;
	extern const char* INK2R_csv_to_sru_template;
}

namespace BAS::SRU::INK2 {
	extern char const* INK2_19_P1_intervall_vers_2_csv;
	void parse(char const* INK2_19_P1_intervall_vers_2_csv);
}

namespace BAS::K2::AR {

  namespace ar_online {
    // Code to operate on open data from URL:https://www.arsredovisning-online.se/bas_kontoplan
  	extern char const* bas_2022_mapping_to_k2_ar_text;

    namespace detail {

      using Lisp = std::string;

      struct State {
        bool success{};
        std::string parsed{};
        std::string msg{};
        std::size_t index{};
        std::string s;
      };

      std::ostream& operator<<(std::ostream& os,State const& state) {
        if (state.success) {
          os << "parsed:" << std::quoted(state.parsed);
        }
        else os << "*ERROR* "  << "parsed:" << std::quoted(state.parsed) << " ==> " << state.msg;
        os << ", stopped at index " << state.index;
        return os;
      }

      struct Parser {
          virtual State run(State const& state) = 0;
      };

      struct End : public Parser {
        virtual State run(State const& state) {
          State result{state};
          if (result.success) {
            if (result.index != state.s.size()) {
              result.success = false;
              result.msg = std::string{"Expected end of input. Got index="} + std::to_string(result.index);
            }
          }
          return result;
        }
      };

      struct AnyAlpha : public Parser {
        virtual State run(State const& state) {
          State result{state};
          if (state.success) {
            if (state.index >= state.s.size()) {
              result.success = false;
              result.msg = "Expected AnyAlpha but encountered end of string";
            }
            else {
              auto ch = state.s[state.index];
              if (std::isalpha(ch)) {
                result.parsed += ch;
                ++result.index;
              }
              else {
                result.success = false;
                result.msg = std::string{"Expected alpha but encountered '"} + ch + '\'';
              }
            }
          }
          return result;
        }
      };

      struct AnyDigit : public Parser {
        virtual State run(State const& state) {
          State result{state};
          if (state.success) {
            if (state.index >= state.s.size()) {
              result.success = false;
              result.msg = "Expected AnyDigit but encountered end of string";
            }
            else {
              auto ch = state.s[state.index];
              if (std::isdigit(ch)) {
                result.parsed += ch;
                ++result.index;
              }
              else {
                result.success = false;
                result.msg = std::string{"Expected digit but encountered '"} + ch + '\'';
              }
            }
          }
          return result;
        }
      };

      struct Char : public Parser {
        char key;
        Char(char key) : key{key} {}
        virtual State run(State const& state) {
          State result{state};
          if (state.success) {
            if (state.index >= state.s.size()) {
              result.success = false;
              result.msg = std::string{"Expected Character '"} + key + "' but encountered end";
            }
            else {
              auto ch = state.s[state.index];
              if (ch == key) {
                result.parsed += ch;
                ++result.index;
              }
              else {
                result.success = false;
                result.msg = std::string{"Expected Character '"} + key + "' but encountered '" + ch + "'";
              }
            }
          }
          return result;
        }
      };

      struct Many : public Parser {
        Parser* parser;
        Many(Parser* parser) : parser{parser} {};
        virtual State run(State const& state) {
          State result{state};
          while (result.success) {
            result = parser->run(result);
          }
          result.success = true; // always succeeds
          return result;
        }
      };

      struct And : public Parser {
        std::vector<Parser*> parsers;
        And(std::initializer_list<Parser*> parsers) : parsers{parsers} {}
        And() : parsers{} {}
        void push_back(Parser* parser) {parsers.push_back(parser);}
        virtual State run(State const& state) {
          State result{state};
          if (state.success) {
            auto success = std::all_of(parsers.begin(),parsers.end(),[&result](Parser* parser){
              result = parser->run(result);
              return result.success;
            });
          }
          return result;
        }
      };

      struct Or : public Parser {
        std::vector<Parser*> parsers;
        Or(std::initializer_list<Parser*> parsers) : parsers{parsers} {}
        void push_back(Parser* parser) {parsers.push_back(parser);}
        virtual State run(State const& state) {
          State result{state};
          if (state.success) {
            auto success = std::any_of(parsers.begin(),parsers.end(),[&state,&result](Parser* parser){
              result = parser->run(state); // try each parser on initial state
              return result.success;
            });
          }          
          return result;
        }
      };

      struct Not : public Parser {
        Parser* parser;
        Not(Parser* parser) : parser{parser} {}
        virtual State run(State const& state) {
          State result{state};
          if (state.success) {
            if (state.index >= state.s.size()) {
              result.success=false;
              result.msg = "Not parser expected to fail but encountered end of input";            
            }
            else {
              result = parser->run(state);
              if (result.success==false) {
                result.parsed += state.s[state.index];
                ++result.index; // accept and advance on failure :)
                result.success=true;
              }
              else {          
                result = state; // revert successful parse      
                result.success=false;
                result.msg = "Not parser expected to fail but succeeded";
              }
            }
          }
          return result;
        }
      };

      struct Word : public Parser {
        std::string key;
        Word(std::string const& key) : key{key} {}
        virtual State run(State const& state) {
          State result{state};
          if (state.success) {
            auto word = new detail::And{};
            for (auto ch : key) {
              word->push_back(new detail::Char{ch});
            }
            result = word->run(state);
          }
          return result;
        }
      };

    } // namespace detail

    class Predicate {};

    Predicate to_predicate(std::string const& field_heading_text, std::string const& bas_accounts_text) {
      Predicate result{};
      // std::cout << "\nto_predicate(" << std::quoted(field_heading_text) << ")";
      auto entry_id = std::hash<std::string>{}(field_heading_text);
      std::cout << "\n\t" << std::to_string(entry_id) << " "  << std::quoted(bas_accounts_text);
      /*
      to_predicate("Nettoomsättning")
        15814542743395917030 "3000-3799"
      to_predicate("Aktiverat arbete för egen räkning")
        5013889680660623303 "3800-3899"
      to_predicate("Övriga rörelseintäkter")
        18287096853477482975 "3900-3999"
      to_predicate("Råvaror och förnödenheter")
        14247317458687115239 "4000-4799 eller 4910-4931"
      to_predicate("Förändring av lager av produkter i arbete, färdiga varor och pågående arbete för annans räkning")
        15698735858152199622 "4900-4999 (förutom 4910-4931, 4960-4969 och 4980-4989)"
      to_predicate("Handelsvaror")
        2341312253238452919 "4960-4969 eller 4980-4989"
      to_predicate("Övriga externa kostnader")
        15983828328320588867 "5000-6999"
      to_predicate("Personalkostnader")
        3368048551155823778 "7000-7699"
      to_predicate("Av- och nedskrivningar av materiella och immateriella anläggningstillgångar")
        17719833423615943326 "7700-7899 (förutom 7740-7749 och 7790-7799)"
      to_predicate("Nedskrivningar av omsättningstillgångar utöver normala nedskrivningar")
        7490942823118381164 "7740-7749 eller 7790-7799"
      to_predicate("Övriga rörelsekostnader")
        8905285061947701061 "7900-7999"
      to_predicate("Resultat från andelar i koncernföretag")
        997745970729285789 "8000-8099 (förutom 8070-8089)"
      to_predicate("Nedskrivningar av finansiella anläggningstillgångar och kortfristiga placeringar")
        5126772769069083656 "8070-8089, 8170-8189, 8270-8289 eller 8370-8389"
      to_predicate("Resultat från andelar i intresseföretag och gemensamt styrda företag")
        6676188007026535397 "8100-8199 (förutom 8113, 8118, 8123, 8133 och 8170-8189)"
      to_predicate("Resultat från övriga företag som det finns ett ägarintresse i")
        6557357244805449310 "8113, 8118, 8123 eller 8133"
      to_predicate("Resultat från övriga finansiella anläggningstillgångar")
        13398399727449445833 "8200-8299 (förutom 8270-8289)"
      to_predicate("Övriga ränteintäkter och liknande resultatposter")
        4710157810919164622 "8300-8399 (förutom 8370-8389)"
      to_predicate("Räntekostnader och liknande resultatposter")
        17654632129722699895 "8400-8499"
      to_predicate("Extraordinära intäkter")
        1259188922580541696 "8710"
      to_predicate("Extraordinära kostnader")
        16389752887058689117 "8750"
      to_predicate("Förändring av periodiseringsfonder")
        17296971319485525067 "8810-8819"
      to_predicate("Erhållna koncernbidrag")
        89297138602359847 "8820-8829"
      to_predicate("Lämnade koncernbidrag")
        7531591489750836154 "8830-8839"
      to_predicate("Övriga bokslutsdispositioner")
        2734042456554829261 "8840-8849 eller 8860-8899"
      to_predicate("Förändring av överavskrivningar")
        15315753260330591485 "8850-8859"
      to_predicate("Skatt på årets resultat")
        12726941330968082302 "8900-8979"
      to_predicate("Övriga skatter")
        5233002284073233210 "8980-8989"
      to_predicate("Koncessioner, patent, licenser, varumärken samt liknande rättigheter")
        14313880560123324485 "1020-1059 eller 1080-1089 (förutom 1088)"
      to_predicate("Hyresrätter och liknande rättigheter")
        13280259054684945024 "1060-1069"
      to_predicate("Goodwill")
        4706826405193251993 "1070-1079"
      to_predicate("Förskott avseende immateriella anläggningstillgångar")
        8435059871618616320 "1088"
      to_predicate("Byggnader och mark")
        17992054989063316172 "1100-1199 (förutom 1120-1129 och 1180-1189)"
      to_predicate("Förbättringsutgifter på annans fastighet")
        12246822043065181435 "1120-1129"
      to_predicate("Pågående nyanläggningar och förskott avseende materiella anläggningstillgångar")
        7304838879679020677 "1180-1189 eller 1280-1289"
      to_predicate("Maskiner och andra tekniska anläggningar")
        15695145767277007193 "1210-1219"
      to_predicate("Inventarier, verktyg och installationer")
        2988580034553160662 "1220-1279 (förutom 1260)"
      to_predicate("Övriga materiella anläggningstillgångar")
        3522836639895033252 "1290-1299"
      to_predicate("Andelar i koncernföretag")
        6072736618874988835 "1310-1319"
      to_predicate("Fordringar hos koncernföretag")
        15259969052810058218 "1320-1329"
      to_predicate("Andelar i intresseföretag och gemensamt styrda företag")
        7828517953880225822 "1330-1339 (förutom 1336-1337)"
      to_predicate("Ägarintressen i övriga företag")
        15344672548621634930 "1336-1337"
      to_predicate("Fordringar hos intresseföretag och gemensamt styrda företag")
        8544230840928436188 "1340-1349 (förutom 1346-1347)"
      to_predicate("Fordringar hos övriga företag som det finns ett ägarintresse i")
        13850917336143100446 "1346-1347"
      to_predicate("Andra långfristiga värdepappersinnehav")
        4601341298284676885 "1350-1359"
      to_predicate("Lån till delägare eller närstående")
        2725614420353494065 "1360-1369"
      to_predicate("Andra långfristiga fordringar")
        2746984126913941224 "1380-1389"
      to_predicate("Råvaror och förnödenheter")
        14247317458687115239 "1410-1429, 1430, 1431 eller 1438"
      to_predicate("Varor under tillverkning")
        3433350423138479992 "1432-1449 (förutom 1438)"
      to_predicate("Färdiga varor och handelsvaror")
        2346049323362567707 "1450-1469"
      to_predicate("Pågående arbete för annans räkning")
        16122789554500341138 "1470-1479"
      to_predicate("Förskott till leverantörer")
        4976455952353268623 "1480-1489"
      to_predicate("Övriga lagertillgångar")
        4802107533869083065 "1490-1499"
      to_predicate("Kundfordringar")
        2995199977553315731 "1500-1559 eller 1580-1589"
      to_predicate("Fordringar hos koncernföretag")
        15259969052810058218 "1560-1569 eller 1660-1669"
      to_predicate("Fordringar hos intresseföretag och gemensamt styrda företag")
        8544230840928436188 "1570-1579 (förutom 1573) eller 1670-1679 (förutom 1673)"
      to_predicate("Fordringar hos övriga företag som det finns ett ägarintresse i")
        13850917336143100446 "1573 eller 1673"
      to_predicate("Övriga fordringar")
        14712306452208124913 "1590-1619, 1630-1659 eller 1680-1689"
      to_predicate("Upparbetad men ej fakturerad intäkt")
        8272007022483877119 "1620-1629"
      to_predicate("Tecknat men ej inbetalat kapital")
        5683943810747285519 "1690-1699"
      to_predicate("Förutbetalda kostnader och upplupna intäkter")
        13922947895874900402 "1700-1799"
      to_predicate("Övriga kortfristiga placeringar")
        9749819421879779127 "1800-1899 (förutom 1860-1869)"
      to_predicate("Andelar i koncernföretag")
        6072736618874988835 "1860-1869"
      to_predicate("Kassa och bank")
        6559204722697948293 "1900-1989"
      to_predicate("Redovisningsmedel")
        315113652544154573 "1990-1999"
      to_predicate("Aktiekapital")
        7437289880448287687 "2081, 2083 eller 2084"
      to_predicate("Ej registrerat aktiekapital")
        3871176652399195503 "2082"
      to_predicate("Uppskrivningsfond")
        14990993219723958545 "2085"
      to_predicate("Reservfond")
        17443004639063377112 "2086"
      to_predicate("Bunden överkursfond")
        3993155805231017139 "2087"
      to_predicate("Balanserat resultat")
        16424774790432980181 "2090, 2091, 2093-2095 eller 2098"
      to_predicate("Fri överkursfond")
        16360101710131019435 "2097"
      to_predicate("Periodiseringsfonder")
        6306324468725266592 "2110-2149"
      to_predicate("Ackumulerade överavskrivningar")
        1229599413901569056 "2150-2159"
      to_predicate("Övriga obeskattade reserver")
        5217832914740960127 "2160-2199"
      to_predicate("Avsättningar för pensioner och liknande förpliktelser enligt lagen (1967:531) om tryggande av pensionsutfästelse m.m.")
        16217660281104500162 "2210-2219"
      to_predicate("Övriga avsättningar")
        15081677615273795926 "2220-2229 eller 2250-2299"
      to_predicate("Övriga avsättningar för pensioner och liknande förpliktelser")
        109226735215221737 "2230-2239"
      to_predicate("Obligationslån")
        14215819343704291797 "2310-2329"
      to_predicate("Checkräkningskredit")
        8122810737165346528 "2330-2339"
      to_predicate("Övriga skulder till kreditinstitut")
        8642848485431095587 "2340-2359"
      to_predicate("Skulder till koncernföretag")
        18427459488302637597 "2360-2369"
      to_predicate("Skulder till intresseföretag och gemensamt styrda företag")
        8703083173403804557 "2370-2379 (förutom 2373)"
      to_predicate("Skulder till övriga företag som det finns ett ägarintresse i")
        7947902124699524542 "2373"
      to_predicate("Övriga skulder")
        15255530962611194028 "2390-2399"
      to_predicate("Övriga skulder till kreditinstitut")
        8642848485431095587 "2410-2419"
      to_predicate("Förskott från kunder")
        9376625536673363084 "2420-2429"
      to_predicate("Pågående arbete för annans räkning")
        16122789554500341138 "2430-2439"
      to_predicate("Leverantörsskulder")
        16403934149583209484 "2440-2449"
      to_predicate("Fakturerad men ej upparbetad intäkt")
        13695742501450611223 "2450-2459"
      to_predicate("Skulder till koncernföretag")
        18427459488302637597 "2460-2469 eller 2860-2869"
      to_predicate("Skulder till intresseföretag och gemensamt styrda företag")
        8703083173403804557 "2470-2479 (förutom 2473) eller 2870-2879 (förutom 2873)"
      to_predicate("Skulder till övriga företag som det finns ett ägarintresse i")
        7947902124699524542 "2473 eller 2873"
      to_predicate("Checkräkningskredit")
        8122810737165346528 "2480-2489"
      to_predicate("Övriga skulder")
        15255530962611194028 "2490-2499 (förutom 2492), 2600-2799, 2810-2859 eller 2880-2899"
      to_predicate("Växelskulder")
        16891141477899543375 "2492"
      to_predicate("Skatteskulder")
        5841233692153537502 "2500-2599"
      to_predicate("Upplupna kostnader och förutbetalda intäkter")
        7191926069038742969 "2900-2999"
      */

      detail::State state{.success=true, .s=bas_accounts_text};
      // 15814542743395917030 "3000-3799"
      // 5013889680660623303 "3800-3899"
      // 18287096853477482975 "3900-3999"
      auto digit = new detail::AnyDigit{};
      auto digits = new detail::Many(digit);
      auto hyphen = new detail::Char{'-'};
      auto end = new detail::End{};
      auto range = new detail::And{digits,hyphen,digits};
      // 14247317458687115239 "4000-4799 eller 4910-4931"
      auto space = new detail::Char{' '};
      auto spaces = new detail::Many{space};
      auto eller = new detail::Word{"eller"};
      auto eller_range = new detail::And{spaces,eller,spaces,range};
      auto optional_eller_range = new detail::Or{end,eller_range};
      auto range_and_optional_eller_range = new detail::And{range,optional_eller_range};
      // state = range_and_optional_eller_range->run(state);

      // 15698735858152199622 "4900-4999 (förutom 4910-4931, 4960-4969 och 4980-4989)"
      // 2341312253238452919 "4960-4969 eller 4980-4989"
      // 15983828328320588867 "5000-6999"
      // 3368048551155823778 "7000-7699"
      // 17719833423615943326 "7700-7899 (förutom 7740-7749 och 7790-7799)"
      // 7490942823118381164 "7740-7749 eller 7790-7799"
      // 8905285061947701061 "7900-7999"
      // 997745970729285789 "8000-8099 (förutom 8070-8089)"
      auto left_bracket = new detail::Char{'('};
      auto forutom = new detail::Word{"förutom"};
      auto comma = new detail::Char{','};
      auto och = new detail::Word{"och"};
      auto alpha = new detail::AnyAlpha{};
      auto alphanumericandspace = new detail::Or{alpha,digit,space};
      auto alphanumericandspaces = new detail::Many{alphanumericandspace};
      auto right_bracket = new detail::Char{')'};
      auto not_right_bracket = new detail::Not{right_bracket};
      auto space_and_not_right_bracket = new detail::Or{space,not_right_bracket};
      auto many_space_and_not_right_bracket = new detail::Many{space_and_not_right_bracket};
      auto comma_och_or_space = new detail::Or{comma,och,space};
      auto exclude_list_separation = new detail::Many{comma_och_or_space};
      auto list_entry = new detail::And{exclude_list_separation,range};
      auto list = new detail::Many{list_entry};
      auto exclude_list = new detail::And{space,left_bracket,forutom,list,right_bracket};
      auto optional_eller_range_or_exclude_list = new detail::Or{eller_range,exclude_list,end};
      auto range_and_optional_eller_range_or_exclude_list = new detail::And{range,optional_eller_range_or_exclude_list,end};
      // state = range_and_optional_eller_range_or_exclude_list->run(state);

      // 5126772769069083656 "8070-8089, 8170-8189, 8270-8289 eller 8370-8389"
      auto comma_space = new detail::And{comma,space};
      auto space_eller_space = new detail::And{space,eller,space};
      auto include_list_separation = new detail::Or{comma_space,space_eller_space};
      auto include_list_entry = new detail::And{include_list_separation,range};
      auto include_list = new detail::Many{include_list_entry};
      auto optional_range_exclude_list_or_include_list = new detail::Or{exclude_list,end,include_list};
      auto range_and_optional_range_exclude_list_or_include_list = new detail::And{range,optional_range_exclude_list_or_include_list,end};
      // state = range_and_optional_range_exclude_list_or_include_list->run(state);

      // 6676188007026535397 "8100-8199 (förutom 8113, 8118, 8123, 8133 och 8170-8189)"
      auto number = new detail::And{digit,digit,digit,digit};
      auto leaf = new detail::Or{range,number};
      auto exclude_leaf_entry = new detail::And{exclude_list_separation,leaf};
      auto exclude_leaf_entries = new detail::Many{exclude_leaf_entry};
      auto exclude_leaf_list = new detail::And{space,left_bracket,forutom,exclude_leaf_entries,right_bracket,end};
      auto include_leaf_entry = new detail::And{include_list_separation,leaf};
      auto include_leaf_list = new detail::Many{include_leaf_entry};
      auto optional_list = new detail::Or{exclude_leaf_list,end,include_leaf_list};
      auto leaf_and_optional_list = new detail::And{leaf,optional_list,end};
      state = leaf_and_optional_list->run(state);
      std::cout << "\n\t" << state;
      // 6557357244805449310 "8113, 8118, 8123 eller 8133"
      // 13398399727449445833 "8200-8299 (förutom 8270-8289)"
      // 4710157810919164622 "8300-8399 (förutom 8370-8389)"
      // 17654632129722699895 "8400-8499"
      // 1259188922580541696 "8710"
      // 16389752887058689117 "8750"
      // 17296971319485525067 "8810-8819"
      // 89297138602359847 "8820-8829"
      // 7531591489750836154 "8830-8839"
      // 2734042456554829261 "8840-8849 eller 8860-8899"
      // 15315753260330591485 "8850-8859"
      // 12726941330968082302 "8900-8979"
      // 5233002284073233210 "8980-8989"
      // 14313880560123324485 "1020-1059 eller 1080-1089 (förutom 1088)"
      // 13280259054684945024 "1060-1069"
      // 4706826405193251993 "1070-1079"
      // 8435059871618616320 "1088"
      // 17992054989063316172 "1100-1199 (förutom 1120-1129 och 1180-1189)"
      // 12246822043065181435 "1120-1129"
      // 7304838879679020677 "1180-1189 eller 1280-1289"
      // 15695145767277007193 "1210-1219"
      // 2988580034553160662 "1220-1279 (förutom 1260)"
      // 3522836639895033252 "1290-1299"
      // 6072736618874988835 "1310-1319"
      // 15259969052810058218 "1320-1329"
      // 7828517953880225822 "1330-1339 (förutom 1336-1337)"
      // 15344672548621634930 "1336-1337"
      // 8544230840928436188 "1340-1349 (förutom 1346-1347)"
      // 13850917336143100446 "1346-1347"
      // 4601341298284676885 "1350-1359"
      // 2725614420353494065 "1360-1369"
      // 2746984126913941224 "1380-1389"
      // 14247317458687115239 "1410-1429, 1430, 1431 eller 1438"
      // 3433350423138479992 "1432-1449 (förutom 1438)"
      // 2346049323362567707 "1450-1469"
      // 16122789554500341138 "1470-1479"
      // 4976455952353268623 "1480-1489"
      // 4802107533869083065 "1490-1499"
      // 2995199977553315731 "1500-1559 eller 1580-1589"
      // 15259969052810058218 "1560-1569 eller 1660-1669"
      // 8544230840928436188 "1570-1579 (förutom 1573) eller 1670-1679 (förutom 1673)"
      // 13850917336143100446 "1573 eller 1673"
      // 14712306452208124913 "1590-1619, 1630-1659 eller 1680-1689"
      // 8272007022483877119 "1620-1629"
      // 5683943810747285519 "1690-1699"
      // 13922947895874900402 "1700-1799"
      // 9749819421879779127 "1800-1899 (förutom 1860-1869)"
      // 6072736618874988835 "1860-1869"
      // 6559204722697948293 "1900-1989"
      // 315113652544154573 "1990-1999"
      // 7437289880448287687 "2081, 2083 eller 2084"
      // 3871176652399195503 "2082"
      // 14990993219723958545 "2085"
      // 17443004639063377112 "2086"
      // 3993155805231017139 "2087"
      // 16424774790432980181 "2090, 2091, 2093-2095 eller 2098"
      // 16360101710131019435 "2097"
      // 6306324468725266592 "2110-2149"
      // 1229599413901569056 "2150-2159"
      // 5217832914740960127 "2160-2199"
      // 16217660281104500162 "2210-2219"
      // 15081677615273795926 "2220-2229 eller 2250-2299"
      // 109226735215221737 "2230-2239"
      // 14215819343704291797 "2310-2329"
      // 8122810737165346528 "2330-2339"
      // 8642848485431095587 "2340-2359"
      // 18427459488302637597 "2360-2369"
      // 8703083173403804557 "2370-2379 (förutom 2373)"
      // 7947902124699524542 "2373"
      // 15255530962611194028 "2390-2399"
      // 8642848485431095587 "2410-2419"
      // 9376625536673363084 "2420-2429"
      // 16122789554500341138 "2430-2439"
      // 16403934149583209484 "2440-2449"
      // 13695742501450611223 "2450-2459"
      // 18427459488302637597 "2460-2469 eller 2860-2869"
      // 8703083173403804557 "2470-2479 (förutom 2473) eller 2870-2879 (förutom 2873)"
      // 7947902124699524542 "2473 eller 2873"
      // 8122810737165346528 "2480-2489"
      // 15255530962611194028 "2490-2499 (förutom 2492), 2600-2799, 2810-2859 eller 2880-2899"
      // 16891141477899543375 "2492"
      // 5841233692153537502 "2500-2599"
      // 7191926069038742969 "2900-2999"

      return result;
    }

  }

  using Tokens = std::vector<std::string>;
  Tokens tokenize(std::string const& bas_accounts_text) {
    Tokens result{};
    auto begin = bas_accounts_text.begin();
    auto current = begin;
    auto end = bas_accounts_text.end();
    std::string token{};
    while (current != end) {
      if (*current!=' ') {
        if (*current=='(' or *current==')' or *current==',' or *current=='-') {
          if (token.size()>0) result.push_back(token);
          token.clear();
          result.push_back(""); result.back() += *current;
        }
        else token += *current;
      }
      else {
        // split + consume white space
        if (token.size()>0) result.push_back(token);
        token.clear();
      }
      ++current;
    }
    if (token.size()>0) result.push_back(token);

    return result;
  }

	struct AREntry {

    class GoesIntoThisEntry {
    public:
      GoesIntoThisEntry(std::string const& bas_accounts_text)
        : m_bas_accounts_text{bas_accounts_text} {
        std::cout << "\nGoesIntoThisEntry(" << std::quoted(bas_accounts_text) << ")"; 
        // Parse AREntry::m_bas_accounts_text
        if (!m_parsed_ok) {
          // Parse single bas account no
          auto ban = BAS::to_account_no(bas_accounts_text);
          if (ban) {
            m_bas_account_ranges.push_back({*ban,*ban});
            std::cout << "\n\tPARSED OK = ** SINGLE **";
            m_parsed_ok = true;
          }
        }
        if (!m_parsed_ok) {
          // Parse text on the form "2900-2999"
          auto const& [first,second] = tokenize::split(bas_accounts_text,'-');
          auto ban1 = BAS::to_account_no(first);
          auto ban2 = BAS::to_account_no(second);
          if (ban1 and ban2) {
            m_bas_account_ranges.push_back({*ban1,*ban2});
            std::cout << "\n\tPARSED OK = ** RANGE **";
            m_parsed_ok = true;
          }
        }
        if (!m_parsed_ok) {
          // "4000-4799 eller 4910-4931"
          auto tokens = tokenize::splits(bas_accounts_text,' ');
          if (tokens.size()==3 and tokens[1] == "eller") {
            bool sub_parse_ok{true};
            if (sub_parse_ok) {
              // Parse token 0 on the form "2900-2999"
              auto const& [first,second] = tokenize::split(tokens[0],'-');
              auto ban1 = BAS::to_account_no(first);
              auto ban2 = BAS::to_account_no(second);
              sub_parse_ok = ban1 and ban2;
              if (sub_parse_ok) {
                m_bas_account_ranges.push_back({*ban1,*ban2});
                std::cout << "\n\tPARSED OK = ** RANGE **";
              }
            }
            if (sub_parse_ok) {
              // Parse token 2 on the form "2900-2999"
              auto const& [first,second] = tokenize::split(tokens[2],'-');
              auto ban1 = BAS::to_account_no(first);
              auto ban2 = BAS::to_account_no(second);
              sub_parse_ok = ban1 and ban2;
              if (sub_parse_ok) {
                m_bas_account_ranges.push_back({*ban1,*ban2});
                std::cout << "\n\tPARSED OK = ** RANGE **";
                sub_parse_ok = true;
              }

            }
            m_parsed_ok = sub_parse_ok;
          }
        }
        if (!m_parsed_ok) {
          // "4900-4999 (förutom 4910-4931, 4960-4969 och 4980-4989)"
          // "7700-7899 (förutom 7740-7749 och 7790-7799)"
          // "8000-8099 (förutom 8070-8089)"
          // "8070-8089, 8170-8189, 8270-8289 eller 8370-8389"
          // "8100-8199 (förutom 8113, 8118, 8123, 8133 och 8170-8189)"
          // "8113, 8118, 8123 eller 8133"
          // "8200-8299 (förutom 8270-8289)"
          // "1020-1059 eller 1080-1089 (förutom 1088)"
          // "1330-1339 (förutom 1336-1337)"
          // "1340-1349 (förutom 1346-1347)"
          // "1410-1429, 1430, 1431 eller 1438"
          // "1570-1579 (förutom 1573) eller 1670-1679 (förutom 1673)"
          // "1573 eller 1673"
          // "1590-1619, 1630-1659 eller 1680-1689"
          // "1800-1899 (förutom 1860-1869)"
          // "2081, 2083 eller 2084"
          // "2090, 2091, 2093-2095 eller 2098"
          // "2370-2379 (förutom 2373)"
          // "2470-2479 (förutom 2473) eller 2870-2879 (förutom 2873)"
          // "2473 eller 2873"
          
        }
        if (!m_parsed_ok) {
          std::cout << "\n\tFAILED TO PARSE";
        }
      }
      bool operator()(BAS::AccountNo bas_account_no) {
        bool result{false};
        std::cout << "\nGoesIntoThisEntry(" << std::quoted(this->m_bas_accounts_text) << ") on " << bas_account_no;
        if (m_parsed_ok) {
          result = m_parsed_ok and std::any_of(m_bas_account_ranges.begin(),m_bas_account_ranges.end(),[bas_account_no,this](auto const& r) {
            auto result = (r.first<=bas_account_no) and (bas_account_no<=r.second); 
            if (true) {
              // Log
              if (result) {
                std::cout << " ** MATCH ** ";
              }
              else {
                std::cout << " no match ";
              }
            }
            return result;
          });
        }
        else {
          std::cout << "\n - NOT PARSED OK (can't use for match)";
        }
        return result;
      }
      bool is_parsed_ok() const {return m_parsed_ok;}
    private:
      std::string m_bas_accounts_text;
      using BASAccountRanges = std::vector<std::pair<BAS::AccountNo,BAS::AccountNo>>;
      BASAccountRanges m_bas_account_ranges{};
      bool m_parsed_ok{false};
    }; // class GoesIntoThisEntry

		AREntry( std::string const& bas_accounts_text
						,std::string const& field_heading_text
						,std::optional<std::string> field_description = std::nullopt)
			:  m_bas_accounts_text{bas_accounts_text}
				,m_field_heading_text{field_heading_text}
				,m_field_description{field_description}
        ,m_goes_into_this_entry{bas_accounts_text} {}
		std::string m_bas_accounts_text;
		std::string m_field_heading_text;
		std::optional<std::string> m_field_description;
		bool accumulate_this_bas_account(BAS::AccountNo bas_account_no,CentsAmount amount) {
			auto result = m_goes_into_this_entry(bas_account_no);
			if (result) {
				m_bas_account_nos.insert(bas_account_no);
				m_amount += amount;
			}
			return result;
		}
		CentsAmount m_amount{};
		std::set<BAS::AccountNo> m_bas_account_nos{};
    GoesIntoThisEntry m_goes_into_this_entry;
	};
	using AREntries = std::vector<AREntry>;

	// A test function to parse the bas_2022_mapping_to_k2_ar_text into AREntries
	AREntries parse(char const* bas_2022_mapping_to_k2_ar_text) {
		AREntries result{};
		// Snippet from the text file to parse
		/*				
		R"(Resultaträkning
		Konto 3000-3799

		Fält: Nettoomsättning
		Beskrivning: Intäkter som genererats av företagets ordinarie verksamhet, t.ex. varuförsäljning och tjänsteintäkter.
		Konto 3800-3899

		Fält: Aktiverat arbete för egen räkning
		Beskrivning: Kostnader för eget arbete där resultatet av arbetet tas upp som en tillgång i balansräkningen.
		Konto 3900-3999

		Fält: Övriga rörelseintäkter
		Beskrivning: Intäkter genererade utanför företagets ordinarie verksamhet, t.ex. valutakursvinster eller realisationsvinster.
		Konto 4000-4799 eller 4910-4931
		*/

		// We are to parse:
		// 1) First a line beginning with "Konto" into a string with the text that remains
		//    E.g., "Konto 3000-3799" ==> OK "3000-3799"
		// 2) Then a line beginning with "Fält:" into a string with the text that remains
		//    E.g., "Fält: Nettoomsättning" ==> OK "Nettoomsättning"
		// 3) Then an optional line beginning with "Beskrivning:" into a string with the text that remains
		//    E.g., "Beskrivning: Intäkter som genererats av företagets ordinarie verksamhet, t.ex. varuförsäljning och tjänsteintäkter."
		//          ==> OK "Intäkter som genererats av företagets ordinarie verksamhet, t.ex. varuförsäljning och tjänsteintäkter."
		// While doing this we shall:
		// a) Skip any empty lines
		// b) Transform remaining text in (1) into a list of BAS accounts
		// c) Transform remaining text in (2) into a ARField = a Pair <ARFieldId,String> (a pair of an enumeration of the field with the field heading)
		// d) Transform remaining text in optional (3) into a text <description>

		struct CashedEntry {
			std::optional<std::string> bas_accounts{};
			std::optional<std::string> field_heading{};
			std::optional<std::string> field_description{};
			std::string to_string() const {
				std::ostringstream out{};
				out << "\n\tbas_accounts:";
				if (bas_accounts) out << std::quoted(*bas_accounts);
				else out << "null";
				out << "\n\tfield_heading:";
				if (field_heading) out << std::quoted(*field_heading);
				else out << "null";
				out << "\n\tfield_description:";
				if (field_description) out << std::quoted(*field_description);
				else out << "null";
				return out.str();
			}

		};

		CashedEntry cached_entry{};

		// Parse using plain c++ stream
		std::istringstream in{bas_2022_mapping_to_k2_ar_text};
		std::string line{};
		while (std::getline(in,line)) {
			if (tokenize::starts_with("Konto",line)) {
				// std::cout << "\n=====================";
				// std::cout << "\n\t" << std::quoted(line);
				{
					// Check the cache
					std::cout << cached_entry.to_string();
					if (cached_entry.bas_accounts and cached_entry.field_heading) {
						std::cout << " ==> OK!";
						result.push_back(AREntry{*cached_entry.bas_accounts,*cached_entry.field_heading,cached_entry.field_description});
					}
					else {
						std::cout << "// Skipped ";
					}
				}
				cached_entry = CashedEntry{}; // new entry / reset
				cached_entry.bas_accounts = tokenize::without_front_word(line);
			}
			else if (tokenize::starts_with("Fält:",line)) {
				// std::cout << "\n\t" << std::quoted(line);
				cached_entry.field_heading = tokenize::without_front_word(line);
			}
			else if (tokenize::starts_with("Beskrivning:",line)) {
				// std::cout << "\n\t" << std::quoted(line);
				cached_entry.field_description = tokenize::without_front_word(line);
			}
			else {
				std::cout << "\n// " << std::quoted(line);
			}
		}
		// Ensure we capture also the last one (NOT triggered by a new lint with "Konto" in it) ;)
		if (cached_entry.bas_accounts and cached_entry.field_heading) {
			std::cout << " ==> OK!";
			result.push_back(AREntry{*cached_entry.bas_accounts,*cached_entry.field_heading,cached_entry.field_description});
		}
		else {
			std::cout << "// Skipped ";
		}
    if (true) {
			for (auto const& entry : result) {
        auto predicate = ar_online::to_predicate(entry.m_field_heading_text,entry.m_bas_accounts_text);
      }
    }
		if (true) {
			std::cout << "\nParsed Entries {";
			std::cout << "\n  From listing at URL https://www.arsredovisning-online.se/bas_kontoplan as of 221118";
			int index{};
      bool all_are_parsed_ok{true};
			for (auto const& entry : result) {
        all_are_parsed_ok = all_are_parsed_ok and entry.m_goes_into_this_entry.is_parsed_ok();
				std::cout << "\n  [" << index++ << "]"; 
				std::cout << " m_bas_accounts_text:" << std::quoted(entry.m_bas_accounts_text);
				std::cout << "\n        m_field_heading_text:" << std::quoted(entry.m_field_heading_text);
				std::cout << "\n        m_field_description:";
				if (entry.m_field_description) std::cout << std::quoted(*entry.m_field_description);
				else std::cout << " - ";
        if (!entry.m_goes_into_this_entry.is_parsed_ok()) {
  				std::cout << "\n        ** NOT PARSED OK **";
        }
			}
			std::cout << "\n} // Parsed Entries";
		}

	#if (__cpp_lib_ranges >= 201911L) // clang libstdc++ (experimental in clang15) does not support std::ranges::istream_view
		// Check that we parsed all entries correct
		std::istringstream words{bas_2022_mapping_to_k2_ar_text};
		auto count = std::ranges::count_if(std::ranges::istream_view<std::string>(words), [](std::string const& word) {return word == "Konto";});
		std::cout << "\nCount of 'Konto' in source text:" << count << " and parsed entry count is:" << result.size();
		if (count == result.size()) std::cout << " ==> OK!";
		else std::cout << " ** ERROR (must be equal = all must be parsed) **";
#else
		// Warn that we have no code to check the input / parsed result correctness
		std::cout << "\nWARNING: I Failed to check parse result of input bas_2022_mapping_to_k2_ar_text (Because this code is compiled with a compiler that does not support std::ranges)";
		std::cout << "\nWARNING: If I have failed to parse bas_2022_mapping_to_k2_ar_text, I may generate an incorrect Annual Financial Statement (Swedish Årsredovisning)";
		std::cout << "\nNOTE: I parse bas_2022_mapping_to_k2_ar_text to create a mapping between BAS accounts and fields on the Annual Financial Statement (Swedish Årsredovisning)";
#endif
		return result;
	}
}

namespace BAS {
	extern char const* bas_2022_account_plan_csv;
}
namespace SKV::XML {
	using XMLMap = std::map<std::string,std::string>;
}
namespace SKV::XML::TAXReturns {
	extern SKV::XML::XMLMap tax_returns_template; // See bottom of this file
}
namespace SKV::XML::VATReturns {
	extern char const* ACCOUNT_VAT_CSV; // See bottom of this source file
}

template <typename I>
std::vector<std::pair<I,I>> to_ranges(std::vector<I> line_nos) {
	std::vector<std::pair<I,I>> result{};
	if (line_nos.size()>0) {
		I begin{line_nos[0]}; 
		I previous{begin};
		for (auto line_ix : line_nos) {
			if (line_ix > previous+1) {
				// Broken sequence - push previous one
				result.push_back({begin,previous});
				begin = line_ix;
			}
			previous = line_ix;
		}
		if (previous > begin) result.push_back({begin,previous});
	}
	return result;
}

template <typename I>
std::ostream& operator<<(std::ostream& os,std::vector<std::pair<I,I>> const& rr) {
	for (auto const& r : rr) {
		if (r.first == r.second) os << " " << r.first;
		else os << " [" << r.first << ".." << r.second << "]";
	}
	return os;
}

namespace doc {

	namespace meta {
		struct Orientation {};
		struct Location {};
		struct Colour {};
		struct Font {};
		struct Width {};
		struct Height {};
		struct Count {};
	}
	using Meta = std::variant<meta::Orientation,meta::Location,meta::Colour,meta::Font,meta::Width,meta::Height,meta::Count>;
	using MetaPtr = std::shared_ptr<Meta>;

	namespace leaf {
		struct PageBreak {};
		struct Text {std::string s;};
	}

	using Leaf = std::variant<leaf::PageBreak,leaf::Text>;
	using LeafPtr = std::shared_ptr<Leaf>;

	struct Component; // Forward
	using ComponentPtr = std::shared_ptr<Component>;
	namespace composite {
		struct Base {
			ComponentPtr parent;
			std::vector<ComponentPtr> childs;
		};
		struct Vector : public Base {}; // A vector of components
		struct Grid : public Base {}; // A Grid x,y of components
		struct SeparatePage : public Base {}; // Ensure this component is on its own page
	}
	using Composite = std::variant<composite::Vector,composite::Grid,composite::SeparatePage>;
	using CompositePtr = std::shared_ptr<Composite>;

	using Defacto = std::variant<LeafPtr,CompositePtr>;
	using DefactoPtr = std::shared_ptr<Defacto>;

	struct Component {
		MetaPtr pMeta;
		DefactoPtr pDefacto;
		friend Component& operator<<(Component& c,ComponentPtr const& p);
	};
	Component& operator<<(Component& c,ComponentPtr const& p) {
		return c;
	}

	ComponentPtr plain_text(std::string const& s) {
		return std::make_shared<Component>(Component {
			.pDefacto = std::make_shared<Defacto>(std::make_shared<Leaf>(leaf::Text{s}))
		});
	}

	ComponentPtr separate_page() {
		return std::make_shared<Component>(Component {
			.pDefacto = std::make_shared<Defacto>(std::make_shared<Composite>(composite::SeparatePage{}))
		});
	}

	class Document {
	public:
		friend Document& operator<<(Document& doc,ComponentPtr const& p);
	private:
		ComponentPtr root{separate_page()};
	};

	Document& operator<<(Document& doc,ComponentPtr const& p) {
		return doc;
	}

}

namespace RTF {
	// Rich Text Format namespace

	struct MetaState {
		void operator()(doc::meta::Orientation const& meta) {}
		void operator()(doc::meta::Location const& meta) {}
		void operator()(doc::meta::Colour const& meta) {}
		void operator()(doc::meta::Font const& meta) {}
		void operator()(doc::meta::Width const& meta) {}
		void operator()(doc::meta::Height const& meta) {}
		void operator()(doc::meta::Count const& meta) {}
	};

	struct OStream {
		std::ostream& os;
		MetaState meta_state{};
	};

	OStream& operator<<(OStream& os,doc::ComponentPtr const& cp); // Forward / future h-file
	OStream& operator<<(OStream& os,doc::Document const& doc); // Forward / future h-file

	struct LeafOStreamer {
		OStream& os;
		void operator()(doc::leaf::PageBreak const& leaf) {
			os.os << "\nTODO: PAGE BREAK";
		}
		void operator()(doc::leaf::Text const& leaf) {
			os.os << " text:" << leaf.s << ":text ";
		}
	};

	struct CompositeOStreamer {
		OStream& os;
		void operator()(doc::composite::Vector const& v) {
			os.os << "\nTODO: CompositeOStreamer Vector";
		}
		void operator()(doc::composite::Grid const& g) {
			os.os << "\nTODO: CompositeOStreamer Grid";
		}
		void operator()(doc::composite::SeparatePage const& g) {
			os.os << "\nTODO: CompositeOStreamer SeparatePage";
		}
	};

	struct DefactoStreamer {
		OStream& os;
		void operator()(doc::LeafPtr const& p) {
			if (p) {
				LeafOStreamer streamer{os};
				std::visit(streamer,*p);
			}
		}
		void operator()(doc::CompositePtr const& p) {
			if (p) {
				CompositeOStreamer ostreamer{os};
				std::visit(ostreamer,*p);
			}
		}
	};

	OStream& operator<<(OStream& os,doc::ComponentPtr const& p) {
		if (p) {
			if (p->pMeta) std::visit(os.meta_state,*p->pMeta);
			DefactoStreamer ostreamer{os};
			if (p->pDefacto) std::visit(ostreamer,*p->pDefacto);
		}
		return os;
	}

	OStream& operator<<(OStream& os,doc::Document const& doc) {
		os.os << "\nTODO: stream doc::Document";
		return os;
	}


} // namespace RTF

namespace HTML {
	// Rich Text Format namespace

	struct MetaState {
		void operator()(doc::meta::Orientation const& meta) {}
		void operator()(doc::meta::Location const& meta) {}
		void operator()(doc::meta::Colour const& meta) {}
		void operator()(doc::meta::Font const& meta) {}
		void operator()(doc::meta::Width const& meta) {}
		void operator()(doc::meta::Height const& meta) {}
		void operator()(doc::meta::Count const& meta) {}
	};

	struct OStream {
		std::ostream& os;
		MetaState meta_state{};
	};

	OStream& operator<<(OStream& os,doc::ComponentPtr const& cp); // Forward / future h-file
	OStream& operator<<(OStream& os,doc::Document const& doc); // Forward / future h-file

	OStream& operator<<(OStream& os,doc::ComponentPtr const& cp) {
		os.os << "\nTODO: stream doc::ComponentPtr";
		return os;
	}

	OStream& operator<<(OStream& os,doc::Document const& doc) {
		os.os << "\nTODO: stream doc::Document";
		return os;
	}

} // namespace HTML

namespace CSV {
	using FieldRow = Key::Path;
	using FieldRows = std::vector<FieldRow>;
	using OptionalFieldRows = std::optional<FieldRows>;

  // 20240528 - Now requires a known encoing of the input stream characters
	// OptionalFieldRows to_field_rows(std::istream& in,char delim=';') {
  //   if (true) {
  //     std::cout << "\nto_field_rows(std::istream& in...";
  //   }
	// 	OptionalFieldRows result{};
	// 	try {
	// 		FieldRows field_rows{};
	// 		std::string entry{};
	// 		while (std::getline(in,entry)) {
	// 			field_rows.push_back({entry,delim});
	// 		}
	// 		result = field_rows;
	// 	}
	// 	catch (std::exception const& e) {
	// 		std::cout << "\nDESIGN INSUFFICIENCY: to_field_rows failed. Exception=" << std::quoted(e.what());
	// 	}
	// 	return result;
	// }

	OptionalFieldRows to_field_rows(auto& in,char delim=';') {
    if (false) {
      std::cout << "\nto_field_rows(auto& in...";
    }
		OptionalFieldRows result{};
		try {
			FieldRows field_rows{};
			std::string raw_entry{};
			// while (std::getline(in.is,raw_entry)) {
      //   // patch from assumed ISO8859_1 to assumed run time environment UTF-8
      //   auto unicode_string = charset::ISO_8859_1::iso8859ToUnicode(raw_entry);
      //   auto entry = encoding::UTF8::unicode_to_utf8(unicode_string);
			// 	field_rows.push_back({entry,delim});
			// }
      while (auto entry = in.getline(encoding::unicode::to_utf8{})) {
				field_rows.push_back({*entry,delim});
      }
			result = field_rows;
		}
		catch (std::exception const& e) {
			std::cout << "\nDESIGN INSUFFICIENCY: to_field_rows failed. Exception=" << std::quoted(e.what());
		}
		return result;
	}

  using TableHeading = FieldRow;
  using OptionalTableHeading = std::optional<TableHeading>;

  // Helper to identify a valid function to_heading(row_0) : FieldRow -> std::optional<TableHeading> 
  template<typename ToHeading, typename FieldRow>
  concept ToHeadingConcept = requires(ToHeading to_heading, FieldRow field_row) {
      { to_heading(field_row) } -> std::convertible_to<std::optional<TableHeading>>;
  };  

  struct Table {
    using Heading = FieldRow;
    using Row = FieldRow;
    using Rows = std::vector<Row>;
    Heading heading;
    Rows rows;
  };
  using OptionalTable = std::optional<Table>;

  OptionalTable to_table(OptionalFieldRows const& field_rows,ToHeadingConcept<FieldRow> auto to_heading) {
    OptionalTable result{};
    if (field_rows and field_rows->size()>0) {
      Table table{};
      if (auto heading = to_heading(field_rows->at(0))) {
        table.heading = *heading;
        table.rows = *field_rows; // Keep all rows inclduing heading row if there is one (TODO: Remove when mechanism to differ between rows with and without a heading row 0 implemented)
        result = table;
      }
    }
    return result;
  }

} // namespace CSV

auto utf_ignore_to_upper_f = [](char ch) {
	if (ch <= 0x7F) return static_cast<char>(std::toupper(ch));
	return ch;
};

std::string utf_ignore_to_upper(std::string const& s) {
	std::string result{};
	std::transform(s.begin(),s.end(),std::back_inserter(result),utf_ignore_to_upper_f);
	return result;
}

std::vector<std::string> utf_ignore_to_upper(std::vector<std::string> const& tokens) {
	std::vector<std::string> result{};
	auto f = [](std::string s) {return utf_ignore_to_upper(s);};
	std::transform(tokens.begin(),tokens.end(),std::back_inserter(result),f);
	return result;
}

bool strings_share_tokens(std::string const& s1,std::string const& s2) {
	bool result{false};
	auto s1_words = utf_ignore_to_upper(tokenize::splits(s1));
	auto s2_words = utf_ignore_to_upper(tokenize::splits(s2));
	for (int i=0; (i < s1_words.size()) and !result;++i) {
		for (int j=0; (j < s2_words.size()) and !result;++j) {
			result = (s1_words[i] == s2_words[j]);
		}
	}
	return result;
}

bool first_in_second_case_insensitive(std::string const& s1, std::string const& s2) {
	auto upper_s1 = utf_ignore_to_upper(s1);
	auto upper_s2 = utf_ignore_to_upper(s2);
	return (upper_s2.find(upper_s1) != std::string::npos);
}

std::optional<unsigned int> to_four_digit_positive_int(std::string const& s) {
	std::optional<unsigned int> result{};
	try {
		if (s.size()==4) {
			if (std::all_of(s.begin(),s.end(),::isdigit)) {
				auto account_no = std::stoi(s);
				if (account_no >= 1000) result = account_no;
			}
		}
	}
	catch (std::exception const& e) { std::cout << "\nDESIGN INSUFFICIENCY: to_four_digit_positive_int(" << s << ") failed. Exception=" << std::quoted(e.what());}
	return result;
}

namespace SKV {
	namespace SRU {

		using AccountNo = unsigned int;
		using OptionalAccountNo = std::optional<AccountNo>;

		OptionalAccountNo to_account_no(std::string const& s) {
			return to_four_digit_positive_int(s);
		}

		using SRUValueMap = std::map<AccountNo,std::optional<std::string>>;
		using OptionalSRUValueMap = std::optional<SRUValueMap>;
		using SRUValueMaps = std::vector<SRUValueMap>;

	} // namespace SRU
}

namespace CSV {
	// For CSV-files
	namespace NORDEA {
		// For NORDEA CSV-files (bank account statements)

		// Assume Finland located bank Nordea swedish web csv format of transactions to/from an account
		/*
		Bokföringsdag;Belopp;Avsändare;Mottagare;Namn;Rubrik;Meddelande;Egna anteckningar;Saldo;Valuta
		2022/06/27;-720,00;51 86 87-9;5343-2795;LOOPIA AB (WEBSUPPORT);LOOPIA AB (WEBSUPPORT);378587992;Webhotell Q2;49537,22;SEK
		2022/06/16;-880,00;51 86 87-9;824-3040;TELIA SVERIGE AB;TELIA SVERIGE AB;19990271223;Mobil Q2;50257,22;SEK
		2022/06/13;-625,00;51 86 87-9;5050-1055;SKATTEVERKET;SKATTEVERKET;16556782817244;Förs Avg;51137,22;SEK
		2022/06/13;-153,71;51 86 87-9;;KORT             BEANSTALK APP   26;KORT             BEANSTALK APP   26;BEANSTALK APP   2656;;;SEK
		2022/06/10;-184,00;51 86 87-9;;KORT             POSTNORD SE     26;KORT             POSTNORD SE     26;POSTNORD SE     2656;;51915,93;SEK
		2022/06/09;-399,90;51 86 87-9;;KORT             KJELL O CO 100  26;KORT             KJELL O CO 100  26;KJELL O CO 100  2656;;52099,93;SEK
		2022/06/03;-1,70;51 86 87-9;;PRIS ENL SPEC;PRIS ENL SPEC;;;52499,83;SEK
		2022/05/31;-446,00;51 86 87-9;5020-7042;Fortnox Finans AB;Fortnox Finans AB;52804292974641;Bokf pr jun/jul/aug;52501,53;SEK
		2022/05/24;786,99;;51 86 87-9;BG KONTOINS;BG KONTOINS;0817-9780 GOOGLE IRELA;;52947,53;SEK
		2022/05/19;179,00;;51 86 87-9;BG KONTOINS;BG KONTOINS;5050-1030 SK5567828172;;52160,54;SEK
		2022/05/18;-610,33;51 86 87-9;;KORT             PAYPAL *HKSITES 26;KORT             PAYPAL *HKSITES 26;PAYPAL *HKSITES 2656;;51981,54;SEK
		2022/05/12;-154,91;51 86 87-9;;KORT             BEANSTALK APP   26;KORT             BEANSTALK APP   26;BEANSTALK APP   2656;;52591,87;SEK
		2022/05/04;-1,70;51 86 87-9;;PRIS ENL SPEC;PRIS ENL SPEC;;;52746,78;SEK
		2022/04/19;-186,25;51 86 87-9;5343-2795;LOOPIA AB (WEBSUPPORT);LOOPIA AB (WEBSUPPORT);375508199;sharedplanet se;52748,48;SEK
		2022/04/12;-145,41;51 86 87-9;;KORT             BEANSTALK APP   26;KORT             BEANSTALK APP   26;BEANSTALK APP   2656;;52934,73;SEK
		2022/04/04;-6,80;51 86 87-9;;PRIS ENL SPEC;PRIS ENL SPEC;;;53080,14;SEK
		*/

		enum element: std::size_t {
			undefined
			,Bokforingsdag = 0
			,Belopp = 1
			,Avsandare = 2
			,Mottagare = 3
			,Namn = 4
			,Rubrik = 5
			,Meddelande = 6
			,Egna_anteckningar = 7
			,Saldo = 8
			,Valuta = 9
			,unknown
		};

		OptionalTaggedAmount to_tagged_amount(FieldRow const& field_row,Table::Heading const& heading = Table::Heading{}) {
			OptionalTaggedAmount result{};
			if (field_row.size() >= 10) {
				auto sDate = field_row[element::Bokforingsdag];
				if (auto date = to_date(sDate)) {
					auto sAmount = field_row[element::Belopp];
					if (auto amount = to_amount(sAmount)) {
						auto cents_amount = to_cents_amount(*amount);
						TaggedAmount ta{*date,cents_amount};
						ta.tags()["Account"] = "NORDEA";
						if (field_row[element::Namn].size() > 0) {
							ta.tags()["Text"] = field_row[element::Namn];
						}
						if (field_row[element::Avsandare].size() > 0) {
							ta.tags()["From"] = field_row[element::Avsandare];
						}
						if (field_row[element::Mottagare].size() > 0) {
							ta.tags()["To"] = field_row[element::Mottagare];
						}
						if (auto saldo = to_amount(field_row[element::Saldo])) {
							ta.tags()["Saldo"] = to_string(to_cents_amount(*saldo));
						}
						if (field_row[element::Meddelande].size() > 0) {
							ta.tags()["Message"] = field_row[element::Meddelande];
						}
						if (field_row[element::Egna_anteckningar].size() > 0) {
							ta.tags()["Notes"] = field_row[element::Egna_anteckningar];
						}
            // Tag with column names as defiend by heading
            if (heading.size() > 0 and heading.size() == field_row.size()) {
              for (int i=0;i<heading.size();++i) {
                auto key = heading[i];
                auto value = field_row[i];
                if (ta.tags().contains(key) == false) {
                  if (value.size() > 0) ta.tags()[key] = field_row[i]; // add column value tagged with column name
                }
                else {
                  std::cout << "\nWarning, to_tagged_amount - Skipped conflicting column name:" << std::quoted(key) << ". Already tagged with value:" << std::quoted(ta.tags()[key]);
                }
              }
            }
            else {
              std::cout << "\nError, to_tagged_amount failed to tag amount with heading defined tags and values. No secure mapping from heading column count:" << heading.size() << " to entry column count:" << field_row.size();
            }

						result = ta;
					}
					else {
						std::cout << "\nNot a valid NORDEA amount: " << std::quoted(sAmount); 
					}
				}
				else {
					if (sDate.size() > 0) std::cout << "\nNot a valid date: " << std::quoted(sDate);
				}
			}
			return result;
		} 
	} // namespace NORDEA 

	namespace SKV {
		// For SKV CSV-files (so called skv-files with tax account statements)

		/*
			;Ing�ende saldo 2022-06-05;625;0;
			2022-06-13;F�rs.avgift moms/arbetsgivardeklaration 220314;-625;;
			2022-06-20;Inbetalning bokf�rd 220613;625;;
			;Utg�ende saldo 2022-07-02;625;0;
		*/

		enum element: std::size_t {
			undefined
			,OptionalDate=0
			,Text=1
			,Belopp=2
			,Saldo=2
			,OptionalZero=3
			,unknown
		};

		OptionalTaggedAmount to_tagged_amount(FieldRow const& field_row,Table::Heading const& heading = Table::Heading{}) {
			OptionalTaggedAmount result{};
			if (field_row.size() == 5) {
				// NOTE: The SKV comma separated file is in fact a end-with-';' field file (so we get five separations where the file only contains four fields...)
				//       I.e., field index 0..3 contains values
        // NOTE! skv-files seems to be ISO_8859_1 encoded! (E.g., 'å' is ASCII 229 etc...)
        // Assume the client calling this function has already transcoded the text into internal character set and encoding (on macOS UNICODE in UTF-8)
				auto sDate = field_row[element::OptionalDate];
				if (auto date = to_date(sDate)) {
					auto sAmount = field_row[element::Belopp];
					if (auto amount = to_amount(sAmount)) {
						auto cents_amount = to_cents_amount(*amount);
						TaggedAmount ta{*date,cents_amount};
						ta.tags()["Account"] = "SKV";
						ta.tags()["Text"] = field_row[element::Text];

            // Tag with column names as defiend by heading
            if (heading.size() > 0 and heading.size() == field_row.size()) {
              for (int i=0;i<heading.size();++i) {
                auto key = heading[i];
                if (ta.tags().contains(key) == false) {
                  // Note: The SKV file may contain both CR (0x0D) and LF (0x0A) even when downloaded to macOS that expects only CR.
                  // We cann trim on the value to get rid of any residue LF
                  // TODO: Make reading of text from file robust against control characters that does not match the expectation of the runtime
                  auto value = tokenize::trim(field_row[i]);
                  if (value.size() > 0) ta.tags()[key] = field_row[i]; // add column value tagged with column name
                }
                else {
                  std::cout << "\nWarning, to_tagged_amount - Skipped conflicting column name:" << std::quoted(key) << ". Already tagged with value:" << std::quoted(ta.tags()[key]);
                }
              }
            }
            else {
              std::cout << "\nError, to_tagged_amount failed to tag amount with heading defined tags and values. No secure mapping from heading column count:" << heading.size() << " to entry column count:" << field_row.size();
            }

						result = ta;
					}
					else {
						std::cout << "\nNot a valid amount: " << std::quoted(sAmount); 
					}
				}
				else {
					// It may be a saldo entry
					/*
							skv-file entry	";Ing�ende saldo 2022-06-05;625;0;"
							field_row				""  "Ing�ende saldo 2022-06-05" "625" "0" ""
							index:           0                           1     2   3   4
					*/
					auto sOptionalZero = field_row[element::OptionalZero]; // index 3
					if (auto zero_amount = to_amount(sOptionalZero)) {
						// No date and the optional zero is present ==> Assume a Saldo entry
						// Pick the date from Text
						auto words = tokenize::splits(field_row[element::Text],' ');
						if (auto saldo_date = to_date(words.back())) {
							// Success
							auto sSaldo = field_row[element::Saldo];
							if (auto saldo = to_amount(sSaldo)) {
								auto cents_saldo = to_cents_amount(*saldo);
								TaggedAmount ta{*saldo_date,cents_saldo};
								ta.tags()["Account"] = "SKV";
								ta.tags()["Text"] = field_row[element::Text];
								ta.tags()["type"] = "saldo";
								// NOTE! skv-files seems to be ISO_8859_1 encoded! (E.g., 'å' is ASCII 229 etc...)
								// TODO: Re-enocode into UTF-8 if/when we add parsing of text into tagged amount (See namespace charset::ISO_8859_1 ...)
								result = ta;
							}
							else {
								std::cout << "\nNot a valid SKV Saldo: " << std::quoted(sSaldo); 
							}
						}
						else {
								std::cout << "\nNot a valid SKV Saldo Date in entry: " << std::quoted(field_row[element::Text]); 
						}
					}
					if (sDate.size() > 0) std::cout << "\nNot a valid SKV date: " << std::quoted(sDate);
				}
			}
			return result;
		}
	} // namespace SKV
  enum class HeadingId {
    Undefined
    ,NORDEA
    ,SKV
    ,unknown
  };

  HeadingId to_csv_heading_id(FieldRow const& field_row) {
    HeadingId result{HeadingId::Undefined};
    if (true) {
      std::cout << "\nfield_row.size() = " << field_row.size();
      std::cout << "\nto_csv_heading_id(field_row:" << std::quoted(to_string(field_row)) << ")";
      for (auto const& field : field_row) std::cout << "\n\t[" << "]" << std::quoted(field);
    }
    if (field_row.size() >= 10) {
      if (true) {
        std::cout << "\nNORDEA File candidate ok";
      }
      // Bokföringsdag;Belopp;Avsändare;Mottagare;Namn;Rubrik;Meddelande;Egna anteckningar;Saldo;Valuta
      // Bokföringsdag;Belopp;Avsändare;Mottagare;Namn;Ytterligare detaljer;Meddelande;Egna anteckningar;Saldo;Valuta;
      // Note: NORDEA web csv format has changed to incorporate and ending ';' (in effect changing ';' from being a separator to being a terminator)
      // TODO 240221: Find a way to handle NORDEA file coming in UTF-8 with a BOM prefix and the SKV-file being ISO-8859-1 encoded (a mess!)
      if (
          true
          and field_row[0].find(R"(Bokföringsdag)") != std::string::npos // avoid mathing to the UTF-8 BOM prefix in a NORDEA file
          and field_row[1] == "Belopp" 
          and field_row[2] == "Avsändare"
          and field_row[3] == "Mottagare" 
          and field_row[4] == "Namn" 
          and (field_row[5] == "Rubrik" or field_row[5] == "Ytterligare detaljer")
          and field_row[6] == "Meddelande" 
          and field_row[7] == "Egna anteckningar" 
          and field_row[8] == "Saldo" 
          and field_row[9] == "Valuta") {
        result = HeadingId::NORDEA;
      }
    }
    else if (field_row.size() == 5) {
      if (true) {
        std::cout << "\nSKV File candidate ok";
      }
      result = HeadingId::SKV;
    }
    return result;
  }
} // namespace CSV


using OptionalDateOrderedTaggedAmounts = std::optional<DateOrderedTaggedAmountsContainer>;
				
/**
 * Return a list of tagged amounts if provided path is to a file with amount values (e.g., a bank account csv statements file)
 */
OptionalDateOrderedTaggedAmounts to_tagged_amounts(std::filesystem::path const& path) {
  if (false) {
    std::cout << "\nto_tagged_amounts(" << path << ")";
  }
  OptionalDateOrderedTaggedAmounts result{};
  CSV::OptionalFieldRows field_rows{};
  std::ifstream ifs{path};
  // NOTE: The mechanism implemented to apply correct decoding and parsing of different files is a mess!
  //       For one, The runtime on Mac uses UTF-8 through the console by default (which Windows and Unix may or may not do).
  //       Also, The NORDEA CSV-file as downloaded through Safary from NORDEA web bank is also UTF-8 encoded (although I am not sure it will be using another browser on another platform?).
  //       Finally, The SKV-file from Swedish Tax Agency web interface gets encoded in ISO8859-1 on Mac using Safari.
  //       For now the whole thing is a patch-work that may or may not continue to work on Mac and will very unlikelly work on Linux or Windows?
  //       TODO 20240527 - Try to refactor this into something more stable and cross-platform at some point in time?!
  if (path.extension() == ".csv") {
    encoding::UTF8::istream utf8_in{ifs};
    field_rows = CSV::to_field_rows(utf8_in,';'); // Call to_field_rows overload for "UTF8" input (assuming a CSV-file is ISO8859-1 encoded, as NORDEA csv-file is)
  }
  else if (path.extension() == ".skv") {
    encoding::ISO_8859_1::istream iso8859_in{ifs};
    field_rows = CSV::to_field_rows(iso8859_in,';'); // Call to_field_rows overload for "ISO8859-1" input (assuming a SKV-file is ISO8859-1 encoded)
  }
	DateOrderedTaggedAmountsContainer dota{};
	if (field_rows) {
		// The file is some form of 'comma separated value' file using ';' as separators
		// NOTE: Both Nordea csv-files (with bank account transaction statements) and Swedish Tax Agency skv-files (with tax account transactions statements)
		// uses ';' as value separators
		if (field_rows->size() > 0) {
      auto csv_heading_id = CSV::to_csv_heading_id(field_rows->at(0));
      switch (csv_heading_id) {
        case CSV::HeadingId::NORDEA: {
          auto to_heading = [](CSV::FieldRow const& field_row) -> CSV::OptionalTableHeading {
            if (field_row.size() > 3) {
              // Note: Requiring at least 3 'columns' is a heuristic to somewhat ensure we have at least a date, an amount and some description to work with
              // E.g. NORDEA bank CSV-file row 0: "Bokföringsdag;Belopp;Avsändare;Mottagare;Namn;Rubrik;Meddelande;Egna anteckningar;Saldo;Valuta"
              return field_row; // Assume csv-file has row 0 as the one naming the columns (as of 2024-06-09 NORDEA web bank csv-file does)
              // NOTE: This approach makes Cratchit dependent on the naming chosen by Nordea in its web bank generated CSV-file...
            }
            else {
              std::cout << "\nDESIGN_INSUFFICIENCY: Failed to use provied field_row " << field_row << " to return a table heading. Insufficient field_row.size()=" << field_row.size();
              return std::nullopt;
            }
          };
          if (auto table = CSV::to_table(field_rows,to_heading)) {
            for (auto const& field_row : table->rows) {
              if (auto o_ta = CSV::NORDEA::to_tagged_amount(field_row,table->heading)) {
                dota.insert(*o_ta);
              }
              else {
                std::cout << "\nSorry, Failed to create tagged amount from field_row " << std::quoted(to_string(field_row));
              }
            }            
          }
          else {
            std::cout << "\nDESIGN_INSUFFICIENCY: Failed to turn " << path << " to a CVS::Table with known heading and data content";
          }
        } break;
        case CSV::HeadingId::SKV: {
          auto to_heading = [](CSV::FieldRow const& field_row) -> CSV::OptionalTableHeading {
            // Assume this is row 0 of an SKV-file which in turn we assume is from SKV, the swedish tax agency, with transactions on 'our' tax account
            // This mean we have no column names in the file and need to hard code them.
            if (field_row.size() == 5) {
              return CSV::TableHeading{{"Bokföringsdag","Rubrik","Belopp","Kolumn_4","Kolumn_5"}};
            }
            else {
              return std::nullopt;
            }
          };
          if (auto table = CSV::to_table(field_rows,to_heading)) {
            for (auto const& field_row : table->rows) {
              if (auto o_ta = CSV::SKV::to_tagged_amount(field_row,table->heading)) {
                dota.insert(*o_ta);
              }
              else {
                std::cout << "\nSorry, Failed to create tagged amount from field_row " << field_row;
              }
            }
          }
          else {
            std::cout << "\nDESIGN_INSUFFICIENCY: Failed to turn " << path << " to a CVS::Table with known heading and data content";
          }
        } break;
        default: {
          // Skip this file (not a known count of values per row)
          std::cout << "\n*Skipped file* " << path << " with csv_heading_id = " << static_cast<int>(csv_heading_id) << ". ERROR, unknown file content)";
        }
      }
		}
	}
	if (dota.size() > 0) result = dota;

  return result;
}

unsigned first_digit(BAS::AccountNo account_no) {
	return account_no / 1000;
}


template <typename Meta,typename Defacto>
class MetaDefacto {
public:
	Meta meta;
	Defacto defacto;
private:
};

namespace BAS {

	// See https://bolagsverket.se/en/foretag/aktiebolag/arsredovisningforaktiebolag/delarochbilagoriarsredovisningen/faststallelseintyg.763.html
	// financial statements approval (fastställelseintyg)
	// See https://bolagsverket.se/en/foretag/aktiebolag/arsredovisningforaktiebolag/delarochbilagoriarsredovisningen.761.html
	// a directors’ report  (förvaltningsberättelse)
	// a profit and loss statement (resultaträkning)
	// a balance sheet (balansräkning)
	// notes (noter).	

	namespace detail {
		// "hidden" poor mans singleton instance creation
		Key::Paths bas_2022_account_plan_paths{};
		Key::Paths to_bas_2022_account_plan_paths() {
			Key::Paths result;
			std::istringstream in{bas_2022_account_plan_csv};
			// TODO: Parse in and assemble a path with nodes:
			// One node per digit in account number?
			// Or each node an optional<string> to handle digit position that is not a "grouping" level?
			// first digit grouping: Always
			// Second digit grouping: sometimes
			// Third digit grouping: Always?
			// ...
			return result;
		}
	}
	Key::Paths const& to_bas_2022_account_plan_paths() {
		// Poor mans "singleton" instance
		static auto const& result = detail::to_bas_2022_account_plan_paths();
		return result;
	}

	void parse_bas_account_plan_csv(std::istream& in,std::ostream& prompt) {
		std::string entry{};
		int line_ix{0};
		std::map<int,int> count_distribution{};
		using field_ix = int;
		using line_no = int;
		std::map<field_ix,std::map<std::string,std::vector<line_no>>> fields_map{};
		while (std::getline(in,entry)) {
			prompt << "\n" << line_ix << entry;
			auto fields = tokenize::splits(entry,';',tokenize::eAllowEmptyTokens::YES);
			int field_ix{0};
			prompt << "\n\tcount:" << fields.size();
			++count_distribution[fields.size()];
			for (auto const& field : fields) {
				prompt << "\n\t  " << field_ix << ":" << std::quoted(field);
				fields_map[field_ix][field].push_back(line_ix);
				++field_ix;
			}
			++line_ix;
		}
		prompt << "\nField Count distribution";
		for (auto const& entry : count_distribution) prompt << "\nfield count:" << entry.first << " entry count:" << entry.second;
		prompt << "\nField Distribution";
		for (auto const& [field_ix,field_map] : fields_map) {
			prompt << "\n\tindex:" << field_ix;
			for (auto const& [field,line_nos] : field_map) {
				prompt << "\n\t  field:" << std::quoted(field) << " line:";
				auto rr = to_ranges(line_nos);
				prompt << rr;
				if (line_nos.size()>0) {
					line_no begin{line_nos[0]}; 
					line_no previous{begin};
					for (auto line_ix : line_nos) {
						if (line_ix > previous+1) {
							// Broken sequence - log previous one
							if (begin == previous) prompt << " " << begin;
							else prompt << " [" << begin << ".." << previous << "]";
							begin = line_ix;
						}
						// prompt << "<" << line_ix << ">";
						previous = line_ix;
					}
					// Log last range
					if (begin == previous) prompt << " " << previous;
					else prompt << " [" << begin << ".." << previous << "]";
				}
			}
		}
	}

	Amount to_cents_amount(Amount const& amount) {
		return round((amount*100.0)/Amount{100.0}); // Amount / Amount = real number 
	}

	enum class AccountKind {
		// Account type is specified as T, S, K or I (asset, debt, cost or income)
		// Swedish: tillgång, skuld, kostnad eller intäkt
		Unknown
		,Asset 	// + or Debit for MORE assets
		,Debt		// - or credit for MORE debt 
		,Income // - or Credit for MORE income
		,Cost		// + or Debit for MORE cost
		,Result // - or Credit for MORE result (counter to Assets)
		,Undefined
	};
	using OptionalAccountKind = std::optional<AccountKind>;

	OptionalAccountKind to_account_kind(BAS::AccountNo const& bas_account_no) {
		OptionalAccountKind result{};
		auto s_account_no = std::to_string(bas_account_no);
		if (s_account_no.size() == 4) {
			switch (s_account_no[0]) {
				case '1': {result = AccountKind::Asset;} break;
				case '2': {result = AccountKind::Debt;} break;
				case '3': {result = AccountKind::Income;} break;
				case '4':
				case '5':
				case '6':
				case '7': {result = AccountKind::Cost;} break;
				case '8': {result = AccountKind::Result;} break;
				case '9': {/* NOT a BAS account (free for use for use as seen fit) */} break;
			}
		}
		return result;
	}

	using BASAccountNumberPath = Key::Path;
	BASAccountNumberPath to_bas__account_number_path(BAS::AccountNo const& bas_account_no) {
		BASAccountNumberPath result{};
		// TODO: Search 
		return result;
	}

	struct AccountMeta {
		std::string name{};
		OptionalAccountKind account_kind{};
		SKV::SRU::OptionalAccountNo sru_code{};
	};
	using AccountMetas = std::map<BAS::AccountNo,BAS::AccountMeta>;

	namespace detail {
		// "Hidden" Global mapping between BAS account no and its registered meta data like name, SRU code etc (from SIE file(s) imported)
		// See accessor global_account_metas().
		AccountMetas global_account_metas{};
	}

	AccountMetas const& global_account_metas() {return detail::global_account_metas;}

	namespace anonymous {

		struct AccountTransaction {
			BAS::AccountNo account_no;
			std::optional<std::string> transtext{};
			Amount amount;
			bool operator<(AccountTransaction const& other) const {
				bool result{false};
				if (account_no == other.account_no) {
					if (amount == other.amount) {
						result = (transtext < other.transtext);
					}
					else result = (amount < other.amount);
				}
				else result = (account_no < other.account_no);
				return result;
			}
		};
		using OptionalAccountTransaction = std::optional<AccountTransaction>;
		using AccountTransactions = std::vector<AccountTransaction>;

		template <typename T>
		struct JournalEntry_t {
			std::string caption{};
			Date date{};
			T account_transactions;
		};
		using JournalEntry = JournalEntry_t<AccountTransactions>;
		using OptionalJournalEntry = std::optional<JournalEntry>;
		using JournalEntries = std::vector<JournalEntry>;
	} // namespace anonymous

	using VerNo = unsigned int;
	using OptionalVerNo = std::optional<VerNo>;
	using Series = char;

	struct JournalEntryMeta {
		Series series;
		OptionalVerNo verno;
		std::optional<bool> unposted_flag{};
		bool operator==(JournalEntryMeta const& other) const = default;
	};
	using OptionalJournalEntryMeta = std::optional<JournalEntryMeta>;

	using MetaEntry = MetaDefacto<JournalEntryMeta,BAS::anonymous::JournalEntry>;
	using OptionalMetaEntry = std::optional<MetaEntry>;
	using MetaEntries = std::vector<MetaEntry>;

	using MetaAccountTransaction = MetaDefacto<BAS::MetaEntry,BAS::anonymous::AccountTransaction>;
	using OptionalMetaAccountTransaction = std::optional<MetaAccountTransaction>;
	using MetaAccountTransactions = std::vector<MetaAccountTransaction>;
} // namespace BAS

// Forward (TODO: Reorganise if/when splitting into proper header/cpp file structure)
Amount to_positive_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje);
Amount to_negative_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje);
void for_each_anonymous_account_transaction(BAS::anonymous::JournalEntry const& aje,auto& f);

namespace BAS {

	Amount mats_sum(BAS::MetaAccountTransactions const& mats) {
		return std::accumulate(mats.begin(),mats.end(),Amount{},[](Amount acc,BAS::MetaAccountTransaction const& mat){
			acc += mat.defacto.amount;
			return acc;
		});
	}

	using MatchesMetaEntry = std::function<bool(BAS::MetaEntry const& me)>;

	BAS::OptionalAccountNo to_account_no(std::string const& s) {
		return to_four_digit_positive_int(s);
	}

	OptionalJournalEntryMeta to_journal_meta(std::string const& s) {
		OptionalJournalEntryMeta result{};
		try {
			const std::regex meta_regex("[A-Z]\\d+"); // series followed by verification number
			if (std::regex_match(s,meta_regex)) result = JournalEntryMeta{
				 .series = s[0]
				,.verno = static_cast<VerNo>(std::stoi(s.substr(1)))};
		}
		catch (std::exception const& e) { std::cout << "\nDESIGN INSUFFICIENCY: to_journal_meta(" << s << ") failed. Exception=" << std::quoted(e.what());}
		return result;
	}

	auto has_greater_amount = [](BAS::anonymous::AccountTransaction const& at1,BAS::anonymous::AccountTransaction const& at2) {
		return (at1.amount > at2.amount);
	};

	auto has_greater_abs_amount = [](BAS::anonymous::AccountTransaction const& at1,BAS::anonymous::AccountTransaction const& at2) {
    // Note: Prefix '::' means 'global namespace', NOT 'namespace above this one'.
		return (abs(at1.amount) > abs(at2.amount));
	};

	BAS::MetaEntry& sort(BAS::MetaEntry& me,auto& comp) {
		std::sort(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),comp);
		return me;
	}

	namespace filter {
		struct is_series {
			BAS::Series required_series;
			bool operator()(MetaEntry const& me) {
				return (me.meta.series == required_series);
			}
		};

		class HasGrossAmount {
		public:
			HasGrossAmount(Amount gross_amount) : m_gross_amount(gross_amount) {}
			bool operator()(BAS::MetaEntry const& me) {
				if (m_gross_amount<0) {
					return (to_negative_gross_transaction_amount(me.defacto) == m_gross_amount);
				}
				else {
					return (to_positive_gross_transaction_amount(me.defacto) == m_gross_amount);
				}
			}
		private:
			Amount m_gross_amount;
		};

		class HasTransactionToAccount {
		public:
			HasTransactionToAccount(BAS::AccountNo bas_account_no) : m_bas_account_no(bas_account_no) {}
			bool operator()(BAS::MetaEntry const& me) {
				return std::any_of(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),[this](BAS::anonymous::AccountTransaction const& at){
					return (at.account_no == this->m_bas_account_no);
				});
			}
		private:
			BAS::AccountNo m_bas_account_no;
		};

		struct is_flagged_unposted {
			bool operator()(MetaEntry const& me) {
				return (me.meta.unposted_flag and *me.meta.unposted_flag); // Rely on C++ short-circuit (https://en.cppreference.com/w/cpp/language/operator_logical)
			}
		};

		struct contains_account {
			BAS::AccountNo account_no;
			bool operator()(MetaEntry const& me) {
				return std::any_of(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),[this](auto const& at){
					return (this->account_no == at.account_no);
				});
			}
		};

		struct matches_meta {
			JournalEntryMeta entry_meta;
			bool operator()(MetaEntry const& me) {
				return (me.meta == entry_meta);
			}
		};

		struct matches_amount {
			Amount amount;
			bool operator()(MetaEntry const& me) {
				return std::any_of(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),[this](auto const& at){
					return (this->amount == at.amount);
				});
			}
		};

		struct matches_heading {
			std::string user_search_string;
			bool operator()(MetaEntry const& me) {
				bool result{false};
				result = strings_share_tokens(user_search_string,me.defacto.caption);
				if (!result) {
					result = std::any_of(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),[this](auto const& at){
						if (at.transtext) return strings_share_tokens(user_search_string,*at.transtext);
						return false;
					});
				}
				return result;
			}
		};

		struct matches_user_search_criteria{
			std::string user_search_string;
			bool operator()(MetaEntry const& me) {
				bool result{false};
				if (!result and user_search_string.size()==1) {
					result = is_series{user_search_string[0]}(me);
				}
				if (auto account_no = to_account_no(user_search_string);!result and account_no) {
					result = contains_account{*account_no}(me);
				}
				if (auto meta = to_journal_meta(user_search_string);!result and meta) {
					result = matches_meta{*meta}(me);
				}
				if (auto amount = to_amount(user_search_string);!result and amount) {
					result = matches_amount{*amount}(me);
				}
				if (!result) {
					result = matches_heading{user_search_string}(me);
				}
				return result;
			}
		};
	} // namespace filter

	// TYPED Journal Entries (to identify patterns of interest in how the individual account transactions of an entry is dispositioned in amount and on semantics of the account)
	namespace anonymous {
		using AccountTransactionType = std::set<std::string>;
		using TypedAccountTransactions = std::map<BAS::anonymous::AccountTransaction,AccountTransactionType>;
		using TypedAccountTransaction = TypedAccountTransactions::value_type;
		using TypedJournalEntry = BAS::anonymous::JournalEntry_t<TypedAccountTransactions>;
	}
	using TypedMetaEntry = MetaDefacto<BAS::JournalEntryMeta,anonymous::TypedJournalEntry>;
	using TypedMetaEntries = std::vector<TypedMetaEntry>;

	void for_each_typed_account_transaction(BAS::TypedMetaEntry const& tme,auto& f) {
		for (auto const& tat : tme.defacto.account_transactions) {
			f(tat);
		}
	}

	namespace kind {

		using BASAccountTopology = std::set<BAS::AccountNo>;
		using AccountTransactionTypeTopology = std::set<std::string>;

		enum class ATType {
			// NOTE: Restrict to 16 values (Or to_at_types_order stops being reliable)
			undefined
			,transfer
			,eu_purchase
			,gross
			,net
			,eu_vat
			,vat
			,cents
			,unknown
		};

		ATType to_at_type(std::string const& prop) {
			ATType result{ATType::undefined};
			static const std::map<std::string,ATType> AT_TYPE_TO_ID_MAP{
				 {"",ATType::undefined}
				,{"transfer",ATType::transfer}
				,{"eu_purchase",ATType::eu_purchase}
				,{"gross",ATType::gross}
				,{"net",ATType::net}
				,{"eu_vat",ATType::eu_vat}
				,{"vat",ATType::vat}
				,{"cents",ATType::cents}
			};
			if (AT_TYPE_TO_ID_MAP.contains(prop)) {
				result = AT_TYPE_TO_ID_MAP.at(prop);
			}
			else {
				result = ATType::unknown;
			}
			return result;
		}

		std::size_t to_at_types_order(BAS::kind::AccountTransactionTypeTopology const& topology) {
			std::size_t result{};
			std::vector<ATType> at_types{};
			for (auto const& prop : topology) at_types.push_back(to_at_type(prop));
			std::sort(at_types.begin(),at_types.end(),[](ATType t1,ATType t2){
				return (t1<t2);
			});
			// Assemble a "number" of "digits" each having value 0..15 (i.e, in effect a hexadecimal number with each digit indicating an at_type enum value)
			for (auto at_type : at_types) result = result*0x10 + static_cast<std::size_t>(at_type);
			return result;
		}

		std::vector<std::string> sorted(AccountTransactionTypeTopology const& topology) {
			std::vector<std::string> result{topology.begin(),topology.end()};
			std::sort(result.begin(),result.end(),[](auto const& s1,auto const& s2){
				return (to_at_type(s1) < to_at_type(s2));
			});
			return result;
		}

		namespace detail {
			template <typename T>
			struct hash {};

			template <>
			struct hash<BASAccountTopology> {
				std::size_t operator()(BASAccountTopology const& bat) {
					std::size_t result{};
					for (auto const& account_no : bat) {
						auto h = std::hash<BAS::AccountNo>{}(account_no);
						result = result ^ (h << 1);
					}
					return result;
				}	
			};

			template <>
			struct hash<AccountTransactionTypeTopology> {
				std::size_t operator()(AccountTransactionTypeTopology const& props) {
					std::size_t result{};
					for (auto const& prop : props) {
						auto h = std::hash<std::string>{}(prop);
						result = result ^ static_cast<std::size_t>((h << 1));
					}
					return result;
				}	
			};
		} // namespace detail

		BASAccountTopology to_accounts_topology(MetaEntry const& me) {
			BASAccountTopology result{};
			auto f = [&result](BAS::anonymous::AccountTransaction const& at) {
				result.insert(at.account_no);
			};
			for_each_anonymous_account_transaction(me.defacto,f);
			return result;
		}

		BASAccountTopology to_accounts_topology(TypedMetaEntry const& tme) {
			BASAccountTopology result{};
			auto f = [&result](BAS::anonymous::TypedAccountTransaction const& tat) {
				auto const& [at,props] = tat;
				result.insert(at.account_no);
			};
			for_each_typed_account_transaction(tme,f);
			return result;
		}

		AccountTransactionTypeTopology to_types_topology(TypedMetaEntry const& tme) {
			AccountTransactionTypeTopology result{};
			auto f = [&result](BAS::anonymous::TypedAccountTransaction const& tat) {
				auto const& [at,props] = tat;
				for (auto const& prop : props) result.insert(prop);
			};
			for_each_typed_account_transaction(tme,f);
			return result;
		}

		std::size_t to_signature(BASAccountTopology const& bat) {
			return detail::hash<BASAccountTopology>{}(bat);
		}

		std::size_t to_signature(AccountTransactionTypeTopology const& met) {
			return detail::hash<AccountTransactionTypeTopology>{}(met);
		}

	} // namespace kind

	namespace group {

	}
} // namespace BAS

using Sru2BasMap = std::map<SKV::SRU::AccountNo,BAS::AccountNos>;

Sru2BasMap sru_to_bas_map(BAS::AccountMetas const& metas) {
	Sru2BasMap result{};
	for (auto const& [bas_account_no,am] : metas) {
		if (am.sru_code) result[*am.sru_code].push_back(bas_account_no);
	}
	return result;
}

namespace SKV { 
	namespace XML {
		namespace VATReturns {
			using BoxNo = unsigned int;
			using BoxNos = std::vector<BoxNo>;

			BoxNos const EU_VAT_BOX_NOS{30,31,32};
			BoxNos const EU_PURCHASE_BOX_NOS{20,21};
			BoxNos const VAT_BOX_NOS{10,11,12,30,31,32,60,61,62,48,49};

			using FormBoxMap = std::map<BoxNo,BAS::MetaAccountTransactions>;

			Amount to_box_49_amount(FormBoxMap const& box_map);

		} // namespace VATReturns 
	} // namespace XML 
} // namespace SKV 

std::set<BAS::AccountNo> to_vat_returns_form_bas_accounts(SKV::XML::VATReturns::BoxNos const& box_nos); // Forward (future header)
std::set<BAS::AccountNo> const& to_vat_accounts(); // Forward (future header)


auto is_any_of_accounts(BAS::MetaAccountTransaction const mat,BAS::AccountNos const& account_nos) {
	return std::any_of(account_nos.begin(),account_nos.end(),[&mat](auto other){
		return other == mat.defacto.account_no;
	});
}

/*

	How can we model bookkeeping of a company?

	It seems we need to be able to model the following aspects.

	One or more Journals with details of each financial transaction, what document is proof of the transaction and how the transaction is accounted for.
	The Ledger in which all Journal transactions are summarised to.

	So, it seems a Journal contains records of how each transaction is accounted for.
	And the Ledger contains transactions as recorded to accounts?

	Can we thus say that an SIE-file is in fact a representation of a Journal?
	And when we read in an SIE file we create the Ledger?

	The Swedish terms can thus be as follows.

	"Bokföring" and "Verifikation" is about entering trasnactions into a Journal?
	"Huvudbok" is the Ledger?

	So then we should implement a model with one or more Journals linked to a single Ledger?

	Journal 1											Ledger
	Journal 2												Account
	Journal 3												Account
	...														Account
	   Journal Entry {										Account
		   Transaction Account <-> Account					   Transaction
		   Transaction Account <-> Account
		   ...
	   }

	From Wikipedia

	*  A document is produced each time a transaction occurs
	*  Bookkeeping first involves recording the details of all of these source documents into multi-column journals.
	*  Each column in a journal normally corresponds to an account.
	*  After a certain period ... each column in each journal .. are then transferred to their respective accounts in the ledger...

*/

// Helper for stream output of otpional string
std::ostream& operator<<(std::ostream& os,std::optional<std::string> const& opt_s) {
	if (opt_s) {
		os << std::quoted(*opt_s);
	}
	return os;
}

std::optional<std::filesystem::path> path_to_existing_file(std::string const& s) {
	std::optional<std::filesystem::path> result{};
	std::filesystem::path path{s};
	if (std::filesystem::exists(path)) result = path;
	return result;
}

// Name, Heading + Amount account transaction
struct NameHeadingAmountAT {
	std::string account_name;
	std::optional<std::string> trans_text{};
	Amount amount;
};
using OptionalNameHeadingAmountAT = std::optional<NameHeadingAmountAT>;

OptionalNameHeadingAmountAT to_name_heading_amount(std::vector<std::string> const& ast) {
	OptionalNameHeadingAmountAT result{};
	switch (ast.size()) {
		case 2: {
			if (auto amount = to_amount(ast[1])) {
				result = NameHeadingAmountAT{.account_name=ast[0],.amount=*amount};
			}
		} break;
		case 3: {
			if (auto amount = to_amount(ast[2])) {
				result = NameHeadingAmountAT{.account_name=ast[0],.trans_text=ast[1],.amount=*amount};
			}		
		} break;
		default:;
	}
	return result;
}

BAS::anonymous::OptionalAccountTransaction to_bas_account_transaction(std::vector<std::string> const& ast) {
	BAS::anonymous::OptionalAccountTransaction result{};
	if (ast.size() > 1) {
			if (auto account_no = BAS::to_account_no(ast[0])) {
				switch (ast.size()) {
					case 2: {
						if (auto amount = to_amount(ast[1])) {
							result = BAS::anonymous::AccountTransaction{.account_no=*account_no,.transtext=std::nullopt,.amount=*amount};
						}
					} break;
					case 3: {
						if (auto amount = to_amount(ast[2])) {
							result = BAS::anonymous::AccountTransaction{.account_no=*account_no,.transtext=ast[1],.amount=*amount};
						}
					} break;
					default:;
				}
			}
	}
	return result;
}

std::ostream& operator<<(std::ostream& os,BAS::anonymous::AccountTransaction const& at) {
	if (BAS::global_account_metas().contains(at.account_no)) os << std::quoted(BAS::global_account_metas().at(at.account_no).name) << ":";
	os << at.account_no;
	os << " " << at.transtext;
	os << " " << to_string(at.amount); // When amount is double there will be no formatting of the amount to ensure two decimal cents digits
	return os;
};

std::string to_string(BAS::anonymous::AccountTransaction const& at) {
	std::ostringstream os{};
	os << at;
	return os.str();
};

std::ostream& operator<<(std::ostream& os,BAS::anonymous::AccountTransactions const& ats) {
	for (auto const& at : ats) {
		// os << "\n\t" << at; 
		os << "\n  " << at; 
	}
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::anonymous::JournalEntry const& aje) {
	os << std::quoted(aje.caption) << " " << aje.date;
	os << aje.account_transactions;
	return os;
};

std::ostream& operator<<(std::ostream& os,BAS::OptionalVerNo const& verno) {
	if (verno and *verno!=0) os << *verno;
	else os << " _ ";
	return os;
}

std::ostream& operator<<(std::ostream& os,std::optional<bool> flag) {
	auto ch = (flag)?((*flag)?'*':' '):' '; // '*' if known and set, else ' '
	os << ch;
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::JournalEntryMeta const& jem) {
	os << jem.unposted_flag << jem.series << jem.verno;
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::MetaAccountTransaction const& mat) {
	os << mat.meta.meta << " " << mat.defacto;
	return os;
};

std::ostream& operator<<(std::ostream& os,BAS::MetaEntry const& me) {
	os << me.meta << " " << me.defacto;
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::MetaEntries const& mes) {
	for (auto const& me : mes) {
		os << "\n" << me;
	}
	return os;
};

std::string to_string(BAS::anonymous::JournalEntry const& aje) {
	std::ostringstream os{};
	os << aje;
	return os.str();
};

std::string to_string(BAS::MetaEntry const& me) {
	std::ostringstream os{};
	os << me;
	return os.str();
};

// TYPED ENTRY

std::ostream& operator<<(std::ostream& os,BAS::kind::BASAccountTopology const& accounts) {
	if (accounts.size()==0) os << " ?";
	else for (auto const& account : accounts) {
		os << " " << account;
	}
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::kind::AccountTransactionTypeTopology const& props) {
	auto sorted_props = BAS::kind::sorted(props); // props is a std::set, sorted_props is a vector
	if (sorted_props.size()==0) os << " ?";
	else for (auto const& prop : sorted_props) {
		os << " " << prop;
	}
	os << " = sort_code: 0x" << std::hex << BAS::kind::to_at_types_order(props) << std::dec;
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::anonymous::TypedAccountTransaction const& tat) {
	auto const& [at,props] = tat;
	os << props << " : " << at;
	return os;
}

template <typename T>
struct IndentedOnNewLine{
	IndentedOnNewLine(T const& val,int count) : val{val},count{count} {}
	T const& val;
	int count;
};

std::ostream& operator<<(std::ostream& os,IndentedOnNewLine<BAS::anonymous::TypedAccountTransactions> const& indented) {
	for (auto const& at : indented.val) {
		os << "\n";
		for (int x = 0; x < indented.count; ++x) os << ' ';
		os << at;
	}
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::anonymous::TypedJournalEntry const& tje) {
	os << std::quoted(tje.caption) << " " << tje.date;
	for (auto const& tat : tje.account_transactions) {
		os << "\n\t" << tat;
	}
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::TypedMetaEntry const& tme) {
	os << tme.meta << " " << tme.defacto;
	return os;
}

// JOURNAL

using BASJournal = std::map<BAS::VerNo,BAS::anonymous::JournalEntry>;
using BASJournalId = char; // The Id of a single BAS journal is a series character A,B,C,...
using BASJournals = std::map<BASJournalId,BASJournal>; // Swedish BAS Journals named "Series" and labeled with "Id" A,B,C,...

TaggedAmount to_tagged_amount(Date const& date,BAS::anonymous::AccountTransaction const& at) {
	auto cents_amount = to_cents_amount(at.amount);
	TaggedAmount result{date,cents_amount};
	result.tags()["BAS"] = std::to_string(at.account_no);
	if (at.transtext and at.transtext->size() > 0) result.tags()["TRANSTEXT"] = *at.transtext;
	return result;
}

TaggedAmounts to_tagged_amounts(BAS::MetaEntry const& me) {
	TaggedAmounts result{};
	auto journal_id = me.meta.series;
	auto verno = me.meta.verno;
	auto date = me.defacto.date;
	auto gross_cents_amount = to_cents_amount(to_positive_gross_transaction_amount(me.defacto));
	TaggedAmount::Tags tags{};
	tags["type"] = "aggregate";
	if (verno) tags["SIE"] = journal_id+std::to_string(*verno);
	tags["vertext"] = me.defacto.caption;
	TaggedAmount aggregate_ta{date,gross_cents_amount,std::move(tags)};
	Key::Path value_ids{};
	auto push_back_as_tagged_amount = [&value_ids,&date,&journal_id,&verno,&result](BAS::anonymous::AccountTransaction const& at){
		auto ta = to_tagged_amount(date,at);
    if (verno) ta.tags()["parent_SIE"] = journal_id+std::to_string(*verno);
    ta.tags()["Ix"]=std::to_string(result.size()); // index 0,1,2...
		result.push_back(ta);
		value_ids += TaggedAmount::to_string(to_value_id(ta));
	};
	for_each_anonymous_account_transaction(me.defacto,push_back_as_tagged_amount);
	// TODO: Create the aggregate amount that refers to all account transaction amounts
	// type=aggregate members=<id>&<id>&<id>...
	aggregate_ta.tags()["_members"] = value_ids.to_string();
	result.push_back(aggregate_ta);
	return result;
}

// A function object able to create Net+Vat BAS::anonymous::AccountTransactions
class ToNetVatAccountTransactions {
public:

	ToNetVatAccountTransactions(BAS::anonymous::AccountTransaction const& net_at, BAS::anonymous::AccountTransaction const& vat_at)
		:  m_net_at{net_at}
		  ,m_vat_at{vat_at}
			,m_gross_vat_rate{static_cast<float>((net_at.amount != 0)?vat_at.amount/(net_at.amount + vat_at.amount):1.0)}
			,m_sign{(net_at.amount<0)?-1:1} /* 0 gets sign + */ {}

	BAS::anonymous::AccountTransactions operator()(Amount remaining_counter_amount,std::string const& transtext,OptionalAmount const& inc_vat_amount = std::nullopt) {
		BAS::anonymous::AccountTransactions result{};
		Amount gross_amount = (inc_vat_amount)?*inc_vat_amount:remaining_counter_amount;
		BAS::anonymous::AccountTransaction net_at{
			.account_no = m_net_at.account_no
			,.transtext = transtext
			,.amount = BAS::to_cents_amount(static_cast<Amount>(m_sign * gross_amount * (1.0-m_gross_vat_rate)))
		};
		BAS::anonymous::AccountTransaction vat_at{
			.account_no = m_vat_at.account_no
			,.transtext = transtext
			,.amount = BAS::to_cents_amount(static_cast<Amount>(m_sign * gross_amount * m_gross_vat_rate))
		};
		result.push_back(net_at);
		result.push_back(vat_at);
		return result;
	}
private:
	BAS::anonymous::AccountTransaction m_net_at;
	BAS::anonymous::AccountTransaction m_vat_at;
	float m_gross_vat_rate;
	int m_sign;
};

struct HeadingAmountDateTransEntry {
	struct Optional {
		std::optional<char> series{};
		std::optional<BAS::AccountNo> gross_account_no{};
		BAS::OptionalMetaEntry current_candidate{};
		std::optional<ToNetVatAccountTransactions> counter_ats_producer{};
		std::optional<SKV::XML::VATReturns::FormBoxMap> vat_returns_form_box_map_candidate{};
	};
	std::string heading{};
	Amount amount;
	Date date{};
	Optional optional{};
};

std::ostream& operator<<(std::ostream& os,HeadingAmountDateTransEntry const& had) {
	if (auto me = had.optional.current_candidate) {
		os << '*';
	}
	else {
		os << ' ';
	}

	if (auto me = had.optional.series) {
		os << *had.optional.series;
	}
	else {
		os << '-';
	}

	if (auto me = had.optional.gross_account_no) {
		os << ' ' << *had.optional.gross_account_no << ' ';
	}
	else {
		os << " nnnn ";
	}

	os << had.heading;
	os << " " << had.amount;
	os << " " << to_string(had.date);
	return os;
}

using OptionalHeadingAmountDateTransEntry = std::optional<HeadingAmountDateTransEntry>;

using HeadingAmountDateTransEntries = std::vector<HeadingAmountDateTransEntry>;

namespace CSV {

	namespace NORDEA {
		class istream : public encoding::UTF8::istream {};

		// Assume Finland located bank Nordea swedish web csv format of transactions to/from an account
		/*
		Bokföringsdag;Belopp;Avsändare;Mottagare;Namn;Rubrik;Meddelande;Egna anteckningar;Saldo;Valuta
		2022/06/27;-720,00;51 86 87-9;5343-2795;LOOPIA AB (WEBSUPPORT);LOOPIA AB (WEBSUPPORT);378587992;Webhotell Q2;49537,22;SEK
		2022/06/16;-880,00;51 86 87-9;824-3040;TELIA SVERIGE AB;TELIA SVERIGE AB;19990271223;Mobil Q2;50257,22;SEK
		2022/06/13;-625,00;51 86 87-9;5050-1055;SKATTEVERKET;SKATTEVERKET;16556782817244;Förs Avg;51137,22;SEK
		2022/06/13;-153,71;51 86 87-9;;KORT             BEANSTALK APP   26;KORT             BEANSTALK APP   26;BEANSTALK APP   2656;;;SEK
		2022/06/10;-184,00;51 86 87-9;;KORT             POSTNORD SE     26;KORT             POSTNORD SE     26;POSTNORD SE     2656;;51915,93;SEK
		2022/06/09;-399,90;51 86 87-9;;KORT             KJELL O CO 100  26;KORT             KJELL O CO 100  26;KJELL O CO 100  2656;;52099,93;SEK
		2022/06/03;-1,70;51 86 87-9;;PRIS ENL SPEC;PRIS ENL SPEC;;;52499,83;SEK
		2022/05/31;-446,00;51 86 87-9;5020-7042;Fortnox Finans AB;Fortnox Finans AB;52804292974641;Bokf pr jun/jul/aug;52501,53;SEK
		2022/05/24;786,99;;51 86 87-9;BG KONTOINS;BG KONTOINS;0817-9780 GOOGLE IRELA;;52947,53;SEK
		2022/05/19;179,00;;51 86 87-9;BG KONTOINS;BG KONTOINS;5050-1030 SK5567828172;;52160,54;SEK
		2022/05/18;-610,33;51 86 87-9;;KORT             PAYPAL *HKSITES 26;KORT             PAYPAL *HKSITES 26;PAYPAL *HKSITES 2656;;51981,54;SEK
		2022/05/12;-154,91;51 86 87-9;;KORT             BEANSTALK APP   26;KORT             BEANSTALK APP   26;BEANSTALK APP   2656;;52591,87;SEK
		2022/05/04;-1,70;51 86 87-9;;PRIS ENL SPEC;PRIS ENL SPEC;;;52746,78;SEK
		2022/04/19;-186,25;51 86 87-9;5343-2795;LOOPIA AB (WEBSUPPORT);LOOPIA AB (WEBSUPPORT);375508199;sharedplanet se;52748,48;SEK
		2022/04/12;-145,41;51 86 87-9;;KORT             BEANSTALK APP   26;KORT             BEANSTALK APP   26;BEANSTALK APP   2656;;52934,73;SEK
		2022/04/04;-6,80;51 86 87-9;;PRIS ENL SPEC;PRIS ENL SPEC;;;53080,14;SEK
		*/
		CSV::NORDEA::istream& operator>>(CSV::NORDEA::istream& in,HeadingAmountDateTransEntry& had) {
			//       0         1      2         3         4    5        6          7              8      9
			// Bokföringsdag;Belopp;Avsändare;Mottagare;Namn;Rubrik;Meddelande;Egna anteckningar;Saldo;Valuta
			//       0         1      2      3            4                             5                                 6                78 9
			// 2021-12-15;-419,65;51 86 87-9;;KORT             BEANSTALK APP   26;KORT             BEANSTALK APP   26;BEANSTALK APP   2656;;;SEK

			// 
			// Bokföringsdag	;Belopp		;Avsändare	;Mottagare	;Namn																	;Rubrik
			// 2022/05/18			;-610,33	;51 86 87-9	;						;KORT             PAYPAL *HKSITES 26	;KORT             PAYPAL *HKSITES 26

			// ;Meddelande						;Egna anteckningar	;Saldo			;Valuta
			// ;PAYPAL *HKSITES 2656	;										;51981,54		;SEK

			// 
			if (auto sEntry = in.getline(encoding::unicode::to_utf8{})) {
				auto tokens = tokenize::splits(*sEntry,';',tokenize::eAllowEmptyTokens::YES);
				// LOG
				if (false) {
// std::cout << "\ncsv count: " << tokens.size(); // Expected 10
					for (int i=0;i<tokens.size();++i) {
// std::cout << "\n\t" << i << " " << tokens[i];
					}
				}
				if (tokens.size()==10) {
					std::string heading{};
					std::optional<Amount> amount{};
					std::optional<Amount> saldo{};
					std::optional<Date> date{};
					std::optional<std::string> valuta{};
					for (int i=0;i<tokens.size();++i) {
						auto const& token = tokens[i];
						switch (i) {
							case element::Bokforingsdag: {date = to_date(token);} break;
							case element::Belopp: {amount = to_amount(token);} break;
							case element::Avsandare: {heading += " Avsändare:" + token;} break;
							case element::Mottagare: {heading += " Mottagare:" + token;} break;
							case element::Namn: {heading += " Namn:" + token;} break;
							case element::Rubrik: {heading += " Rubrik:" + token;} break;
							case element::Meddelande: {heading += " Meddelande:" + token;} break;
							case element::Egna_anteckningar: {heading += " Egna Anteckningar:" + token;} break;
							case element::Saldo: {saldo = to_amount(token);} break;
							case element::Valuta: {valuta = token;} break;
						}
					}

					while (true) {
						had.heading = heading;
						if (amount and valuta == "SEK") had.amount = *amount;
						else {in.raw_in.setstate(std::istream::failbit);break;}
						if (date) had.date = *date;
						else {in.raw_in.setstate(std::istream::failbit);break;}
						break;
					}
				}
			}
			return in;
		}
	} // namespace NORDEA

	using CSVParseResult = std::optional<HeadingAmountDateTransEntry>;

	CSVParseResult parse_TRANS(auto& in) {
		CSVParseResult result{};
		auto pos = in.raw_in.tellg();
		HeadingAmountDateTransEntry had{};
		if (in >> had) {
			// std::cout << "\nfrom csv: " << had;
			result = had;
			pos = in.raw_in.tellg(); // accept new stream position
		}
		in.raw_in.seekg(pos); // Reset position in case of failed parse
		in.raw_in.clear(); // Reset failbit to allow try for other parse
		return result;		
	}

	HeadingAmountDateTransEntries from_stream(auto& in,BAS::OptionalAccountNo gross_bas_account_no = std::nullopt) {
		HeadingAmountDateTransEntries result{};
		parse_TRANS(in); // skip first line with field names
		while (auto had = parse_TRANS(in)) {
			if (gross_bas_account_no) {
// std::cout << "\nfrom_stream to gross_bas_account_no:" << *gross_bas_account_no;
				// Add a template with the gross amount transacted to provided bas account no
				BAS::MetaEntry me{
					.meta = {
						.series = 'A'
					}
					,.defacto = {
						.caption = had->heading
						,.date = had->date
					}
				};
				BAS::anonymous::AccountTransaction gross_at{
					.account_no = *gross_bas_account_no
					,.amount = had->amount
				};
				me.defacto.account_transactions.push_back(gross_at);
				had->optional.current_candidate = me;
			}
			result.push_back(*had);
		}
		return result;
	}
} // namespace CSV

namespace SIE {

  // Parses SIE entries assumed to be in UTF8 encoding.
  // See encoding::CP437::istream::getline mechanism in from_sie_file

	struct Tag {
		std::string const expected;
	};

	// #RAR 0 20210501 20220430
	// #RAR -1 20200501 20210430	
	struct Rar {
		std::string tag;
		int year_no;
		Date first_day_yyyymmdd;
		Date last_day_yyyymmdd;
	};

	// Opening balance for balance sheet
	// #IB <year no> <account> <balance> <quantity>
	// Year no is specified using 0 for the current year and -1 for the previous year
	// Example:
	// #IB 0 1930 23780.78
	struct Ib {
		std::string tag;
		int year_no;
		BAS::AccountNo account_no; 
		Amount opening_balance;
		std::optional<float> quantity{};
	};

	// #ORGNR CIN 
	struct OrgNr {
		std::string tag;
		std::string CIN;
		// acq_no 
		// act_no
	};

	// #FNAMN company name
	// #FNAMN "The ITfied AB"
	struct FNamn {
		std::string tag;
		std::string company_name;
	};

	// #ADRESS contact distribution address postal address tel
	// #ADRESS "Kjell-Olov H?gdal" "Aron Lindgrens v?g 6, lgh 1801" "17668 J?rf?lla" "070-6850408" 	
	struct Adress {
		std::string tag;
		std::string contact;
		std::string distribution_address;
		std::string postal_address;
		std::string tel;
	};

	// #KONTO 8422 "Dr?jsm?lsr?ntor f?r leverant?rsskulder"
	struct Konto {
		std::string tag;
		BAS::AccountNo account_no;
		std::string name;
	};

	struct Sru {
		std::string tag;
		BAS::AccountNo bas_account_no;
		SKV::SRU::AccountNo sru_account_no;
	};

	struct Trans {
		// Spec: #TRANS account no {object list} amount transdate transtext quantity sign
		// Ex:   #TRANS 1920 {} 802 "" "" 0
		std::string tag;
		BAS::AccountNo account_no;
		std::string object_list{};
		Amount amount;
		std::optional<Date> transdate{};
		std::optional<std::string> transtext{};
		std::optional<float> quantity{};
		std::optional<std::string> sign{};
	};
	struct Ver {
		// Spec: #VER series verno verdate vertext regdate sign
		// Ex:   #VER A 3 20210510 "Beanstalk" 20210817
		std::string tag;
		BAS::Series series;
		BAS::VerNo verno;
		Date verdate;
		std::string vertext;
		std::optional<Date> regdate{};
		std::optional<std::string> sign{};
		std::vector<Trans> transactions{};
	};
	struct AnonymousLine {std::string str{};};
	using SIEFileEntry = std::variant<OrgNr,FNamn,Adress,Rar,Ib,Konto,Sru,Ver,Trans,AnonymousLine>;
	using SIEParseResult = std::optional<SIEFileEntry>;

	std::istream& operator>>(std::istream& utf8_in,Tag const& tag) {
		if (utf8_in.good()) {
			// std::cout << "\n>>Tag[expected:" << tag.expected;
			auto pos = utf8_in.tellg();
			std::string token{};
			if (utf8_in >> token) {
				// std::cout << ",read:" << token << "]";
				if (token != tag.expected) {
					utf8_in.seekg(pos); // Reset position for failed tag
					utf8_in.setstate(std::ios::failbit); // failed to read expected tag
					// std::cout << " rejected";
				}
			}
			else {
				// std::cout << "null";
			}
		}		
		return utf8_in;
	}

	struct optional_YYYYMMdd_parser {
		std::optional<Date>& date;
	};
	struct YYYYMMdd_parser {
		Date& date;
	};
	std::istream& operator>>(std::istream& utf8_in,optional_YYYYMMdd_parser& p) {
		if (utf8_in.good()) {
			auto pos = utf8_in.tellg();
			try {
				// std::chrono::from_stream(utf8_in,p.fmt,p.date); // Not utf8_in g++11 libc++?
				std::string sDate{};
				utf8_in >> std::quoted(sDate);
				if (sDate.size()==0) {
					pos = utf8_in.tellg(); // Accept this parse position (e.g., "" is to be accepted)
				}
				else if (auto date = to_date(sDate);date) {
					p.date = *date;
					pos = utf8_in.tellg(); // Accept this parse position (an actual date was parsed)
				}
			}
			catch (std::exception const& e) { /* swallow all failures silently for optional parse */}
			utf8_in.seekg(pos); // Reset position utf8_in case of failed parse
			utf8_in.clear(); // Reset failbit to allow try for other parse
		}
		return utf8_in;
	}
	std::istream& operator>>(std::istream& utf8_in,YYYYMMdd_parser& p) {
		if (utf8_in.good()) {
			std::optional<Date> od{};
			optional_YYYYMMdd_parser op{od};
			utf8_in >> op;
			if (op.date) {
				p.date = *op.date;
			}
			else {
				utf8_in.setstate(std::ios::failbit);
			}
		}
		return utf8_in;
	}

	struct optional_Text_parser {
		std::optional<std::string>& text;
	};
	std::istream& operator>>(std::istream& utf8_in,optional_Text_parser& p) {
		if (utf8_in.good()) {
			auto pos = utf8_in.tellg();
			std::string text{};
			if (utf8_in >> std::quoted(text)) {
				p.text = text;
				pos = utf8_in.tellg(); // accept this parse pos
			}
			utf8_in.clear(); // Reset failbit to allow try for other parse
			utf8_in.seekg(pos); // Reset position utf8_in case of failed parse
		}
		return utf8_in;
	}
	struct Scraps {};
	std::istream& operator>>(std::istream& utf8_in,Scraps& p) {
		if (utf8_in.good()) {
			std::string scraps{};
			std::getline(utf8_in,scraps);
			// std::cout << "\nscraps:" << scraps;
		}
		return utf8_in;		
	}

	SIEParseResult parse_RAR(std::istream& utf8_in,std::string const& sie_field_tag) {
		SIEParseResult result{};
		// #RAR 0 20210501 20220430
		// #RAR -1 20200501 20210430	
		// struct Rar {
		// 	std::string tag;
		// 	int year_no;
		// 	YYYYMMDD first_day_yyyymmdd;
		// 	YYYYMMDD last_day_yyyymmdd;
		// };
		SIE::Rar rar{};
		auto pos = utf8_in.tellg();
		YYYYMMdd_parser first_day_parser{rar.first_day_yyyymmdd};
		YYYYMMdd_parser last_day_parser{rar.last_day_yyyymmdd};
		if (utf8_in >> Tag{sie_field_tag} >> rar.year_no >> first_day_parser >> last_day_parser) {
			result = rar;
			pos = utf8_in.tellg(); // accept new stream position
		}
		utf8_in.seekg(pos); // Reset position utf8_in case of failed parse
		utf8_in.clear(); // Reset failbit to allow try for other parse
		return result;
	}

	SIEParseResult parse_IB(std::istream& utf8_in,std::string const& sie_field_tag) {
		SIEParseResult result{};
		Scraps scraps{};
		// struct Ib {
		// 	std::string tag;
		// 	int year_no;
		// 	BAS::AccountNo account_no; 
		// 	Amount opening_balance;
		// 	std::optional<float> quantity{};
		// };
		SIE::Ib ib{};
		auto pos = utf8_in.tellg();
		if (utf8_in >> Tag{sie_field_tag} >> ib.year_no >> ib.account_no >> ib.opening_balance >> scraps) {
			result = ib;
			pos = utf8_in.tellg(); // accept new stream position
		}
		utf8_in.seekg(pos); // Reset position utf8_in case of failed parse
		utf8_in.clear(); // Reset failbit to allow try for other parse
		return result;
	}

	SIEParseResult parse_ORGNR(std::istream& utf8_in,std::string const& sie_field_tag) {
		SIEParseResult result{};
		// #ORGNR CIN 
		// struct OrgNr {
		// 	std::string tag;
		// 	std::string CIN;
		// 	// acq_no 
		// 	// act_no
		// };
		SIE::OrgNr orgnr{};
		auto pos = utf8_in.tellg();
		if (utf8_in >> Tag{sie_field_tag} >> orgnr.CIN) {
			result = orgnr;
			pos = utf8_in.tellg(); // accept new stream position
		}
		utf8_in.seekg(pos); // Reset position utf8_in case of failed parse
		utf8_in.clear(); // Reset failbit to allow try for other parse

		return result;
	}

	SIEParseResult parse_FNAMN(std::istream& utf8_in,std::string const& sie_field_tag) {
		SIEParseResult result{};
		// #FNAMN company name
		// struct FNamn {
		// 	std::string tag;
		// 	std::string company_name;
		// };
		SIE::FNamn fnamn{};
		auto pos = utf8_in.tellg();
		if (utf8_in >> Tag{sie_field_tag} >> std::quoted(fnamn.company_name)) {
			result = fnamn;
			pos = utf8_in.tellg(); // accept new stream position
		}
		utf8_in.seekg(pos); // Reset position utf8_in case of failed parse
		utf8_in.clear(); // Reset failbit to allow try for other parse
		return result;
	}

	SIEParseResult parse_ADRESS(std::istream& utf8_in,std::string const& sie_field_tag) {
		SIEParseResult result{};
		// #ADRESS contact distribution address postal address tel
		// struct Adress {
		// 	std::string tag;
		// 	std::string contact;
		// 	std::string distribution_address;
		// 	std::string postal_address;
		// 	std::string tel;
		// };
		SIE::Adress adress{};
		auto pos = utf8_in.tellg();
		if (utf8_in >> Tag{sie_field_tag} >> std::quoted(adress.contact) >> std::quoted(adress.distribution_address) >> std::quoted(adress.postal_address) >> std::quoted(adress.tel)) {
			result = adress;
			pos = utf8_in.tellg(); // accept new stream position
		}
		utf8_in.seekg(pos); // Reset position utf8_in case of failed parse
		utf8_in.clear(); // Reset failbit to allow try for other parse
		return result;
	}

	SIEParseResult parse_KONTO(std::istream& utf8_in,std::string const& sie_field_tag) {
	// // #KONTO 8422 "Dr?jsm?lsr?ntor f?r leverant?rsskulder"
	// struct Konto {
	// 	std::string tag;
	// 	BAS::AccountNo account_no;
	// 	std::string name;
	// };
		SIEParseResult result{};
		SIE::Konto konto{};
		auto pos = utf8_in.tellg();
		if (utf8_in >> Tag{sie_field_tag} >> konto.account_no >> std::quoted(konto.name)) {
			result = konto;
			pos = utf8_in.tellg(); // accept new stream position
		}
		utf8_in.seekg(pos); // Reset position utf8_in case of failed parse
		utf8_in.clear(); // Reset failbit to allow try for other parse
		return result;		
	}

	SIEParseResult parse_SRU(std::istream& utf8_in,std::string const& sru_tag) {
		SIEParseResult result{};
		SIE::Sru sru{};
		auto pos = utf8_in.tellg();
		if (utf8_in >> Tag{sru_tag} >> sru.bas_account_no >> sru.sru_account_no) {
			result = sru;
			pos = utf8_in.tellg(); // accept new stream position
		}
		utf8_in.seekg(pos); // Reset position utf8_in case of failed parse
		utf8_in.clear(); // Reset failbit to allow try for other parse
		return result;		
	}

	SIEParseResult parse_TRANS(std::istream& utf8_in,std::string const& trans_tag) {
		SIEParseResult result{};
		Scraps scraps{};
		auto pos = utf8_in.tellg();
		// #TRANS 2610 {} 25900 "" "" 0
		SIE::Trans trans{};
		optional_YYYYMMdd_parser optional_transdate_parser{trans.transdate};
		optional_Text_parser optional_transtext_parser{trans.transtext};
		if (utf8_in >> Tag{trans_tag} >> trans.account_no >> trans.object_list >> trans.amount >> optional_transdate_parser >> optional_transtext_parser >> scraps) {
			// std::cout << trans_tag << trans.account_no << " " << trans.amount;
			result = trans;
			pos = utf8_in.tellg(); // accept new stream position
		}
		utf8_in.seekg(pos); // Reset position utf8_in case of failed parse
		utf8_in.clear(); // Reset failbit to allow try for other parse
		return result;		
	}

	SIEParseResult parse_Tag(std::istream& utf8_in,std::string const& tag) {
		SIEParseResult result{};
		Scraps scraps{};
		auto pos = utf8_in.tellg();
		if (utf8_in >> Tag{tag}) {
			result = AnonymousLine{}; // Success but not data
			pos = utf8_in.tellg(); // accept new stream position
		}
		utf8_in.seekg(pos); // Reset position utf8_in case of failed parse
		utf8_in.clear(); // Reset failbit to allow try for other parse
		return result;		
	}

	SIEParseResult parse_VER(std::istream& utf8_in) {
		SIEParseResult result{};
		Scraps scraps{};
		auto pos = utf8_in.tellg();
		// #VER A 1 20210505 "M�nadsavgift PG" 20210817
		SIE::Ver ver{};
		YYYYMMdd_parser verdate_parser{ver.verdate};
		if (utf8_in >> Tag{"#VER"} >> ver.series >> ver.verno >> verdate_parser >> std::quoted(ver.vertext) >> scraps >> scraps) {
			if (false) {
				// std::cout << "\nVer: " << ver.series << " " << ver.verno << " " << ver.vertext;
			}
			while (true) {
				if (auto entry = parse_TRANS(utf8_in,"#TRANS")) {
					// std::cout << "\nTRANS :)";	
					ver.transactions.push_back(std::get<Trans>(*entry));					
				}
				else if (auto entry = parse_TRANS(utf8_in,"#BTRANS")) {
					// Ignore
					// std::cout << " Ignored";
				}
				else if (auto entry = parse_TRANS(utf8_in,"#RTRANS")) {
					// Ignore
					// std::cout << " Ignored";
				}
				else if (auto entry = parse_Tag(utf8_in,"}")) {
					break;
				}
				else {
					std::cout << "\nERROR - Unexpected input while parsing #VER SIE entry";
					break;
				}
			}
			result = ver;
			pos = utf8_in.tellg(); // accept new stream position
		}
		utf8_in.seekg(pos); // Reset position utf8_in case of failed parse
		utf8_in.clear(); // Reset failbit to allow try for other parse
		return result;
	}

	SIEParseResult parse_any_line(std::istream& utf8_in) {
		if (utf8_in.fail()) {
			// std::cout << "\nparse_any_line utf8_in==fail";
		}
		auto pos = utf8_in.tellg();
		std::string line{};
		if (std::getline(utf8_in,line)) {
			// std::cout << "\n\tany=" << line;
			return AnonymousLine{.str=line};
		}
		else {
			// std::cout << "\n\tany null";
			utf8_in.seekg(pos);
			return {};
		}
	}

	// ===============================================================
	// BEGIN operator<< framework for SIE::T stream to text stream in SIE file representation
	// ===============================================================

	/**
	 * NOTE ABOUT UTF-8 TO Code Page 437 used as the character set of an SIE file
	 * 
	 * The convertion is made in overloaded operator<<(SIE::OStream& sieos,char ch) called by operator<<(SIE::OStream& sieos,std::string s).
	 * 
	 * But I have not yet overloaded operator<<(SIE::OStream& for things like Amount, account_no etc.
	 * SO - basically it is mainly transtext and vertext that is fed to the operator<<(SIE::OStream& sieos,std::string s).
	 * TAKE CARE to not mess this up or you will get UTF-8 encoded text into the SIE file that will mess things up quite a lot...
	 */

	struct OStream {
		std::ostream& os;
		encoding::UTF8::ToUnicodeBuffer to_unicode_buffer{};
	};

	SIE::OStream& operator<<(SIE::OStream& sieos,char ch) {
		// Assume ch is a byte in an UTF-8 stream and convert it to CP437 charachter set in file
		if (auto unicode = sieos.to_unicode_buffer.push(ch)) {
			auto cp437_ch = charset::CP437::UnicodeToCP437(*unicode);
			sieos.os.put(cp437_ch);
		}
		return sieos;
	}

	SIE::OStream& operator<<(SIE::OStream& sieos,std::string s) {
    if (false) {
      std::cout << "\nto SIE:" << std::quoted(s);
    }
		for (char ch : s) {
			sieos << ch; // Stream through operator<<(SIE::OStream& sieos,char ch) that will transform utf-8 encoded Unicode, to char encoded CP437
		}
		return sieos;
	}

	SIE::OStream& operator<<(SIE::OStream& sieos,SIE::Trans const& trans) {
		// #TRANS account no {object list} amount transdate transtext quantity sign
		//                                           o          o        o      o
		// #TRANS 1920 {} -890 "" "" 0
		sieos.os << "\n#TRANS"
		<< " " << trans.account_no
		<< " " << "{}"
		<< " " << trans.amount
		<< " " << std::quoted("");
		if (trans.transtext) sieos << " \"" << *trans.transtext << "\"";
		else sieos.os << " " << std::quoted("");
		return sieos;
	}
	
	SIE::OStream& operator<<(SIE::OStream& sieos,SIE::Ver const& ver) {
		// #VER A 1 20210505 "M�nadsavgift PG" 20210817	
		sieos.os << "\n#VER" 
		<< " " << ver.series 
		<< " " << ver.verno
		<< " " << to_string(ver.verdate); // TODO: make compiler find operator<< above (why can to_string use it but we can't?)
		sieos << " \"" << ver.vertext << "\"";
		sieos.os << "\n{";
		for (auto const& trans : ver.transactions) {
			sieos << trans;
		}
		sieos.os << "\n}";
		return sieos;
	}

	// ===============================================================
	// END operator<< framework for SIE::T stream to text stream in SIE file representation
	// ===============================================================

} // namespace SIE

SIE::Trans to_sie_t(BAS::anonymous::AccountTransaction const& trans) {
	SIE::Trans result{
		.account_no = trans.account_no
		,.amount = trans.amount
		,.transtext = trans.transtext // TODO: Ensure the text is charset::CP437, i.e., codepage 437
	};
	return result;
}

SIE::Ver to_sie_t(BAS::MetaEntry const& me) {
		/*
		Series series;
		BAS::VerNo verno;
		Date verdate;
		std::string vertext;
		*/

	SIE::Ver result{
		.series = me.meta.series
		,.verno = (me.meta.verno)?*me.meta.verno:0
		,.verdate = me.defacto.date
		,.vertext = me.defacto.caption};
	for (auto const& trans : me.defacto.account_transactions) {
		result.transactions.push_back(to_sie_t(trans));
	}
	return result;
}

bool is_vat_returns_form_at(std::vector<SKV::XML::VATReturns::BoxNo> const& box_nos,BAS::anonymous::AccountTransaction const& at) {
	auto const& bas_account_nos = to_vat_returns_form_bas_accounts(box_nos);
	return bas_account_nos.contains(at.account_no);
}

bool is_vat_account(BAS::AccountNo account_no) {
	auto const& vat_accounts = to_vat_accounts();
	return vat_accounts.contains(account_no);
}

auto is_vat_account_at = [](BAS::anonymous::AccountTransaction const& at){
	return is_vat_account(at.account_no);
};

Amount to_positive_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje) {
	Amount result = std::accumulate(aje.account_transactions.begin(),aje.account_transactions.end(),Amount{},[](Amount acc,BAS::anonymous::AccountTransaction const& account_transaction){
		acc += (account_transaction.amount>0)?account_transaction.amount:0;
		return acc;
	});
	return result;
}

Amount to_negative_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje) {
	Amount result = std::accumulate(aje.account_transactions.begin(),aje.account_transactions.end(),Amount{},[](Amount acc,BAS::anonymous::AccountTransaction const& account_transaction){
		acc += (account_transaction.amount<0)?account_transaction.amount:0;
		return acc;
	});
	return result;
}
 
bool does_balance(BAS::anonymous::JournalEntry const& aje) {
	auto positive_gross_transaction_amount = to_positive_gross_transaction_amount(aje);
	auto negative_gross_amount = to_negative_gross_transaction_amount(aje);
	// std::cout << "\ndoes_balance: positive_gross_transaction_amount=" << positive_gross_transaction_amount << "  negative_gross_amount=" << negative_gross_amount;
	// std::cout << "\n\tsum=" << positive_gross_transaction_amount + negative_gross_amount;
	// TODO: FIX Ronding errors somewhere that makes the positive and negative sum to be some infinitesimal value NOT zero ...
	return (BAS::to_cents_amount(positive_gross_transaction_amount + negative_gross_amount) == 0); // Fix for amounts not correct to the cents...
}

// Pick the negative or positive gross amount and return it without sign
OptionalAmount to_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje) {
	// std::cout << "\nto_gross_transaction_amount: " << aje;
	OptionalAmount result{};
	if (does_balance(aje)) {
		result = to_positive_gross_transaction_amount(aje); // Pick the positive alternative
	}
	else if (aje.account_transactions.size() == 1) {
		result = abs(aje.account_transactions.front().amount);
	}
	else {
		// Does NOT balance, and more than one account transaction.
		// Define the gross amount as the largest account absolute transaction amount
		auto max_at_iter = std::max_element(aje.account_transactions.begin(),aje.account_transactions.end(),[](auto const& at1,auto const& at2) {
			return abs(at1.amount) < abs(at2.amount);
		});
		if (max_at_iter != aje.account_transactions.end()) result = abs(max_at_iter->amount);
	}
	// if (result) std::cout << "\n\t==> " << *result;
	return result;	
}

BAS::anonymous::OptionalAccountTransaction gross_account_transaction(BAS::anonymous::JournalEntry const& aje) {
	BAS::anonymous::OptionalAccountTransaction result{};
	auto trans_amount = to_positive_gross_transaction_amount(aje);
	auto iter = std::find_if(aje.account_transactions.begin(),aje.account_transactions.end(),[&trans_amount](auto const& at){
		return abs(at.amount) == trans_amount;
	});
	if (iter != aje.account_transactions.end()) result = *iter;
	return result;
}

Amount to_account_transactions_sum(BAS::anonymous::AccountTransactions const& ats) {
	Amount result = std::accumulate(ats.begin(),ats.end(),Amount{},[](Amount acc,BAS::anonymous::AccountTransaction const& at){
		acc += at.amount;
		return acc;
	});
	return result;
}

bool have_opposite_signs(Amount a1,Amount a2) {
	return ((a1 > 0) and (a2 < 0)) or ((a1 < 0) and (a2 > 0)); // Note: false also for a1==a2==0
}

BAS::anonymous::AccountTransactions counter_account_transactions(BAS::anonymous::JournalEntry const& aje,BAS::anonymous::AccountTransaction const& gross_at) {
	BAS::anonymous::AccountTransactions result{};
	// Gather all ats with opposite sign and that sums upp to gross_at amount
	std::copy_if(aje.account_transactions.begin(),aje.account_transactions.end(),std::back_inserter(result),[&gross_at](auto const& at){
		return (have_opposite_signs(at.amount,gross_at.amount));
	});
	if (to_account_transactions_sum(result) != -gross_at.amount) result.clear();
	return result;
}

BAS::anonymous::OptionalAccountTransaction net_account_transaction(BAS::anonymous::JournalEntry const& aje) {
	BAS::anonymous::OptionalAccountTransaction result{};
	auto trans_amount = to_positive_gross_transaction_amount(aje);
	auto iter = std::find_if(aje.account_transactions.begin(),aje.account_transactions.end(),[&trans_amount](auto const& at){
		return (abs(at.amount) < trans_amount and not is_vat_account_at(at));
		// return abs(at.amount) == 0.8*trans_amount;
	});
	if (iter != aje.account_transactions.end()) result = *iter;
	return result;
}

BAS::anonymous::OptionalAccountTransaction vat_account_transaction(BAS::anonymous::JournalEntry const& aje) {
	BAS::anonymous::OptionalAccountTransaction result{};
	auto trans_amount = to_positive_gross_transaction_amount(aje);
	auto iter = std::find_if(aje.account_transactions.begin(),aje.account_transactions.end(),[&trans_amount](auto const& at){
		return is_vat_account_at(at);
		// return abs(at.amount) == 0.2*trans_amount;
	});
	if (iter != aje.account_transactions.end()) result = *iter;
	return result;
}

class AccountTransactionTemplate {
public:
	AccountTransactionTemplate(Amount gross_amount,BAS::anonymous::AccountTransaction const& at) 
		:  m_at{at}
			,m_percent{static_cast<int>(round(at.amount*100 / gross_amount))}  {}
	BAS::anonymous::AccountTransaction operator()(Amount amount) const {
		// BAS::anonymous::AccountTransaction result{.account_no = m_account_no,.transtext="",.amount=amount*m_factor};
		BAS::anonymous::AccountTransaction result{
			 .account_no = m_at.account_no
			,.transtext = m_at.transtext
			,.amount=static_cast<Amount>(round(amount*m_percent)/100.0)};
		return result;
	}
	int percent() const {return m_percent;}
private:
	BAS::anonymous::AccountTransaction m_at;
	int m_percent;
	friend std::ostream& operator<<(std::ostream& os,AccountTransactionTemplate const& att);
};
using AccountTransactionTemplates = std::vector<AccountTransactionTemplate>;

class JournalEntryTemplate {
public:

	JournalEntryTemplate(BAS::Series series,BAS::MetaEntry const& me) : m_series{series} {
		if (auto optional_gross_amount = to_gross_transaction_amount(me.defacto)) {
			auto gross_amount = *optional_gross_amount;
			if (gross_amount >= 0.01) {
				std::transform(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),std::back_inserter(templates),[gross_amount](BAS::anonymous::AccountTransaction const& at){
					AccountTransactionTemplate result{gross_amount,at};
					return result;
				});
				std::sort(this->templates.begin(),this->templates.end(),[](auto const& e1,auto const& e2){
					return (abs(e1.percent()) > abs(e2.percent())); // greater to lesser
				});
			}
			else {
				std::cout << "DESIGN INSUFFICIENY - JournalEntryTemplate constructor failed to determine gross amount";
			}
		}
	}

	BAS::Series series() const {return m_series;}

	BAS::anonymous::AccountTransactions operator()(Amount amount) const {
		BAS::anonymous::AccountTransactions result{};
		std::transform(templates.begin(),templates.end(),std::back_inserter(result),[amount](AccountTransactionTemplate const& att){
			return att(amount);
		});
		return result;
	}
	friend std::ostream& operator<<(std::ostream&, JournalEntryTemplate const&);
private:
	BAS::Series m_series;
	AccountTransactionTemplates templates{};
}; // class JournalEntryTemplate

// ==================================================================================
// Had -> journal_entry -> Template

using JournalEntryTemplateList = std::vector<JournalEntryTemplate>;
using OptionalJournalEntryTemplate = std::optional<JournalEntryTemplate>;

OptionalJournalEntryTemplate to_template(BAS::MetaEntry const& me) {
	OptionalJournalEntryTemplate result({me.meta.series,me});
	return result;
}

BAS::MetaEntry to_meta_entry(BAS::TypedMetaEntry const& tme) {
	BAS::MetaEntry result {
		.meta = tme.meta
		,.defacto = {
			.caption = tme.defacto.caption
			,.date = tme.defacto.date
		}
	};
	for (auto const& [at,props] : tme.defacto.account_transactions) {
		result.defacto.account_transactions.push_back(at);
	}
	return result;
}

OptionalJournalEntryTemplate to_template(BAS::TypedMetaEntry const& tme) {
	return to_template(to_meta_entry(tme));
}

BAS::MetaEntry to_journal_entry(HeadingAmountDateTransEntry const& had,JournalEntryTemplate const& jet) {
	BAS::MetaEntry result{};
	result.meta = {
		.series = jet.series()
	};
	result.defacto.caption = had.heading;
	result.defacto.date = had.date;
	result.defacto.account_transactions = jet(abs(had.amount)); // Ignore sign to have template apply its sign
	return result;
}

std::ostream& operator<<(std::ostream& os,AccountTransactionTemplate const& att) {
	os << "\n\t" << att.m_at.account_no << " " << att.m_percent;
	return os;
}

std::ostream& operator<<(std::ostream& os,JournalEntryTemplate const& entry) {
	os << "template: series " << entry.series();
	std::for_each(entry.templates.begin(),entry.templates.end(),[&os](AccountTransactionTemplate const& att){
		os << "\n\t" << att;
	});
	return os;
}

bool had_matches_trans(HeadingAmountDateTransEntry const& had,BAS::anonymous::JournalEntry const& aje) {
	return strings_share_tokens(had.heading,aje.caption);
}

// ==================================================================================

bool are_same_and_less_than_100_cents_apart(Amount const& a1,Amount const& a2) {
	bool result = (abs(abs(a1) - abs(a2)) < 1.0);
	return result;
}

BAS::MetaEntry swapped_ats_entry(BAS::MetaEntry const& me,BAS::anonymous::AccountTransaction const& target_at,BAS::anonymous::AccountTransaction const& new_at) {
	BAS::MetaEntry result{me};
	auto iter = std::find_if(result.defacto.account_transactions.begin(),result.defacto.account_transactions.end(),[&target_at](auto const& entry){
		return (entry.account_no == target_at.account_no);
	});
	if (iter != result.defacto.account_transactions.end()) {
		result.defacto.account_transactions.erase(iter);
		result.defacto.account_transactions.push_back(new_at);
	}
	else {
		std::cout << "\nswapped_ats_entry failed. Could not match target " << target_at << " with new_at " << new_at;
	}
	BAS::sort(result,BAS::has_greater_abs_amount);
	return result;
}

// #3
BAS::MetaEntry updated_amounts_entry(BAS::MetaEntry const& me,BAS::anonymous::AccountTransaction const& at) {
// std::cout << "\nupdated_amounts_entry";
// std::cout << "\nme:" << me;
// std::cout << "\nat:" << at;
	
	BAS::MetaEntry result{me};
	BAS::sort(result,BAS::has_greater_abs_amount);
// std::cout << "\npre-result:" << result;

	auto iter = std::find_if(result.defacto.account_transactions.begin(),result.defacto.account_transactions.end(),[&at](auto const& entry){
		return (entry.account_no == at.account_no);
	});
	auto at_index = std::distance(result.defacto.account_transactions.begin(),iter);
// std::cout << "\nat_index = " << at_index;
	if (iter == result.defacto.account_transactions.end()) {
		result.defacto.account_transactions.push_back(at);
		result = updated_amounts_entry(result,at); // recurse with added entry
	}
	else if (me.defacto.account_transactions.size()==4) {
// std::cout << "\n4 OK";
		// Assume 0: Transaction Amount, 1: Amount no VAT, 3: VAT, 4: rounding amount
		auto& trans_amount = result.defacto.account_transactions[0].amount;
		auto& ex_vat_amount = result.defacto.account_transactions[1].amount;
		auto& vat_amount = result.defacto.account_transactions[2].amount;
		auto& round_amount = result.defacto.account_transactions[3].amount;

		auto abs_trans_amount = abs(trans_amount);
		auto abs_ex_vat_amount = abs(ex_vat_amount);
		auto abs_vat_amount = abs(vat_amount);
		auto abs_round_amount = abs(round_amount);
		auto abs_at_amount = abs(at.amount);

		auto trans_amount_sign = static_cast<int>(trans_amount / abs(trans_amount));
		auto vat_sign = static_cast<int>(vat_amount/abs_vat_amount); // +-1
		auto at_sign = static_cast<int>(at.amount/abs_at_amount);

		auto vat_rate = static_cast<int>(round(abs_vat_amount*100/abs_ex_vat_amount));
// std::cout << "\nabs_vat_amount:" << abs_vat_amount << " abs_ex_vat_amount:" << abs_ex_vat_amount << " vat_rate:" << vat_rate;
		switch (vat_rate) {
			case 25:
			case 12:
			case 6: {
// std::cout << "\nVAT OK";
				switch (at_index) {
					case 0: {
						// Assume update gross transaction amount
// std::cout << "\nUpdate Gross Transaction Amount";
					} break;
					case 1: {
						// Assume update amount ex VAT
// std::cout << "\n Update Net Amount Ex VAT";
						abs_ex_vat_amount = abs_at_amount;
						abs_vat_amount = round(vat_rate*abs_ex_vat_amount)/100;

						ex_vat_amount = vat_sign*abs_ex_vat_amount;
						vat_amount = vat_sign*abs_vat_amount;
						round_amount = -1*round(100*(trans_amount + ex_vat_amount + vat_amount))/100;
					} break;
					case 2: {
						// Assume update VAT amount
// std::cout << "\n Update VAT";
					} break;
					case 3: {
						// Assume update the rounding amount
// std::cout << "\n Update Rounding";
						if (at.amount < 1.0) {
							// Assume a cent rounding amount
							// Automate the following algorithm.
							// 1. Assume the (Transaction amount + at.amount) is the "actual" adjusted (rounded) transaction amount
							// 4. Adjust Amount ex VAT and VAT to fit adjusted transaction amount
							round_amount = at.amount;
							// std::cout << "\ntrans_amount_sign " << trans_amount_sign;
							// std::cout << "\nvat_sign " << vat_sign; 
							auto adjusted_trans_amount = abs(trans_amount+round_amount);
							// std::cout << "\nadjusted_trans_amount " << trans_amount_sign*adjusted_trans_amount;
							// std::cout << "\nvat_rate " << vat_rate;
							auto reverse_vat_factor = vat_rate*1.0/(100+vat_rate); // E.g. 25/125 = 0.2
							// std::cout << "\nreverse_vat_factor " << reverse_vat_factor;
							vat_amount = vat_sign * round(reverse_vat_factor*100*adjusted_trans_amount)/100.0; // round to cents
							// std::cout << "\nvat_amount " << vat_amount;
							ex_vat_amount = vat_sign * round((1.0 - reverse_vat_factor)*100*adjusted_trans_amount)/100.0; // round to cents
							// std::cout << "\nex_vat_amount " << ex_vat_amount;
						}
					} break;
				}
			} break;
			default: {
// std::cout << "\nUnknown VAT rate ";
				// Unknown VAT rate
				// Do no adjustment
				// Leave to user to deal with this update
			}
		}
	}
	else {
		// Todo: Future needs may require adjusting transaction amounts to still sum upp to the transaction amount?
		// For now, handle as a simple "swap out" of the given amount
		*iter = at;
	}
// std::cout << "\nresult:" << result;
	return result;
}

struct OEnvironmentValueOStream {
	std::ostream& os;
};

class SRUEnvironment {
public:
	std::optional<std::string> at(SKV::SRU::AccountNo const& sru_code) {
		std::optional<std::string> result;
		if (m_sru_values.contains(sru_code)) result = m_sru_values[sru_code];
		return result;
	}
	void set(SKV::SRU::AccountNo const& sru_code,std::string value) {
		m_sru_values[sru_code] = value;
	}
private:
	SKV::SRU::SRUValueMap m_sru_values{};
	friend std::ostream& operator<<(std::ostream& os,SRUEnvironment const& sru_env);
	friend OEnvironmentValueOStream& operator<<(OEnvironmentValueOStream& env_val_os,SRUEnvironment const& sru_env);
};
std::ostream& operator<<(std::ostream& os,SRUEnvironment const& sru_env) {
	for (auto const& [sru_code,value]: sru_env.m_sru_values) {
		os << "\n\tSRU:" << sru_code << " = ";
		if (value) os << std::quoted(*value);
		else os << "null";
	}
	return os;
}

OEnvironmentValueOStream& operator<<(OEnvironmentValueOStream& env_val_os,std::string const& s) {
	env_val_os.os << s;
	return env_val_os;
}

OEnvironmentValueOStream& operator<<(OEnvironmentValueOStream& env_val_os,SRUEnvironment const& sru_env) {
	for (auto const& [sru_code,value]: sru_env.m_sru_values) {
		if (value) env_val_os << ";" << std::to_string(sru_code) << "=" << *value;
	}
	return env_val_os;
}

using SRUEnvironments = std::map<std::string,SRUEnvironment>;

struct Balance {
	BAS::AccountNo account_no;
	Amount opening_balance;
	Amount change;
	Amount end_balance;
};

std::ostream& operator<<(std::ostream& os,Balance const& balance) {
	os << balance.account_no << " ib:" << balance.opening_balance << " period:" << balance.change << " " << balance.end_balance;
	return os;
}

using Balances = std::vector<Balance>;
using BalancesMap = std::map<Date,Balances>;

std::ostream& operator<<(std::ostream& os,BalancesMap const& balances_map) {
	for (auto const& [date,balances] : balances_map) {
		os << date <<  " balances";
		for (auto const& balance : balances) {
			os << "\n\t" << balance;
		}
	}
	return os;
}

class SIEEnvironment {
public:
	SIE::OrgNr organisation_no{};
	SIE::FNamn organisation_name{};
	SIE::Adress organisation_address{};

	BASJournals& journals() {return m_journals;}
	BASJournals const& journals() const {return m_journals;}
	bool is_unposted(BAS::Series series, BAS::VerNo verno) const {
		bool result{true}; // deafult unposted
		if (verno_of_last_posted_to.contains(series)) result = (verno > this->verno_of_last_posted_to.at(series));
		return result;
	}
	SKV::SRU::OptionalAccountNo sru_code(BAS::AccountNo const& bas_account_no) {
		SKV::SRU::OptionalAccountNo result{};
		try {
			auto iter = std::find_if(account_metas().begin(),account_metas().end(),[&bas_account_no](auto const& entry){
				return (entry.first == bas_account_no);
			});
			if (iter != account_metas().end()) {
				result = iter->second.sru_code;
			}
		}
		catch (std::exception const& e) {} // Ignore/silence
		return result;
	}
	BAS::OptionalAccountNos to_bas_accounts(SKV::SRU::AccountNo const& sru_code) {
		BAS::OptionalAccountNos result{};
		try {
			BAS::AccountNos bas_account_nos{};
			std::for_each(account_metas().begin(),account_metas().end(),[&sru_code,&bas_account_nos](auto const& entry){
				if (entry.second.sru_code == sru_code) bas_account_nos.push_back(entry.first);
			});
			if (bas_account_nos.size() > 0) result = bas_account_nos;
		}
		catch (std::exception const& e) {
			std::cout << "\nto_bas_accounts failed. Exception=" << std::quoted(e.what());
		}
		return result;
	}

	void post(BAS::MetaEntry const& me) {
    // std::cout << "\npost(" << me << ")"; 
		if (me.meta.verno) {
			m_journals[me.meta.series][*me.meta.verno] = me.defacto;
			verno_of_last_posted_to[me.meta.series] = *me.meta.verno;
		}
		else {
			std::cout << "\nSIEEnvironment::post failed - can't post an entry with null verno";
		}
	}
	std::optional<BAS::MetaEntry> stage(BAS::MetaEntry const& me) {
    // std::cout << "\nstage(" << me << ")"  << std::flush; 
		std::optional<BAS::MetaEntry> result{};
		if (does_balance(me.defacto)) {
			if (this->already_in_posted(me) == false) {
        result = this->add(me); // return entry if it is no longer unposted / staged for posting
      }
			else result = this->update(me);
		}
		else {
			std::cout << "\nSorry, Failed to stage. Entry Does not Balance";
		}
		return result;
	}

  // Try to stage all provided entries for posting
	BAS::MetaEntries stage(SIEEnvironment const& staged_sie_environment) {
    // std::cout << "\nstage(sie environment)"  << std::flush; 
		BAS::MetaEntries result{};
		for (auto const& [series,journal] : staged_sie_environment.journals()) {
			for (auto const& [verno,aje] : journal) {
				auto je = this->stage({{.series=series,.verno=verno},aje});
				if (!je) {
					result.push_back({{.series=series,.verno=verno},aje}); // no longer staged
				}
			}
		}
		return result;
	}
	BAS::MetaEntries unposted() const {
		// std::cout << "\nunposted()";
		BAS::MetaEntries result{};
    for (auto const& [series,journal] : this->m_journals) {
      for (auto const& [verno,je] : journal) {
        if (this->is_unposted(series,verno)) {
          BAS::MetaEntry bjer{
            .meta = {
              .series = series
              ,.verno = verno
            }
            ,.defacto = je
          };
          result.push_back(bjer);
        }        
      }
    }
		return result;
	}

	BAS::AccountMetas const & account_metas() const {return BAS::detail::global_account_metas;} // const ref global instance

	void set_year_date_range(DateRange const& dr) {
		this->year_date_range = dr;
		// std::cout << "\nset_year_date_range <== " << *this->year_date_range;
	}

	void set_account_name(BAS::AccountNo bas_account_no ,std::string const& name) {
		if (BAS::detail::global_account_metas.contains(bas_account_no)) {
			if (BAS::detail::global_account_metas[bas_account_no].name != name) {
				std::cout << "\nWARNING: BAS Account " << bas_account_no << " name " << std::quoted(BAS::detail::global_account_metas[bas_account_no].name) << " changed to " << std::quoted(name);
			}
		}
		BAS::detail::global_account_metas[bas_account_no].name = name; // Mutate global instance
	}
	void set_account_SRU(BAS::AccountNo bas_account_no, SKV::SRU::AccountNo sru_code) {
		if (BAS::detail::global_account_metas.contains(bas_account_no)) {
			if (BAS::detail::global_account_metas[bas_account_no].sru_code) {
				if (*BAS::detail::global_account_metas[bas_account_no].sru_code != sru_code) {
					std::cout << "\nWARNING: BAS Account " << bas_account_no << " SRU Code " << *BAS::detail::global_account_metas[bas_account_no].sru_code << " changed to " << sru_code;
				}
			}
		}
		BAS::detail::global_account_metas[bas_account_no].sru_code = sru_code; // Mutate global instance
	}

	void set_opening_balance(BAS::AccountNo bas_account_no,Amount opening_balance) {
		if (this->opening_balance.contains(bas_account_no) == false) this->opening_balance[bas_account_no] = opening_balance;
		else {
			std::cout << "\nDESIGN INSUFFICIENCY - set_opening_balance failed. Balance for bas_account_no:" << bas_account_no;
			std::cout << " is already registered as " << this->opening_balance[bas_account_no] << ".";
			std::cout << " Provided opening_balance:" << opening_balance << " IGNORED";
		}
	}

	BalancesMap balances_at(Date date) {
		BalancesMap result{};
		for (auto const& ob : this->opening_balance) {
			// struct Balance {
			// 	BAS::AccountNo account_no;
			// 	Amount opening_balance;
			// 	Amount change;
			// 	Date date;
			// 	Amount end_balance;
			// };				
			result[date].push_back(Balance{
				.account_no = ob.first
				,.opening_balance = ob.second
				,.change = -1
				,.end_balance = -1
			});
		}
		return result;
	}

	OptionalDateRange fiscal_year_date_range() const {
		return this->year_date_range;
	}

  OptionalAmount opening_balance_of(BAS::AccountNo bas_account_no) {
    OptionalAmount result{};
    if (this->opening_balance.contains(bas_account_no)) {
      result = this->opening_balance.at(bas_account_no);
    }
    return result;
  }

  std::map<BAS::AccountNo,Amount> const& opening_balances() const {
    return this->opening_balance;
  }
	
private:
	BASJournals m_journals{};
	OptionalDateRange year_date_range{};
	std::map<char,BAS::VerNo> verno_of_last_posted_to{};
	std::map<BAS::AccountNo,Amount> opening_balance{};

	BAS::MetaEntry add(BAS::MetaEntry me) {
    // std::cout << "\nadd(" << me << ")"  << std::flush; 
		BAS::MetaEntry result{me};
		// Ensure a valid series
		if (me.meta.series < 'A' or 'M' < me.meta.series) {
			me.meta.series = 'A';
			std::cout << "\nadd(me) assigned series 'A' to entry with no series assigned";
		}
		// Assign "actual" sequence number
		auto verno = largest_verno(me.meta.series) + 1;
    // std::cout << "\n\tSetting actual ver no:" << verno;
		result.meta.verno = verno;
    if (m_journals[me.meta.series].contains(verno) == false) {
		  m_journals[me.meta.series][verno] = me.defacto;
    }
    else {
      std::cout << "\nDESIGN INSUFFICIENCY: Ignored adding new voucher with already existing ID " << me.meta.series << verno;
    }
		return result;
	}

	BAS::MetaEntry update(BAS::MetaEntry const& me) {
    // std::cout << "\nupdate(" << me << ")" << std::flush; 
		BAS::MetaEntry result{me};
		if (me.meta.verno and *me.meta.verno > 0) {
			auto journal_iter = m_journals.find(me.meta.series);
			if (journal_iter != m_journals.end()) {
				if (me.meta.verno) {
					auto entry_iter = journal_iter->second.find(*me.meta.verno);
					if (entry_iter != journal_iter->second.end()) {
						entry_iter->second = me.defacto; // update
            // std::cout << "\nupdated :" << entry_iter->second;
            // std::cout << "\n    --> :" << me;
					}
				}
			}
		}
		return result;
	}
	BAS::VerNo largest_verno(BAS::Series series) {
		auto const& journal = m_journals[series];
		return std::accumulate(journal.begin(),journal.end(),unsigned{},[](auto acc,auto const& entry){
			return (acc<entry.first)?entry.first:acc;
		});
	}
	bool already_in_posted(BAS::MetaEntry const& me) {
		bool result{false};
		if (me.meta.verno and *me.meta.verno > 0) {
			auto journal_iter = m_journals.find(me.meta.series);
			if (journal_iter != m_journals.end()) {
				if (me.meta.verno) {
					auto entry_iter = journal_iter->second.find(*me.meta.verno);
					result = (entry_iter != journal_iter->second.end());
				}
			}
		}
		return result;
	}
}; // class SIEEnvironment

using OptionalSIEEnvironment = std::optional<SIEEnvironment>;
using SIEEnvironments = std::map<std::string,SIEEnvironment>;

BAS::AccountMetas matches_bas_or_sru_account_no(BAS::AccountNo const& to_match_account_no,SIEEnvironment const& sie_env) {
	BAS::AccountMetas result{};
	// Assume match to BAS account or SRU account 
	// for (auto const& [account_no,am] : model->sie["current"].account_metas()) {
	// 	if ((*to_match_account_no == account_no) or (am.sru_code and (*to_match_account_no == *am.sru_code))) {
	// 		prompt << "\n  " << account_no << " " << std::quoted(am.name);
	// 		if (am.sru_code) prompt << " SRU:" << *am.sru_code;
	// 	}
	// }
	for (auto const& [account_no,am] : sie_env.account_metas()) {
		if ((to_match_account_no == account_no) or (am.sru_code and (to_match_account_no == *am.sru_code))) {
			result[account_no] = am;
		}
	}
	return result;
}

BAS::AccountMetas matches_bas_account_name(std::string const& s,SIEEnvironment const& sie_env) {
	BAS::AccountMetas result{};
	// Assume match to account name
	// for (auto const& [account_no,am] : model->sie["current"].account_metas()) {
	// 	if (first_in_second_case_insensitive(ast[1],am.name)) {
	// 		prompt << "\n  " << account_no << " " << std::quoted(am.name);
	// 		if (am.sru_code) prompt << " SRU:" << *am.sru_code;
	// 	}
	// }
	for (auto const& [account_no,am] : sie_env.account_metas()) {
		if (first_in_second_case_insensitive(s,am.name)) {
			result[account_no] = am;
		}
	}
	return result;
}

void for_each_anonymous_journal_entry(SIEEnvironment const& sie_env,auto& f) {
	for (auto const& [journal_id,journal] : sie_env.journals()) {
		for (auto const& [verno,aje] : journal) {
			f(aje);
		}
	}
}

void for_each_anonymous_journal_entry(SIEEnvironments const& sie_envs,auto& f) {
	for (auto const& [year_id,sie_env] : sie_envs) {
		for_each_anonymous_journal_entry(sie_env,f);
	}
}

void for_each_meta_entry(SIEEnvironment const& sie_env,auto& f) {
	for (auto const& [series,journal] : sie_env.journals()) {
		for (auto const& [verno,aje] : journal) {
			f(BAS::MetaEntry{.meta ={.series=series,.verno=verno,.unposted_flag=sie_env.is_unposted(series,verno)},.defacto=aje});
		}
	}
}

void for_each_meta_entry(SIEEnvironments const& sie_envs,auto& f) {
	for (auto const& [year_id,sie_env] : sie_envs) {
		for_each_meta_entry(sie_env,f);
	}
}

void for_each_anonymous_account_transaction(BAS::anonymous::JournalEntry const& aje,auto& f) {
	for (auto const& at : aje.account_transactions) {
		f(at);
	}
}

void for_each_anonymous_account_transaction(SIEEnvironment const& sie_env,auto& f) {
	auto f_caller = [&f](BAS::anonymous::JournalEntry const& aje){for_each_anonymous_account_transaction(aje,f);};
	for_each_anonymous_journal_entry(sie_env,f_caller);
}

void for_each_meta_account_transaction(BAS::MetaEntry const& me,auto& f) {
	for (auto const& at : me.defacto.account_transactions) {
		f(BAS::MetaAccountTransaction{
			.meta = me
			,.defacto = at
		});
	}
}

void for_each_meta_account_transaction(SIEEnvironment const& sie_env,auto& f) {
	auto f_caller = [&f](BAS::MetaEntry const& me){for_each_meta_account_transaction(me,f);};
	for_each_meta_entry(sie_env,f_caller);
}

void for_each_meta_account_transaction(SIEEnvironments const& sie_envs,auto& f) {
	auto f_caller = [&f](BAS::MetaEntry const& me){for_each_meta_account_transaction(me,f);};
	for (auto const& [year_id,sie_env] : sie_envs) {
		for_each_meta_entry(sie_env,f_caller);
	}
}

OptionalAmount account_sum(SIEEnvironment const& sie_env,BAS::AccountNo account_no) {
	OptionalAmount result{};
	auto f = [&account_no,&result](BAS::anonymous::AccountTransaction const& at) {
		if (at.account_no == account_no) {
			if (!result) result = at.amount;
			else *result += at.amount;
		}
	};
	for_each_anonymous_account_transaction(sie_env,f);
	return result;
}

OptionalAmount to_ats_sum(SIEEnvironments const& sie_envs,BAS::AccountNos const& bas_account_nos) {
	OptionalAmount result{};
	try {
		Amount amount{};
		auto f = [&amount,&bas_account_nos](BAS::MetaAccountTransaction const& mat) {
			if (std::any_of(bas_account_nos.begin(),bas_account_nos.end(),[&mat](auto const&  bas_account_no){ return (mat.defacto.account_no==bas_account_no);})) {
				amount += mat.defacto.amount;
			}
		};
		for_each_meta_account_transaction(sie_envs,f);
		result = amount;
	}
	catch (std::exception const& e) {
		std::cout << "\nto_ats_sum failed. Excpetion=" << std::quoted(e.what());
	}
	return result;
}

std::optional<std::string> to_ats_sum_string(SIEEnvironments const& sie_envs,BAS::AccountNos const& bas_account_nos) {
	std::optional<std::string> result{};
	if (auto const& ats_sum = to_ats_sum(sie_envs,bas_account_nos)) result = to_string(*ats_sum);
	return result;
}

auto to_typed_meta_entry = [](BAS::MetaEntry const& me) -> BAS::TypedMetaEntry {
	// std::cout << "\nto_typed_meta_entry: " << me; 
	BAS::anonymous::TypedAccountTransactions typed_ats{};

  /*
  use the following tagging

  "gross" for the transaction with the whole transaction amount (balances debit and credit to one account)
  "vat" for a vat part amount
  "net" for ex VAT amount
  "transfer" For amount just "passing though" and account (a debit and credit in the same transaction = ending up being half the gross amount on each balancing side)
  "counter" (for non VAT amount that counters (balances) the gross amount)
  */

	if (auto optional_gross_amount = to_gross_transaction_amount(me.defacto)) {
		auto gross_amount = *optional_gross_amount; 
		// Direct type detection based on gross_amount and account meta data
		for (auto const& at : me.defacto.account_transactions) {
			if (round(abs(at.amount)) == round(gross_amount)) typed_ats[at].insert("gross");
			if (is_vat_account_at(at)) typed_ats[at].insert("vat");
			if (abs(at.amount) < 1) typed_ats[at].insert("cents");
			if (round(abs(at.amount)) == round(gross_amount / 2)) typed_ats[at].insert("transfer"); // 20240519 I no longer understand this? A transfer if half the gross? Strange?
		}

		// Ex vat amount Detection
		Amount ex_vat_amount{},vat_amount{};
		for (auto const& at : me.defacto.account_transactions) {
			if (!typed_ats.contains(at)) {
				// Not gross, Not VAT (above) => candidate for ex VAT
				ex_vat_amount += at.amount;
			}
			else if (typed_ats.at(at).contains("vat")) {
				vat_amount += at.amount;
			}
		}
    std::string net_or_counter_tag = (vat_amount != 0)?std::string{"net"}:std::string{"counter"};
		if (abs(round(abs(ex_vat_amount)) + round(abs(vat_amount)) - gross_amount) <= 1) {
			// ex_vat + vat within cents of gross
			// tag non typed ats as ex-vat
			for (auto const& at : me.defacto.account_transactions) {
				if (!typed_ats.contains(at)) {
					typed_ats[at].insert(net_or_counter_tag);
				}
			}
		}
	}
	else {
		std::cout << "\nDESIGN INSUFFICIENCY - to_typed_meta_entry failed to determine gross amount";
	}
	// Identify an EU Purchase journal entry
	// Example:
	// typed: A27 Direktinköp EU 20210914
	// 	 gross : "PlusGiro":1920 "" -6616.93
	// 	 eu_vat vat : "Utgående moms omvänd skattskyldighet, 25 %":2614 "Momsrapport (30)" -1654.23
	// 	 eu_vat vat : "Ingående moms":2640 "" 1654.23
	// 	 eu_purchase : "Varuvärde Inlöp annat EG-land (Momsrapport ruta 20)":9021 "Momsrapport (20)" 6616.93
	// 	 eu_purchase : "Motkonto Varuvärde Inköp EU/Import":9099 "Motkonto Varuvärde Inköp EU/Import" -6616.93
	// 	 gross : "Elektroniklabb - Verktyg och maskiner":1226 "Favero Assioma DUO-Shi" 6616.93
	Amount eu_vat_amount{},eu_purchase_amount{};
	for (auto const& at : me.defacto.account_transactions) {
		// Identify transactions to EU VAT and EU Purchase tagged accounts
		if (is_vat_returns_form_at(SKV::XML::VATReturns::EU_VAT_BOX_NOS,at)) {
			typed_ats[at].insert("eu_vat");
			eu_vat_amount = at.amount;
		}
		if (is_vat_returns_form_at(SKV::XML::VATReturns::EU_PURCHASE_BOX_NOS,at)) {
			typed_ats[at].insert("eu_purchase");
			eu_purchase_amount = at.amount;
		}
	}
	for (auto const& at : me.defacto.account_transactions) {
		// Identify counter transactions to EU VAT and EU Purchase tagged accounts
		if (at.amount == -eu_vat_amount) typed_ats[at].insert("eu_vat"); // The counter trans for EU VAT
		if ((first_digit(at.account_no) == 4 or first_digit(at.account_no) == 9) and (at.amount == -eu_purchase_amount)) typed_ats[at].insert("eu_purchase"); // The counter trans for EU Purchase
	}
	// Mark gross accounts for EU VAT transaction journal entry
	for (auto const& at : me.defacto.account_transactions) {
		// We expect two accounts left unmarked and they are the gross accounts
		if (!typed_ats.contains(at) and (abs(at.amount) == abs(eu_purchase_amount))) {
			typed_ats[at].insert("gross");
		}
	}

	// Finally add any still untyped at with empty property set
	for (auto const& at : me.defacto.account_transactions) {
		if (!typed_ats.contains(at)) typed_ats.insert({at,{}});
	}

	BAS::TypedMetaEntry result{
		.meta = me.meta
		,.defacto = {
			.caption = me.defacto.caption
			,.date = me.defacto.date
			,.account_transactions = typed_ats
		}
	};
	return result;
};

// Journal Entry VAT Type
enum class JournalEntryVATType {
	Undefined = -1
	,NoVAT = 0
	,SwedishVAT = 1
	,EUVAT = 2
	,VATReturns = 3
  ,VATClearing = 4 // VAT Returns Cleared by Swedish Skatteverket (SKV) 
  ,SKVInterest = 6
  ,SKVFee = 7
	,VATTransfer = 8 // VAT Clearing and Settlement in one Journal Entry
	,Unknown
// 	case 0: {
		// No VAT in candidate. 
// case 1: {
// 	// Swedish VAT detcted in candidate.
// case 2: {
// 	// EU VAT detected in candidate.
// case 3: {
		//  M2 "Momsrapport 2021-07-01 - 2021-09-30" 20210930
		// 	 vat = sort_code: 0x6 : "Utgående moms, 25 %":2610 "" 83300
		// 	 eu_vat vat = sort_code: 0x56 : "Utgående moms omvänd skattskyldighet, 25 %":2614 "" 1654.23
		// 	 vat = sort_code: 0x6 : "Ingående moms":2640 "" -1690.21
		// 	 vat = sort_code: 0x6 : "Debiterad ingående moms":2641 "" -849.52
		// 	 vat = sort_code: 0x6 : "Redovisningskonto för moms":2650 "" -82415
		// 	 cents = sort_code: 0x7 : "Öres- och kronutjämning":3740 "" 0.5
// case 4: {
	// 10  A2 "Utbetalning Moms från Skattekonto" 20210506
	// transfer = sort_code: 0x1 : "Avräkning för skatter och avgifter (skattekonto)":1630 "Utbetalning" -802
	// transfer = sort_code: 0x1 : "Avräkning för skatter och avgifter (skattekonto)":1630 "Momsbeslut" 802
	// transfer vat = sort_code: 0x16 : "Momsfordran":1650 "" -802
	// transfer = sort_code: 0x1 : "PlusGiro":1920 "" 802
};

std::ostream& operator<<(std::ostream& os,JournalEntryVATType const& vat_type) {
	switch (vat_type) {
		case JournalEntryVATType::Undefined: {os << "Undefined";} break;
		case JournalEntryVATType::NoVAT: {os << "No VAT";} break;
		case JournalEntryVATType::SwedishVAT: {os << "Swedish VAT";} break;
		case JournalEntryVATType::EUVAT: {os << "EU VAT";} break;
		case JournalEntryVATType::VATReturns: {os << "VAT Returns";} break;
    case JournalEntryVATType::VATClearing: {os << "VAT Returns Clearing";} break; // VAT Returns Cleared by Swedish Skatteverket (SKV) 
    case JournalEntryVATType::SKVInterest: {os << "SKV Interest";} break; // SKV applied interest to holding on the SKV account
    case JournalEntryVATType::SKVFee: {os << "SKV Fee";} break; // SKV applied a fee to be paied (e.g., for missing to leave a report before deadline)
		case JournalEntryVATType::VATTransfer: {os << "VAT Transfer";} break;

		case JournalEntryVATType::Unknown: {os << "Unknown";} break;

		default: os << "operator<< failed for vat_type with integer value " << static_cast<int>(vat_type);
	}
	return os;
}

JournalEntryVATType to_vat_type(BAS::TypedMetaEntry const& tme) {
	JournalEntryVATType result{JournalEntryVATType::Undefined};
	static bool const log{true};
	// Count each type of property (NOTE: Can be less than transaction count as they may overlap, e.g., two or more gross account transactions)
	std::map<std::string,unsigned int> props_counter{};
	for (auto const& [at,props] : tme.defacto.account_transactions) {
		for (auto const& prop : props) props_counter[prop]++;
	}
	// LOG
	if (log) {
		for (auto const& [prop,count] : props_counter) {
			std::cout << "\n" << std::quoted(prop) << " count:" << count; 
		}
	}
	// Calculate total number of properties (NOTE: Can be more that the transactions as e.g., vat and eu_vat overlaps)
	auto props_sum = std::accumulate(props_counter.begin(),props_counter.end(),unsigned{0},[](auto acc,auto const& entry){
		acc += entry.second;
		return acc;
	});
	// Identify what type of VAT the candidate defines
	if ((props_counter.size() == 1) and props_counter.contains("gross")) {
		result = JournalEntryVATType::NoVAT; // NO VAT (All gross i.e., a Debit and credit that is not a VAT and with the same amount)
		if (log) std::cout << "\nTemplate is an NO VAT, all gross amount transaction :)"; // gross,gross
	}
  else if ((props_counter.size() == 2) and props_counter.contains("gross") and props_counter.contains("counter")) {
		result = JournalEntryVATType::NoVAT; // NO VAT and only one gross and more than one counter gross
		if (log) std::cout << "\nTemplate is an NO VAT, gross + counter gross transaction :)"; // gross,counter...    
  }
	else if ((props_counter.size() == 3) and props_counter.contains("gross") and props_counter.contains("net") and props_counter.contains("vat") and !props_counter.contains("eu_vat")) {
		if (props_sum == 3) {
			if (log) std::cout << "\nTemplate is a SWEDISH PURCHASE/sale"; // (gross,net,vat);
			result = JournalEntryVATType::SwedishVAT; // Swedish VAT
		}
	}
	else if ((props_counter.size() == 4) and props_counter.contains("gross") and props_counter.contains("net") and props_counter.contains("vat") and props_counter.contains("cents") and !props_counter.contains("eu_vat")) {
		if (props_sum == 4) {
			if (log) std::cout << "\nTemplate is a SWEDISH PURCHASE/sale"; // (gross,net,vat);
			result = JournalEntryVATType::SwedishVAT; // Swedish VAT
		}
	}
	else if (
		(     (props_counter.contains("gross"))
			and (props_counter.contains("eu_purchase"))
			and (props_counter.contains("eu_vat")))) {
		result = JournalEntryVATType::EUVAT; // EU VAT
		if (log) std::cout << "\nTemplate is an EU PURCHASE :)"; // gross,gross,eu_vat,eu_vat,eu_purchase,eu_purchase
	}
	else if (std::all_of(props_counter.begin(),props_counter.end(),[](std::map<std::string,unsigned int>::value_type const& entry){ return (entry.first == "vat") or (entry.first == "eu_vat") or  (entry.first == "cents");})) {
		result = JournalEntryVATType::VATReturns; // All VATS (probably a VAT report)
	}
  else if (tme.defacto.account_transactions.size() == 2 and std::all_of(tme.defacto.account_transactions.begin(),tme.defacto.account_transactions.end(),[](auto const& tat){
      auto const& [at,props] = tat;
      return (at.account_no == 1630 or at.account_no == 2650); // SKV account updated with VAT, i.e., cleared
      // Note: Sometimes BAS account 1650 is used to book (and clear) VAT to be paied to SKV (or maybe yhen it is paied when the VAT Returns is created)?
    })) {
		result = JournalEntryVATType::VATClearing; // SKV account cleared against 2650
  }
	else if (std::all_of(props_counter.begin(),props_counter.end(),[](std::map<std::string,unsigned int>::value_type const& entry){ return (entry.first == "transfer") or (entry.first == "vat");})) {
		result = JournalEntryVATType::VATTransfer; // All transfer of vat (probably a VAT settlement with Swedish Tax Agency)
	}
  else if (tme.defacto.account_transactions.size() == 2 and std::all_of(tme.defacto.account_transactions.begin(),tme.defacto.account_transactions.end(),[](auto const& tat){
      auto const& [at,props] = tat;
      return (at.account_no == 8314 or at.account_no == 1630);
    })) {
    // One account 1630 (SKV tax account) and one account 8314 (tax free interest gain)
		result = JournalEntryVATType::SKVInterest; // SKV gained interest
	}
	else if (false) { // TODO 231105: Implement a criteria to identify an SKV Fee event
    // bokförs på konto 6992 övriga ej avdragsgilla kostnader
		result = JournalEntryVATType::SKVFee;
	}
  else if (false) { // TODO 20240516: Implement criteria and type to idendify tax free SKV interest loss
    // (at.account_no == 8423 or at.account_no == 1630);
  }
	else {
		if (log) std::cout << "\nFailed to recognise the VAT type";
	}
	return result;
}

void for_each_typed_meta_entry(SIEEnvironments const& sie_envs,auto& f) {
	auto f_caller = [&f](BAS::MetaEntry const& me) {
		auto tme = to_typed_meta_entry(me);
		f(tme);
	};
	for_each_meta_entry(sie_envs,f_caller);
}

using TypedMetaEntryMap = std::map<BAS::kind::AccountTransactionTypeTopology,std::vector<BAS::TypedMetaEntry>>; // AccountTransactionTypeTopology -> TypedMetaEntry
using MetaEntryTopologyMap = std::map<std::size_t,TypedMetaEntryMap>; // hash -> TypeMetaEntry
// TODO: Consider to make MetaEntryTopologyMap an unordered_map (as it is already a map from hash -> TypedMetaEntry)
//       All we should have to do is to define std::hash for this type to make std::unordered_map find it? 

MetaEntryTopologyMap to_meta_entry_topology_map(SIEEnvironments const& sie_envs) {
	MetaEntryTopologyMap result{};
	// Group on Type Topology
	MetaEntryTopologyMap meta_entry_topology_map{};
	auto h = [&result](BAS::TypedMetaEntry const& tme){
		auto types_topology = BAS::kind::to_types_topology(tme);
		auto signature = BAS::kind::to_signature(types_topology);
		result[signature][types_topology].push_back(tme);							
	};
	for_each_typed_meta_entry(sie_envs,h);
	return result;
}

struct TestResult {
	std::ostringstream prompt{"null"};
	bool failed{true};
};

std::ostream& operator<<(std::ostream& os,TestResult const& tr) {
	os << tr.prompt.str();
	return os;
}

// A typed sub-meta-entry is a subset of transactions of provided typed meta entry
// that are all of the same "type" and that all sums to zero (do balance) 
std::vector<BAS::TypedMetaEntry> to_typed_sub_meta_entries(BAS::TypedMetaEntry const& tme) {
	std::vector<BAS::TypedMetaEntry> result{};
	// TODO: When needed, identify sub-entries of typed account transactions that balance (sums to zero)
	result.push_back(tme); // For now, return input as the single sub-entry
	return result;
}

BAS::anonymous::TypedAccountTransactions to_alternative_tats(SIEEnvironments const& sie_envs,BAS::anonymous::TypedAccountTransaction const& tat) {
	BAS::anonymous::TypedAccountTransactions result{};
	result.insert(tat); // For now, return ourself as the only alternative
	return result;
}

bool operator==(BAS::TypedMetaEntry const& tme1,BAS::TypedMetaEntry const& tme2) {
	return (BAS::kind::to_types_topology(tme1) == BAS::kind::to_types_topology(tme2));
}

BAS::TypedMetaEntry to_tats_swapped_tme(BAS::TypedMetaEntry const& tme,BAS::anonymous::TypedAccountTransaction const& target_tat,BAS::anonymous::TypedAccountTransaction const& new_tat) {
	BAS::TypedMetaEntry result{tme};
	// TODO: Implement actual swap of tats
	return result;
}

BAS::OptionalMetaEntry to_meta_entry_candidate(BAS::TypedMetaEntry const& tme,Amount const& gross_amount) {
	BAS::OptionalMetaEntry result{};
	// TODO: Implement actual generation of a candidate using the provided typed meta entry and the gross amount
	auto order_code = BAS::kind::to_at_types_order(BAS::kind::to_types_topology(tme));
	BAS::MetaEntry me_candidate{
		.meta = tme.meta
		,.defacto = {
			.caption = tme.defacto.caption
			,.date = tme.defacto.date
			,.account_transactions = {}
		}
	};
	switch (order_code) {
		// <DETECTED TOPOLOGIES>
		// 	 eu_vat vat cents = sort_code: 0x567

		// 	 gross net vat = sort_code: 0x346
		case 0x346:
		// 	 gross net vat cents = sort_code: 0x3467
		case 0x3467: {
			// With Swedish VAT (with rounding)
			if (tme.defacto.account_transactions.size() == 3 or tme.defacto.account_transactions.size() == 4) {
				// One gross account + single counter {net,vat} and single rounding trans
				for (auto const& tat : tme.defacto.account_transactions) {
					switch (BAS::kind::to_at_types_order(tat.second)) {
						case 0x3: {
							// gross
							me_candidate.defacto.account_transactions.push_back({
								.account_no = tat.first.account_no
								,.transtext = tat.first.transtext
								,.amount = gross_amount
							});
						}; break;
						case 0x4: {
							// net
							me_candidate.defacto.account_transactions.push_back({
								.account_no = tat.first.account_no
								,.transtext = tat.first.transtext
								,.amount = static_cast<Amount>(gross_amount*0.8) // NOTE: Hard coded 25% VAT
							});
						}; break;
						case 0x6: {
							// VAT
							me_candidate.defacto.account_transactions.push_back({
								.account_no = tat.first.account_no
								,.transtext = tat.first.transtext
								,.amount = static_cast<Amount>(gross_amount*0.2) // NOTE: Hard coded 25% VAT
							});
						}; break;
						case 0x7: {
							// Cents
							me_candidate.defacto.account_transactions.push_back({
								.account_no = tat.first.account_no
								,.transtext = tat.first.transtext
								,.amount = static_cast<Amount>(0.0) // NOTE: No rounding here
								// NOTE: Applying a rounding scheme has to "guess" what to aim for.
								//       It seems some sellers aim at making the gross amount without cents.
								//       But I have seen Telia invoices with rounding although both gross and net amounts
								//       are with cents (go figure how that come?)
							});
						}; break;
					}
				}
				result = me_candidate;
			}
			else {
				// Multipple counter transaction aggretates not yet supported
			}
		} break;

		// 	 transfer gross cents = sort_code: 0x137
		// 	 gross vat cents = sort_code: 0x367
		// 	 vat cents = sort_code: 0x67

		// 	 eu_purchase gross eu_vat vat = sort_code: 0x2356
		case 0x2356: {
			// With EU VAT
			if (tme.defacto.account_transactions.size() == 6) {
				// One gross + one counter gross trasnaction (EU Transactions between countries happens without charging the buyer with VAT)
				// But to populate the VAT Returns form we need four more "fake" transactions
				// One "fake" EU VAT + a counter "fake" VAT transactions (zero VAT to pay for the buyer)
				// One "fake" EU Purchase + a counter EU Purchase (to not duble book the purchase in the buyers journal) 
				for (auto const& tat : tme.defacto.account_transactions) {
					switch (BAS::kind::to_at_types_order(tat.second)) {
						case 0x2: {
							// eu_purchase +/-
							me_candidate.defacto.account_transactions.push_back({
								.account_no = tat.first.account_no
								,.transtext = tat.first.transtext
								,.amount = (tat.first.amount<0)?-abs(gross_amount):abs(gross_amount)
							});
						} break;
						case 0x3: {
							// gross +/-
							me_candidate.defacto.account_transactions.push_back({
								.account_no = tat.first.account_no
								,.transtext = tat.first.transtext
								,.amount = (tat.first.amount<0)?-abs(gross_amount):abs(gross_amount)
							});
						} break;
						case 0x5: {
							// eu_vat +/-
							auto vat_amount = static_cast<Amount>(((tat.first.amount<0)?-1.0:1.0) * 0.2 * abs(gross_amount));
							me_candidate.defacto.account_transactions.push_back({
								.account_no = tat.first.account_no
								,.transtext = tat.first.transtext
								,.amount = vat_amount
							});
						} break;
						// NOTE: case 0x6: vat will hit the same transaction as the eu_vat tagged account trasnactiopn is also tagged vat ;)
					} // switch
				} // for ats
				result = me_candidate;
			}

		} break;

		// 	 gross = sort_code: 0x3
		case 0x3: {
			// With gross, counter gross
			if (tme.defacto.account_transactions.size() == 2) {
				for (auto const& tat : tme.defacto.account_transactions) {
					switch (BAS::kind::to_at_types_order(tat.second)) {
						case 0x3: {
							// gross +/-
							me_candidate.defacto.account_transactions.push_back({
								.account_no = tat.first.account_no
								,.transtext = tat.first.transtext
								,.amount = (tat.first.amount<0)?-abs(gross_amount):abs(gross_amount)
							});
						}; break;
					} // switch
				} // for ats
				result = me_candidate;
			}
			else {
				// Multipple gounter gross accounts not yet supportered
			}
		}

		// 	 gross net = sort_code: 0x34

		// 	 gross vat = sort_code: 0x36
		// 	 transfer = sort_code: 0x1
		// 	 transfer vat = sort_code: 0x16
	} // switch
	// result = to_meta_entry(tme); // Return with unmodified amounts!
	return result;
}

bool are_same_and_less_than_100_cents_apart(BAS::anonymous::AccountTransaction const& at1, BAS::anonymous::AccountTransaction const& at2) {
	return (     (at1.account_no == at2.account_no)
	         and (at1.transtext == at2.transtext)
					 and (are_same_and_less_than_100_cents_apart(at1.amount,at2.amount)));
}

bool are_same_and_less_than_100_cents_apart(BAS::anonymous::AccountTransactions const& ats1, BAS::anonymous::AccountTransactions const& ats2) {
	bool result{true};
	if (ats1.size() >= ats2.size()) {
		for (int i=0;i<ats1.size() and result;++i) {
			if (i<ats2.size()) {
				result = are_same_and_less_than_100_cents_apart(ats1[i],ats2[i]);
			}
			else {
				result = abs(ats1[i].amount) < 1.0; // Do not care about cents
			}
		}
	}
	else {
		return are_same_and_less_than_100_cents_apart(ats2,ats1); // Recurse with swapped arguments
	}
	return result;
}

bool are_same_and_less_than_100_cents_apart(BAS::MetaEntry const& me1, BAS::MetaEntry const& me2) {
	return (     	(me1.meta == me2.meta)
						and (me1.defacto.caption == me2.defacto.caption)
						and (me1.defacto.date == me2.defacto.date)
						and (are_same_and_less_than_100_cents_apart(me1.defacto.account_transactions,me2.defacto.account_transactions)));
}

TestResult test_typed_meta_entry(SIEEnvironments const& sie_envs,BAS::TypedMetaEntry const& tme) {
	TestResult result{};
	result.prompt << "test_typed_meta_entry=";
	auto sub_tmes = to_typed_sub_meta_entries(tme);
	for (auto const& sub_tme : sub_tmes) {
		for (auto const& tat : sub_tme.defacto.account_transactions) {
			auto alt_tats = to_alternative_tats(sie_envs,tat);
			for (auto const& alt_tat : alt_tats) {
				auto alt_tme = to_tats_swapped_tme(tme,tat,alt_tat);
				result.prompt << "\n\t\t" <<  "Swapped " << tat << " with " << alt_tat;
				// Test that we can do a roundtrip and get the alt_tme back
				auto gross_amount = std::accumulate(alt_tme.defacto.account_transactions.begin(),alt_tme.defacto.account_transactions.end(),Amount{0},[](auto acc, auto const& tat){
					if (tat.first.amount > 0) acc += tat.first.amount;
					return acc;
				});
				auto raw_alt_candidate = to_meta_entry(alt_tme); // Raw conversion
				auto alt_candidate = to_meta_entry_candidate(alt_tme,gross_amount); // Generate from gross amount
				if (alt_candidate and are_same_and_less_than_100_cents_apart(*alt_candidate,raw_alt_candidate)) {
					result.prompt << "\n\t\t" << "Success, are less that 100 cents apart :)!";
					result.prompt << "\n\t\t      raw: " << raw_alt_candidate;
					result.prompt << "\n\t\tgenerated: " << *alt_candidate;
					result.failed = false;
				}
				else {
					result.prompt << "\n\t\t" << "FAILED, ARE NOT SAME OR SAME BUT MORE THAN 100 CENTS APART";
					result.prompt << "\n\t\t      raw: " << raw_alt_candidate;
					if (alt_candidate) result.prompt << "\n\t\tgenerated: " << *alt_candidate;
					else result.prompt << "\n\t\tgenerated: " << " null";
				}
			}
		}
	}
	result.prompt << "\n\t\t" << " *DONE*";
	return result;
}

using AccountsTopologyMap = std::map<std::size_t,std::map<BAS::kind::BASAccountTopology,BAS::TypedMetaEntries>>;

AccountsTopologyMap to_accounts_topology_map(BAS::TypedMetaEntries const& tmes) {
	AccountsTopologyMap result{};
	auto g = [&result](BAS::TypedMetaEntry const& tme) {
		auto accounts_topology = BAS::kind::to_accounts_topology(tme);
		auto signature = BAS::kind::to_signature(accounts_topology);
		result[signature][accounts_topology].push_back(tme);
	};
	std::for_each(tmes.begin(),tmes.end(),g);
	return result;
}


struct GrossAccountTransactions {
	BAS::anonymous::AccountTransactions result;
	void operator()(BAS::anonymous::JournalEntry const& aje) {
		if (auto at = gross_account_transaction(aje)) {
			result.push_back(*at);
		}
	}
};

struct NetAccountTransactions {
	BAS::anonymous::AccountTransactions result;
	void operator()(BAS::anonymous::JournalEntry const& aje) {
		if (auto at = net_account_transaction(aje)) {
			result.push_back(*at);
		}
	}
};

struct VatAccountTransactions {
	BAS::anonymous::AccountTransactions result;
	void operator()(BAS::anonymous::JournalEntry const& aje) {
		if (auto at = vat_account_transaction(aje)) {
			result.push_back(*at);
		}
	}
};

BAS::anonymous::AccountTransactions to_gross_account_transactions(BAS::anonymous::JournalEntry const& aje) {
	GrossAccountTransactions ats{};
	ats(aje);
	return ats.result;
}

BAS::anonymous::AccountTransactions to_gross_account_transactions(SIEEnvironments const& sie_envs) {
	GrossAccountTransactions ats{};
	for_each_anonymous_journal_entry(sie_envs,ats);
	return ats.result;
}

BAS::anonymous::AccountTransactions to_net_account_transactions(SIEEnvironments const& sie_envs) {
	NetAccountTransactions ats{};
	for_each_anonymous_journal_entry(sie_envs,ats);
	return ats.result;
}

BAS::anonymous::AccountTransactions to_vat_account_transactions(SIEEnvironments const& sie_envs) {
	VatAccountTransactions ats{};
	for_each_anonymous_journal_entry(sie_envs,ats);
	return ats.result;
}

struct T2 {
	BAS::MetaEntry me;
	struct CounterTrans {
		BAS::AccountNo linking_account;
		BAS::MetaEntry me;
	};
	std::optional<CounterTrans> counter_trans{};
};
using T2s = std::vector<T2>;

using T2Entry = std::pair<BAS::MetaEntry,T2::CounterTrans>;
using T2Entries = std::vector<T2Entry>;

std::ostream& operator<<(std::ostream& os,T2Entry const& t2e) {
	os << t2e.first.meta;
	os << " <- " << t2e.second.linking_account << " ->";
	os << " " << t2e.second.me.meta;
	os << "\n 1:" << t2e.first;
	os << "\n 2:" << t2e.second.me;
	return os;
}

std::ostream& operator<<(std::ostream& os,T2Entries const& t2es) {
	for (auto const& t2e : t2es) os << "\n\n" << t2e;
	return os;
}

T2Entries to_t2_entries(T2s const& t2s) {
	T2Entries result{};
	for (auto const& t2 : t2s) {
		if (t2.counter_trans) result.push_back({t2.me,*t2.counter_trans});
	}
	return result;
}

struct CollectT2s {
	T2s t2s{};
	T2Entries result() const { 
		return to_t2_entries(t2s);
	}
	void operator() (BAS::MetaEntry const& me) {
		auto t2_iter = t2s.begin();
		for (;t2_iter != t2s.end();++t2_iter) {
			if (!t2_iter->counter_trans) {
				// No counter trans found yet
				auto at_iter1 = std::find_if(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),[&t2_iter](BAS::anonymous::AccountTransaction const& at1){
					auto  at_iter2 = std::find_if(t2_iter->me.defacto.account_transactions.begin(),t2_iter->me.defacto.account_transactions.end(),[&at1](BAS::anonymous::AccountTransaction const& at2){
						return (at1.account_no == at2.account_no) and (at1.amount == -at2.amount);
					});
					return (at_iter2 != t2_iter->me.defacto.account_transactions.end());
				});
				if (at_iter1 != me.defacto.account_transactions.end()) {
					// iter refers to an account transaction in me to the same account but a counter amount as in t2.je
					T2::CounterTrans counter_trans{.linking_account = at_iter1->account_no,.me = me};
					t2_iter->counter_trans = counter_trans;
					break;
				}
			}
		}
		if (t2_iter == t2s.end()) {
			t2s.push_back(T2{.me = me});
		}
	}
};

T2Entries t2_entries(SIEEnvironments const& sie_envs) {
	CollectT2s collect_t2s{};
	for_each_meta_entry(sie_envs,collect_t2s);
	return collect_t2s.result();
}

BAS::OptionalMetaEntry find_meta_entry(SIEEnvironment const& sie_env, std::vector<std::string> const& ast) {
	BAS::OptionalMetaEntry result{};
	try {
		if ((ast.size()==1) and (ast[0].size()>=2)) {
			// Assume A1,M13 etc as designation for the meta entry to find
			auto series = ast[0][0];
			auto s_verno = ast[0].substr(1);
			auto verno = std::stoi(s_verno);
			auto f = [&series,&verno,&result](BAS::MetaEntry const& me) {
				if (me.meta.series == series and me.meta.verno == verno) result = me;
			};
			for_each_meta_entry(sie_env,f);
		}
	}
	catch (std::exception const& e) {
		std::cout << "\nfind_meta_entry failed. Exception=" << std::quoted(e.what());
	}
	return result;
}

// SKV Electronic API (file formats for upload)

namespace SKV { // SKV

	int to_tax(Amount amount) {return to_double(trunc(amount));} // See https://www4.skatteverket.se/rattsligvagledning/2477.html?date=2014-01-01#section22-1
	int to_fee(Amount amount) {return to_double(trunc(amount));} 

	OptionalDateRange to_date_range(std::string const& period_id) {
		OptionalDateRange result{};
		try {
			auto today = to_today();
			const std::regex is_year_quarter("^\\s*\\d\\d-Q[1-4]\\s*$"); // YY-Q1..YY-Q4
			const std::regex is_anonymous_quarter("Q[1-4]"); // Q1..Q4
			if (period_id.size()==0) {
				// default to current quarter
				result = to_quarter_range(today);
			}
			else if (std::regex_match(period_id,is_year_quarter)) {
				// Create quarter range of given year YY-Qx
				auto current_century = static_cast<int>(today.year()) / 100;
				std::chrono::year year{current_century*100 + std::stoi(period_id.substr(0,2))};
				std::chrono::month end_month{3u * static_cast<unsigned>(period_id[4] - '0')};
				result = to_quarter_range(year/end_month/std::chrono::last);
			}
			else if (std::regex_match(period_id,is_anonymous_quarter)) {
				// Create quarter range of current year Qx
				std::chrono::month end_month{3u * static_cast<unsigned>(period_id[1]-'0')};
				result = to_quarter_range(today.year()/end_month/std::chrono::last);
			}
			else throw std::runtime_error(std::string{"Can't interpret period_id=\""} + period_id + "\"");
		}
		catch (std::exception const& e) {
			std::cout << "\nERROR, to_date_range failed. Exception = " << std::quoted(e.what());
		}
		return result;
	}

	struct ContactPersonMeta {
		std::string name{};
		std::string phone{};
		std::string e_mail{};
	};
	using OptionalContactPersonMeta = std::optional<ContactPersonMeta>;

	struct OrganisationMeta {
		std::string org_no{};
		std::vector<ContactPersonMeta> contact_persons{};
	};
	using OptionalOrganisationMeta = std::optional<OrganisationMeta>;

	namespace SRU { // SKV::SRU

		struct OStream {
			std::ostream& os;
			encoding::UTF8::ToUnicodeBuffer to_unicode_buffer{};
			operator bool() {return static_cast<bool>(os);}
		};

		OStream& operator<<(OStream& sru_os,char ch) {
			// Assume ch is a byte in an UTF-8 stream and convert it to ISO8859_1 charachter set in file to SKV
			// std::cout << " " << std::hex << static_cast<unsigned int>(ch) << std::dec;
			if (auto unicode = sru_os.to_unicode_buffer.push(ch)) {
				auto iso8859_ch = charset::ISO_8859_1::UnicodeToISO8859(*unicode);
				// std::cout << ":" << std::hex << static_cast<unsigned int>(iso8859_ch) << std::dec;
				sru_os.os.put(iso8859_ch);
			}
			return sru_os;
		}

		OStream& operator<<(OStream& sru_os,std::string s) {
			// std::cout << "\n" << std::quoted(s);
			for (char ch : s) {
				sru_os << ch; // Stream through operator<<(OStream& sieos,char ch) that will tarnsform utf-8 encoded Unicode, to char encoded ISO 8859-1
			}
			return sru_os;
		}


		// See https://skatteverket.se/download/18.96cca41179bad4b1aad958/1636640681760/SKV269_27.pdf
		// 1. #DATABESKRIVNING_START
		// 2. #PRODUKT SRU
		// 3. #MEDIAID (ej obligatorisk)
		// 4. #SKAPAD (ej obligatorisk)
		// 5. #PROGRAM (ej obligatorisk)
		// 6. #FILNAMN (en post)
		// 7. #DATABESKRIVNING_SLUT
		// 8. #MEDIELEV_START
		// 9. #ORGNR
		// 10. #NAMN
		// 11. #ADRESS (ej obligatorisk)
		// 12. #POSTNR
		// 13. #POSTORT
		// 14. #AVDELNING (ej obligatorisk)
		// 15. #KONTAKT (ej obligatorisk)
		// 16. #EMAIL (ej obligatorisk)
		// 17. #TELEFON (ej obligatorisk)
		// 18. #FAX (ej obligatorisk)
		// 19. #MEDIELEV_SLUT

		using SRUFileTagMap = std::map<std::string,std::string>;
		using SRUFileTagMaps = std::vector<SRUFileTagMap>;

		using Blankett = std::pair<SRUFileTagMap,SKV::SRU::SRUValueMap>;
		using Blanketter = std::vector<Blankett>;


		struct FilesMapping {
			SRUFileTagMap info{};
			Blanketter blanketter{};
		};
		using OptionalFilesMapping = std::optional<FilesMapping>;

		struct InfoOStream {
			OStream& sru_os;
			operator bool() {return static_cast<bool>(sru_os);}
		};

		struct BlanketterOStream {
			OStream& sru_os;
			operator bool() {return static_cast<bool>(sru_os);}
		};

		SRUFileTagMap to_example_info_sru_file() {
			SRUFileTagMap result{};
			return result;	
		}

		SRUFileTagMap to_example_blanketter_sru_file() {
			SRUFileTagMap result{};
			return result;
		}

		std::string to_tag(std::string const& tag,SRUFileTagMap const& tag_map) {
// std::cout << "\nto_tag" << std::flush;
			std::ostringstream os{};
			os << tag << " ";
			if (tag_map.contains(tag)) os << tag_map.at(tag);
			else os << "?" << tag << "?";
			return os.str();
		}

		InfoOStream& operator<<(InfoOStream& os,FilesMapping const& fm) {

// std::cout << "operator<<(InfoOStream& os" << std::flush;
			// See https://skatteverket.se/download/18.96cca41179bad4b1aad958/1636640681760/SKV269_27.pdf
				// INFO.SRU/INFOSRU

			// 1. #DATABESKRIVNING_START
			os.sru_os << "#DATABESKRIVNING_START"; // NOTE: Empty lines not allowed (so no new-line for first entry)

			// 2. #PRODUKT SRU
			os.sru_os << "\n" << "#PRODUKT SRU";

			// 3. #MEDIAID (ej obligatorisk)
				// #MEDIAID DISK_12

			// 4. #SKAPAD (ej obligatorisk)
				// #SKAPAD 20130428 174557

			// 5. #PROGRAM (ej obligatorisk)
			os.sru_os << "\n" << "#PROGRAM" << " " << "CRATCHIT (Github Open Source)";
				// #PROGRAM SRUDEKLARATION 1.4

			// 6. #FILNAMN (en post)
				// #FILNAMN blanketter.sru
			os.sru_os << "\n" << "#FILNAMN" << " " <<  "blanketter.sru";

			// 7. #DATABESKRIVNING_SLUT
			os.sru_os << "\n" << "#DATABESKRIVNING_SLUT";

			// 8. #MEDIELEV_START
			os.sru_os << "\n" << "#MEDIELEV_START";

			// 9. #ORGNR
				// #ORGNR 191111111111
			// os.sru_os << "\n" << "#ORGNR" << " " << "191111111111";
			os.sru_os << "\n"  << to_tag("#ORGNR",fm.info);


			// 10. #NAMN
				// #NAMN Databokföraren
			// os.sru_os << "\n" << "#NAMN" << " " << "Databokföraren";
			os.sru_os << "\n" << to_tag("#NAMN",fm.info);

			// 11. #ADRESS (ej obligatorisk)
				// #ADRESS BOX 159

			// 12. #POSTNR
				// #POSTNR 12345
			// os.sru_os << "\n" << "#POSTNR" << " " << "12345";
			os.sru_os << "\n" << to_tag("#POSTNR",fm.info);

			// 13. #POSTORT
				// #POSTORT SKATTSTAD
			// os.sru_os << "\n" << "#POSTORT" << " " << "SKATTSTAD";
			// os.sru_os << "\n" << "#POSTORT" << " " << "Järfälla";
			os.sru_os << "\n" << to_tag("#POSTORT",fm.info);
			
			// 14. #AVDELNING (ej obligatorisk)
				// #AVDELNING Ekonomi
			// 15. #KONTAKT (ej obligatorisk)
				// #KONTAKT KARL KARLSSON
			// 16. #EMAIL (ej obligatorisk)
				// #EMAIL kk@Databokföraren
			// 17. #TELEFON (ej obligatorisk)
				// #TELEFON 08-2121212
			// 18. #FAX (ej obligatorisk)
				// #FAX 08-1212121

			// 19. #MEDIELEV_SLUT
			os.sru_os << "\n" << "#MEDIELEV_SLUT";

			os.sru_os.os << std::flush;

			return os;
		}

		BlanketterOStream& operator<<(BlanketterOStream& os,FilesMapping const& fm) {
			
			for (int i=0;i<fm.blanketter.size();++i) {
				if (i>0) os.sru_os << "\n"; // NOTE: Empty lines not allowed (so no new-line for first entry)

				// Posterna i ett blankettblock måste förekomma i följande ordning:
				// 1. #BLANKETT
				// #BLANKETT N7-2013P1
				// os.sru_os << "#BLANKETT" << " " << "N7-2013P1"; 
				os.sru_os << to_tag("#BLANKETT",fm.blanketter[i].first);

				// 2. #IDENTITET
				// #IDENTITET 193510250100 20130426 174557
				// os.sru_os << "\n" << "#IDENTITET" << " " << "193510250100 20130426 174557"; 
				os.sru_os << "\n" << to_tag("#IDENTITET",fm.blanketter[i].first);

				// 3. #NAMN (ej obligatorisk)
				// #NAMN Kalle Andersson

				// 4. #SYSTEMINFO och #UPPGIFT i valfri ordning och antal, dock endast en #SYSTEMINFO
				// #SYSTEMINFO klarmarkerad 20130426 u. a.
				os.sru_os << "\n" << "#SYSTEMINFO" << " " << "u.a.";

				if (false) {
					// Hard coded example data
					SRUValueMap sru_value{};
					// #UPPGIFT 4530 169780001096
					sru_value[4530] = "169780001096";
					// #UPPGIFT 7011 20120201
					sru_value[7011] = "20120201";
					// #UPPGIFT 7012 20130131
					sru_value[7012] = "20130131";
					// #UPPGIFT 8580 Anders Andersson
					sru_value[8580] = "Anders Andersson";
					// #UPPGIFT 8585 20120315
					sru_value[8585] = "20120315";
					// #UPPGIFT 8346 1000
					sru_value[8346] = "1000";
					// #UPPGIFT 8345 1250
					sru_value[8345] = "1250";
					// #UPPGIFT 8344 50500
					sru_value[8344] = "50500";
					// #UPPGIFT 8343 89500
					sru_value[8343] = "89500";
					// #UPPGIFT 8342 12500
					sru_value[8342] = "12500";
					// #UPPGIFT 8341 8500
					sru_value[8341] = "8500";
					// #UPPGIFT 8340 2555
					sru_value[8340] = "2555";

					for (auto const& [account_no,value] : sru_value) {
						if (value) os.sru_os << "\n" << "#UPPGIFT" << " " << std::to_string(account_no) << " " << *value;
					}
				}

				for (auto const& [account_no,value] : fm.blanketter[i].second) {
					if (value) os.sru_os << "\n" << "#UPPGIFT" << " " << std::to_string(account_no) << " " << *value;
				}


				// 5. #BLANKETTSLUT
				// #BLANKETTSLUT
				os.sru_os << "\n" << "#BLANKETTSLUT";
			}
			// Filen avslutas med #FIL_SLUT
			os.sru_os << "\n" << "#FIL_SLUT";

			// #FIL_SLUT
			return os;
		}

		namespace K10 { // SKV::SRU::K10

			OptionalFilesMapping to_files_mapping() {
				OptionalFilesMapping result{};
				try {
					FilesMapping fm {
						.info = to_example_info_sru_file()
					};
					// Blankett blankett{SRUFileTagMaps{},SKV::SRU::SRUValueMap{}}; 
					Blankett blankett{to_example_blanketter_sru_file(),SKV::SRU::SRUValueMap{}}; 
					fm.blanketter.push_back(blankett);
					result = fm;
				}
				catch (std::exception const& e) {
					std::cout << "\nDESIGN INSUFFICIENCY: to_files_mapping failed. Excpetion=" << std::quoted(e.what());
				}
				return result;
			}

		} // namespace // SKV::SRU::K10
	} // namespace // SKV::SRU

	namespace XML { // SKV::XML

		struct DeclarationMeta {
			std::string creation_date_and_time{}; // e.g.,2021-01-30T07:42:25
			std::string declaration_period_id{}; // e.g., 202101
		};
		using OptionalDeclarationMeta = std::optional<DeclarationMeta>;

		struct TaxDeclaration {
			std::string employee_12_digit_birth_no{}; // e.g., 198202252386
			Amount paid_employer_fee{};
			Amount paid_tax{};
		};
		using TaxDeclarations = std::vector<TaxDeclaration>;

		std::string to_12_digit_orgno(std::string generic_org_no) {
			std::string result{};
			// The SKV XML IT-system requires 12 digit organisation numbers with digits only
			// E.g., SIE-file organisation XXXXXX-YYYY has to be transformed into 16XXXXXXYYYY
			// See https://sv.wikipedia.org/wiki/Organisationsnummer
			std::string sdigits = filtered(generic_org_no,::isdigit);
			switch (sdigits.size()) {
				case 10: result = std::string{"16"} + sdigits; break;
				case 12: result = sdigits; break;
				default: throw std::runtime_error(std::string{"ERROR: to_12_digit_orgno failed, invalid input generic_org_no:"} + "\"" + generic_org_no + "\""); break;
			}
			return result;
		}

		struct TagValuePair {
			std::string tag{};
			std::string value;
		};

		std::string to_value(XMLMap const& xml_map,std::string tag) {
			if (xml_map.contains(tag) and xml_map.at(tag).size() > 0) {
				return xml_map.at(tag);
			}
			else throw std::runtime_error(std::string{"to_string failed, no value for tag:"} + tag);
		}

		XMLMap::value_type to_entry(XMLMap const& xml_map,std::string tag) {
			auto iter = xml_map.find(tag);
			if (iter != xml_map.end()) {
				// std::cout << "\nto_entry[" << std::quoted(iter->first) << "] = " << std::quoted(iter->second);
				return *iter;
			}
			else throw std::runtime_error(std::string{"to_entry failed, tag:"} + tag + " not defined");
		}
		
		struct EmployerDeclarationOStream {
			std::ostream& os;
		};

		EmployerDeclarationOStream& operator<<(EmployerDeclarationOStream& edos,std::string const& s) {
			auto& os = edos.os;
			os << s;
			return edos;
		}

		EmployerDeclarationOStream& operator<<(EmployerDeclarationOStream& edos,XMLMap::value_type const& entry) {
			Key::Path p{entry.first};
			std::string indent(p.size(),' ');
			edos << indent << "<" << p.back() << ">" << entry.second << "</" << p.back() << ">";
			return edos;
		}

		EmployerDeclarationOStream& operator<<(EmployerDeclarationOStream& edos,XMLMap const& xml_map) {
			try {
				Key::Path p{};
				// IMPORTANT: No empty line (nor any white space) allowed before the "<?xml..." tag! *sigh*
				edos << R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?>)";
				edos << "\n" << R"(<Skatteverket omrade="Arbetsgivardeklaration")";
					p += "Skatteverket";
					edos << "\n" << R"(xmlns="http://xmls.skatteverket.se/se/skatteverket/da/instans/schema/1.1")";
					edos << "\n" << R"(xmlns:agd="http://xmls.skatteverket.se/se/skatteverket/da/komponent/schema/1.1")";
					edos << "\n" << R"(xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://xmls.skatteverket.se/se/skatteverket/da/instans/schema/1.1 http://xmls.skatteverket.se/se/skatteverket/da/arbetsgivardeklaration/arbetsgivardeklaration_1.1.xsd">)";

					edos << "\n" << R"(<agd:Avsandare>)";
						p += "agd:Avsandare";
						edos << "\n" << to_entry(xml_map,p + "agd:Programnamn");
						edos << "\n" << to_entry(xml_map,p + "agd:Organisationsnummer");
						edos << "\n" << R"(<agd:TekniskKontaktperson>)";
							p += "agd:TekniskKontaktperson";
							edos << "\n" << to_entry(xml_map,p + "agd:Namn");
							edos << "\n" << to_entry(xml_map,p + "agd:Telefon");
							edos << "\n" << to_entry(xml_map,p + "agd:Epostadress");
							--p;
						edos << "\n" << R"(</agd:TekniskKontaktperson>)";
						edos << "\n" << to_entry(xml_map,p + "agd:Skapad");
						--p;
					edos << "\n" << R"(</agd:Avsandare>)";

					edos << "\n" << R"(<agd:Blankettgemensamt>)";
						p += "agd:Blankettgemensamt";
						edos << "\n" << R"(<agd:Arbetsgivare>)";
							p += "agd:Arbetsgivare";
							edos << "\n" << to_entry(xml_map,p + "agd:AgRegistreradId");
							edos << "\n" << R"(<agd:Kontaktperson>)";
								p += "agd:Kontaktperson";
								edos << "\n" << to_entry(xml_map,p + "agd:Namn");
								edos << "\n" << to_entry(xml_map,p + "agd:Telefon");
								edos << "\n" << to_entry(xml_map,p + "agd:Epostadress");
								--p;
							edos << "\n" << R"(</agd:Kontaktperson>)";
							--p;
						edos << "\n" << R"(</agd:Arbetsgivare>)";
						--p;
					edos << "\n" << R"(</agd:Blankettgemensamt>)";

					edos << "\n" << R"(<!-- Uppgift 1 HU -->)";
					edos << "\n" << R"(<agd:Blankett>)";
						p += "agd:Blankett";
						edos << "\n" << R"(<agd:Arendeinformation>)";
							p += "agd:Arendeinformation";
							edos << "\n" << to_entry(xml_map,p + "agd:Arendeagare");
							edos << "\n" << to_entry(xml_map,p + "agd:Period");
							--p;
						edos << "\n" << R"(</agd:Arendeinformation>)";
						edos << "\n" << R"(<agd:Blankettinnehall>)";
							p += "agd:Blankettinnehall";
							edos << "\n" << R"(<agd:HU>)";
								p += "agd:HU";
								edos << "\n" << R"(<agd:ArbetsgivareHUGROUP>)";
									p += "agd:ArbetsgivareHUGROUP";
									edos << "\n" << R"(<agd:AgRegistreradId faltkod="201">)" << to_value(xml_map,p + R"(agd:AgRegistreradId faltkod="201")") << "</agd:AgRegistreradId>";
									--p;
								edos << "\n" << R"(</agd:ArbetsgivareHUGROUP>)";
								edos << "\n" << R"(<agd:RedovisningsPeriod faltkod="006">)" << to_value(xml_map,p + R"(agd:RedovisningsPeriod faltkod="006")") << "</agd:RedovisningsPeriod>";
								edos << "\n" << R"(<agd:SummaArbAvgSlf faltkod="487">)" <<  to_value(xml_map,p + R"(agd:SummaArbAvgSlf faltkod="487")") << "</agd:SummaArbAvgSlf>";
								edos << "\n" << R"(<agd:SummaSkatteavdr faltkod="497">)" <<  to_value(xml_map,p + R"(agd:SummaSkatteavdr faltkod="497")") << "</agd:SummaSkatteavdr>";
								--p;
							edos << "\n" << R"(</agd:HU>)";
							--p;
						edos << "\n" << R"(</agd:Blankettinnehall>)";
						--p;
					edos << "\n" << R"(</agd:Blankett>)";

					edos << "\n" << R"(<!-- Uppgift 1 IU -->)";
					edos << "\n" << R"(<agd:Blankett>)";
						p += "agd:Blankett";
						edos << "\n" << R"(<agd:Arendeinformation>)";
							p += "agd:Arendeinformation";
							edos << "\n" << to_entry(xml_map,p + "agd:Arendeagare");
							edos << "\n" << to_entry(xml_map,p + "agd:Period");
							--p;
						edos << "\n" << R"(</agd:Arendeinformation>)";
						edos << "\n" << R"(<agd:Blankettinnehall>)";
							p += "agd:Blankettinnehall";
							edos << "\n" << R"(<agd:IU>)";
								p += "agd:IU";
								edos << "\n" << R"(<agd:ArbetsgivareIUGROUP>)";
									p += "agd:ArbetsgivareIUGROUP";
									edos << "\n" << R"(<agd:AgRegistreradId faltkod="201">)" << to_value(xml_map,p + R"(agd:AgRegistreradId faltkod="201")") << "</agd:AgRegistreradId>";
									--p;
								edos << "\n" << R"(</agd:ArbetsgivareIUGROUP>)";
								edos << "\n" << R"(<agd:BetalningsmottagareIUGROUP>)";
									p += "agd:BetalningsmottagareIUGROUP";
									edos << "\n" << R"(<agd:BetalningsmottagareIDChoice>)";
										p += "agd:BetalningsmottagareIDChoice";
										edos << "\n" << R"(<agd:BetalningsmottagarId faltkod="215">)" <<  to_value(xml_map,p + R"(agd:BetalningsmottagarId faltkod="215")") <<  "</agd:BetalningsmottagarId>";
										--p;
									edos << "\n" << R"(</agd:BetalningsmottagareIDChoice>)";
									--p;
								edos << "\n" << R"(</agd:BetalningsmottagareIUGROUP>)";
								edos << "\n" << R"(<agd:RedovisningsPeriod faltkod="006">)" <<  to_value(xml_map,p + R"(agd:RedovisningsPeriod faltkod="006")") << "</agd:RedovisningsPeriod>";
								edos << "\n" << R"(<agd:Specifikationsnummer faltkod="570">)" <<  to_value(xml_map,p + R"(agd:Specifikationsnummer faltkod="570")") << "</agd:Specifikationsnummer>";
								edos << "\n" << R"(<agd:AvdrPrelSkatt faltkod="001">)" <<  to_value(xml_map,p + R"(agd:AvdrPrelSkatt faltkod="001")") << "</agd:AvdrPrelSkatt>";
								--p;
							edos << "\n" << R"(</agd:IU>)";
							--p;
						edos << "\n" << R"(</agd:Blankettinnehall>)";
						--p;
					edos << "\n" << R"(</agd:Blankett>)";
					--p;
				edos << "\n" << R"(</Skatteverket>)";
			}
			catch (std::exception const& e) {
				std::cout << "\nERROR: Failed to generate skv-file, excpetion=" << e.what();
				edos.os.setstate(std::ostream::badbit);
			}
			return edos;
		}

		bool to_employee_contributions_and_PAYE_tax_return_file(std::ostream& os,XMLMap const& xml_map) {
			try {
				EmployerDeclarationOStream edos{os};
				edos << xml_map;
			}
			catch (std::exception const& e) {
				std::cout << "\nERROR: Failed to generate skv-file, excpetion=" << e.what();
			}
			return static_cast<bool>(os);
		}

		namespace VATReturns { // SKV::XML::VATReturns

			// An example provided by Swedish Tax Agency at https://www.skatteverket.se/download/18.3f4496fd14864cc5ac99cb2/1415022111801/momsexempel_v6.txt
			auto eskd_template_0 = R"(<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE eSKDUpload PUBLIC "-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV" "https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd">
<eSKDUpload Version="6.0">
  <OrgNr>599900-0465</OrgNr>
  <Moms>
    <Period>201507</Period>
    <ForsMomsEjAnnan>100000</ForsMomsEjAnnan>
    <UttagMoms>200000</UttagMoms>
    <UlagMargbesk>300000</UlagMargbesk>
    <HyrinkomstFriv>400000</HyrinkomstFriv>
    <InkopVaruAnnatEg>5000</InkopVaruAnnatEg>
    <InkopTjanstAnnatEg>6000</InkopTjanstAnnatEg>
    <InkopTjanstUtomEg>7000</InkopTjanstUtomEg>
    <InkopVaruSverige>8000</InkopVaruSverige>
    <InkopTjanstSverige>9000</InkopTjanstSverige>
    <MomsUlagImport>10000</MomsUlagImport>
    <ForsVaruAnnatEg>11000</ForsVaruAnnatEg>
    <ForsVaruUtomEg>12000</ForsVaruUtomEg>
    <InkopVaruMellan3p>13000</InkopVaruMellan3p>
    <ForsVaruMellan3p>14000</ForsVaruMellan3p>
    <ForsTjSkskAnnatEg>15000</ForsTjSkskAnnatEg>
    <ForsTjOvrUtomEg>16000</ForsTjOvrUtomEg>
    <ForsKopareSkskSverige>17000</ForsKopareSkskSverige>
    <ForsOvrigt>18000</ForsOvrigt>
    <MomsUtgHog>200000</MomsUtgHog>
    <MomsUtgMedel>15000</MomsUtgMedel>
    <MomsUtgLag>5000</MomsUtgLag>
    <MomsInkopUtgHog>2500</MomsInkopUtgHog>
    <MomsInkopUtgMedel>1000</MomsInkopUtgMedel>
    <MomsInkopUtgLag>500</MomsInkopUtgLag>
    <MomsImportUtgHog>2000</MomsImportUtgHog>
    <MomsImportUtgMedel>350</MomsImportUtgMedel>
    <MomsImportUtgLag>150</MomsImportUtgLag>
    <MomsIngAvdr>1000</MomsIngAvdr>
    <MomsBetala>225500</MomsBetala>
    <TextUpplysningMoms>Bla bla bla bla</TextUpplysningMoms>
  </Moms>
</eSKDUpload>)";

			// An example eskd-file generated by the online program Fortnox
			auto eskd_template_1 = R"(<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE eSKDUpload PUBLIC "-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV" "https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd">
<eSKDUpload Version="6.0">
  <OrgNr>556782-8172</OrgNr>
  <Moms>
    <Period>202109</Period>
    <ForsMomsEjAnnan>333200</ForsMomsEjAnnan>
    <InkopVaruAnnatEg>6616</InkopVaruAnnatEg>
    <MomsUtgHog>83300</MomsUtgHog>
    <MomsInkopUtgHog>1654</MomsInkopUtgHog>
    <MomsIngAvdr>2539</MomsIngAvdr>
    <MomsBetala>82415</MomsBetala>
  </Moms>
</eSKDUpload>)";

		// An example eskd-file generated by Swedish Tax Agency web service at https://www1.skatteverket.se/es/skapaeskdfil/tillStart.do
		auto eskd_template_2 = R"(<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE eSKDUpload PUBLIC "-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV" "https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd">
<eSKDUpload Version="6.0">
  <OrgNr>556782-8172</OrgNr>
  <Moms>
    <Period>202203</Period>
    <ForsMomsEjAnnan>333200</ForsMomsEjAnnan>
    <InkopVaruAnnatEg>6616</InkopVaruAnnatEg>
    <MomsUlagImport>597</MomsUlagImport>
    <ForsTjSkskAnnatEg>957</ForsTjSkskAnnatEg>
    <MomsUtgHog>83300</MomsUtgHog>
    <MomsInkopUtgHog>1654</MomsInkopUtgHog>
    <MomsImportUtgHog>149</MomsImportUtgHog>
    <MomsIngAvdr>2688</MomsIngAvdr>
    <MomsBetala>82415</MomsBetala>
  </Moms>
</eSKDUpload>)";

			// Deduced mapping between some VAT Return form box numbers and XML tag names to hold corresponding amount
			// Amount		VAT Return Box			XML Tag
			// 333200		05									"ForsMomsEjAnnan"
			// 83300		10									"MomsUtgHog"
			// 6616			20									"InkopVaruAnnatEg"
			// 1654			30									"MomsInkopUtgHog"
			// 957			39									"ForsTjSkskAnnatEg"
			// 2688			48									"MomsIngAvdr"
			// 82415		49									"MomsBetala"
			// 597			50									"MomsUlagImport"
			// 149			60									"MomsImportUtgHog"

			// Meta-data required to frame a VAT Returns form to SKV
			struct VatReturnsMeta {
				DateRange period;
				std::string period_to_declare; // VAT Returns period e.g., 202203 for Mars 2022 (also implying Q1 jan-mars)
			};
			using OptionalVatReturnsMeta = std::optional<VatReturnsMeta>;

			OptionalVatReturnsMeta to_vat_returns_meta(DateRange const& period) {
				OptionalVatReturnsMeta result{};
				try {
					std::string period_to_declare = to_string(period.end()); // YYYYMMDD
					result = {
						.period = period
						,.period_to_declare = period_to_declare.substr(0,6) // only YYYYMM
					};
				}
				catch (std::exception const& e) {
					std::cout << "ERROR, to_vat_returns_meta failed. Excpetion=" << std::quoted(e.what());
				}
				return result;
			}

			Amount to_form_sign(BoxNo box_no, Amount amount) {
				// All VAT Returns form amounts are without sign except box 49 where + means VAT to pay and - means VAT to "get back"
				switch (box_no) {
					// TODO: Consider it is in fact so that all amounts to SKV are to have the opposite sign of those in BAS?
					case 49: return -1 * amount; break; // The VAT Returns form must encode VAT to be paied as positive (opposite of how it is booked in BAS, negative meaning a debt to SKV)
					default: return abs(amount);
				}
			}

			std::ostream& operator<<(std::ostream& os,SKV::XML::VATReturns::FormBoxMap const& fbm) {
				for (auto const& [boxno,mats] : fbm) {
					auto mat_sum = BAS::mats_sum(mats);
					os << "\nVAT returns Form[" << boxno << "] = " << to_tax(to_form_sign(boxno,mat_sum)) << " (from sum " <<  mat_sum << ")";
					Amount rolling_amount{};
					for (auto const& mat : mats) {
						rolling_amount += mat.defacto.amount;
						os << "\n\t" << to_string(mat.meta.defacto.date) << " " << mat << " " << rolling_amount;
					}
				}
				return os;
			}

			struct OStream {
				std::ostream& os;
				operator bool() {return static_cast<bool>(os);}
			};

			OStream& operator<<(OStream& os,std::string const& s) {
				os.os << s;
				return os;
			}

			OStream& operator<<(OStream& os,XMLMap::value_type const& entry) {
				Key::Path p{entry.first};
				std::string indent(p.size(),' ');
				os << indent << "<" << p.back() << ">" << entry.second << "</" << p.back() << ">";
				return os;
			}

			OStream& operator<<(OStream& os,SKV::XML::XMLMap const& xml_map) {
				Key::Path p{};
				os << R"(<!DOCTYPE eSKDUpload PUBLIC "-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV" "https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd">)";
				os << "\n" << R"(<eSKDUpload Version="6.0">)";
				p += R"(eSKDUpload Version="6.0")";
				//   <OrgNr>556782-8172</OrgNr>
				os << "\n" << to_entry(xml_map,p + "OrgNr");
				os << "\n  " << R"(<Moms>)";
				p += R"(Moms)";
				// NOTE: It seems SKV requires the XML tags to come in a required sequence
				// See DTD file 
				// <!ELEMENT Moms (Period, ForsMomsEjAnnan?, UttagMoms?, UlagMargbesk?, HyrinkomstFriv?, InkopVaruAnnatEg?, InkopTjanstAnnatEg?, InkopTjanstUtomEg?, InkopVaruSverige?, InkopTjanstSverige?, MomsUlagImport?, ForsVaruAnnatEg?, ForsVaruUtomEg?, InkopVaruMellan3p?, ForsVaruMellan3p?, ForsTjSkskAnnatEg?, ForsTjOvrUtomEg?, ForsKopareSkskSverige?, ForsOvrigt?, MomsUtgHog?, MomsUtgMedel?, MomsUtgLag?, MomsInkopUtgHog?, MomsInkopUtgMedel?, MomsInkopUtgLag?, MomsImportUtgHog?, MomsImportUtgMedel?, MomsImportUtgLag?, MomsIngAvdr?, MomsBetala?, TextUpplysningMoms?)>
				//     <!ELEMENT Period (#PCDATA)>
				//     <!ELEMENT ForsMomsEjAnnan (#PCDATA)>
				//     <!ELEMENT UttagMoms (#PCDATA)>
				//     <!ELEMENT UlagMargbesk (#PCDATA)>
				//     <!ELEMENT HyrinkomstFriv (#PCDATA)>
				//     <!ELEMENT InkopVaruAnnatEg (#PCDATA)>
				//     <!ELEMENT InkopTjanstAnnatEg (#PCDATA)>
				//     <!ELEMENT InkopTjanstUtomEg (#PCDATA)>
				//     <!ELEMENT InkopVaruSverige (#PCDATA)>
				//     <!ELEMENT InkopTjanstSverige (#PCDATA)>
				//     <!ELEMENT MomsUlagImport (#PCDATA)>
				//     <!ELEMENT ForsVaruAnnatEg (#PCDATA)>
				//     <!ELEMENT ForsVaruUtomEg (#PCDATA)>
				//     <!ELEMENT InkopVaruMellan3p (#PCDATA)>
				//     <!ELEMENT ForsVaruMellan3p (#PCDATA)>
				//     <!ELEMENT ForsTjSkskAnnatEg (#PCDATA)>
				//     <!ELEMENT ForsTjOvrUtomEg (#PCDATA)>
				//     <!ELEMENT ForsKopareSkskSverige (#PCDATA)>
				//     <!ELEMENT ForsOvrigt (#PCDATA)>
				//     <!ELEMENT MomsUtgHog (#PCDATA)>
				//     <!ELEMENT MomsUtgMedel (#PCDATA)>
				//     <!ELEMENT MomsUtgLag (#PCDATA)>
				//     <!ELEMENT MomsInkopUtgHog (#PCDATA)>
				//     <!ELEMENT MomsInkopUtgMedel (#PCDATA)>
				//     <!ELEMENT MomsInkopUtgLag (#PCDATA)>
				//     <!ELEMENT MomsImportUtgHog (#PCDATA)>
				//     <!ELEMENT MomsImportUtgMedel (#PCDATA)>
				//     <!ELEMENT MomsImportUtgLag (#PCDATA)>
				//     <!ELEMENT MomsIngAvdr (#PCDATA)>
				//     <!ELEMENT MomsBetala (#PCDATA)>
				//     <!ELEMENT TextUpplysningMoms (#PCDATA)>

				//     <Period>202203</Period>
				os << "\n" << to_entry(xml_map,p + "Period");
				//     <ForsMomsEjAnnan>333200</ForsMomsEjAnnan>
				os << "\n" << to_entry(xml_map,p + "ForsMomsEjAnnan");
				//     <InkopVaruAnnatEg>6616</InkopVaruAnnatEg>
				os << "\n" << to_entry(xml_map,p + "InkopVaruAnnatEg");
				//     <MomsUlagImport>597</MomsUlagImport>
				os << "\n" << to_entry(xml_map,p + "MomsUlagImport");
				//     <ForsTjSkskAnnatEg>957</ForsTjSkskAnnatEg>
				os << "\n" << to_entry(xml_map,p + "ForsTjSkskAnnatEg");
				//     <MomsUtgHog>83300</MomsUtgHog>
				os << "\n" << to_entry(xml_map,p + "MomsUtgHog");
				//     <MomsInkopUtgHog>1654</MomsInkopUtgHog>
				os << "\n" << to_entry(xml_map,p + "MomsInkopUtgHog");
				//     <MomsImportUtgHog>149</MomsImportUtgHog>
				os << "\n" << to_entry(xml_map,p + "MomsImportUtgHog");
				//     <MomsIngAvdr>2688</MomsIngAvdr>
				os << "\n" << to_entry(xml_map,p + "MomsIngAvdr");
				//     <MomsBetala>82415</MomsBetala>
				os << "\n" << to_entry(xml_map,p + "MomsBetala");
				--p;
				os << "\n  " << R"(</Moms>)";
				--p;
				os << "\n" << R"(</eSKDUpload>)";
				return os;
			}

			std::map<BoxNo,std::string> BOXNO_XML_TAG_MAP {
				// Amount		VAT Return Box			XML Tag
				// 333200		05									"ForsMomsEjAnnan"
				{5,R"(ForsMomsEjAnnan)"}
				// 83300		10									"MomsUtgHog"
				,{10,R"(MomsUtgHog)"}
				// 6616			20									"InkopVaruAnnatEg"
				,{20,R"(InkopVaruAnnatEg)"}
				// 1654			30									"MomsInkopUtgHog"
				,{30,R"(MomsInkopUtgHog)"}
				// 957			39									"ForsTjSkskAnnatEg"
				,{39,R"(ForsTjSkskAnnatEg)"}
				// 2688			48									"MomsIngAvdr"
				,{48,R"(MomsIngAvdr)"}
				// 82415		49									"MomsBetala"
				,{49,R"(MomsBetala)"}
				// 597			50									"MomsUlagImport"
				,{50,R"(MomsUlagImport)"}
				// 149			60									"MomsImportUtgHog"
				,{60,R"(MomsImportUtgHog)"}
			};

			std::string to_xml_tag(BoxNo const& box_no) {
				std::string result{"??"};
				if (BOXNO_XML_TAG_MAP.contains(box_no)) {
					auto const& tag = BOXNO_XML_TAG_MAP.at(box_no);
					if (tag.size() > 0) result = tag;
					else throw std::runtime_error{std::string{"ERROR: to_xml_tag failed. tag for box_no:"} + std::to_string(box_no) + " of zero length"};  
				}
				else throw std::runtime_error{std::string{"ERROR: to_xml_tag failed. box_no:"} + std::to_string(box_no) + " not defined"};
				return result;
			}

			std::string to_11_digit_orgno(std::string generic_org_no) {
				std::string result{generic_org_no};
				switch (result.size()) {
					case 10: result = result.substr(0,7) + "-" + result.substr(7,4);
						break;
					case 11: if (result[6] != '-') throw std::runtime_error(std::string{"ERROR: to_11_digit_orgno failed, can't process org_no="} + generic_org_no);
						break;
					case 12: result = to_11_digit_orgno(result.substr(2)); // recurce with assumed prefix "16" removed
						break;							
					default: throw std::runtime_error(std::string{"ERROR: to_11_digit_orgno failed, wrong length org_no="} + generic_org_no);
						break;
				}
				return result;
			}

			Key::Paths account_vat_form_mapping() {
				Key::Paths result{};
				std::istringstream is{ACCOUNT_VAT_CSV};
				std::string row{};
				while (std::getline(is,row)) {
					result.push_back(Key::Path{row,';'});
				}
				return result;
			}

			BAS::AccountNos to_accounts(BoxNo box_no) {
				static auto const ps = account_vat_form_mapping();
				BAS::AccountNos result{};
				return std::accumulate(ps.begin(),ps.end(),BAS::AccountNos{},[&box_no](auto acc,Key::Path const& p){
					try {
						std::ostringstream os{};
						os << std::setfill('0') << std::setw(2) << box_no;
						if (p[2].find(os.str()) != std::string::npos) acc.push_back(std::stoi(p[0]));

					}
					catch (std::exception const& e) { std::cout << "\nDESIGN INSUFFICIENCY: to_accounts::lambda failed. Exception=" << std::quoted(e.what());}					
					return acc;
				});
				return result;
			}

			std::set<BAS::AccountNo> to_accounts(BoxNos const& box_nos) {
				std::set<BAS::AccountNo> result{};
				for (auto const& box_no : box_nos) {
					auto vat_account_nos = to_accounts(box_no);
					std::copy(vat_account_nos.begin(),vat_account_nos.end(),std::inserter(result,result.end()));
				}
				return result;
			}

			std::set<BAS::AccountNo> to_vat_accounts() {
				return to_accounts({10,11,12,30,31,32,60,61,62,48,49});
			}			

			BAS::MetaAccountTransactions to_mats(SIEEnvironment const& sie_env,auto const& matches_mat) {
				BAS::MetaAccountTransactions result{};
				auto x = [&matches_mat,&result](BAS::MetaAccountTransaction const& mat){
					if (matches_mat(mat)) result.push_back(mat);
				};
				for_each_meta_account_transaction(sie_env,x);
				return result;
			}

			BAS::MetaAccountTransactions to_mats(SIEEnvironments const& sie_envs,auto const& matches_mat) {
				BAS::MetaAccountTransactions result{};
				auto x = [&matches_mat,&result](BAS::MetaAccountTransaction const& mat){
					if (matches_mat(mat)) result.push_back(mat);
				};
				for_each_meta_account_transaction(sie_envs,x);
				return result;
			}

			std::optional<SKV::XML::XMLMap> to_xml_map(FormBoxMap const& vat_returns_form_box_map,SKV::OrganisationMeta const& org_meta,SKV::XML::DeclarationMeta const& form_meta) {
				std::optional<SKV::XML::XMLMap> result{};
				try {
					XMLMap xml_map{};
						// Amount		VAT Return Box			XML Tag
						// 333200		05									"ForsMomsEjAnnan"
						// 83300		10									"MomsUtgHog"
						// 6616			20									"InkopVaruAnnatEg"
						// 1654			30									"MomsInkopUtgHog"
						// 957			39									"ForsTjSkskAnnatEg"
						// 2688			48									"MomsIngAvdr"
						// 82415		49									"MomsBetala"
						// 597			50									"MomsUlagImport"
						// 149			60									"MomsImportUtgHog"
						Key::Path p{};
						// <!DOCTYPE eSKDUpload PUBLIC "-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV" "https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd">
						// <eSKDUpload Version="6.0">
						p += R"(eSKDUpload Version="6.0")";
						//   <OrgNr>556782-8172</OrgNr>
						xml_map[p+"OrgNr"] = to_11_digit_orgno(org_meta.org_no);
						//   <Moms>
						p += R"(Moms)";
						//     <Period>202203</Period>
						xml_map[p+"Period"] = form_meta.declaration_period_id;
						for (auto const& [box_no,mats] : vat_returns_form_box_map)  {
							xml_map[p+to_xml_tag(box_no)] = std::to_string(to_tax(to_form_sign(box_no,BAS::mats_sum(mats))));
						}
						//     <ForsMomsEjAnnan>333200</ForsMomsEjAnnan>
						//     <InkopVaruAnnatEg>6616</InkopVaruAnnatEg>
						//     <MomsUlagImport>597</MomsUlagImport>
						//     <ForsTjSkskAnnatEg>957</ForsTjSkskAnnatEg>
						//     <MomsUtgHog>83300</MomsUtgHog>
						//     <MomsInkopUtgHog>1654</MomsInkopUtgHog>
						//     <MomsImportUtgHog>149</MomsImportUtgHog>
						//     <MomsIngAvdr>2688</MomsIngAvdr>
						//     <MomsBetala>82415</MomsBetala>
						--p;
						//   </Moms>
						--p;
						// </eSKDUpload>;
						result = xml_map;
				}
				catch (std::exception const& e) {
					std::cout << "\nERROR, to_xml_map failed. Expection=" << std::quoted(e.what());
				}
				return result;
			}

			BAS::MetaAccountTransaction dummy_mat(Amount amount) {
				return {
					.meta = {
						.meta = {
							.series = 'X'
						}
						,.defacto = {
							.caption = "Dummy..."
						}
					}
					,.defacto = {
						.amount = amount
					}
				};
			}

			BAS::MetaAccountTransactions to_vat_returns_mats(BoxNo box_no,SIEEnvironments const& sie_envs,auto mat_predicate) {
				auto account_nos = to_accounts(box_no);
				return to_mats(sie_envs,[&mat_predicate,&account_nos](BAS::MetaAccountTransaction const& mat) {
					return (mat_predicate(mat) and is_any_of_accounts(mat,account_nos));
				});
			}

			Amount to_box_49_amount(FormBoxMap const& box_map) {
				BoxNos vat_box_nos{10,11,12,30,31,32,60,61,62,48};
				auto box_49_amount = std::accumulate(vat_box_nos.begin(),vat_box_nos.end(),Amount{},[&box_map](Amount acc,BoxNo box_no){
					if (box_map.contains(box_no)) {
						auto mats_sum = BAS::mats_sum(box_map.at(box_no));
						acc += mats_sum;
						std::cout << "\n\tto_box_49_amount += [" << box_no << "]:" << mats_sum << " = " << acc;
					}
					return acc;
				});
				return box_49_amount;
			}

			std::optional<FormBoxMap> to_form_box_map(SIEEnvironments const& sie_envs,auto mat_predicate) {
				std::optional<FormBoxMap> result{};
				try {
					FormBoxMap box_map{};
					// Amount		VAT Return Box			XML Tag
					// 333200		05									"ForsMomsEjAnnan"
					box_map[5] = to_vat_returns_mats(5,sie_envs,mat_predicate);
					// box_map[5].push_back(dummy_mat(333200));
					// 83300		10									"MomsUtgHog"
					box_map[10] = to_vat_returns_mats(10,sie_envs,mat_predicate);
					// box_map[10].push_back(dummy_mat(83300));
					// 6616			20									"InkopVaruAnnatEg"
					box_map[20] = to_vat_returns_mats(20,sie_envs,mat_predicate);
					// box_map[20].push_back(dummy_mat(6616));
					// 1654			30									"MomsInkopUtgHog"
					box_map[30] = to_vat_returns_mats(30,sie_envs,mat_predicate);
					// box_map[30].push_back(dummy_mat(1654));
					// 957			39									"ForsTjSkskAnnatEg"
					box_map[39] = to_vat_returns_mats(39,sie_envs,mat_predicate);
					// box_map[39].push_back(dummy_mat(957));
					// 2688			48									"MomsIngAvdr"
					box_map[48] = to_vat_returns_mats(48,sie_envs,mat_predicate);
					// box_map[48].push_back(dummy_mat(2688));
					// 597			50									"MomsUlagImport"
					box_map[50] = to_vat_returns_mats(50,sie_envs,mat_predicate);
					// box_map[50].push_back(dummy_mat(597));
					// 149			60									"MomsImportUtgHog"
					box_map[60] = to_vat_returns_mats(60,sie_envs,mat_predicate);
					// box_map[60].push_back(dummy_mat(149));

					// NOTE: Box 49, vat designation id R1, R2 is a  t a r g e t  account, NOT a source.
					box_map[49].push_back(dummy_mat(to_box_49_amount(box_map)));

					result = box_map;
				}
				catch (std::exception const& e) {
					std::cout << "\nERROR, to_form_box_map failed. Expection=" << std::quoted(e.what());
				}
				return result;
			}

			bool quarter_has_VAT_consilidation_entry(SIEEnvironments const& sie_envs,DateRange const& period) {
				bool result{};
				auto f = [&period,&result](BAS::TypedMetaEntry const& tme) {
					auto order_code = BAS::kind::to_at_types_order(BAS::kind::to_types_topology(tme));
					// gross vat cents = sort_code: 0x367  M1 Momsrapport 2020-04-01 - 2020-06-30 20200630
					// gross vat cents = sort_code: 0x367  M2 Momsrapport 2020-07-01 - 2020-09-30 20200930
					// gross vat cents = sort_code: 0x367  M3 Momsrapport 2020-10-01 - 2020-12-31 20201231
					// gross vat cents = sort_code: 0x367  M4 Momsrapport 2021-01-01 - 2021-03-31 20210331
					// gross vat cents = sort_code: 0x367  M1 Momsrapport 2021-04-01 - 2021-06-30 20210630
					// eu_vat vat cents = sort_code: 0x567 M2 Momsrapport 2021-07-01 - 2021-09-30 20210930
					// [ eu_vat vat = sort_code: 0x56] M3 "Momsrapport 20211001...20211230" 20211230

					// NOTE: A VAT consolidation entry will have a detectable gross VAT entry if we have no income to declare.
					if (period.contains(tme.defacto.date)) {
// std::cout << "\nquarter_has_VAT_consilidation_entry, scanning " << tme.meta.series;
// if (tme.meta.verno) std::cout << *tme.meta.verno;
// std::cout << " order_code:" << std::hex << order_code << std::dec;
						result = result or  (order_code == 0x56) or (order_code == 0x367) or (order_code == 0x567);
					}
				};
				for_each_typed_meta_entry(sie_envs,f);
				return result;
			}
			
			HeadingAmountDateTransEntries to_vat_returns_hads(SIEEnvironments const& sie_envs) {
				HeadingAmountDateTransEntries result{};
				try {
					// Create a had for last quarter if there is no journal entry in the M series for it
					// Otherwise create a had for current quarter
					auto today = to_today();
					auto current_quarter = to_quarter_range(today);
					auto previous_quarter = to_three_months_earlier(current_quarter);
					auto vat_returns_range = DateRange{previous_quarter.begin(),current_quarter.end()}; // previous and "current" two quarters
					// NOTE: By spanning previous and "current" quarters we can catch-up if we made any changes to prevuious quarter aftre having created the VAT returns consolidation
					// NOTE: making changes in a later VAT returns form for changes in previous one should be a low-crime offence?

					// Loop through quarters 
					for (int i=0;i<3;++i) {
// std::cout << "\nto_vat_returns_hads, checking vat_returns_range " << vat_returns_range;
						// Check three quartes back for missing VAT consilidation journal entry
						if (quarter_has_VAT_consilidation_entry(sie_envs,current_quarter) == false) {
							auto vat_returns_meta = to_vat_returns_meta(vat_returns_range);
							auto is_vat_returns_range = [&vat_returns_meta](BAS::MetaAccountTransaction const& mat){
								return vat_returns_meta->period.contains(mat.meta.defacto.date);
							};
							if (auto box_map = to_form_box_map(sie_envs,is_vat_returns_range)) {
								// box_map is an std::map<BoxNo,BAS::MetaAccountTransactions>
								// We need a per-account amount to counter (consolidate) into 1650 (VAT to get back) or 2650 (VAT to pay)
								// 2650 "Redovisningskonto för moms" SRU:7369
								// 1650 "Momsfordran" SRU:7261
  							std::cout << "\nPeriod:" << to_string(vat_returns_range.begin()) << "..." << to_string(vat_returns_range.end());
								std::map<BAS::AccountNo,Amount> account_amounts{};
								for (auto const& [box_no,mats] : *box_map)  {
									std::cout << "\n\t[" << box_no << "]";
									for (auto const& mat : mats) {
										account_amounts[mat.defacto.account_no] += mat.defacto.amount;
										std::cout << "\n\t\t" << to_string(mat.meta.defacto.date) << " account_amounts[" << mat.defacto.account_no << "] += " << mat.defacto.amount << " saldo:" << account_amounts[mat.defacto.account_no];
									}
								}

								// Valid amount if > 1.0 SEK (takes care of invalid entries caused by rounding errors)
								if (abs(account_amounts[0]) >= 1.0) {
									std::ostringstream heading{};
									heading << "Momsrapport " << current_quarter;
									HeadingAmountDateTransEntry had{
										.heading = heading.str()
										,.amount = account_amounts[0] // to_form_box_map uses a dummy transaction to account 0 for the net VAT
										,.date = vat_returns_range.end()
										,.optional = {
											.vat_returns_form_box_map_candidate = box_map
										}
									};
									result.push_back(had);
								}
							}
						}
						current_quarter = to_three_months_earlier(current_quarter);
						vat_returns_range = to_three_months_earlier(vat_returns_range);
					}
				}
				catch (std::exception const& e) {
					std::cout << "\nERROR: to_vat_returns_had failed. Excpetion = " << std::quoted(e.what());
				}
				return result;
			}

		} // namespace VATReturns
	} // namespace XML

	namespace CSV {
		// See https://www.skatteverket.se/foretag/moms/deklareramoms/periodisksammanstallning/lamnaperiodisksammanstallningmedfiloverforing.4.7eada0316ed67d72822104.html
		// Note 1: I have failed to find an actual technical specification document that specifies the format of the file to send for the form "Periodisk Sammanställning"
		// Note 2: This CSV format is one of three file formats (CSV, SRU and XML) I know of so far that Swedish tax agency uses for file uploads?

		// Example: "SKV574008;""
		namespace EUSalesList {

			using FormAmount = unsigned int; // All EU Sales List amounts are positive (as opposed to BAS account sales that are credit = negative)

			// Correct sign and rounding
			Amount to_eu_sales_list_amount(Amount amount) {
				return abs(round(amount)); // All amounts in the sales list form are defined to be positve (although sales in BAS are negative credits)
			}

			// Correct amount type for the form
			FormAmount to_form_amount(Amount amount) {
				return to_double(to_eu_sales_list_amount(amount));
			}

			std::string to_10_digit_orgno(std::string generic_org_no) {
				std::string result{};
				// The SKV CSV IT-system requires 10 digit organisation numbers with digits only
				// E.g., SIE-file organisation XXXXXX-YYYY has to be transformed into XXXXXXYYYY
				// See https://sv.wikipedia.org/wiki/Organisationsnummer
				std::string sdigits = filtered(generic_org_no,::isdigit);
				switch (sdigits.size()) {
					case 10: result = sdigits; break;
					case 12: result = sdigits.substr(2); // Assume the two first are the prefix "16"
					default: throw std::runtime_error(std::string{"ERROR: to_10_digit_orgno failed, invalid input generic_org_no:"} + "\"" + generic_org_no + "\""); break;
				}
				return result;
			}


			struct SwedishVATRegistrationID {std::string twenty_digits{};};
			struct EUVATRegistrationID {
				std::string with_country_code{};
				bool operator<(EUVATRegistrationID const& other) const {return with_country_code < other.with_country_code;}
			};

			struct Month {
				// Månadskoden för den månad eller det kalenderkvartal uppgifterna gäller, till exempel 2012 för december 2020, 2101 för januari 2021, 
				std::string yymm;
			};

			struct Quarter {
				// Kvartalskoden för det kvartar som uppgifterna gäller, till exempel 20-4 för fjärde kvartalet 2020 eller 21-1 för första kvartalet 2021.
				std::string yy_hyphen_quarter_seq_no{};
				bool operator<(Quarter const& other) const {return yy_hyphen_quarter_seq_no < other.yy_hyphen_quarter_seq_no;};
			};
			using PeriodID = std::variant<Quarter,Month>;

			struct Contact {std::string name_max_35_characters;};
			struct Phone {
				// Swedish telephone numbers are between eight and ten digits long. 
				// They start with a two to four digit area code. 
				// A three digit code starting with 07 indicates that the number is for a mobile phone. 
				// All national numbers start with one leading 0, and international calls are specified by 00 or +. 
				// The numbers are written with the area code followed by a hyphen, 
				// and then two to three groups of digits separated by spaces.
				std::string swedish_format_no_space_max_17_chars{}; // E.g., +CCAA-XXXXXXX where AA is area code without leading zero (070 => 70)
			}; // Consider https://en.wikipedia.org/wiki/National_conventions_for_writing_telephone_numbers#Sweden

			struct FirstRow {
				char const* entry = "SKV574008";
			};

		// Swedish Specification
		/*
		• Det 12-siffriga momsregistreringsnumret för den som är skyldig att lämna uppgifterna. Med eller utan landskoden SE.
		• Månads- eller kvartalskoden för den månad eller det kalenderkvartal uppgifterna gäller, till exempel 2012 för december 2020, 2101 för januari 2021, 20-4 för fjärde kvartalet 2020 eller 21-1 för första kvartalet 2021.
		• Namnet på personen som är ansvarig för de lämnade uppgifterna (högst 35 tecken).
		• Telefonnummer till den som är ansvarig för uppgifterna (endast siffror, med bindestreck efter riktnumret eller utlandsnummer, som inleds med plustecken (högst 17 tecken)).
		• Frivillig uppgift om e-postadress till den som är ansvarig för uppgifterna.		
		*/
		// Example: "556000016701;2001;Per Persson;0123-45690; post@filmkopia.se"
			struct SecondRow {
				SwedishVATRegistrationID vat_registration_id{};
				PeriodID period_id{};
				Contact name{};
				Phone phone_number{};
				std::optional<std::string> e_mail{};
			};

		// Swedish Specification
			/*
			• Köparens momsregistreringsnummer (VAT-nummer) med inledande landskod.
			• Värde av varuleveranser (högst 12 siffror inklusive eventuellt minustecken).
			• Värde av trepartshandel (högst 12 siffror inklusive eventuellt minustecken).
			• Värde av tjänster enligt huvudregeln (högst 12 siffror inklusive eventuellt minustecken)."
			*/
			struct RowN {
				EUVATRegistrationID vat_registration_id{};
				std::optional<FormAmount> goods_amount{};
				std::optional<FormAmount> three_part_business_amount{};
				std::optional<FormAmount> services_amount{};
			};

			struct Form {
				static constexpr FirstRow first_row{};
				SecondRow second_row{};
				std::vector<RowN> third_to_n_row{};
			};

			class OStream {
			public:
				std::ostream& os;
				operator bool() const {return static_cast<bool>(os);}
			};

			OStream& operator<<(OStream& os,char ch) {
				os.os << ch;
				return os;
			}

			OStream& operator<<(OStream& os,std::string const& s) {
				os.os << s;
				return os;
			}

			struct PeriodIDStreamer {
				OStream& os;
				void operator()(Month const& month) {
					os << month.yymm;
				}	
				void operator()(Quarter const& quarter) {
					os << quarter.yy_hyphen_quarter_seq_no;
				}	
			};

			OStream& operator<<(OStream& os,PeriodID const& period_id) {
				PeriodIDStreamer streamer{os};
				std::visit(streamer,period_id);
				return os;
			}
			
			OStream& operator<<(OStream& os,FirstRow const& row) {
				os.os << row.entry << ';';
				return os;
			}

			// Example: "556000016701;2001;Per Persson;0123-45690; post@filmkopia.se"
			OStream& operator<<(OStream& os,SecondRow const& row) {
				os << row.vat_registration_id.twenty_digits;
				os << ';' << row.period_id;
				os << ';' << row.name.name_max_35_characters;
				os << ';' << row.phone_number.swedish_format_no_space_max_17_chars;
				if (row.e_mail) os << ';' << *row.e_mail;
				return os;
			}

			OStream& operator<<(OStream& os,std::optional<FormAmount> const& ot) {
				os << ';';
				if (ot) os << std::to_string(*ot); // to_string to solve ambugiouty (TODO: Try to get rid of ambugiouty? and use os << *ot ?)
				return os;
			}

			// Example: FI01409351;16400;;;
			OStream& operator<<(OStream& os,RowN const& row) {
				os << row.vat_registration_id.with_country_code;
				os << row.goods_amount;
				os << row.three_part_business_amount;
				os << row.services_amount;
				return os;
			}

			OStream& operator<<(OStream& os,Form const& form) {
				os << form.first_row;
				os << '\n' << form.second_row;
				std::for_each(form.third_to_n_row.begin(),form.third_to_n_row.end(),[&os](auto row) {
				 	os << '\n' << row;
				});
				return os;
			}

			Quarter to_eu_list_quarter(Date const& date) {
				auto quarter = to_quarter(date);
				std::ostringstream os{};
				os << (static_cast<int>(date.year()) % 100u) << "-" << quarter.ix;
				return {os.str()};
			}

			EUVATRegistrationID to_eu_vat_id(SKV::XML::VATReturns::BoxNo const& box_no,BAS::MetaAccountTransaction const& mat) {
				std::ostringstream os{};
				if (!mat.defacto.transtext) {
						os << "* transtext " << std::quoted("") << " for " << mat << " does not define the EU VAT ID for this transaction *";						
				}
				else {
					// See https://en.wikipedia.org/wiki/VAT_identification_number#European_Union_VAT_identification_numbers
					const std::regex eu_vat_id("^[A-Z]{2}\\w*"); // Must begin with two uppercase charachters for the country code
					if (std::regex_match(*mat.defacto.transtext,eu_vat_id)) {
						os << *mat.defacto.transtext;
					}
					else {
						os << "* transtext " << std::quoted(*mat.defacto.transtext) << " for " << mat << " does not define the EU VAT ID for this transaction *";						
					}
				}
				return {os.str()};
			}

			std::vector<RowN> sie_to_eu_sales_list_rows(SKV::XML::VATReturns::FormBoxMap const& box_map) {
				// Default example data
				// FI01409351;16400;;;
				std::vector<RowN> result{};
				// Generate a row for each EU VAT Id
				std::map<EUVATRegistrationID,RowN> vat_id_map{};
				for (auto const& [box_no,mats] : box_map) {
					switch (box_no) {
						case 35: {
						} break;
						case 38: {

						} break;
						case 39: {
							auto x = [box_no=box_no,&vat_id_map](BAS::MetaAccountTransaction const& mat){
								auto eu_vat_id = to_eu_vat_id(box_no,mat);
								if (!vat_id_map.contains(eu_vat_id)) vat_id_map[eu_vat_id].vat_registration_id = eu_vat_id;
								if (!vat_id_map[eu_vat_id].services_amount) vat_id_map[eu_vat_id].services_amount = 0;
								*vat_id_map[eu_vat_id].services_amount += to_form_amount(mat.defacto.amount);
							};
							std::for_each(mats.begin(),mats.end(),x);
						} break;
					}
				}
				std::transform(vat_id_map.begin(),vat_id_map.end(),std::back_inserter(result),[](auto const& entry){
					return entry.second;
				});
				return result;
			}
			
			SwedishVATRegistrationID to_swedish_vat_registration_id(SKV::OrganisationMeta const& org_meta) {
				std::ostringstream os{};
				os << "SE" << to_10_digit_orgno(org_meta.org_no) << "01";
				return {os.str()};
			}

			std::optional<Form> vat_returns_to_eu_sales_list_form(SKV::XML::VATReturns::FormBoxMap const& box_map,SKV::OrganisationMeta const& org_meta,DateRange const& period) {
				std::optional<Form> result{};
				try {
					if (org_meta.contact_persons.size()==0) throw std::runtime_error(std::string{"vat_returns_to_eu_sales_list_form failed - zero org_meta.contact_persons"});

					Form form{};
					// struct SecondRow {
					// 	SwedishVATRegistrationID vat_registration_id{};
					// 	PeriodID period_id{};
					// 	Contact name{};
					// 	Phone phone_number{};
					// 	std::optional<std::string> e_mail{};
					// };
					// Default example data
					// 556000016701;2001;Per Persson;0123-45690; post@filmkopia.se					
					SecondRow second_row{
						.vat_registration_id = to_swedish_vat_registration_id(org_meta)
						,.period_id = to_eu_list_quarter(period.end()) // Support for quarter Sales List form so far
						,.name = {org_meta.contact_persons[0].name}
						,.phone_number = {org_meta.contact_persons[0].phone}
						,.e_mail = {org_meta.contact_persons[0].e_mail}
					};
					form.second_row = second_row;
					auto rows = sie_to_eu_sales_list_rows(box_map);
					std::copy(rows.begin(),rows.end(),std::back_inserter(form.third_to_n_row));
					result = form;
				}
				catch (std::exception& e) {
					std::cout << "\nvat_returns_to_eu_sales_list_form failed. Exception = " << std::quoted(e.what());
				}
				return result;
			}
		} // namespace EUSalesList
	} // namespace CSV {
} // namespace SKV

// Expose operator<< for type aliases defined in SKV::SRU (for SRU-file output) 
using SKV::SRU::operator<<;
// expose operator<< for type alias FormBoxMap, which being an std::map template is causing compiler to consider all std::operator<< in std and not in the one in SKV::XML::VATReturns
// See https://stackoverflow.com/questions/13192947/argument-dependent-name-lookup-and-typedef
using SKV::XML::VATReturns::operator<<;

std::set<BAS::AccountNo> to_vat_returns_form_bas_accounts(SKV::XML::VATReturns::BoxNos const& box_nos) {
	return SKV::XML::VATReturns::to_accounts(box_nos);
}

std::set<BAS::AccountNo> const& to_vat_accounts() {
	static auto const vat_accounts = SKV::XML::VATReturns::to_vat_accounts(); // cache
	// Define in terms of how SKV VAT returns form defines linking to BAS Accounts for correct content
	return vat_accounts;
}

std::optional<SKV::XML::XMLMap> to_skv_xml_map(SKV::OrganisationMeta sender_meta,SKV::XML::DeclarationMeta declaration_meta,SKV::OrganisationMeta employer_meta,SKV::XML::TaxDeclarations tax_declarations) {
	// std::cout << "\nto_skv_map" << std::flush;
	std::optional<SKV::XML::XMLMap> result{};
	SKV::XML::XMLMap xml_map{SKV::XML::TAXReturns::tax_returns_template};
	// sender_meta -> Skatteverket^agd:Avsandare.*
	Key::Path p{};
	try {
		if (sender_meta.contact_persons.size()==0) throw std::runtime_error(std::string{"to_skv_xml_map failed - zero sender_meta.contact_persons"});
		if (employer_meta.contact_persons.size()==0) throw std::runtime_error(std::string{"to_skv_xml_map failed - zero employer_meta.contact_persons"});
		if (tax_declarations.size()==0) throw std::runtime_error(std::string{"to_skv_xml_map failed - zero tax_declarations"});
		// <?xml version="1.0" encoding="UTF-8" standalone="no"?>
		// <Skatteverket omrade="Arbetsgivardeklaration"
		
		p += "Skatteverket";
		//   xmlns="http://xmls.skatteverket.se/se/skatteverket/da/instans/schema/1.1"
		//   xmlns:agd="http://xmls.skatteverket.se/se/skatteverket/da/komponent/schema/1.1"
		//   xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://xmls.skatteverket.se/se/skatteverket/da/instans/schema/1.1 http://xmls.skatteverket.se/se/skatteverket/da/arbetsgivardeklaration/arbetsgivardeklaration_1.1.xsd">
		//   <agd:Avsandare>
		p += "agd:Avsandare";
		//     <agd:Programnamn>Programmakarna AB</agd:Programnamn>
		xml_map[p + "agd:Programnamn"] = "https://github.com/kjelloh/cratchit";
		//     <agd:Organisationsnummer>190002039006</agd:Organisationsnummer>
		xml_map[p + "agd:Organisationsnummer"] = SKV::XML::to_12_digit_orgno(sender_meta.org_no);
		//     <agd:TekniskKontaktperson>
		p += "agd:TekniskKontaktperson";
		//       <agd:Namn>Valle Vadman</agd:Namn>
		xml_map[p + "agd:Namn"] = sender_meta.contact_persons[0].name;
		//       <agd:Telefon>23-2-4-244454</agd:Telefon>
		xml_map[p + "agd:Telefon"] = sender_meta.contact_persons[0].phone;
		//       <agd:Epostadress>valle.vadman@programmakarna.se</agd:Epostadress>
		xml_map[p + "agd:Epostadress"] = sender_meta.contact_persons[0].e_mail;
		--p;
		//     </agd:TekniskKontaktperson>
		//     <agd:Skapad>2021-01-30T07:42:25</agd:Skapad>
		xml_map[p + "agd:Skapad"] = declaration_meta.creation_date_and_time;
		--p;
		//   </agd:Avsandare>
		//   <agd:Blankettgemensamt>
		p += "agd:Blankettgemensamt";

		//     <agd:Arbetsgivare>
		p += "agd:Arbetsgivare";
		// employer_meta -> Skatteverket^agd:Blankettgemensamt^agd:Arbetsgivare.*
		//       <agd:AgRegistreradId>165560269986</agd:AgRegistreradId>
		xml_map[p + "agd:AgRegistreradId"] = SKV::XML::to_12_digit_orgno(employer_meta.org_no);
		//       <agd:Kontaktperson>
		p += "agd:Kontaktperson";
		//         <agd:Namn>Ville Vessla</agd:Namn>
		xml_map[p + "agd:Namn"] = employer_meta.contact_persons[0].name;
		//         <agd:Telefon>555-244454</agd:Telefon>
		xml_map[p + "agd:Telefon"] = employer_meta.contact_persons[0].phone;
		//         <agd:Epostadress>ville.vessla@foretaget.se</agd:Epostadress>
		xml_map[p + "agd:Epostadress"] = employer_meta.contact_persons[0].e_mail;
		--p;
		//       </agd:Kontaktperson>
		--p;
		//     </agd:Arbetsgivare>
		--p;
		//   </agd:Blankettgemensamt>
		//   <!-- Uppgift 1 HU -->
		//   <agd:Blankett>
		p += "agd:Blankett";
		//     <agd:Arendeinformation>
		p += "agd:Arendeinformation";
		//       <agd:Arendeagare>165560269986</agd:Arendeagare>
		xml_map[p + "agd:Arendeagare"] = SKV::XML::to_12_digit_orgno(employer_meta.org_no);
		//       <agd:Period>202101</agd:Period>
		xml_map[p + "agd:Period"] = declaration_meta.declaration_period_id;
		--p;
		//     </agd:Arendeinformation>
		//     <agd:Blankettinnehall>
		p += "agd:Blankettinnehall";
		//       <agd:HU>
		p += "agd:HU";
		// employer_meta + sum(tax_declarations) -> Skatteverket^agd:Kontaktperson^agd:Blankettinnehall^agd:HU.*
		//         <agd:ArbetsgivareHUGROUP>
		p += "agd:ArbetsgivareHUGROUP";
		//           <agd:AgRegistreradId faltkod="201">165560269986</agd:AgRegistreradId>
// std::cout << "\nxml_map[" << p + R"(agd:AgRegistreradId faltkod="201")" << "] = " << SKV::XML::to_12_digit_orgno(employer_meta.org_no);
		xml_map[p + R"(agd:AgRegistreradId faltkod="201")"] = SKV::XML::to_12_digit_orgno(employer_meta.org_no);
		--p;
		//         </agd:ArbetsgivareHUGROUP>
		//         <agd:RedovisningsPeriod faltkod="006">202101</agd:RedovisningsPeriod>
		xml_map[p + R"(agd:RedovisningsPeriod faltkod="006")"] = declaration_meta.declaration_period_id;
		//         <agd:SummaArbAvgSlf faltkod="487">0</agd:SummaArbAvgSlf>
		auto fee_sum = std::accumulate(tax_declarations.begin(),tax_declarations.end(),Amount{0},[](auto acc,auto const& tax_declaration){
			// Amount paid_employer_fee;
			// Amount paid_tax;
			return acc + tax_declaration.paid_employer_fee;
		});
		xml_map[p + R"(agd:SummaArbAvgSlf faltkod="487")"] = std::to_string(SKV::to_fee(fee_sum));
		//         <agd:SummaSkatteavdr faltkod="497">0</agd:SummaSkatteavdr>
		auto tax_sum = std::accumulate(tax_declarations.begin(),tax_declarations.end(),Amount{0},[](auto acc,auto const& tax_declaration){
			// Amount paid_employer_fee;
			// Amount paid_tax;
			return acc + tax_declaration.paid_tax;
		});
		xml_map[p + R"(agd:SummaSkatteavdr faltkod="497")"] = std::to_string(SKV::to_tax(tax_sum));
		--p;
		//       </agd:HU>
		--p;
		//     </agd:Blankettinnehall>
		--p;
		//   </agd:Blankett>
		//   <!-- Uppgift 1 IU -->
		//   <agd:Blankett>
		p += "agd:Blankett";
		//     <agd:Arendeinformation>
		p += "agd:Arendeinformation";
		//       <agd:Arendeagare>165560269986</agd:Arendeagare>
		xml_map[p + R"(agd:Arendeagare)"] = SKV::XML::to_12_digit_orgno(employer_meta.org_no);
		//       <agd:Period>202101</agd:Period>
		xml_map[p + R"(agd:Period)"] = declaration_meta.declaration_period_id;
		--p;
		//     </agd:Arendeinformation>
		//     <agd:Blankettinnehall>
		p += "agd:Blankettinnehall";
		//       <agd:IU>
		p += "agd:IU";
		// employer_meta + tax_declaration[i] -> Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU.*
		//         <agd:ArbetsgivareIUGROUP>
		p += "agd:ArbetsgivareIUGROUP";
		//           <agd:AgRegistreradId faltkod="201">165560269986</agd:AgRegistreradId>
		xml_map[p + R"(agd:AgRegistreradId faltkod="201")"] = SKV::XML::to_12_digit_orgno(employer_meta.org_no);
		--p;
		//         </agd:ArbetsgivareIUGROUP>
		//         <agd:BetalningsmottagareIUGROUP>
		p += "agd:BetalningsmottagareIUGROUP";
		//           <agd:BetalningsmottagareIDChoice>
		p += "agd:BetalningsmottagareIDChoice";
		//             <agd:BetalningsmottagarId faltkod="215">198202252386</agd:BetalningsmottagarId>
		xml_map[p + R"(agd:BetalningsmottagarId faltkod="215")"] = tax_declarations[0].employee_12_digit_birth_no;
		--p;
		//           </agd:BetalningsmottagareIDChoice>
		--p;
		//         </agd:BetalningsmottagareIUGROUP>
		//         <agd:RedovisningsPeriod faltkod="006">202101</agd:RedovisningsPeriod>
		xml_map[p + R"(agd:RedovisningsPeriod faltkod="006")"] = declaration_meta.declaration_period_id;;
		//         <agd:Specifikationsnummer faltkod="570">001</agd:Specifikationsnummer>
		xml_map[p + R"(agd:Specifikationsnummer faltkod="570")"] = "001";
		//         <agd:AvdrPrelSkatt faltkod="001">0</agd:AvdrPrelSkatt>
		xml_map[p + R"(agd:AvdrPrelSkatt faltkod="001")"] = std::to_string(SKV::to_tax(tax_declarations[0].paid_tax));
		--p;
		//       </agd:IU>
		--p;
		//     </agd:Blankettinnehall>
		--p;
		//   </agd:Blankett>
		--p;
		// </Skatteverket>
		result = xml_map;
	}
	catch (std::exception const& e) {
		std::cout << "\nERROR: to_skv_xml_map failed, exception=" << e.what();
	}
	if (result) {
		for (auto const& [tag,value] : *result) {
			// std::cout << "\nto_skv_xml_map: " << tag << " = " << std::quoted(value);
		}
	}
	return result;
}

/*
to_skv_xml_map: Skatteverket^agd:Avsandare^agd:Organisationsnummer = "165567828172"
to_skv_xml_map: Skatteverket^agd:Avsandare^agd:Programnamn = "https://github.com/kjelloh/cratchit"
to_skv_xml_map: Skatteverket^agd:Avsandare^agd:Skapad = "2022-06-13T15:39:51"
to_skv_xml_map: Skatteverket^agd:Avsandare^agd:TekniskKontaktperson^agd:Epostadress = "ville.vessla@foretaget.se"
to_skv_xml_map: Skatteverket^agd:Avsandare^agd:TekniskKontaktperson^agd:Namn = "Kjell-Olov Hogdal"
to_skv_xml_map: Skatteverket^agd:Avsandare^agd:TekniskKontaktperson^agd:Telefon = "555-244454"
to_skv_xml_map: Skatteverket^agd:Blankett^agd:Arendeinformation^agd:Arendeagare = "165567828172"
to_skv_xml_map: Skatteverket^agd:Blankett^agd:Arendeinformation^agd:Period = "202101"
to_skv_xml_map: Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:HU^agd:ArbetsgivareHUGROUP^agd:AgRegistreradId faltkod="201" = "165567828172"
to_skv_xml_map: Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:HU^agd:RedovisningsPeriod faltkod="006" = "202101"
to_skv_xml_map: Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:HU^agd:SummaArbAvgSlf faltkod="487" = "0"
to_skv_xml_map: Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:HU^agd:SummaSkatteavdr faltkod="497" = "0"
to_skv_xml_map: Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:ArbetsgivareIUGROUP^agd:AgRegistreradId faltkod="201" = "165567828172"
to_skv_xml_map: Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:ArbetsgivareIUGROUP^agd:AgRegistreradId faltkod="201") = "165560269986"
to_skv_xml_map: Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:AvdrPrelSkatt faltkod="001" = "0"
to_skv_xml_map: Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:BetalningsmottagareIUGROUP^agd:BetalningsmottagareIDChoice^agd:BetalningsmottagarId faltkod="215" = "196412178516"
to_skv_xml_map: Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:RedovisningsPeriod faltkod="006" = "202101"
to_skv_xml_map: Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:Specifikationsnummer faltkod="570" = "001"
to_skv_xml_map: Skatteverket^agd:Blankettgemensamt^agd:Arbetsgivare^agd:AgRegistreradId = "165567828172"
to_skv_xml_map: Skatteverket^agd:Blankettgemensamt^agd:Arbetsgivare^agd:Kontaktperson^agd:Epostadress = "ville.vessla@foretaget.se"
to_skv_xml_map: Skatteverket^agd:Blankettgemensamt^agd:Arbetsgivare^agd:Kontaktperson^agd:Namn = "Kjell-Olov Hogdal"
to_skv_xml_map: Skatteverket^agd:Blankettgemensamt^agd:Arbetsgivare^agd:Kontaktperson^agd:Telefon = "555-244454"
to_skv_xml_map: Skatteverket^agd:Kontaktperson^agd:Arendeinformation^agd:Arendeagare = "165560269986"
to_skv_xml_map: Skatteverket^agd:Kontaktperson^agd:Arendeinformation^agd:Period = "202101"
to_skv_xml_map: Skatteverket^agd:Kontaktperson^agd:Blankettinnehall^agd:HU^agd:ArbetsgivareHUGROUP^agd:AgRegistreradId faltkod="201" = "16556026998"
to_skv_xml_map: Skatteverket^agd:Kontaktperson^agd:Blankettinnehall^agd:HU^agd:RedovisningsPeriod faltkod="006" = "202101"
to_skv_xml_map: Skatteverket^agd:Kontaktperson^agd:Blankettinnehall^agd:HU^agd:SummaArbAvgSlf faltkod="487" = "0"
to_skv_xml_map: Skatteverket^agd:Kontaktperson^agd:Blankettinnehall^agd:HU^agd:SummaSkatteavdr faltkod="497" = "0"
*/

// "2021-01-30T07:42:25"
std::string to_skv_date_and_time(std::chrono::time_point<std::chrono::system_clock> const current) {
	std::ostringstream os{};
	auto now_timet = std::chrono::system_clock::to_time_t(current);
	auto now_local = localtime(&now_timet);
	os << std::put_time(now_local,"%Y-%m-%dT%H:%M:%S");
	return os.str();
}

std::optional<SKV::XML::XMLMap> cratchit_to_skv(SIEEnvironment const& sie_env,	std::vector<SKV::ContactPersonMeta> const& organisation_contacts, std::vector<std::string> const& employee_birth_ids) {
// std::cout << "\ncratchit_to_skv" << std::flush;
// std::cout << "\ncratchit_to_skv organisation_no.CIN=" << sie_env.organisation_no.CIN;
	std::optional<SKV::XML::XMLMap> result{};
	try {
		SKV::OrganisationMeta sender_meta{};sender_meta.contact_persons.push_back({});
		SKV::XML::DeclarationMeta declaration_meta{};
		SKV::OrganisationMeta employer_meta{};employer_meta.contact_persons.push_back({});
		SKV::XML::TaxDeclarations tax_declarations{};tax_declarations.push_back({});
		// declaration_meta.creation_date_and_time = "2021-01-30T07:42:25";
		declaration_meta.creation_date_and_time = to_skv_date_and_time(std::chrono::system_clock::now());
		declaration_meta.declaration_period_id = "202101";
		// employer_meta.org_no = "165560269986";
		employer_meta.org_no = SKV::XML::to_12_digit_orgno(sie_env.organisation_no.CIN);
		// sender_meta.org_no = "190002039006";
		sender_meta.org_no = employer_meta.org_no;
		if (organisation_contacts.size()>0) {
			// Use orgnaisation contact person as the file sender technical contact
			sender_meta.contact_persons[0] = organisation_contacts[0];

			// Use organisation contact person as employer contact
			employer_meta.contact_persons[0] = organisation_contacts[0];
		}
		else {
			throw std::runtime_error("cratchit_to_skv failed. No organisation_contacts provided");
		}
		if (employee_birth_ids.size() > 0) {
			tax_declarations[0].employee_12_digit_birth_no = employee_birth_ids[0];
			tax_declarations[0].paid_tax = 0;
		}
		else {
			throw std::runtime_error("cratchit_to_skv failed. No employee_birth_ids provided");
		}
		result = to_skv_xml_map(sender_meta,declaration_meta,employer_meta,tax_declarations);
	}
	catch (std::exception const& e) {
		std::cout << "\nERROR: Failed to create SKV data from SIE Environment, expection=" << e.what();
	}
	return result;
}

using EnvironmentValue = std::map<std::string,std::string>;
using Environment = std::multimap<std::string,EnvironmentValue>;

class ImmutableFileManager {
private:    
    std::filesystem::path directoryPath_;
    std::string prefix_;
    std::string suffix_;
    using key_type = std::pair<std::chrono::year_month_day, std::chrono::hh_mm_ss<std::chrono::seconds>>;
    struct new_to_old {
      bool operator()(key_type const& key1,key_type const& key2) const {
        // Note: std::chrono::hh_mm_ss does NOT define any comparisson operators so we need this proprietary compare functor
        if (key1.first == key2.first) return (key1.second.to_duration() > key2.second.to_duration());
        return key1.first > key2.first;
      }
    };
    std::map<key_type, std::filesystem::path,new_to_old> files_;
    /*
error: invalid operands to binary expression (
'const std::pair<std::chrono::year_month_day, std::chrono::hh_mm_ss<std::chrono::duration<long long>>>' and 
'const std::pair<std::chrono::year_month_day, std::chrono::hh_mm_ss<std::chrono::duration<long long>>>')
    */
    void loadFiles() {
      std::cout << "\nloadFiles";
      files_.clear();
      for (const auto& entry : std::filesystem::directory_iterator(directoryPath_)) {
        std::cout << "\n\tfile:" << entry.path();
        if (entry.is_regular_file()) {
          auto filePath = entry.path();
          std::cout << ",regular";
          if (filePath.filename().string().starts_with(prefix_) && filePath.extension() == suffix_) {
            std::cout << ",is " << prefix_ << "..." << suffix_;
            auto ftime = std::filesystem::last_write_time(filePath);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());

            auto tp = std::chrono::floor<std::chrono::days>(sctp);
            std::chrono::year_month_day ymd{tp};
            std::chrono::hh_mm_ss tod{std::chrono::duration_cast<std::chrono::seconds>(sctp - tp)};

            std::cout << ", key:{ymd:}" << ymd << ",tod:" << tod;
            auto file_count_before = files_.size();
            files_[{ymd, tod}] = filePath;
            std::cout << "\n\t\tfile_count_before:" << file_count_before << " file_count_after:" << files_.size();
            if (files_.size() == file_count_before) std::cout << " KEY CONFLICT!!";
          }              
        }
      }      
    }
public:
    ImmutableFileManager(std::filesystem::path const& directoryPath,std::string const& prefix,std::string const& suffix) 
      :  directoryPath_{directoryPath} 
        ,prefix_{prefix} 
        ,suffix_{suffix} {
      this->loadFiles();
    }
    
    std::optional<std::filesystem::path> getOldestFile() const {
      std::optional<std::filesystem::path> result{};
      if (not files_.empty()) {
        result = files_.rbegin()->second; // last listed path
      }
      return result;
    }
    std::optional<std::filesystem::path> getNewestFile() const {
      std::optional<std::filesystem::path> result{};
      if (not files_.empty()) {
        result = files_.begin()->second; // first listed path
      }
      return result;
    }
    std::optional<std::filesystem::path> generateNewFilePath() const {
      std::optional<std::filesystem::path> result{};
      auto now = std::chrono::system_clock::now();
      auto tp = std::chrono::floor<std::chrono::seconds>(now); // Round down to seconds

      // Format date and time as part of the file name
      std::ostringstream oss;
      // Note: Vexing syntax - the format string for std::format requires the ':' inside place holder "{...}" to prefix any format specifiers! Took me hours!!
      std::string creation_date_and_time = std::format("{:%Y%m%d_%H%M%S}", tp);
      oss << prefix_ << '_' << creation_date_and_time << suffix_;

      std::filesystem::path newFilePath = directoryPath_ / oss.str();

      // Handle path colisision (should happen only if new path requested within the same second, but still..)
      int counter = 1;
      int const MAX_COUNT{10}; // Max 10 file paths that differs on counter
      while (counter <= MAX_COUNT) {
          if (not std::filesystem::exists(newFilePath)) {
            result = newFilePath;
            break;
          }
          // Append a counter and try again
          oss.str("");
          oss << prefix_ << '_' << creation_date_and_time << '_' << counter << suffix_;
          newFilePath = directoryPath_ / oss.str();
          ++counter;
      }

      return result;
    }

    std::optional<std::filesystem::path> getPreviousFile(const std::filesystem::path& filePath) const {
      std::optional<std::filesystem::path> result{};
      auto it = std::adjacent_find(files_.begin(), files_.end(),
          [&](const auto& pair1, const auto& pair2) { return pair1.second == filePath; });

      if (it != files_.end()) {
          // If found, return the second file in the pair (which is the 'previous' file in the new-to-old ordered list)
          result = std::next(it)->second;
      }
      return result;
    }

    std::vector<std::filesystem::path> paths() {
      std::vector<std::filesystem::path> result{};
      for (auto const& entry : files_) {
        result.push_back(entry.second);
      }
      return result;
    }
};

void test_immutable_file_manager() {
  if (false) {
    std::cout << "\ntest_immutable_file_manager";
    std::filesystem::path folder{"."};
    ImmutableFileManager ifm{"./test","immutable_file",".txt"};
    std::cout << "\nimmutable files";
    for (auto const& p : ifm.paths()) {
      std::cout << "\n\tfile:" << p;
    }
    if (auto new_path = ifm.generateNewFilePath()) {
      std::ofstream ofs{*new_path};
      if (ofs) {
        std::cout << "\ncreated new file:" << *new_path;
        ofs << "This is an immutable file";
      }
    }
    std::cout << "\ntest_immutable_file_manager calls exit(0) :)";
    exit(0);
  }
}


OptionalJournalEntryTemplate template_of(OptionalHeadingAmountDateTransEntry const& had,SIEEnvironment const& sie_environ) {
	OptionalJournalEntryTemplate result{};
	if (had) {
		BAS::MetaEntries candidates{};
		for (auto const& je : sie_environ.journals()) {
			auto const& [series,journal] = je;
			for (auto const& [verno,aje] : journal) {								
				if (aje.caption.find(had->heading) != std::string::npos) {
					candidates.push_back({
						.meta = {
							.series = series
							,.verno = verno}
						,.defacto = aje});
				}
			}
		}
		// select the entry with the latest date
		std::nth_element(candidates.begin(),candidates.begin(),candidates.end(),[](auto const& je1, auto const& je2){
			return (je1.defacto.date > je2.defacto.date);
		});
		result = to_template(candidates.front());
	}
	return result;
}

std::ostream& operator<<(std::ostream& os,EnvironmentValue const& ev) {
	bool not_first{false};
	std::for_each(ev.begin(),ev.end(),[&not_first,&os](auto const& entry){
		if (not_first) {
			os << ";"; // separator
		}
		os <<  entry.first;
		os <<  "=";
		os <<  entry.second;
		not_first = true;
	});
	return os;
}

// "belopp=1389,50;datum=20221023;rubrik=Klarna"
std::string to_string(EnvironmentValue const& ev) {
	std::ostringstream os{};
	os << ev;
	return os.str();
}

std::ostream& operator<<(std::ostream& os,Environment::value_type const& entry) {
	os << entry.first << " " << std::quoted(to_string(entry.second));
	return os;
}

std::ostream& operator<<(std::ostream& os,Environment const& env) {
	for (auto const& entry : env) {
		os << "\n\t" << entry;
	}	
	return os;
}

// "belopp=1389,50;datum=20221023;rubrik=Klarna"
EnvironmentValue to_environment_value(std::string const s) {
	EnvironmentValue result{};
	auto kvps = tokenize::splits(s,';');
	for (auto const& kvp : kvps) {
		auto const& [name,value] = tokenize::split(kvp,'=');
		result[name] = value;
	}
	return result;
}
std::string to_string(Environment::value_type const& entry) {
	std::ostringstream os{};
	os << entry;
	return os.str();
}

EnvironmentValue to_environment_value(HeadingAmountDateTransEntry const had) {
	// std::cout << "\nto_environment_value: had.amount" << had.amount << " had.date" << had.date;
	std::ostringstream os{};
	os << had.amount;
	EnvironmentValue ev{};
	ev["rubrik"] = had.heading;
	ev["belopp"] = os.str();
	ev["datum"] = to_string(had.date);
	return ev;
}

OptionalHeadingAmountDateTransEntry to_had(EnvironmentValue const& ev) {
	OptionalHeadingAmountDateTransEntry result{};
	HeadingAmountDateTransEntry had{};
	while (true) {
		if (auto iter = ev.find("rubrik");iter != ev.end()) had.heading = iter->second;
		else break;
		if (auto iter = ev.find("belopp");iter != ev.end()) had.amount = *to_amount(iter->second); // assume success
		else break;
		if (auto iter = ev.find("datum");iter != ev.end()) had.date = *to_date(iter->second); // assume success
		else break;
		result = had;
		break;
	}
	return result;
}

OptionalHeadingAmountDateTransEntry to_had(BAS::MetaEntry const& me) {
	OptionalHeadingAmountDateTransEntry result{};
	auto gross_amount = to_positive_gross_transaction_amount(me.defacto);
	HeadingAmountDateTransEntry had{
		.heading = me.defacto.caption
		,.amount = gross_amount
		,.date = me.defacto.date
		,.optional = {
			.current_candidate = me
		}
	};
	result = had;
	return result;
}

OptionalHeadingAmountDateTransEntry to_had(TaggedAmount const& ta) {
  OptionalHeadingAmountDateTransEntry result{};
  std::string heading{};

  if (auto o_value = ta.tag_value("Account")) heading += std::string{"Account:"} + *o_value;
  if (auto o_value = ta.tag_value("Text")) heading += std::string{" Text:"} + *o_value;
  if (auto o_value = ta.tag_value("Message")) heading += std::string{" Message:"} + *o_value;
  if (auto o_value = ta.tag_value("From")) heading += std::string{" From:"} + *o_value;
  if (auto o_value = ta.tag_value("To")) heading += std::string{" To:"} + *o_value;

  HeadingAmountDateTransEntry had{
    .heading = heading
    // ,.amount = static_cast<Amount>(ta.cents_amount())
    ,.amount = to_amount(to_units_and_cents(ta.cents_amount()))
    ,.date = ta.date()
  };
  result = had;
  return result;
}

OptionalHeadingAmountDateTransEntry to_had(std::vector<std::string> const& tokens) {
  if (tokens.size()==3) {
    HeadingAmountDateTransEntry had {
      .heading = tokens[0]
      ,.amount = *to_amount(tokens[1]) // Assume success
      ,.date = *to_date(tokens[2]) // Assume success
    };
    return had;
  }
  return std::nullopt;
}

EnvironmentValue to_environment_value(SKV::ContactPersonMeta const& cpm) {
	EnvironmentValue ev{};
	ev["name"] = cpm.name;
	ev["phone"] = cpm.phone;
	ev["e-mail"] = cpm.e_mail;
	return ev;
}

EnvironmentValue to_environment_value(TaggedAmount const& ta) {
	EnvironmentValue ev{};
	ev["yyyymmdd_date"] = to_string(ta.date());
	ev["cents_amount"] = to_string(ta.cents_amount());
	for (auto const& entry : ta.tags()) {
		ev[entry.first] = entry.second;
	}
	return ev;
}

OptionalTaggedAmount to_tagged_amount(EnvironmentValue const& ev) {
	OptionalTaggedAmount result{};
	OptionalDate date{};
	OptionalCentsAmount cents_amount{};
	TaggedAmount::OptionalValueId value_id{};
	TaggedAmount::Tags tags{};
	for (auto const& entry : ev) {
		if (entry.first == "yyyymmdd_date") date = to_date(entry.second);
		else if (entry.first == "cents_amount") cents_amount = to_cents_amount(entry.second);
		else tags[entry.first] = entry.second;
	}
	if (date and cents_amount) {
    result = TaggedAmount{*date,*cents_amount,std::move(tags)};
	}
  // TODO 240524 - Remove when fully functional tagged amounts to and from SIE is in place
  //               For now, discard any stored tagged amounts that represents SIE journal entries
  if (true and result) {
    if (result->tag_value("BAS") or result->tag_value("SIE")) return std::nullopt; // discard / filter out stored SIE environment tagged amounts (start fresh)
  }
	return result;
}

std::optional<SRUEnvironments::value_type> to_sru_environments_entry(EnvironmentValue const& ev) {
	try {
// std::cout << "\nto_sru_environments_entry";
		// "4531=360000;4532=360000;year_id=0"
		SRUEnvironment sru_env{};
		auto& year_id = ev.at("year_id");
		for (auto const& [key,value] : ev) {
// std::cout << "\nkey:" << key << " value:" << value;
			if (auto const& sru_code = SKV::SRU::to_account_no(key)) {
				sru_env.set(*sru_code,value);
			}
		}
		return SRUEnvironments::value_type{year_id,sru_env};
	}
	catch (std::exception const& e) {
		std::cout << "\nto_sru_environments_entry failed. Exception=" << std::quoted(e.what());
	}
	return std::nullopt;
}

SKV::OptionalContactPersonMeta to_contact(EnvironmentValue const& ev) {
	SKV::OptionalContactPersonMeta result;
	SKV::ContactPersonMeta cpm{};
	while (true) {
		if (auto iter = ev.find("name");iter != ev.end()) cpm.name = iter->second;
		else break;
		if (auto iter = ev.find("phone");iter != ev.end()) cpm.phone = iter->second;
		else break;
		if (auto iter = ev.find("e-mail");iter != ev.end()) cpm.e_mail = iter->second;
		else break;
		result = cpm;
		break;
	}
	return result;
}

std::optional<std::string> to_employee(EnvironmentValue const& ev) {
	std::optional<std::string> result{};
	std::string birth_id{};
	while (true) {
		if (auto iter = ev.find("birth-id");iter != ev.end()) birth_id = iter->second;
		else break;
		result = birth_id;
		break;
	}
	return result;
}

enum class PromptState {
	Undefined
	,Root
  ,LUARepl
	,TAIndex
	,AcceptNewTAs
	,HADIndex
	,VATReturnsFormIndex
	,JEIndex
	// Manual Build generator states
	,GrossDebitorCreditOption // User selects if Gross account is Debit or Credit
	,CounterTransactionsAggregateOption // User selects what kind of counter trasnaction aggregate to create
	,GrossAccountInput
	,NetVATAccountInput
	,JEAggregateOptionIndex
	,EnterHA
	,ATIndex
	,EditAT
	,CounterAccountsEntry
	,SKVEntryIndex
	,QuarterOptionIndex
	,SKVTaxReturnEntryIndex
	,K10INK1EditOptions
	,INK2AndAppendixEditOptions
	,EnterIncome
  ,EnterDividend
	,EnterContact
	,EnterEmployeeID
	,Unknown
};

auto falling_date = [](auto const& had1,auto const& had2){
	return (had1.date > had2.date);
};

using PromptOptionsList = std::vector<std::string>;

std::ostream& operator<<(std::ostream& os,PromptOptionsList const& pol) {
	for (auto const& option : pol) {
		os << "\n" << option;
	}
	return os;
}

PromptOptionsList options_list_of_prompt_state(PromptState const& prompt_state) {
	std::vector<std::string> result{};
	result.push_back("Options:");
	switch (prompt_state) {
		case PromptState::Undefined: {result.push_back("DESIGN INSUFFICIENCY: options_list_of_prompt_state have no action for State PromptState::Undefined ");} break;
		case PromptState::Root: {
			result.push_back("<Heading> <Amount> <Date> : Entry of new Heading Amount Date (HAD) Transaction entry");
			result.push_back("-hads : lists current Heading Amount Date (HAD) entries");
			result.push_back("-sie <sie file path> : imports a new input sie file. Please enclose path with \"\" if it contains space.");
			result.push_back("-sie : lists transactions in input sie-file");
			result.push_back("-env : lists cratchit environment");
			result.push_back("-tas <first date> <last date> : Selects tagged amounts in the period first date .. last date");
			result.push_back("-tas : Selects last selected tagged amounts");
			result.push_back("-csv <csv file path> : Imports Comma Seperated Value file of Web bank account transactions");
			result.push_back("                       Stores them as Heading Amount Date (HAD) entries.");			
			result.push_back("'q' or 'Quit'");
		} break;
    case PromptState::LUARepl: {
      result.push_back("Please enter a valid lua script to execute");
    } break;
		case PromptState::TAIndex: {
      result.push_back("The following options are available for Tagged Amounts selection.");
      result.push_back("<Enter> : Lists the currently selected tagged amounts");
      result.push_back("<index> - Selects tagged amount with provided index");
      result.push_back("-has_tag <regular expression> - Keep tagged amounts with tag matching regular expression");
      result.push_back("-has_not_tag <regular expression> - Keep tagged amounts with tag NOT matching regular expression");
      result.push_back("-is_tagged <tag name>=<regular expression> - Keep tagged amounts with named tag value matching regular expression");
      result.push_back("-is_not_tagged <tag name>=<regular expression> - Keep tagged amounts with named tag value NOT matching regular expression");
      result.push_back("-to_bas_account <bas account number> - Tag current selection of tagged amounts with provided BAS account number.");
      result.push_back("-amount_trails - Groups tagged amounts on transaction amount and lists them");
      result.push_back("-aggregates - Reduces tagged amounts to only aggregates (E.g., SIE entries referring to single account tagged amounts)");
      result.push_back("-todo - Lists tagged amounts subject to 'TODO' actions.");
    } break;
		case PromptState::AcceptNewTAs: {
			result.push_back("1:YES");
			result.push_back("<Enter>:No");
		} break;
		case PromptState::HADIndex: {result.push_back("PromptState::HADIndex");} break;
		case PromptState::VATReturnsFormIndex: {result.push_back("PromptState::VATReturnsFormIndex");} break;
		case PromptState::JEIndex: {result.push_back("PromptState::JEIndex");} break;
		case PromptState::GrossDebitorCreditOption: {
			result.push_back("0: As is ");
			result.push_back("1: (+) Force to Debit ");
			result.push_back("2: (-) Force to Credit ");
		} break;
		case PromptState::CounterTransactionsAggregateOption: {
			result.push_back("0: Gross counter transaction account aggregate");
			result.push_back("1: {Net, VAT} counter transaction accounts aggregate");
		} break;
		case PromptState::GrossAccountInput: {
			result.push_back("Please enter counter transaction account");
		} break;
		case PromptState::NetVATAccountInput: {
			result.push_back("Please enter Net + VAT counter accounts (separated by space");
		} break;
		case PromptState::JEAggregateOptionIndex: {
			result.push_back("0 1 x counter transactions aggregate");
			result.push_back("1 n x counter transactions aggregate");
			result.push_back("2 Edit account transactions");
			result.push_back("3 STAGE as-is");
		} break;
		case PromptState::EnterHA: {result.push_back("PromptState::EnterHA");} break;
		case PromptState::ATIndex: {
      result.push_back("Please enter index of entry to edit, or <BAS account> <Amount> to add a new entry;'-' to exit back to journal entry");
    } break;
		case PromptState::EditAT: {
			result.push_back("Please Enter new Account, new Amount (with decimal comma) or new transaction text; '-' to exit back to candidate");
		} break;
		case PromptState::CounterAccountsEntry: {result.push_back("PromptState::CounterAccountsEntry");} break;
		case PromptState::SKVEntryIndex: {result.push_back("PromptState::SKVEntryIndex");} break;
		case PromptState::QuarterOptionIndex: {result.push_back("PromptState::QuarterOptionIndex");} break;
		case PromptState::SKVTaxReturnEntryIndex: {result.push_back("PromptState::SKVTaxReturnEntryIndex");} break;
		case PromptState::K10INK1EditOptions: {result.push_back("PromptState::K10INK1EditOptions");} break;
		case PromptState::INK2AndAppendixEditOptions: {result.push_back("PromptState::INK2AndAppendixEditOptions");} break;
		case PromptState::EnterIncome: {result.push_back("PromptState::EnterIncome");} break;
		case PromptState::EnterDividend: {result.push_back("PromptState::EnterDividend");} break;
		case PromptState::EnterContact: {result.push_back("PromptState::EnterContact");} break;
		case PromptState::EnterEmployeeID: {result.push_back("PromptState::EnterEmployeeID");} break;
		case PromptState::Unknown: {result.push_back("DESIGN INSUFFICIENCY: options_list_of_prompt_state have no action for State PromptState::Unknown ");} break;
	}
	return result;
}

class ConcreteModel {
public:
  lua_State* L{nullptr}; // LUA void* to its environment
  ~ConcreteModel() {
    if (L != nullptr) {
      lua_close(this->L);
    }
  }
	std::vector<SKV::ContactPersonMeta> organisation_contacts{};
	std::vector<std::string> employee_birth_ids{};
	std::string user_input{};
	PromptState prompt_state{PromptState::Root};
	size_t had_index{};
	// BAS::MetaEntries template_candidates{};
	BAS::TypedMetaEntries template_candidates{};
	BAS::anonymous::AccountTransactions at_candidates{};
	// BAS::anonymous::AccountTransaction at{};
  std::size_t at_index{};
  std::string prompt{};
	bool quit{};
	std::map<std::string,std::filesystem::path> sie_file_path{};
	SIEEnvironments sie{};
	SRUEnvironments sru{};
	HeadingAmountDateTransEntries heading_amount_date_entries{};
	DateOrderedTaggedAmountsContainer all_date_ordered_tagged_amounts{};
	DateOrderedTaggedAmountsContainer selected_date_ordered_tagged_amounts{};
	DateOrderedTaggedAmountsContainer new_date_ordered_tagged_amounts{};
	size_t ta_index{};
	std::filesystem::path staged_sie_file_path{"cratchit.se"};

	std::optional<DateOrderedTaggedAmountsContainer::iterator> to_ta_iter(std::size_t ix) {
		std::optional<DateOrderedTaggedAmountsContainer::iterator> result{};
		auto ta_iter = this->selected_date_ordered_tagged_amounts.begin();
		auto end = this->selected_date_ordered_tagged_amounts.end();
		// std::cout << "\nto_had_iter had_index:" << had_index << " end-begin:" << std::distance(had_iter,end);
		if (ix < std::distance(ta_iter,end)) {
			std::advance(ta_iter,ix); // zero based index
			result = ta_iter;
		}
		return result;
	}
	std::optional<HeadingAmountDateTransEntries::iterator> to_had_iter(std::size_t ix) {
		std::optional<HeadingAmountDateTransEntries::iterator> result{};
		auto had_iter = this->heading_amount_date_entries.begin();
		auto end = this->heading_amount_date_entries.end();
		// std::cout << "\nto_had_iter had_index:" << had_index << " end-begin:" << std::distance(had_iter,end);
		if (ix < std::distance(had_iter,end)) {
			std::advance(had_iter,ix); // zero based index
			result = had_iter;
		}
		return result;
	}

  std::optional<BAS::anonymous::AccountTransactions::iterator> to_at_iter(std::optional<HeadingAmountDateTransEntries::iterator> had_iter,std::size_t ix) {
    std::cout << "\nto_at_iter(" << ix << ")";
    std::optional<BAS::anonymous::AccountTransactions::iterator> result{};
    if (had_iter) {
      std::cout << ",had_iter ok";
			if ((*had_iter)->optional.current_candidate) {
        std::cout << ",optional ok";
				auto at_iter = (*had_iter)->optional.current_candidate->defacto.account_transactions.begin();
				auto end = (*had_iter)->optional.current_candidate->defacto.account_transactions.end();
				if (ix < std::distance(at_iter,end)) {
          std::cout << ",ix ok";
          std::advance(at_iter,ix);
          std::cout << " --> selected at:" << *at_iter;
          result = at_iter;
        }
			}
    }
    return result;
  }

	std::optional<DateOrderedTaggedAmountsContainer::iterator> selected_ta() {
		return to_ta_iter(this->ta_index);
	}

	std::optional<HeadingAmountDateTransEntries::iterator> selected_had() {
		return to_had_iter(this->had_index);
	}
  
	void erease_selected_had() {
		if (auto had_iter = to_had_iter(this->had_index)) {
			this->heading_amount_date_entries.erase(*had_iter);
			this->had_index = this->heading_amount_date_entries.size()-1; // default to select the now last one
		}
	}

	std::optional<HeadingAmountDateTransEntries::iterator> find_had(HeadingAmountDateTransEntry const& had) {
		std::optional<HeadingAmountDateTransEntries::iterator> result{};
		auto iter = std::find_if(this->heading_amount_date_entries.begin(),this->heading_amount_date_entries.end(),[&had](HeadingAmountDateTransEntry const& other){
			return ((had.date == other.date) and had.heading == other.heading); // "same" if same date and heading
		});
		if (iter != this->heading_amount_date_entries.end()) result = iter;
		return result;
	}

	HeadingAmountDateTransEntries const& refreshed_hads(void) {
		auto vat_returns_hads = SKV::XML::VATReturns::to_vat_returns_hads(this->sie);
		for (auto const& vat_returns_had : vat_returns_hads) {
			if (auto o_iter = this->find_had(vat_returns_had)) {
				auto iter = *o_iter;
				// TODO: When/if we actually save the state of iter->optional.vat_returns_form_box_map_candidate, then remove the condition in following if-statement
				if (!iter->optional.vat_returns_form_box_map_candidate or (are_same_and_less_than_100_cents_apart(iter->amount,vat_returns_had.amount) == false)) {
					// NOTE: iter->optional.vat_returns_form_box_map_candidate currently does not survive restart of cratchit (is not saved to nor retreived from environment file)
					*iter = vat_returns_had;
					std::cout << "\n*UPDATED* " << vat_returns_had;
				}
			}
			else {
				this->heading_amount_date_entries.push_back(vat_returns_had);
				std::cout << "\n*NEW* " << vat_returns_had;
			}
		}
		std::sort(this->heading_amount_date_entries.begin(),this->heading_amount_date_entries.end(),falling_date);
		return this->heading_amount_date_entries;
	}

  OptionalDateRange to_fiscal_year_date_range(std::string const& year_id) {
    OptionalDateRange result{};
		if (this->sie.contains(year_id)) {
      result = this->sie[year_id].fiscal_year_date_range();
    }
    return result;
  }

  PromptState to_previous_state(PromptState const& current_state) {
    std::cout << "\nto_previous_state";
    PromptState result{PromptState::Root};
    if (current_state == PromptState::ATIndex) {
      result = PromptState::JEAggregateOptionIndex;
    }
    return result;
  }

  std::string to_prompt_for_entering_state(PromptState const& new_state) {
    std::ostringstream result{"\n<>"};
    std::cout << "\nto_prompt_for_entering_state";
    auto had_iter = this->selected_had();
    if (had_iter and new_state == PromptState::ATIndex) {
      auto& had = *(*had_iter);
      unsigned int i{};
      std::for_each(had.optional.current_candidate->defacto.account_transactions.begin(),had.optional.current_candidate->defacto.account_transactions.end(),[&i,&result](auto const& at){
        result << "\n  " << i++ << " " << at;
      });
    }
    else {
      result << options_list_of_prompt_state(new_state);
    }

    return result.str();
  }

private:
}; // ConcreteModel

using Model = std::unique_ptr<ConcreteModel>; // "as if" immutable (pass around the same instance)

Amount get_INK1_Income(Model const& model) {
	Amount result{};
	if (auto amount = model->sru["0"].at(1000)) {
		std::istringstream is{*amount};
		is >> result;
	}
	return result;
}
Amount get_K10_Dividend(Model const& model) {
	Amount result{};
	if (auto amount = model->sru["0"].at(4504)) {
		// use amount assigned to SRU 4504
		std::istringstream is{*amount};
		is >> result;
	}
	else if (auto amount = account_sum(model->sie["current"],2898)) {
		// Use any amount accounted for in 2898
		result = *amount;
	}
	return result;
}

namespace SKV {
	namespace SRU {

		OptionalSRUValueMap to_sru_value_map(Model const& model,::CSV::FieldRows const& field_rows) {
			OptionalSRUValueMap result{};
			try {
// std::cout << "\nto_sru_value_map";
				std::map<SKV::SRU::AccountNo,BAS::OptionalAccountNos> sru_to_bas_accounts{};
				for (int i=0;i<field_rows.size();++i) {
					auto const& field_row = field_rows[i];
// std::cout << "\n\t" << static_cast<std::string>(field_row);
					if (field_row.size() > 1) {
						auto const& field_1 = field_row[1];
// std::cout << "\n\t\t[1]=" << std::quoted(field_1);
						if (auto sru_code = to_account_no(field_1)) {
// std::cout << " ok! ";
							if (field_row.size() > 3) {
								auto mandatory = (field_row[3].find("J") != std::string::npos);
								if (mandatory) {
// std::cout << " Mandatory.";
								}
								else {
// std::cout << " optional ;)";
								}
								sru_to_bas_accounts[*sru_code] = model->sie["current"].to_bas_accounts(*sru_code);
							}
							else {
// std::cout << " NO [3]";
							}
						}
						else {
// std::cout << " NOT SRU";
						}
					}
					else {
// std::cout << " null (does not exist)";
					}
				}
				// Now retreive the sru values from bas accounts as mapped
				SRUValueMap sru_value_map{};
				for (auto const& [sru_code,bas_account_nos] : sru_to_bas_accounts) {
// std::cout << "\nSRU:" << sru_code;
					if (bas_account_nos) {
// for (auto const& bas_account_no : *bas_account_nos) std::cout << "\n\tBAS:" << bas_account_no;
						sru_value_map[sru_code] = to_ats_sum_string(model->sie,*bas_account_nos);
// std::cout << "\n\t------------------";
// std::cout << "\n\tSUM:" << sru_code << " = ";
// if (sru_value_map[sru_code]) std::cout << *sru_value_map[sru_code];
// else std::cout << " null";
					}
					else {
// std::cout << "\n\tNO BAS Accounts map to SRU:" << sru_code;
						if (auto const& stored_value = model->sru["0"].at(sru_code)) {
							sru_value_map[sru_code] = stored_value;
// std::cout << "\n\tstored:" << *stored_value;

						}
						// // K10
						// SRU:4531	"Antal ägda andelar vid årets ingång"
						// SRU:4532	"Totala antalet andelar i hela företaget vid årets ingång"
						// SRU:4511	"Årets gränsbelopp enligt förenklingsregeln"				(=183 700 * ägar_andel)
						// SRU:4501	"Sparat utdelningsutrymme från föregående år * 103%)	(103% av förra årets sparade utrymme, förra årets SRU:4724)
						// SRU:4502	"Gränsbelopp enligt förenklingsregeln"					(SRU:4511 + SRU:4501
						// SRU:4503	"Gränsbelopp att utnyttja vid p. 1.7 nedan"				(SRU:4502)

						// // Utdelning som beskattas I TJÄNST"
						// SRU:4504	"Utdelning"										(Från BAS:2898, SRU:7369)
						// SRU:4721	"Gränsbelopp SRU:4503"
						// SRU:4722	"Sparat utdelningsutrymme"							(SRU:4504 - SRU:4721)
						// SRU:4724	"Sparat utdelningsutrymme till nästa år"					(SRU:4722)

						// // Utdelning som beskattas i KAPITAL
						// SRU:4506	"Utdelning"										(SRU:4504)
						// SRU:4507	"Utdelning i Kapital"
						// SRU:4508	Det minsta av (2/3 av gränsbelopp vs 2/3 av utdelning Kapital)
						// SRU:4509	Resterande utdelning (utdelning kapital - gränsbelopp om positivt annars 0)
						// SRU:4515	"Utdelning som tas upp i kapital"						(Till INK1 p. 7.7. SRU:1100)

						// // INK1
						// SRU:1000 	"1.1 Lön Förmåner, Sjukpenning mm"	(Från förtryckta uppgifter)
						// SRU:1100	"7.2 Ränteinkomster, utdelningar..."	(från K10 SRU:4515	"Utdelning som tas upp i kapital")

					}
				}
				result = sru_value_map;
			}
			catch (std::exception const& e) {
				std::cout << "\nto_sru_value_map failed. Excpetion=" << std::quoted(e.what());
			}
			return result;
		}
	}
} // namespace SKV {


using Command = std::string;
struct Quit {};
struct Nop {};

using Msg = std::variant<Nop,Quit,Command>;
struct Cmd {std::optional<Msg> msg{};};
using Ux = std::vector<std::string>;

Cmd to_cmd(std::string const& user_input) {
	Cmd result{Nop{}};
	if (user_input == "quit" or user_input=="q") result.msg = Quit{};
	else result.msg = Command{user_input};
	return result;
}

class FilteredSIEEnvironment {
public:
	FilteredSIEEnvironment(SIEEnvironment const& sie_environment,BAS::MatchesMetaEntry matches_meta_entry)
		:  m_sie_environment{sie_environment}
			,m_matches_meta_entry{matches_meta_entry} {}

	void for_each(auto const& f) const {
		auto f_if_match = [this,&f](BAS::MetaEntry const& me){
			if (this->m_matches_meta_entry(me)) f(me);
		};
		for_each_meta_entry(m_sie_environment,f_if_match);
	}
private:
	SIEEnvironment const& m_sie_environment;
	BAS::MatchesMetaEntry m_matches_meta_entry{};
};

std::ostream& operator<<(std::ostream& os,FilteredSIEEnvironment const& filtered_sie_environment) {
	struct stream_entry_to {
		std::ostream& os;
		void operator()(BAS::MetaEntry const& me) const {
			os << '\n' << me;
		}
	};
	os << "\n*Filter BEGIN*";
	filtered_sie_environment.for_each(stream_entry_to{os});
	os << "\n*Filter end*";
	return os;
}

std::ostream& operator<<(std::ostream& os,SIEEnvironment const& sie_environment) {
	for (auto const& je : sie_environment.journals()) {
		auto& [series,journal] = je;
		for (auto const& [verno,entry] : journal) {
			BAS::MetaEntry me {
				.meta = {
					 .series = series
					,.verno = verno
					,.unposted_flag = sie_environment.is_unposted(series,verno)
				}
				,.defacto = entry
			};
			os << '\n' << me;
		}
	}
	return os;
}

BAS::MetaEntry to_entry(SIE::Ver const& ver) {
	BAS::MetaEntry result{
		.meta = {
			.series = ver.series
			,.verno = ver.verno
		}
	};
	result.defacto.caption = ver.vertext;
	result.defacto.date = ver.verdate;
	for (auto const& trans : ver.transactions) {								
		result.defacto.account_transactions.push_back(BAS::anonymous::AccountTransaction{
			.account_no = trans.account_no
			,.transtext = trans.transtext
			,.amount = trans.amount
		});
	}
	return result;
}

OptionalSIEEnvironment from_sie_file(std::filesystem::path const& sie_file_path) {
	OptionalSIEEnvironment result{};
	std::ifstream ifs{sie_file_path};
  encoding::CP437::istream is{ifs};
	if (is) {
    // Read in the SIE file and transcode it to UTF8
    std::string s_utf8{};
    while (auto entry = is.getline(encoding::unicode::to_utf8{})) {
      s_utf8 += *entry;
      s_utf8 += "\n";
    }
    // Create a stream with the UTF8 encoded SIE file entries for internal parsing
    std::istringstream utf8_in{s_utf8};
		SIEEnvironment sie_environment{};
		while (true) {
			// std::cout << "\nparse";
			if (auto opt_entry = SIE::parse_ORGNR(utf8_in,"#ORGNR")) {
				SIE::OrgNr orgnr = std::get<SIE::OrgNr>(*opt_entry);
				sie_environment.organisation_no = orgnr;
			}
			else if (auto opt_entry = SIE::parse_FNAMN(utf8_in,"#FNAMN")) {
				SIE::FNamn fnamn = std::get<SIE::FNamn>(*opt_entry);
				sie_environment.organisation_name = fnamn;
			}
			else if (auto opt_entry = SIE::parse_ADRESS(utf8_in,"#ADRESS")) {
				SIE::Adress adress = std::get<SIE::Adress>(*opt_entry);
				sie_environment.organisation_address = adress;
			}
			else if (auto opt_entry = SIE::parse_RAR(utf8_in,"#RAR")) {
				SIE::Rar rar = std::get<SIE::Rar>(*opt_entry);
				if (rar.year_no == 0) {
					// Process only "current" year in read sie file
					sie_environment.set_year_date_range(DateRange{rar.first_day_yyyymmdd,rar.last_day_yyyymmdd});
				}
			}
			else if (auto opt_entry = SIE::parse_KONTO(utf8_in,"#KONTO")) {
				SIE::Konto konto = std::get<SIE::Konto>(*opt_entry);
				sie_environment.set_account_name(konto.account_no,konto.name);
			}
			else if (auto opt_entry = SIE::parse_SRU(utf8_in,"#SRU")) {
				SIE::Sru sru = std::get<SIE::Sru>(*opt_entry);
				sie_environment.set_account_SRU(sru.bas_account_no,sru.sru_account_no);
			}
			else if (auto opt_entry = SIE::parse_IB(utf8_in,"#IB")) {
				SIE::Ib ib = std::get<SIE::Ib>(*opt_entry);
				// std::cout << "\nIB " << ib.account_no << " = " << ib.opening_balance;
				if (ib.year_no == 0) sie_environment.set_opening_balance(ib.account_no,ib.opening_balance); // Only use "current" year opening balance
				// NOTE: The SIE-file defines a "year 0" that is "current year" as seen from the data in the file
				// See the #RAR tag that maps year_no 0,-1,... to actual date range (period / accounting year)
				// Example:
				// #RAR 0 20210501 20220430
				// #RAR -1 20200501 20210430				
			}
			else if (auto opt_entry = SIE::parse_VER(utf8_in)) {
				SIE::Ver ver = std::get<SIE::Ver>(*opt_entry);
				// std::cout << "\n\tVER!";
				auto me = to_entry(ver);
				sie_environment.post(me);
			}
			else if (auto opt_entry = SIE::parse_any_line(utf8_in)) {
				SIE::AnonymousLine al = std::get<SIE::AnonymousLine>(*opt_entry);
				// std::cout << "\n\tANY=" << al.str;
			}
			else break;
		}
		result = std::move(sie_environment);
	}
	return result;
}

void unposted_to_sie_file(SIEEnvironment const& sie,std::filesystem::path const& p) {
	// std::cout << "\nunposted_to_sie_file " << p;
	std::ofstream os{p};
	SIE::OStream sieos{os};
	// auto now = std::chrono::utc_clock::now();
	auto now = std::chrono::system_clock::now();
	auto now_timet = std::chrono::system_clock::to_time_t(now);
	auto now_local = localtime(&now_timet);
	sieos.os << "\n" << "#GEN " << std::put_time(now_local, "%Y%m%d");
	for (auto const& entry : sie.unposted()) {
		std::cout << "\nUnposted:" << entry; 
		sieos << to_sie_t(entry);
	}
}

std::vector<std::string> quoted_tokens(std::string const& cli) {
	// std::cout << "quoted_tokens count:" << cli.size();
	// for (char ch : cli) {
	// 	std::cout << " " << ch << ":" << static_cast<unsigned>(ch);
	// }
	std::vector<std::string> result{};
	std::istringstream in{cli};
	std::string token{};
	while (in >> std::quoted(token)) {
		// std::cout << "\nquoted token:" << token;
		result.push_back(token);
	}
	return result;
}

std::string prompt_line(PromptState const& prompt_state) {
	std::ostringstream prompt{};
	// prompt << options_list_of_prompt_state(prompt_state);
	prompt << "cratchit";
	switch (prompt_state) {
		case PromptState::Root: {
			prompt << ":";
		} break;
    case PromptState::LUARepl: {
			prompt << ":lua";
    } break;
		case PromptState::TAIndex: {
			prompt << ":tas";
		} break;
		case PromptState::AcceptNewTAs: {
			prompt << ":tas:accept";
		} break;
		case PromptState::HADIndex: {
			prompt << ":had";
		} break;
		case PromptState::VATReturnsFormIndex: {
			prompt << ":had:vat";
		} break;
		case PromptState::JEIndex: {
			prompt << ":had:je";
		} break;
		case PromptState::GrossDebitorCreditOption: {
			prompt << "had:aggregate:gross 0+or-:";
		} break;
		case PromptState::CounterTransactionsAggregateOption: {		
			prompt << "had:aggregate:counter:";
		} break;
		case PromptState::GrossAccountInput: {
			prompt << "had:aggregate:counter:gross";
		} break;
		case PromptState::NetVATAccountInput: {
			prompt << "had:aggregate:counter:net+vat";
		} break;
		case PromptState::JEAggregateOptionIndex: {
			prompt << ":had:je:1*at";
		} break;
		case PromptState::EnterHA: {
			prompt << ":had:je:ha";
		} break;
		case PromptState::ATIndex: {
			prompt << ":had:je:at";
		} break;
		case PromptState::EditAT: {
			prompt << ":had:je:at:edit";
		} break;
		case PromptState::CounterAccountsEntry: {
			prompt << "had:je:cat";
		} break;
		case PromptState::SKVEntryIndex: {
			prompt << ":skv";
		} break;
		case PromptState::QuarterOptionIndex: {
			prompt << ":skv:tax_return:period";
		} break;
		case PromptState::SKVTaxReturnEntryIndex: {
			prompt << ":skv:tax_return";
		} break;
		case PromptState::K10INK1EditOptions: {
			prompt << ":skv:to_tax";
		} break;
		case PromptState::INK2AndAppendixEditOptions: {
			prompt << ":skv:ink2";
		} break;
		case PromptState::EnterIncome: {
			prompt << ":skv:income?";
		} break;
		case PromptState::EnterDividend: {
			prompt << ":skv:dividend?";
		} break;
		case PromptState::EnterContact: {
			prompt << ":contact";
		} break;
	  case PromptState::EnterEmployeeID: {
			prompt << ":employee";
		} break;
		default: {
			prompt << ":??";
		}
	}
	prompt << ">";
	return prompt.str();
}

std::string to_had_listing_prompt(HeadingAmountDateTransEntries const& hads) {
		// Prepare to Expose hads (Heading Amount Date transaction entries) to the user
		std::stringstream prompt{};
		unsigned int index{0};
		std::vector<std::string> sHads{};
		std::transform(hads.begin(),hads.end(),std::back_inserter(sHads),[&index](auto const& had){
			std::stringstream os{};
			os << index++ << " " << had;
			return os.str();
		});
		prompt << "\n" << std::accumulate(sHads.begin(),sHads.end(),std::string{"Please select:"},[](auto acc,std::string const& entry) {
			acc += "\n  " + entry;
			return acc;
		});
		return prompt.str();
}

std::optional<int> to_signed_ix(std::string const& s) {
	std::optional<int> result{};
	try {
		const std::regex signed_ix_regex("^\\s*[+-]?\\d+$"); // +/-ddd...
		if (std::regex_match(s,signed_ix_regex)) result = std::stoi(s);
	}
	catch (...) {}
	return result;
}

// cratchit lua-to-cratchit interface
namespace lua_faced_ifc {
  // "hidden" implementation details
  namespace detail {
    OptionalHeadingAmountDateTransEntry to_had(lua_State *L)
    {
      // Check if the argument is a table
      if (!lua_istable(L, -1))
      {
        luaL_error(L, "Invalid argument. Expected a table representing a heading amount date object.");
        return {};
      }

      HeadingAmountDateTransEntry had;

      // Get the 'heading' field
      lua_getfield(L, -1, "heading");
      if (!lua_isstring(L, -1))
      {
        luaL_error(L, "Invalid table. Expected a string for 'heading'.");
        lua_pop(L, 1); // remove value from stack
        return {};
      }
      had.heading = lua_tostring(L, -1);
      lua_pop(L, 1); // remove value from stack

      // Get the 'amount' field
      lua_getfield(L, -1, "amount");
      if (!lua_isnumber(L, -1))
      {
        luaL_error(L, "Invalid table. Expected a number for 'amount'.");
        lua_pop(L, 1); // remove value from stack
        return {};
      }
      had.amount = lua_tonumber(L, -1);
      lua_pop(L, 1); // remove value from stack

      // Get the 'date' field
      lua_getfield(L, -1, "date");
      if (!lua_isstring(L, -1))
      {
        luaL_error(L, "Invalid table. Expected a string for 'date'.");
        lua_pop(L, 1); // remove value from stack
        return {};
      }
      had.date = *to_date(lua_tostring(L, -1)); // assume success
      lua_pop(L, 1); // remove value from stack

      // ... (repeat for other fields)

      return had;
    }
    static int had_to_string(lua_State *L)
    {
      // expect a lua table on the stack representing a HeadingAmountDateTransEntry
      if (auto had = detail::to_had(L)) {
        std::stringstream os{};
        os << *had;
        lua_pushstring(L, os.str().c_str());
        return 1;
      }
      else {
        return luaL_error(L, "Invalid argument. Expected a table on the form {heading = '', amount = 0, date = ''}");
      }
    };
  }
  // Expect a single string on the lua stack on the form "<heading> <amount> <date>"
  static int to_had(lua_State *L) {
      // Check if the argument is a string
      if (!lua_isstring(L, 1)) {
          // If not, throw an error
          return luaL_error(L, "Invalid argument. Expected a string.");
      }

      // Get the string from the Lua stack
      const char *str = lua_tostring(L, 1);

      // Create a table
      lua_newtable(L);

      /*
      // lua table to represent a HeadingAmountDateTransEntry
      headingAmountDateTransEntry = {
          heading = "",
          amount = 0,  -- Assuming Amount is a numeric type
          date = "",  -- Assuming Date is a string
          optional = {
              series = nil,  -- Assuming char can be represented as a single character string
              gross_account_no = nil,  -- Assuming AccountNo can be represented as a string or number
              current_candidate = nil,  -- Assuming OptionalMetaEntry can be represented as a string or number
              counter_ats_producer = nil,  -- Assuming ToNetVatAccountTransactions can be represented as a string or number
              vat_returns_form_box_map_candidate = nil  -- Assuming FormBoxMap can be represented as a string or number
          }
      }
      */

      // For simplicity, let's assume the string is "key=value" format
      std::string command(str);
      auto tokens = tokenize::splits(command,tokenize::SplitOn::TextAmountAndDate);
      if (auto had = ::to_had(tokens)) {
          // Push the key
          lua_pushstring(L, "heading");
          // Push the value
          lua_pushstring(L, had->heading.c_str());
          // Set the table value
          lua_settable(L, -3);

          // Push the key
          lua_pushstring(L, "amount");
          // Push the value
          lua_pushnumber(L, to_double(had->amount));
          // Set the table value
          lua_settable(L, -3);

          // Push the key
          lua_pushstring(L, "date");
          // Push the value
          lua_pushstring(L, to_string(had->date).c_str());
          // Set the table value
          lua_settable(L, -3);

          // Push the key
          lua_pushstring(L, "optional");
          // Push the value
          lua_newtable(L);

          // Push the key
          lua_pushstring(L, "series");
          // Push the value
          if (had->optional.series) {
            std::string series_str(1, *had->optional.series);
            lua_pushstring(L, series_str.c_str());
          }
          else lua_pushnil(L);
          // Set the table value
          lua_settable(L, -3);

          // Push the key
          lua_pushstring(L, "gross_account_no");
          // Push the value
          if (had->optional.gross_account_no) lua_pushstring(L, std::to_string(*had->optional.gross_account_no).c_str());
          else lua_pushnil(L);
          // Set the table value
          lua_settable(L, -3);

          // Push the key
          lua_pushstring(L, "current_candidate");
          // Push the value
          lua_pushnil(L);
          // Set the table value
          lua_settable(L, -3);

          // Push the key
          lua_pushstring(L, "counter_ats_producer");
          // Push the value
          lua_pushnil(L);
          // Set the table value
          lua_settable(L, -3);

          // Push the key
          lua_pushstring(L, "vat_returns_form_box_map_candidate");
          // Push the value
          lua_pushnil(L);
          // Set the table value
          lua_settable(L, -3);

          // Set the table value
          lua_settable(L, -3);
      }
      else {
          return luaL_error(L, "Invalid argument. Expected a string on the form '<heading> <amount> <date>'.");
      }

      // Create a new table to be used as the metatable
      lua_newtable(L);

      // Push the __tostring function onto the stack
      lua_pushstring(L, "__tostring");
      lua_pushcfunction(L, detail::had_to_string);
      lua_settable(L, -3);

      // Set the new table as the metatable of the original table
      lua_setmetatable(L, -2);      

      // The table is already on the stack, so just return the number of results
      return 1;
  }

  void register_cratchit_functions(lua_State *L) {
    lua_pushcfunction(L, lua_faced_ifc::to_had);
    lua_setglobal(L, "to_had");
  }

} // namespace lua
// ==================================================
// *** class Updater declaration ***
// ==================================================
class Updater {
public:
	Model model;
	Cmd operator()(Command const& command);
	Cmd operator()(Quit const& quit);
  Cmd operator()(Nop const& nop);
private:
	BAS::TypedMetaEntries all_years_template_candidates(auto const& matches);
  std::pair<std::string,PromptState> transition_prompt_state(PromptState const& from_state,PromptState const& to_state);
};

// ==================================================
// *** class Updater Definition ***
// ==================================================
Cmd Updater::operator()(Command const& command) {
  // std::cout << "\noperator(command=" << std::quoted(command) << ")";
  std::ostringstream prompt{};
  auto ast = quoted_tokens(command);
  if (false) {
    // DEBUG, Trace tokens
    std::cout << "\ntokens:";
    std::ranges::copy(ast,std::ostream_iterator<std::string>(std::cout, ";"));
  }
  if (ast.size() == 0) {
    // User hit <Enter> with no input
    if (model->prompt_state == PromptState::TAIndex) {
      prompt << options_list_of_prompt_state(model->prompt_state);
      // List current selection
      prompt << "\n<SELECTED>";
      // for (auto const& ta : model->all_date_ordered_tagged_amounts.tagged_amounts()) {
      int index = 0;
      for (auto const& ta : model->selected_date_ordered_tagged_amounts) {	
        prompt << "\n\t" << index++ << ". " << ta;
      }				
    }
    else if (model->prompt_state == PromptState::AcceptNewTAs) {
      // Reject new tagged amounts
      model->prompt_state = PromptState::TAIndex;
      prompt << "\n*Rejected*";
    }
    else if (model->prompt_state == PromptState::VATReturnsFormIndex) {
      // Assume the user wants to accept current Journal Entry Candidate
      if (auto had_iter = model->selected_had()) {
        auto& had = *(*had_iter);
        if (had.optional.current_candidate) {
          // We have a journal entry candidate - reset any VAT Returns form candidate for current had
          had.optional.vat_returns_form_box_map_candidate = std::nullopt;
          prompt << "VAT Consilidation Candidate " << *had.optional.current_candidate;
          model->prompt_state = PromptState::JEAggregateOptionIndex;
        }
      }
    }
    else {
      prompt << options_list_of_prompt_state(model->prompt_state);
    }
  }
  else {
    // We have at least one token in user input
    int signed_ix{};
    std::istringstream is{ast[0]};
    bool do_assign = (command.find('=') != std::string::npos);
    /* ======================================================


              Act on on Index?


      ====================================================== */
    if (auto signed_ix = to_signed_ix(ast[0]);
              do_assign == false
          and signed_ix 
          and model->prompt_state != PromptState::EditAT
          and model->prompt_state != PromptState::EnterIncome
          and model->prompt_state != PromptState::EnterDividend) {
      // std::cout << "\nAct on ix = " << *signed_ix << " in state:" << static_cast<int>(model->prompt_state);
      size_t ix = abs(*signed_ix);
      bool do_remove = (ast[0][0] == '-');
      // Act on prompt state index input
      switch (model->prompt_state) {
        case PromptState::Root: {
        } break;
        case PromptState::LUARepl: {
          prompt << "\nSorry, a single number in LUA script state does nothing. Please enter a valid LUA expression";
        } break;
        case PromptState::TAIndex: {
          model->ta_index = ix;
          if (auto ta_ptr_iter = model->selected_ta()) {
            auto& ta = *(*ta_ptr_iter);
            prompt << "\n" << ta;
          }
        } break;
        case PromptState::AcceptNewTAs: {
          switch (ix) {
            case 1: {
              // Accept the new tagged amounts created
              model->all_date_ordered_tagged_amounts += model->new_date_ordered_tagged_amounts;
              model->selected_date_ordered_tagged_amounts += model->new_date_ordered_tagged_amounts;
              model->prompt_state = PromptState::TAIndex;
              prompt << "\n*Accepted*";
              prompt << "\n\n" << options_list_of_prompt_state(model->prompt_state);
            } break;
            default: {
              prompt << "\nPlease enter a valid option";
              prompt << "\n\n" << options_list_of_prompt_state(model->prompt_state);
            }
          }
        } break;
        case PromptState::HADIndex: {
          model->had_index = ix;
          // Note: Side effect = model->selected_had() uses model->had_index to return the corresponing had ref.
          if (auto had_iter = model->selected_had()) {
            auto& had = *(*had_iter);
            prompt << "\n" << had;
            bool do_prepend = (ast.size() == 4) and (ast[1] == "<--"); // Command <Index> "<--" <Heading> <Date>
            // TODO In state PromptState::HADIndex (-hads) allow command <Index> <Heading> | <Amount> | <Date> to easilly modify single property of selected had
            // Command: 4 "Betalning Faktura" modifies the heading of had with index 4
            // Command: 4 20240123 modifies the date of had with index 4
            // Command: 4 173,50 modifies the amount of had with index 4
            OptionalDate new_date = (ast.size() == 2) ? to_date(ast[1]) : std::nullopt;
            OptionalAmount new_amount = (ast.size() == 2 and not new_date) ? to_amount(ast[1]) : std::nullopt;
            std::optional<std::string> new_heading = (ast.size() == 2 and not new_amount and not new_date) ? std::optional<std::string>(ast[1]) : std::nullopt;
            if (do_remove) {
              model->heading_amount_date_entries.erase(*had_iter);
              prompt << " REMOVED";
              model->prompt_state = PromptState::Root;
            }
            else if (do_assign) {
              prompt << "\nSorry, ASSIGN not yet implemented for your input " << std::quoted(command);
            }
            else if (do_prepend) {
              std::cout << "\nprepend with heading:" << std::quoted(ast[2]) << " Date:" << ast[3];
              HeadingAmountDateTransEntry prepended_had {
                .heading = ast[2]
                ,.amount = abs(had.amount) // always an unsigned amount for prepended had
                ,.date = *to_date(ast[3]) // Assume success
              };
              model->heading_amount_date_entries.push_back(prepended_had);
              prompt << to_had_listing_prompt(model->refreshed_hads());
            }
            else if (new_heading) {
              had.heading = *new_heading;
              prompt << "\n  *new* --> " << had;
            }
            else if (new_amount) {
              had.amount = *new_amount;
              prompt << "\n  *new* --> " << had;
            }
            else if (new_date) {
              had.date = *new_date;
              prompt << "\n  *new* --> " << had;
            }
            else if (ast.size() == 4 and (ast[1] == "-initiated_as")) {
              // Command <Had Index> "-initiated_as" <Heading> <Date>
              if (auto init_date = to_date(ast[3])) {
                HeadingAmountDateTransEntry initiating_had {
                  .heading = ast[2]
                  ,.amount = abs(had.amount) // always an unsigned amount for initiating had
                  ,.date = *init_date
                };
                model->heading_amount_date_entries.push_back(initiating_had);
                prompt << to_had_listing_prompt(model->refreshed_hads());
              } 
              else {
                prompt << "\nI failed to interpret " << std::quoted(ast[3]) << " as a date";
                prompt << "\nPlease enter a valid date for the event that intiated the indexed had (Syntax: <had Index> -initiated_as <Heading> <Date>)";
              }
            }            
            else {
              // selected HAD and list template options
              if (had.optional.vat_returns_form_box_map_candidate) {
                // provide the user with the ability to edit the propsed VAT Returns form
                {
                  // Adjust the sum in box 49
                  had.optional.vat_returns_form_box_map_candidate->at(49).clear();
                  had.optional.vat_returns_form_box_map_candidate->at(49).push_back(SKV::XML::VATReturns::dummy_mat(-SKV::XML::VATReturns::to_box_49_amount(*had.optional.vat_returns_form_box_map_candidate)));
                  for (auto const& [box_no,mats] : *had.optional.vat_returns_form_box_map_candidate)  {
                    prompt << "\n" << box_no << ": [" << box_no << "] = " << BAS::mats_sum(mats);
                  }
                  BAS::MetaEntry me{
                    .meta = {
                      .series = 'M'
                    }
                    ,.defacto = {
                      .caption = had.heading
                      ,.date = had.date
                    }
                  };
                  std::map<BAS::AccountNo,Amount> account_amounts{};
                  for (auto const& [box_no,mats] : *had.optional.vat_returns_form_box_map_candidate)  {
                    for (auto const& mat : mats) {
                      account_amounts[mat.defacto.account_no] += mat.defacto.amount;
                      // std::cout << "\naccount_amounts[" << mat.defacto.account_no << "] += " << mat.defacto.amount;
                    }
                  }
                  for (auto const& [account_no,amount] : account_amounts) {
                    // account_amounts[0] = 4190.54
                    // account_amounts[2614] = -2364.4
                    // account_amounts[2640] = 2364.4
                    // account_amounts[2641] = 4190.54
                    // account_amounts[3308] = -888.1
                    // account_amounts[9021] = 11822

                    // std::cout << "\naccount_amounts[" << account_no << "] = " << amount;

                    // account_no == 0 is the dummy account for the VAT Returns form "sum" VAT
                    // Book this on BAS 2650
                    // NOTE: Is "sum" is positive we could use 1650 (but 2650 is viable for both positive and negative VAT "debts")
                    if (account_no==0) {
                      me.defacto.account_transactions.push_back({
                        .account_no = 2650
                        ,.amount = trunc(-amount)
                      });
                      me.defacto.account_transactions.push_back({
                        .account_no = 3740
                        ,.amount = BAS::to_cents_amount(-amount - trunc(-amount)) // to_tax(-amount) + diff = -amount
                      });
                    }
                    else {
                      me.defacto.account_transactions.push_back({
                        .account_no = account_no
                        ,.amount = BAS::to_cents_amount(-amount)
                      });
                    }
                    // Hard code reversal of VAT Returns report of EU Purchase (to have it not turn up on next report)
                    if (account_no == 9021) {
                      me.defacto.account_transactions.push_back({
                        .account_no = 9099
                        ,.transtext = "Avbokning (20) 9021"
                        ,.amount = BAS::to_cents_amount(amount)
                      });
                    }
                    // Hard code reversal of VAT Returns report of EU sales of services (to have it not turn up on next report)
                    if (account_no == 3308) {
                      me.defacto.account_transactions.push_back({
                        .account_no = 9099
                        ,.transtext = "Avbokning (39) 3308"
                        ,.amount = BAS::to_cents_amount(amount)
                      });
                    }
                  }
                  had.optional.current_candidate = me;

                  prompt << "\nCandidate: " << me;
                  model->prompt_state = PromptState::VATReturnsFormIndex;
                }
              }
              else if (had.optional.current_candidate) {
                prompt << "\n\t" << *had.optional.current_candidate;
                if (had.optional.counter_ats_producer) {
                  // We alreade have a "counter transactions" producer.
                  // Go directly to state for user to apply it to complete the candidate
                  model->prompt_state = PromptState::EnterHA;
                }
                else {
                  // The user has already selected a candidate
                  // But have not yet gone through assigning a "counter transaction" producer
                  model->prompt_state = PromptState::JEAggregateOptionIndex;
                }
              }
              else {
                // Selected HAD is "naked" (no candidate for book keeping assigned)
                model->had_index = ix;
                model->template_candidates = this->all_years_template_candidates([had](BAS::anonymous::JournalEntry const& aje){
                  return had_matches_trans(had,aje);
                });

                {
                  // hard code a template for Inventory gross + n x (ex_vat + vat) journal entry
                  // 0  *** "Amazon" 20210924
                  // 	"Leverantörsskuld":2440 "" -1489.66
                  // 	"Invenmtarier":1221 "Garmin Edge 130" 1185.93
                  // 	"Debiterad ingående moms":2641 "Garmin Edge 130" 303.73

                  // struct JournalEntry {
                  // 	std::string caption{};
                  // 	Date date{};
                  // 	AccountTransactions account_transactions;
                  // };

                  Amount amount{1000};
                  BAS::MetaEntry me{
                    .meta = {
                      .series = 'A'
                    }
                    ,.defacto = {
                      .caption = "Bestallning Inventarie"
                      ,.date = had.date
                    }
                  };
                  BAS::anonymous::AccountTransaction gross_at{
                    .account_no = 2440
                    ,.amount = -amount
                  };
                  BAS::anonymous::AccountTransaction net_at{
                    .account_no = 1221
                    ,.amount = static_cast<Amount>(amount*0.8f)
                  };
                  BAS::anonymous::AccountTransaction vat_at{
                    .account_no = 2641
                    ,.amount = static_cast<Amount>(amount*0.2f)
                  };
                  me.defacto.account_transactions.push_back(gross_at);
                  me.defacto.account_transactions.push_back(net_at);
                  me.defacto.account_transactions.push_back(vat_at);
                  model->template_candidates.push_back(to_typed_meta_entry(me));
                }

                // List options for how the HAD may be registered into the books based on available candidates
                unsigned ix = 0;
                for (int i=0; i < model->template_candidates.size(); ++i) {
                  prompt << "\n    " << ix++ << " " << model->template_candidates[i];
                }
                model->prompt_state = PromptState::JEIndex;
              }
            }
          }
          else {
            prompt << "\nplease enter a valid index";
          }
        } break;
        case PromptState::VATReturnsFormIndex: {
          if (ast.size() > 0) {
            // Asume the user has selected an index for an entry on the proposed VAT Returns form to edit
            if (auto had_iter = model->selected_had()) {
              auto& had = *(*had_iter);
              if (had.optional.vat_returns_form_box_map_candidate) {
                if (had.optional.vat_returns_form_box_map_candidate->contains(ix)) {
                  auto box_no = ix;
                  auto& mats = had.optional.vat_returns_form_box_map_candidate->at(box_no);
                  if (auto amount = to_amount(ast[1]);amount and mats.size()>0) {
                    auto mats_sum = BAS::mats_sum(mats);
                    auto sign = (mats_sum<0)?-1:1;
                    // mats_sum + diff = amount
                    auto diff = sign*(abs(*amount)) - mats_sum;
                    mats.push_back({
                      .defacto = {
                        .account_no = mats[0].defacto.account_no
                        ,.transtext = "diff"
                        ,.amount = diff
                      }
                    });
// std::cout << "\n[" << box_no << "]";
                    for (auto const& mat : mats) {
// std::cout << "\n\t" << mat;
                    }
// std::cout << "\n\t--------------------";
// std::cout << "\n\tsum " << BAS::mats_sum(mats);
                  }
                  else {
                    prompt << "\nPlease enter an entry index and a positive amount (will apply the sign required by the form)";
                  }
                }
                else {
                  prompt << "\nPlease enter a valid VAT Returns form entry index";
                  // provide the user with the ability to edit the propsed VAT Returns form
                }
                {
                  // Adjust the sum in box 49
                  had.optional.vat_returns_form_box_map_candidate->at(49).clear();
                  had.optional.vat_returns_form_box_map_candidate->at(49).push_back(SKV::XML::VATReturns::dummy_mat(-SKV::XML::VATReturns::to_box_49_amount(*had.optional.vat_returns_form_box_map_candidate)));
                  for (auto const& [box_no,mats] : *had.optional.vat_returns_form_box_map_candidate)  {
                    prompt << "\n" << box_no << ": [" << box_no << "] = " << BAS::mats_sum(mats);
                  }
                  BAS::MetaEntry me{
                    .meta = {
                      .series = 'M'
                    }
                    ,.defacto = {
                      .caption = had.heading
                      ,.date = had.date
                    }
                  };
                  std::map<BAS::AccountNo,Amount> account_amounts{};
                  for (auto const& [box_no,mats] : *had.optional.vat_returns_form_box_map_candidate)  {
                    for (auto const& mat : mats) {
                      account_amounts[mat.defacto.account_no] += mat.defacto.amount;
// std::cout << "\naccount_amounts[" << mat.defacto.account_no << "] += " << mat.defacto.amount;
                    }
                  }
                  for (auto const& [account_no,amount] : account_amounts) {
                    // account_amounts[0] = 4190.54
                    // account_amounts[2614] = -2364.4
                    // account_amounts[2640] = 2364.4
                    // account_amounts[2641] = 4190.54
                    // account_amounts[3308] = -888.1
                    // account_amounts[9021] = 11822
// std::cout << "\naccount_amounts[" << account_no << "] = " << amount;
                    // account_no == 0 is the dummy account for the VAT Returns form "sum" VAT
                    // Book this on BAS 2650
                    // NOTE: Is "sum" is positive we could use 1650 (but 2650 is viable for both positive and negative VAT "debts")
                    if (account_no==0) {
                      me.defacto.account_transactions.push_back({
                        .account_no = 2650
                        ,.amount = trunc(-amount)
                      });
                      me.defacto.account_transactions.push_back({
                        .account_no = 3740
                        ,.amount = BAS::to_cents_amount(-amount - trunc(-amount)) // to_tax(-amount) + diff = -amount
                      });
                    }
                    else {
                      me.defacto.account_transactions.push_back({
                        .account_no = account_no
                        ,.amount = BAS::to_cents_amount(-amount)
                      });
                    }
                    // Hard code reversal of VAT Returns report of EU Purchase (to have it not turn up on next report)
                    if (account_no == 9021) {
                      me.defacto.account_transactions.push_back({
                        .account_no = 9099
                        ,.transtext = "Avbokning (20) 9021"
                        ,.amount = BAS::to_cents_amount(amount)
                      });
                    }
                    // Hard code reversal of VAT Returns report of EU sales of services (to have it not turn up on next report)
                    if (account_no == 3308) {
                      me.defacto.account_transactions.push_back({
                        .account_no = 9099
                        ,.transtext = "Avbokning (39) 3308"
                        ,.amount = BAS::to_cents_amount(amount)
                      });
                    }
                  }
                  had.optional.current_candidate = me;

                  prompt << "\nCandidate: " << me;
                  model->prompt_state = PromptState::VATReturnsFormIndex;
                }
              }
              else {
                prompt << "\nPlease re-enter a valid HAD and journal entry candidate (I seem to no longer have a valid VAT Returns form candidate to process)";
              }
            }
            else {
              prompt << "\nPlease re-enter a valid HAD index (It seems I have no recoprd of a selected HAD at the moment)";
            }
          }
        } break;
        case PromptState::JEIndex: {
          // Wait for user to choose how to create a journal entry from selected HAD
          if (auto had_iter = model->selected_had()) {
            // We have a selected HAD ok
            auto& had = *(*had_iter);
            if (auto account_no = BAS::to_account_no(command)) {
              // Assume user entered an account number for a Gross + 1..n <Ex vat, Vat> account entries
              BAS::MetaEntry me{
                .defacto = {
                    .caption = had.heading
                  ,.date = had.date
                }
              };
              me.defacto.account_transactions.emplace_back(BAS::anonymous::AccountTransaction{.account_no=*account_no,.amount=had.amount});
              had.optional.current_candidate = me;
              prompt << "\ncandidate:" << me;
              model->prompt_state = PromptState::GrossDebitorCreditOption;
            }
            else {
              // Assume user selected an entry index as base for a template
              auto tme_iter = model->template_candidates.begin();
              auto tme_end = model->template_candidates.end();
              auto at_iter = model->at_candidates.begin();
              auto at_end = model->at_candidates.end();
              if (ix < std::distance(tme_iter,tme_end)) {
                std::advance(tme_iter,ix);
                auto tme = *tme_iter;
                auto vat_type = to_vat_type(tme);
                std::cout << "\nvat_type = " << vat_type;
                switch (vat_type) {
                  case JournalEntryVATType::Undefined:
                    prompt << "\nSorry, I encountered Undefined VAT type for " << tme;
                    break; // *NOP*
                  case JournalEntryVATType::Unknown:
                    prompt << "\nSorry, I encountered Unknown VAT type for " << tme;
                    break; // *NOP*
                  case JournalEntryVATType::NoVAT: {
                    // No VAT in candidate. 
                    // Continue with 
                    // 1) Some proposed gross account transactions
                    // 2) a n x gross Counter aggregate

                    // NOTE 20240516 - A transfer to or from the SKV account is for now treated as a "No VAT" or "plain" transfer

                    auto tp = to_template(*tme_iter);
                    if (tp) {
                      auto me = to_journal_entry(had,*tp);
                      prompt << "\nPlain transfer " << me;
                      had.optional.current_candidate = me;
                      model->prompt_state = PromptState::JEAggregateOptionIndex;
                    }
                  } break;
                  case JournalEntryVATType::SwedishVAT: {
                    // Swedish VAT detcted in candidate.
                    // Continue with 
                    // 2) a n x {net,vat} counter aggregate
                    auto tp = to_template(*tme_iter);
                    if (tp) {
                      auto me = to_journal_entry(had,*tp);
                      prompt << "\nSwedish VAT candidate " << me;
                      had.optional.current_candidate = me;
                      model->prompt_state = PromptState::JEAggregateOptionIndex;
                    }
                  } break;
                  case JournalEntryVATType::EUVAT: {
                    // EU VAT detected in candidate.
                    // Continue with a 
                    // 2) n x gross counter aggregate + an EU VAT Returns "virtual" aggregate
                    // #1 hard code for EU VAT Candidate
                    BAS::MetaEntry me {
                      .defacto = {
                        .caption = had.heading
                        ,.date = had.date
                      }
                    };
                    for (auto const& [at,props] : tme_iter->defacto.account_transactions) {
                      if (props.contains("gross") or props.contains("eu_purchase")) {
                        int sign = (at.amount < 0)?-1:1; // 0 treated as +
                        BAS::anonymous::AccountTransaction new_at{
                          .account_no = at.account_no
                          ,.transtext = std::nullopt
                          ,.amount = sign*abs(had.amount)
                        };
                        me.defacto.account_transactions.push_back(new_at);
                      }
                      else if (props.contains("vat")) {
                        int sign = (at.amount < 0)?-1:1; // 0 treated as +
                        BAS::anonymous::AccountTransaction new_at{
                          .account_no = at.account_no
                          ,.transtext = std::nullopt
                          ,.amount = sign*abs(had.amount*0.2f)
                        };
                        me.defacto.account_transactions.push_back(new_at);
                        prompt << "\nNOTE: Assumed 25% VAT for " << new_at;
                      }
                    }											
                    prompt << "\nEU VAT candidate " << me;
                    had.optional.current_candidate = me;
                    model->prompt_state = PromptState::JEAggregateOptionIndex;
                  } break;
                  case JournalEntryVATType::VATReturns: {
                    //  M2 "Momsrapport 2021-07-01 - 2021-09-30" 20210930
                    // 	 vat = sort_code: 0x6 : "Utgående moms, 25 %":2610 "" 83300
                    // 	 eu_vat vat = sort_code: 0x56 : "Utgående moms omvänd skattskyldighet, 25 %":2614 "" 1654.23
                    // 	 vat = sort_code: 0x6 : "Ingående moms":2640 "" -1690.21
                    // 	 vat = sort_code: 0x6 : "Debiterad ingående moms":2641 "" -849.52
                    // 	 vat = sort_code: 0x6 : "Redovisningskonto för moms":2650 "" -82415
                    // 	 cents = sort_code: 0x7 : "Öres- och kronutjämning":3740 "" 0.5

                    // TODO: Consider to iterate over all bas accounts defined for VAT Return form
                    //       and create a candidate that will zero out these for period given by date (end of VAT period)
                    BAS::MetaEntry me {
                      .defacto = {
                        .caption = had.heading
                        ,.date = had.date
                      }
                    };
                    me.defacto.account_transactions.push_back({
                      .account_no = 2610 // "Utgående moms, 25 %"
                      ,.transtext = std::nullopt
                      ,.amount = 0
                    });
                    me.defacto.account_transactions.push_back({
                      .account_no = 2614 // "Utgående moms omvänd skattskyldighet, 25 %"
                      ,.transtext = std::nullopt
                      ,.amount = 0
                    });
                    me.defacto.account_transactions.push_back({
                      .account_no = 2640 // "Ingående moms"
                      ,.transtext = std::nullopt
                      ,.amount = 0
                    });
                    me.defacto.account_transactions.push_back({
                      .account_no = 2641 // "Debiterad ingående moms"
                      ,.transtext = std::nullopt
                      ,.amount = 0
                    });
                    me.defacto.account_transactions.push_back({
                      .account_no = 2650 // "Redovisningskonto för moms"
                      ,.transtext = std::nullopt
                      ,.amount = had.amount
                    });
                    me.defacto.account_transactions.push_back({
                      .account_no = 3740 // "Öres- och kronutjämning"
                      ,.transtext = std::nullopt
                      ,.amount = 0
                    });
                    prompt << "\nVAT Consolidate candidate " << me;
                    had.optional.current_candidate = me;
                    model->prompt_state = PromptState::JEAggregateOptionIndex;
                  } break;
                  case JournalEntryVATType::VATClearing: {
                    auto tp = to_template(*tme_iter);
                    if (tp) {
                      auto me = to_journal_entry(had,*tp);
                      prompt << "\nVAT clearing candidate " << me;
                      had.optional.current_candidate = me;
                      model->prompt_state = PromptState::JEAggregateOptionIndex;
                    }                  
                  } break;
                  case JournalEntryVATType::SKVInterest: {
                    auto tp = to_template(*tme_iter);
                    if (tp) {
                      auto me = to_journal_entry(had,*tp);
                      prompt << "\nTax free SKV interest " << me;
                      had.optional.current_candidate = me;
                      model->prompt_state = PromptState::JEAggregateOptionIndex;
                    }
                  } break;
                  case JournalEntryVATType::SKVFee: {
                    if (false) {

                    }
                    else {
                      prompt << "\nSorry, I have yet to become capable to create an SKV Fee entry from template " << tme;
                    }
                  } break;
                  case JournalEntryVATType::VATTransfer: {
                    // 10  A2 "Utbetalning Moms från Skattekonto" 20210506
                    // transfer = sort_code: 0x1 : "Avräkning för skatter och avgifter (skattekonto)":1630 "Utbetalning" -802
                    // transfer = sort_code: 0x1 : "Avräkning för skatter och avgifter (skattekonto)":1630 "Momsbeslut" 802
                    // transfer vat = sort_code: 0x16 : "Momsfordran":1650 "" -802
                    // transfer = sort_code: 0x1 : "PlusGiro":1920 "" 802
                    if (tme.defacto.account_transactions.size()>0) {
                      bool first{true};
                      Amount amount{};
                      if (std::all_of(tme.defacto.account_transactions.begin(),tme.defacto.account_transactions.end(),[&amount,&first](auto const& entry){
                        // Lambda that returns true as long as all entries have the same absolute amount as the first entry
                        if (first) {
                          amount = abs(entry.first.amount);
                          first = false;
                          return true;
                        }
                        return (abs(entry.first.amount) == amount);
                      })) {
                        BAS::MetaEntry me {
                          .defacto = {
                            .caption = had.heading
                            ,.date = had.date
                          }
                        };
                        for (auto const& tat : tme.defacto.account_transactions) {
                          auto sign = (tat.first.amount < 0)?-1:+1;
                          me.defacto.account_transactions.push_back({
                            .account_no = tat.first.account_no
                            ,.transtext = std::nullopt
                            ,.amount = sign*abs(had.amount)
                          });
                        }
                        had.optional.current_candidate = me;
                        prompt << "\nVAT Settlement candidate " << me;
                        model->prompt_state = PromptState::JEAggregateOptionIndex;
                      }
                    }
                  } break;
                }
              }
              else if (auto at_ix = (ix - std::distance(tme_iter,tme_end));at_ix < std::distance(at_iter,at_end)) {
                prompt << "\nTODO: Implement acting on selected gross account transaction " << model->at_candidates[at_ix];
              }
              else {
                prompt << "\nPlease enter a valid index";
              }
            }
          }
          else {
            prompt << "\nPlease re-enter a valid HAD index (It seems I have no record of a selected HAD at the moment)";
          }
        } break;
        case PromptState::GrossDebitorCreditOption: {
          if (auto had_iter = model->selected_had()) {
            auto& had = *(*had_iter);
            if (had.optional.current_candidate) {
              if (had.optional.current_candidate->defacto.account_transactions.size()==1) {
                switch (ix) {
                  case 0: {
                    // As is
                    model->prompt_state = PromptState::CounterTransactionsAggregateOption;
                  } break;
                  case 1: {
                    // Force debit
                    had.optional.current_candidate->defacto.account_transactions[0].amount = abs(had.optional.current_candidate->defacto.account_transactions[0].amount);
                    model->prompt_state = PromptState::CounterTransactionsAggregateOption;
                  } break;
                  case 2: {
                      // Force credit
                    had.optional.current_candidate->defacto.account_transactions[0].amount = -1.0f * abs(had.optional.current_candidate->defacto.account_transactions[0].amount);
                    model->prompt_state = PromptState::CounterTransactionsAggregateOption;
                  } break;
                  default: {
                    prompt << "\nPlease enter a valid index. I don't know how to interpret option " << ix;
                  }
                }
              }
              else {
                prompt << "\nPlease re-enter a valid HAD and journal entry candidate (It seems current candidate have more than one transaction defined which confuses me)";
              }
              prompt << "\ncandidate:" << *had.optional.current_candidate;
            }
            else {
              prompt << "\nPlease re-enter a valid HAD and journal entry candidate (I seem to no longer have a valid journal entry candidate for current HAD to process)";
            }
          }
          else {
            prompt << "\nPlease re-enter a valid HAD index (It seems I have no recoprd of a selected HAD at the moment)";
          }
        } break;
        case PromptState::CounterTransactionsAggregateOption: {
          if (auto had_iter = model->selected_had()) {
            auto& had = *(*had_iter);
            if (had.optional.current_candidate) {
              if (had.optional.current_candidate->defacto.account_transactions.size()==1) {
                switch (ix) {
                  case 0: {
                    // Gross counter transaction aggregate
                    model->prompt_state = PromptState::GrossAccountInput;
                  } break;
                  case 1: {
                    // {net,VAT} counter transactions aggregate
                    model->prompt_state = PromptState::NetVATAccountInput;
                  } break;										
                  default: {
                    prompt << "\nPlease enter a valid index. I don't know how to interpret option " << ix;
                  }
                }
              }
              else {
                prompt << "\nPlease re-enter a valid HAD and journal entry candidate (It seems current candidate have more than one transaction defined which confuses me)";
              }
              prompt << "\ncandidate:" << *had.optional.current_candidate;
            }
            else {
              prompt << "\nPlease re-enter a valid HAD and journal entry candidate (I seem to no longer have a valid journal entry candidate for current HAD to process)";
            }
          }
          else {
            prompt << "\nPlease re-enter a valid HAD index (It seems I have no recoprd of a selected HAD at the moment)";
          }
        } break;
        case PromptState::GrossAccountInput: {
          // Act on user gross account number input
          if (auto had_iter = model->selected_had()) {
            auto& had = *(*had_iter);
            if (had.optional.current_candidate) {
              if (had.optional.current_candidate->defacto.account_transactions.size()==1) {
                if (ast.size() == 1) {
                  auto gross_counter_account_no = BAS::to_account_no(ast[0]);
                  if (gross_counter_account_no) {
                    Amount gross_transaction_amount = had.optional.current_candidate->defacto.account_transactions[0].amount;
                    had.optional.current_candidate->defacto.account_transactions.push_back(BAS::anonymous::AccountTransaction{
                      .account_no = *gross_counter_account_no
                      ,.amount = -1.0f * gross_transaction_amount
                    }
                    );
                    prompt << "\nmutated candidate:" << *had.optional.current_candidate;
                    model->prompt_state = PromptState::JEAggregateOptionIndex;											
                  }
                  else {
                    prompt << "\nPlease enter a a valid single gross counter amount account number (it seems I don't understand your input " << std::quoted(command) << ")";
                  }
                }
                else {
                  prompt << "\nPlease enter two single gross counter amount account number (it seems I interpret " << std::quoted(command) << " the wrong number of arguments";
                }
              }
              else {
                prompt << "\nPlease re-enter a valid HAD and journal entry candidate (It seems current candidate have more than one transaction defined which confuses me)";
              }
              prompt << "\ncandidate:" << *had.optional.current_candidate;
            }
            else {
              prompt << "\nPlease re-enter a valid HAD and journal entry candidate (I seem to no longer have a valid journal entry candidate for current HAD to process)";
            }
          }
          else {
            prompt << "\nPlease re-enter a valid HAD index (It seems I have no recoprd of a selected HAD at the moment)";
          }
        } break;
        case PromptState::NetVATAccountInput: {
          if (auto had_iter = model->selected_had()) {
            auto& had = *(*had_iter);
            if (had.optional.current_candidate) {
              if (had.optional.current_candidate->defacto.account_transactions.size()==1) {
                if (ast.size() == 2) {
                  auto net_counter_account_no = BAS::to_account_no(ast[0]);
                  auto vat_counter_account_no = BAS::to_account_no(ast[1]);
                  if (net_counter_account_no and vat_counter_account_no) {
                    Amount gross_transaction_amount = had.optional.current_candidate->defacto.account_transactions[0].amount;
                    had.optional.current_candidate->defacto.account_transactions.push_back(BAS::anonymous::AccountTransaction{
                      .account_no = *net_counter_account_no
                      ,.amount = -0.8f * gross_transaction_amount
                    }
                    );
                    had.optional.current_candidate->defacto.account_transactions.push_back(BAS::anonymous::AccountTransaction{
                      .account_no = *vat_counter_account_no
                      ,.amount = -0.2f * gross_transaction_amount
                    }
                    );
                    prompt << "\nNOTE: Cratchit currently assumes 25% VAT";
                    model->prompt_state = PromptState::JEAggregateOptionIndex;
                  }
                  else {
                    prompt << "\nPlease enter a a valid single gross counter amount account number (it seems I don't understand your input " << std::quoted(command) << ")";
                  }
                }
                else {
                  prompt << "\nPlease enter two single gross counter amount account number (it seems I interpret " << std::quoted(command) << " the wrong number of arguments";
                }
              }
              else {
                prompt << "\nPlease re-enter a valid HAD and journal entry candidate (It seems current candidate have more than one transaction defined which confuses me)";
              }
              prompt << "\ncandidate:" << *had.optional.current_candidate;
            }
            else {
              prompt << "\nPlease re-enter a valid HAD and journal entry candidate (I seem to no longer have a valid journal entry candidate for current HAD to process)";
            }
          }
          else {
            prompt << "\nPlease re-enter a valid HAD index (It seems I have no recoprd of a selected HAD at the moment)";
          }
        } break;
        case PromptState::JEAggregateOptionIndex: {
          // ":had:je:1or*";
          if (auto had_iter = model->selected_had()) {
            auto& had = *(*had_iter);
            if (had.optional.current_candidate) {
              // We need a typed entry to do some clever decisions
              auto tme = to_typed_meta_entry(*had.optional.current_candidate);
              prompt << "\n" << tme;
              auto vat_type = to_vat_type(tme); 
              switch (vat_type) {
                case JournalEntryVATType::NoVAT: {
                  // No VAT in candidate. 
                  // Continue with 
                  // 1) Some propose gross account transactions
                  // 2) a n x gross Counter aggregate
                } break;
                case JournalEntryVATType::SwedishVAT: {
                  // Swedish VAT detcted in candidate.
                  // Continue with 
                  // 2) a n x {net,vat} counter aggregate
                } break;
                case JournalEntryVATType::EUVAT: {
                  // EU VAT detected in candidate.
                  // Continue with a 
                  // 2) n x gross counter aggregate + an EU VAT Returns "virtual" aggregate
                } break;
                case JournalEntryVATType::VATReturns: {
                  // All VATS (VAT Report?)
                } break;
                case JournalEntryVATType::VATTransfer: {
                  // All VATS and one gross non-vat (assume bank transfer of VAT to/from tax agency?)
                } break;
                default: {std::cout << "\nDESIGN INSUFFICIENCY - Unknown JournalEntryVATType " << vat_type;}
              }
              // std::map<std::string,unsigned int> props_counter{};
              // for (auto const& [at,props] : tme.defacto.account_transactions) {
              // 	for (auto const& prop : props) props_counter[prop]++;
              // }
              // for (auto const& [prop,count] : props_counter) {
              // 	prompt << "\n" << std::quoted(prop) << " count:" << count; 
              // }
              // auto props_sum = std::accumulate(props_counter.begin(),props_counter.end(),unsigned{0},[](auto acc,auto const& entry){
              // 	acc += entry.second;
              // 	return acc;
              // });
              // int vat_type{-1}; // unidentified VAT
              // // Identify what type of VAT the candidate defines
              // if ((props_counter.size() == 1) and props_counter.contains("gross")) {
              // 	vat_type = 0; // NO VAT (gross, counter gross)
              // 	prompt << "\nTemplate is an NO VAT transaction :)"; // gross,gross
              // }
              // else if ((props_counter.size() == 3) and props_counter.contains("gross") and props_counter.contains("net") and props_counter.contains("vat") and !props_counter.contains("eu_vat")) {
              // 	if (props_sum == 3) {
              // 		prompt << "\nTemplate is a SWEDISH PURCHASE/sale"; // (gross,net,vat);
              // 		vat_type = 1; // Swedish VAT
              // 	}
              // }
              // else if (
              // 	(     (props_counter.contains("gross"))
              // 		and (props_counter.contains("eu_purchase"))
              // 		and (props_counter.contains("eu_vat")))) {
              // 	vat_type = 2; // EU VAT
              // 	prompt << "\nTemplate is an EU PURCHASE :)"; // gross,gross,eu_vat,eu_vat,eu_purchase,eu_purchase
              // }
              // else {
              // 	prompt << "\nFailed to recognise the VAT type";
              // }

              switch (ix) {
                case 0: {
                  // Try to stage gross + single counter transactions aggregate
                  if (does_balance(had.optional.current_candidate->defacto) == false) {
                    // list at candidates from found entries with account transaction that counter the gross account
                    std::cout << "\nCurrent candidate does not balance";
                  }
                  else if (std::any_of(had.optional.current_candidate->defacto.account_transactions.begin(),had.optional.current_candidate->defacto.account_transactions.end(),[](BAS::anonymous::AccountTransaction const& at){
                    return abs(at.amount) < 1.0;
                  })) {
                    // Assume the user need to specify rounding by editing proposed account transactions
                    if (false) {
                      // 20240526 - new 'enter state' prompt mechanism
                      auto new_state = PromptState::ATIndex;
                      prompt << model->to_prompt_for_entering_state(new_state);
                      model->prompt_state = new_state;
                    }
                    else {
                      unsigned int i{};
                      std::for_each(had.optional.current_candidate->defacto.account_transactions.begin(),had.optional.current_candidate->defacto.account_transactions.end(),[&i,&prompt](auto const& at){
                        prompt << "\n  " << i++ << " " << at;
                      });
                      model->prompt_state = PromptState::ATIndex;
                    }
                  }
                  else {
                    // Stage as-is
                    if (auto staged_me = model->sie["current"].stage(*had.optional.current_candidate)) {
                      prompt << "\n" << *staged_me << " STAGED";
                      model->heading_amount_date_entries.erase(*had_iter);
                      model->prompt_state = PromptState::HADIndex;
                    }
                    else {
                      prompt << "\nSORRY - Failed to stage entry";
                      model->prompt_state = PromptState::Root;
                    }
                  }
                } break;
                case 1: {
                  // net + vat counter aggregate
                  BAS::anonymous::OptionalAccountTransaction net_at;
                  BAS::anonymous::OptionalAccountTransaction vat_at;
                  for (auto const& [at,props] : tme.defacto.account_transactions) {
                    if (props.contains("net")) net_at = at;
                    if (props.contains("vat")) vat_at = at;
                  }
                  if (!net_at) std::cout << "\nNo net_at";
                  if (!vat_at) std::cout << "\nNo vat_at";
                  if (net_at and vat_at) {
                    had.optional.counter_ats_producer = ToNetVatAccountTransactions{*net_at,*vat_at};
                    
                    BAS::anonymous::AccountTransactions ats_to_keep{};
                    std::remove_copy_if(
                      had.optional.current_candidate->defacto.account_transactions.begin()
                      ,had.optional.current_candidate->defacto.account_transactions.end()
                      ,std::back_inserter(ats_to_keep)
                      ,[&net_at,&vat_at](auto const& at){
                        return ((at.account_no == net_at->account_no) or (at.account_no == vat_at->account_no));
                    });
                    had.optional.current_candidate->defacto.account_transactions = ats_to_keep;
                  }
                  prompt << "\ncadidate: " << *had.optional.current_candidate;
                  model->prompt_state = PromptState::EnterHA;
                } break;
                case 2: {
                  // Allow the user to edit individual account transactions
                  if (true) {
                    // 20240526 - new 'enter state' prompt mechanism
                    auto new_state = PromptState::ATIndex;
                    prompt << model->to_prompt_for_entering_state(new_state);
                    model->prompt_state = new_state;
                  }
                  else {
                    unsigned int i{};
                    std::for_each(had.optional.current_candidate->defacto.account_transactions.begin(),had.optional.current_candidate->defacto.account_transactions.end(),[&i,&prompt](auto const& at){
                      prompt << "\n  " << i++ << " " << at;
                    });
                    model->prompt_state = PromptState::ATIndex;
                  }
                } break;
                case 3: {
                  // Stage the candidate
                  if (auto staged_me = model->sie["current"].stage(*had.optional.current_candidate)) {
                    prompt << "\n" << *staged_me << " STAGED";
                    model->heading_amount_date_entries.erase(*had_iter);
                    model->prompt_state = PromptState::HADIndex;
                  }
                  else {
                    prompt << "\nSORRY - Failed to stage entry";
                    model->prompt_state = PromptState::Root;
                  }
                }
                default: {
                  prompt << "\nPlease enter a valid had index";
                } break;			
              }
            }
            else {
              prompt << "\nPlease re-enter a valid HAD and journal entry candidate (I seem to no longer have a valid journal entry candidate for current HAD to process)";
            }
          }
          else {
            prompt << "\nPlease re-enter a valid had index";
          }

        } break;
        case PromptState::ATIndex: {
          if (auto had_iter = model->selected_had()) {
            auto& had = **had_iter;
            if (ast.size() == 2) {
              auto bas_account_no = BAS::to_account_no(ast[0]);
              auto amount = to_amount(ast[1]);
              if (bas_account_no and amount) {
                // push back a new account transaction with detected BAS account and amount
                BAS::anonymous::AccountTransaction at {
                  .account_no = *bas_account_no
                  ,.amount = *amount
                };
                had.optional.current_candidate->defacto.account_transactions.push_back(at);
                unsigned int i{};
                std::for_each(had.optional.current_candidate->defacto.account_transactions.begin(),had.optional.current_candidate->defacto.account_transactions.end(),[&i,&prompt](auto const& at){
                  prompt << "\n  " << i++ << " " << at;
                });
              }
              else {
                prompt << "\nSorry, I failed to understand your entry. Press <Enter> to get help" << std::quoted(command);
              }
            }
            else if (auto at_iter = model->to_at_iter(model->selected_had(),ix)) {
              model->at_index = ix;
              prompt << "\nAccount Transaction:" << **at_iter;
              if (do_remove) {
                had.optional.current_candidate->defacto.account_transactions.erase(*at_iter);
              }
              model->prompt_state = PromptState::EditAT;
            }
            else {
              prompt << "\nEntered index does not refer to an Account Transaction Entry in current Heading Amount Date entry";
              model->prompt_state = PromptState::JEAggregateOptionIndex;
            }
          }
          else {
            prompt << "\nSorry, I seems to have lost track of the HAD you selected. Please re-select a valid HAD";
            model->prompt_state = PromptState::HADIndex;            
          }

        } break;

        case PromptState::CounterAccountsEntry: {
          prompt << "\nThe Counter Account Entry state is not yet active in this version of cratchit";
        } break;

        case PromptState::SKVEntryIndex: {
          // prompt << "\n1: Arbetsgivardeklaration (Employer’s contributions and PAYE tax return form)";
          // prompt << "\n2: Periodisk Sammanställning (EU sales list)"
          switch (ix) {
            case 0: {
              // Assume Employer’s contributions and PAYE tax return form

              // List Tax Return Form skv options (user data edit)
              auto const& [delta_prompt,prompt_state] = this->transition_prompt_state(model->prompt_state,PromptState::SKVTaxReturnEntryIndex);
              prompt << delta_prompt;
              model->prompt_state = prompt_state;
            } break;
            case 1: {
              // Create current quarter, previous quarter or two previous quarters option
              auto today = to_today();
              auto current_qr = to_quarter_range(today);
              auto previous_qr = to_three_months_earlier(current_qr);
              auto quarter_before_previous_qr = to_three_months_earlier(previous_qr);
              auto two_previous_quarters = DateRange{quarter_before_previous_qr.begin(),previous_qr.end()};

              prompt << "\n0: Track Current Quarter " << current_qr;
              prompt << "\n1: Report Previous Quarter " << previous_qr;
              prompt << "\n2: Check Quarter before previous " << quarter_before_previous_qr;
              prompt << "\n3: Check Previous two Quarters " << two_previous_quarters;
              model->prompt_state = PromptState::QuarterOptionIndex;								
            } break;
            case 2: {
              // Create K10 form files
              if (auto fm = SKV::SRU::K10::to_files_mapping()) {
                std::filesystem::path info_file_path{"to_skv/K10/INFO.SRU"};
                std::filesystem::create_directories(info_file_path.parent_path());
                auto info_std_os = std::ofstream{info_file_path};
                SKV::SRU::OStream info_sru_os{info_std_os};
                SKV::SRU::InfoOStream info_os{info_sru_os};
                if (info_os << *fm) {
                  prompt << "\nCreated " << info_file_path;
                }
                else {
                  prompt << "\nSorry, FAILED to create " << info_file_path;
                }

                std::filesystem::path blanketter_file_path{"to_skv/K10/BLANKETTER.SRU"};
                std::filesystem::create_directories(blanketter_file_path.parent_path());
                auto blanketter_std_os = std::ofstream{blanketter_file_path};
                SKV::SRU::OStream blanketter_sru_os{blanketter_std_os};
                SKV::SRU::BlanketterOStream blanketter_os{blanketter_sru_os};
                if (blanketter_os << *fm) {
                  prompt << "\nCreated " << blanketter_file_path;
                }
                else {
                  prompt << "\nSorry, FAILED to create " << blanketter_file_path;
                }
              }
              else {
                prompt << "\nSorry, failed to create data for input to K10 SRU-files";
              }
            } break;
            case 3: {
              // Generate the K10 and INK1 forms as SRU-files

              // We need two input values verified by the user
              // 1. The Total income to tax (SKV SRU Code 1000)
              // 2. The dividend to Tax (SKV SRU Code 4504)
              Amount income = get_INK1_Income(model);
              prompt << "\n1: INK1 1.1 Lön, förmåner, sjukpenning m.m. = " << income;
              Amount dividend = get_K10_Dividend(model);
              prompt << "\n2: K10 1.6 Utdelning = " << dividend;
              prompt << "\n3: Continue (Create K10 and INK1)";
              model->prompt_state = PromptState::K10INK1EditOptions;					
            } break;
            case 4: {
              // "\n4: INK2 + INK2S + INK2R (Company Tax Returns form(s))";
              prompt << "\n1: INK2::meta_xx = ?? (TODO: Add edit of meta data for INK2 forms)";
              prompt << "\n2: Generate INK2";
              model->prompt_state = PromptState::INK2AndAppendixEditOptions;
            } break;
            default: {prompt << "\nPlease enter a valid index";} break;
          }
        } break;

        case PromptState::QuarterOptionIndex: {
          auto today = to_today();
          auto current_qr = to_quarter_range(today);
          auto previous_qr = to_three_months_earlier(current_qr);
          auto quarter_before_previous_qr = to_three_months_earlier(previous_qr);
          auto two_previous_quarters = DateRange{quarter_before_previous_qr.begin(),previous_qr.end()};
          OptionalDateRange period_range{};
          switch (ix) {
            case 0: {
              prompt << "\nCurrent Quarter " << current_qr << " (to track)";
              period_range = current_qr;
            } break;
            case 1: {
              prompt << "\nPrevious Quarter " << previous_qr << " (to report)";
              period_range = previous_qr;
            } break;
            case 2: {
              prompt << "\nQuarter before previous " << quarter_before_previous_qr << " (to check)";
              period_range = quarter_before_previous_qr;
            } break;
            case 3: {
              prompt << "\nPrevious two Quarters " << two_previous_quarters << " (to check)";
              period_range = two_previous_quarters;
            }
            default: {
              prompt << "\nPlease select a valid option (it seems option " << ix << " is unknown to me";
            } break;
          }
          if (period_range) {
            // Create VAT Returns form for selected period
            prompt << "\nVAT Returns for " << *period_range;
            if (auto vat_returns_meta = SKV::XML::VATReturns::to_vat_returns_meta(*period_range)) {
              SKV::OrganisationMeta org_meta {
                .org_no = model->sie["current"].organisation_no.CIN
                ,.contact_persons = model->organisation_contacts
              };
              SKV::XML::DeclarationMeta form_meta {
                .declaration_period_id = vat_returns_meta->period_to_declare
              };
              auto is_quarter = [&vat_returns_meta](BAS::MetaAccountTransaction const& mat){
                return vat_returns_meta->period.contains(mat.meta.defacto.date);
              };
              auto box_map = SKV::XML::VATReturns::to_form_box_map(model->sie,is_quarter);
              if (box_map) {
                prompt << *box_map;
                auto xml_map = SKV::XML::VATReturns::to_xml_map(*box_map,org_meta,form_meta);
                if (xml_map) {
                  std::filesystem::path skv_files_folder{"to_skv"};
                  std::filesystem::path skv_file_name{std::string{"moms_"} + vat_returns_meta->period_to_declare + ".eskd"};
                  std::filesystem::path skv_file_path = skv_files_folder / skv_file_name;
                  std::filesystem::create_directories(skv_file_path.parent_path());
                  std::ofstream skv_file{skv_file_path};
                  SKV::XML::VATReturns::OStream vat_returns_os{skv_file};
                  if (vat_returns_os << *xml_map) {
                    prompt << "\nCreated " << skv_file_path;
                    SKV::XML::VATReturns::OStream vat_returns_prompt{prompt};
                    vat_returns_prompt << "\n" << *xml_map;
                  }
                  else prompt << "\nSorry, failed to create the file " << skv_file_path;
                }
                else prompt << "\nSorry, failed to map form data to XML Data required for the VAR Returns form file";
                // Generate an EU Sales List form for the VAt Returns form
                if (auto eu_list_form = SKV::CSV::EUSalesList::vat_returns_to_eu_sales_list_form(*box_map,org_meta,*period_range)) {
                  auto eu_list_quarter = SKV::CSV::EUSalesList::to_eu_list_quarter(period_range->end());
                  std::filesystem::path skv_files_folder{"to_skv"};						
                  std::filesystem::path skv_file_name{std::string{"periodisk_sammanstallning_"} + eu_list_quarter.yy_hyphen_quarter_seq_no + "_" + to_string(today) + ".csv"};						
                  std::filesystem::path eu_list_form_file_path = skv_files_folder / skv_file_name;
                  std::filesystem::create_directories(eu_list_form_file_path.parent_path());
                  std::ofstream eu_list_form_file_stream{eu_list_form_file_path};
                  SKV::CSV::EUSalesList::OStream os{eu_list_form_file_stream};
                  if (os << *eu_list_form) {
                    prompt << "\nCreated file " << eu_list_form_file_path << " OK";
                    SKV::CSV::EUSalesList::OStream eu_sales_list_prompt{prompt};
                    eu_sales_list_prompt << "\n" <<  *eu_list_form;
                  }
                  else {
                    prompt << "\nSorry, failed to write " << eu_list_form_file_path;
                  }
                }
                else {
                  prompt << "\nSorry, failed to acquire required data for the EU List form file";
                }
              }
              else prompt << "\nSorry, failed to gather form data required for the VAT Returns form";
            }
            else {
              prompt << "\nSorry, failed to gather meta-data for the VAT returns form for period " << *period_range;
            }									
          }
        } break;
        case PromptState::SKVTaxReturnEntryIndex: {
          switch (ix) {
            case 1: {model->prompt_state = PromptState::EnterContact;} break;
            case 2: {model->prompt_state = PromptState::EnterEmployeeID;} break;
            case 3: {
              if (ast.size() == 2) {
                // Assume Tax Returns form
                // Assume second argument is period
                if (auto xml_map = cratchit_to_skv(model->sie["current"],model->organisation_contacts,model->employee_birth_ids)) {
                  auto period_to_declare = ast[1];
                  // Brute force the period into the map (TODO: Inject this value in a better way into the production code above?)
                  (*xml_map)[R"(Skatteverket^agd:Blankett^agd:Arendeinformation^agd:Period)"] = period_to_declare;
                  (*xml_map)[R"(Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:HU^agd:RedovisningsPeriod faltkod="006")"] = period_to_declare;
                  (*xml_map)[R"(Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:RedovisningsPeriod faltkod="006")"] = period_to_declare;
                  (*xml_map)[R"(Skatteverket^agd:Kontaktperson^agd:Blankettinnehall^agd:HU^agd:RedovisningsPeriod faltkod="006")"] = period_to_declare;
                  std::filesystem::path skv_files_folder{"to_skv"};
                  std::filesystem::path skv_file_name{std::string{"arbetsgivaredeklaration_"} + period_to_declare + ".xml"};						
                  std::filesystem::path skv_file_path = skv_files_folder / skv_file_name;
                  std::filesystem::create_directories(skv_file_path.parent_path());
                  std::ofstream skv_file{skv_file_path};
                  if (SKV::XML::to_employee_contributions_and_PAYE_tax_return_file(skv_file,*xml_map)) {
                    prompt << "\nCreated " << skv_file_path;
                    prompt << "\nUpload file to Swedish Skatteverket";
                    prompt << "\n1. Browse to https://skatteverket.se/foretag";
                    prompt << "\n2. Click on 'Lämna arbetsgivardeklaration'";
                    prompt << "\n3. Log in";
                    prompt << "\n4. Choose to represent your company";
                    prompt << "\n5. Click on 'Deklarera via fil' and follow the instructions to upload the file " << skv_file_path;
                  }
                  else {
                    prompt << "\nSorry, failed to create " << skv_file_path;
                  }
                }	
                else {
                  prompt << "\nSorry, failed to acquire required data to generate xml-file to SKV";
                }
                model->prompt_state = PromptState::Root;
              }
              else {
                prompt << "\nPlease provide second argument = period on the form YYYYMM (e.g., enter '3 202205' to generate Tax return form for May 2022)";
              }
            } break;
            default: {prompt << "\nPlease enter a valid index";} break;
          }
        } break;

        case PromptState::K10INK1EditOptions: {
          switch (ix) {
            case 1: {model->prompt_state = PromptState::EnterIncome;} break;
            case 2: {model->prompt_state = PromptState::EnterDividend;} break;
            case 3: {
              if (false) {
                // 230420 - began hard coding generation of corrected K10 for last year + K10 and INK1 for this year
                // PAUS for now (I ended up sending in a manually edited INFO.SRU and BLANKETTER.SRU)
                // TODO: Consider to:
                // 1. Use CSV-files as read by skv_specs_mapping_from_csv_files()
                //    Get rid of hard coded SKV::SRU::INK1::k10_csv_to_sru_template and SKV::SRU::INK1::ink1_csv_to_sru_template?
                //    NOTE: They both use the same csv-files as input (2 at compile time and 1 at run-time)
                //    Actually - Decide if these csv-files should be hard coded or read at run-time?
                // 2. Consider to expose these csv-file as actual forms for the user to edit?
                //    I imagine we can generate indexed entries to each field and have the user select end edit its value?
                //    Question is how to implemented computation as defined by SKV for these forms (rates and values gets redefined each year!)
                // ...
                /*
                  SKV::SRU::FilesMapping fm {
                    .info = to_info_sru_file_tag_map(model);
                  };
                  fm.blanketter.push_back(to_k10_blankett(2021));
                  fm.blanketter.push_back(to_k10_blankett(2022));
                  fm.blanketter.push_back(to_ink1_blankett(2022));
                */
              }
              else {
                // TODO: Split to allow edit of Income and Dividend before entering the actual generation phase/code
                // k10_csv_to_sru_template
                SKV::SRU::OptionalSRUValueMap k10_sru_value_map{};
                SKV::SRU::OptionalSRUValueMap ink1_sru_value_map{};

                std::istringstream k10_is{SKV::SRU::INK1::k10_csv_to_sru_template};
                encoding::UTF8::istream utf8_K10_in{k10_is};
                if (auto field_rows = CSV::to_field_rows(utf8_K10_in)) {
                  // LOG
                  for (auto const& field_row : *field_rows) {
                    if (field_row.size()>0) prompt << "\n";
                    for (int i=0;i<field_row.size();++i) {
                      prompt << " [" << i << "]" << field_row[i];
                    }
                  }
                  // Acquire the SRU Values required for the K10 Form
                  k10_sru_value_map = SKV::SRU::to_sru_value_map(model,*field_rows);
                }
                else {
                  prompt << "\nSorry, failed to acquire a valid template for the K10 form";
                }
                // ink1_csv_to_sru_template
                std::istringstream ink1_is{SKV::SRU::INK1::ink1_csv_to_sru_template};
                encoding::UTF8::istream utf8_ink1_in{ink1_is};
                if (auto field_rows = CSV::to_field_rows(utf8_ink1_in)) {
                  for (auto const& field_row : *field_rows) {
                    if (field_row.size()>0) prompt << "\n";
                    for (int i=0;i<field_row.size();++i) {
                      prompt << " [" << i << "]" << field_row[i];
                    }
                  }
                  // Acquire the SRU Values required for the INK1 Form
                  ink1_sru_value_map = SKV::SRU::to_sru_value_map(model,*field_rows);
                }
                else {
                  prompt << "\nSorry, failed to acquire a valid template for the INK1 form";
                }
                if (k10_sru_value_map and ink1_sru_value_map) {
                  SKV::SRU::SRUFileTagMap info_sru_file_tag_map{};
                  {
                    // Assume we are to send in with sender being this company?
                    // 9. #ORGNR
                      // #ORGNR 191111111111
                    info_sru_file_tag_map["#ORGNR"] = model->sie["current"].organisation_no.CIN;		
                    // 10. #NAMN
                      // #NAMN Databokföraren
                    info_sru_file_tag_map["#NAMN"] = model->sie["current"].organisation_name.company_name;

                    auto postal_address = model->sie["current"].organisation_address.postal_address; // "17668 J?rf?lla" split in <space> to get ZIP and Town
                    auto postal_address_tokens = tokenize::splits(postal_address,' ');

                    // 12. #POSTNR
                      // #POSTNR 12345
                    if (postal_address_tokens.size() > 0) {
                      info_sru_file_tag_map["#POSTNR"] = postal_address_tokens[0];
                    }
                    else {
                      info_sru_file_tag_map["#POSTNR"] = "?POSTNR?";
                    }
                    // 13. #POSTORT
                      // #POSTORT SKATTSTAD
                    if (postal_address_tokens.size() > 1) {
                      info_sru_file_tag_map["#POSTORT"] = postal_address_tokens[1]; 
                    }
                    else {
                      info_sru_file_tag_map["#POSTORT"] = "?POSTORT?";
                    }
                  }
                  SKV::SRU::SRUFileTagMap k10_sru_file_tag_map{};
                  {
                    // #BLANKETT N7-2013P1
                    // k10_sru_file_tag_map["#BLANKETT"] = "K10-2021P4"; // See file "_Nyheter_from_beskattningsperiod_2021P4_.pdf" (https://skatteverket.se/download/18.96cca41179bad4b1aac351/1642600897155/Nyheter_from_beskattningsperiod_2021P4.zip)
                    k10_sru_file_tag_map["#BLANKETT"] = "K10-2022P4"; // See table 1, entry K10 column "blankett-block" in file "_Nyheter_from_beskattningsperiod_2022P4-3.pdf" (https://skatteverket.se/download/18.48cfd212185efbb440b65a8/1679495970044/_Nyheter_from_beskattningsperiod_2022P4-3.zip)
                    // #IDENTITET 193510250100 20130426 174557
                    std::ostringstream os{};
                    if (model->employee_birth_ids[0].size()>0) {
                      os << " " << model->employee_birth_ids[0];
                    }
                    else {
                      os << " " << "?PERSONNR?";											
                    }
                    auto today = to_today();
                    os << " " << today;
                    os << " " << "120000";
                    k10_sru_file_tag_map["#IDENTITET"] = os.str();
                  }
                  SKV::SRU::SRUFileTagMap ink1_sru_file_tag_map{};
                  {
                    // #BLANKETT N7-2013P1
                    // ink1_sru_file_tag_map["#BLANKETT"] = "INK1-2021P4"; // See file "_Nyheter_from_beskattningsperiod_2021P4_.pdf" (https://skatteverket.se/download/18.96cca41179bad4b1aac351/1642600897155/Nyheter_from_beskattningsperiod_2021P4.zip)
                    ink1_sru_file_tag_map["#BLANKETT"] = " INK1-2022P4"; // See entry INK1 column "blankett-block" in file "_Nyheter_from_beskattningsperiod_2022P4-3.pdf" (https://skatteverket.se/download/18.48cfd212185efbb440b65a8/1679495970044/_Nyheter_from_beskattningsperiod_2022P4-3.zip)
                    // #IDENTITET 193510250100 20130426 174557
                    std::ostringstream os{};
                    if (model->employee_birth_ids[0].size()>0) {
                      os << " " << model->employee_birth_ids[0];
                    }
                    else {
                      os << " " << "?PERSONNR?";											
                    }
                    auto today = to_today();
                    os << " " << today;
                    os << " " << "120000";

                    ink1_sru_file_tag_map["#IDENTITET"] = os.str();
                  }
                  SKV::SRU::FilesMapping fm {
                    .info = info_sru_file_tag_map
                  };
                  SKV::SRU::Blankett k10_blankett{k10_sru_file_tag_map,*k10_sru_value_map}; 
                  fm.blanketter.push_back(k10_blankett);
                  SKV::SRU::Blankett ink1_blankett{ink1_sru_file_tag_map,*ink1_sru_value_map}; 
                  fm.blanketter.push_back(ink1_blankett);

                  std::filesystem::path info_file_path{"to_skv/SRU/INFO.SRU"};
                  std::filesystem::create_directories(info_file_path.parent_path());
                  auto info_std_os = std::ofstream{info_file_path};
                  SKV::SRU::OStream info_sru_os{info_std_os};
                  SKV::SRU::InfoOStream info_os{info_sru_os};

                  if (info_os << fm) {
                    prompt << "\nCreated " << info_file_path;
                  }
                  else {
                    prompt << "\nSorry, FAILED to create " << info_file_path;
                  }

                  std::filesystem::path blanketter_file_path{"to_skv/SRU/BLANKETTER.SRU"};
                  std::filesystem::create_directories(blanketter_file_path.parent_path());
                  auto blanketter_std_os = std::ofstream{blanketter_file_path};
                  SKV::SRU::OStream blanketter_sru_os{blanketter_std_os};
                  SKV::SRU::BlanketterOStream blanketter_os{blanketter_sru_os};

                  if (blanketter_os << fm) {
                    prompt << "\nCreated " << blanketter_file_path;
                  }
                  else {
                    prompt << "\nSorry, FAILED to create " << blanketter_file_path;
                  }

                }
                else {
                  prompt << "\nSorry, Failed to acquirer the data for the K10 and INK1 forms";									
                }
              } // if false
            } break;
            default: {prompt << "\nPlease enter a valid index";} break;
          }
        } break;

        case PromptState::INK2AndAppendixEditOptions: {
          switch (ix) {
            case 1: {
              prompt << "\nTODO: Add edit of INK2 meta data";
            } break;
            case 2: {
              prompt << "\nTODO: Add generation of INK2 forms into SRU-file (See code for INK1 and K10)";
            } break;
          }
        } break;

        case PromptState::EnterIncome:
        case PromptState::EnterDividend:
        case PromptState::EnterHA:
        case PromptState::EnterContact:
        case PromptState::EnterEmployeeID:
        case PromptState::EditAT:
        case PromptState::Undefined:
        case PromptState::Unknown:
          prompt << "\nPlease enter \"word\" like text (index option not available in this state)";
          break;
      }
    }
    /* ======================================================



              Act on on Command?


      ====================================================== */

    // BEGIN LUA REPL Hack
    else if (ast[0] == "-lua") {
      model->prompt_state = PromptState::LUARepl;
    }
    else if (model->prompt_state == PromptState::LUARepl) {
      // Execute input as a lua script
      if (model->L == nullptr) {
        model->L = luaL_newstate();
        luaL_openlibs(model->L);
        // Register cratchit functions in Lua
        lua_faced_ifc::register_cratchit_functions(model->L);
        prompt << "\nNOTE: NEW Lua environment created.";
      }
      int r = luaL_dostring(model->L,command.c_str());
      if (r != LUA_OK) {
        prompt << "\nSorry, ERROR:" << r;
        std::string sErrorMsg = lua_tostring(model->L,-1);
        prompt << " " << std::quoted(sErrorMsg);
      }
    }
    // END LUA REPL Hack

    else if (ast[0] == "-version" or ast[0] == "-v") {
      prompt << "\nCratchit Version " << VERSION;
    }
    else if (ast[0] == "-tas") {
      // Enter tagged Amounts mode for specified period (from any state)
      if (ast.size() == 1 and model->selected_date_ordered_tagged_amounts.size() > 0) {
        // Enter into current selection
        model->prompt_state = PromptState::TAIndex;
      // List current selection
        prompt << "\n<SELECTED>";
        // for (auto const& ta : model->all_date_ordered_tagged_amounts.tagged_amounts()) {
        int index = 0;
        for (auto const& ta : model->selected_date_ordered_tagged_amounts) {	
          prompt << "\n\t" << index++ << ". " << ta;
        }				
      }
      else {
        // Period required
        OptionalDate begin{}, end{};
        if (ast.size() == 3) {
            begin = to_date(ast[1]);
            end = to_date(ast[2]);					
        }
        if (begin and end) {
          model->selected_date_ordered_tagged_amounts.clear();
          for (auto const& ta : model->all_date_ordered_tagged_amounts.in_date_range({*begin,*end})) {
            model->selected_date_ordered_tagged_amounts.insert(ta);
          }				
          model->prompt_state = PromptState::TAIndex;
          prompt << "\n<SELECTED>";
          // for (auto const& ta : model->all_date_ordered_tagged_amounts.tagged_amounts()) {
          int index = 0;
          for (auto const& ta : model->selected_date_ordered_tagged_amounts) {	
            prompt << "\n\t" << index++ << ". " << ta;
          }				

        }
        else {
          prompt << "\nPlease enter the tagged amounts state on the form '-tas yyyymmdd yyyymmdd'";
        }
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-has_tag") {
      if (ast.size() == 2) {
        auto has_tag = [tag = ast[1]](TaggedAmount const& ta) {
          return ta.tags().contains(tag);
        };
        TaggedAmounts reduced{};
        std::ranges::copy(model->selected_date_ordered_tagged_amounts | std::views::filter(has_tag),std::back_inserter(reduced));				
        model->selected_date_ordered_tagged_amounts = reduced;
      }
      else {
        prompt << "\nPlease provide the tag name you want to filter on";
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-has_not_tag") {
      if (ast.size() == 2) {
        auto has_not_tag = [tag = ast[1]](TaggedAmount const& ta) {
          return !ta.tags().contains(tag);
        };
        TaggedAmounts reduced{};
        std::ranges::copy(model->selected_date_ordered_tagged_amounts | std::views::filter(has_not_tag),std::back_inserter(reduced));				
        model->selected_date_ordered_tagged_amounts = reduced;
      }
      else {
        prompt << "\nPlease provide the tag name you want to filter on";
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-is_tagged") {
      if (ast.size() == 2) {
        auto [tag,pattern] = tokenize::split(ast[1],'=');
        if (tag.size()>0) {
          auto is_tagged = [tag=tag,pattern=pattern](TaggedAmount const& ta) {
            const std::regex pattern_regex(pattern); 
            return (ta.tags().contains(tag) and std::regex_match(ta.tags().at(tag),pattern_regex));
          };
          TaggedAmounts reduced{};
          std::ranges::copy(model->selected_date_ordered_tagged_amounts | std::views::filter(is_tagged),std::back_inserter(reduced));				
          model->selected_date_ordered_tagged_amounts = reduced;
        }
        else {
          prompt << "\nPlease provide '<tag name>=<tag_value or regular expression>' to filter on";
        }
      }
      else {
        prompt << "\nPlease provide '<tag name>=<tag_value or regular expression>' to filter on";
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-is_not_tagged") {
      if (ast.size() == 2) {
        auto [tag,pattern] = tokenize::split(ast[1],'=');
        if (tag.size()>0) {
          auto is_not_tagged = [tag=tag,pattern=pattern](TaggedAmount const& ta) {
            const std::regex pattern_regex(pattern); 
            return (ta.tags().contains(tag)==false or std::regex_match(ta.tags().at(tag),pattern_regex)==false);
          };
          TaggedAmounts reduced{};
          std::ranges::copy(model->selected_date_ordered_tagged_amounts | std::views::filter(is_not_tagged),std::back_inserter(reduced));				
          model->selected_date_ordered_tagged_amounts = reduced;
        }
        else {
          prompt << "\nPlease provide '<tag name>=<tag_value or regular expression>' to filter on";
        }
      }
      else {
        prompt << "\nPlease provide '<tag name>=<tag_value or regular expression>' to filter on";
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-to_bas_account") {
      // -to_bas <BAS account no | Bas Account Name>
      if (ast.size() == 2) {
        if (auto bas_account = BAS::to_account_no(ast[1])) {
          TaggedAmounts created{};
          auto new_ta = [bas_account = *bas_account](TaggedAmount const& ta){
            auto date = ta.date();
            auto cents_amount = ta.cents_amount();
            auto source_tags = ta.tags();
            TaggedAmount::Tags tags{};
            tags["BAS"]=std::to_string(bas_account);
            TaggedAmount result{date,cents_amount,std::move(tags)};
            return result;
          };
          std::ranges::transform(model->selected_date_ordered_tagged_amounts,std::back_inserter(created),new_ta);
          model->new_date_ordered_tagged_amounts = created;
          prompt << "\n<CREATED>";
          // for (auto const& ta : model->all_date_ordered_tagged_amounts.tagged_amounts()) {
          int index = 0;
          for (auto const& ta : model->new_date_ordered_tagged_amounts) {	
            prompt << "\n\t" << index++ << ". " << ta;
          }				
          model->prompt_state = PromptState::AcceptNewTAs;
          prompt << "\n" << options_list_of_prompt_state(model->prompt_state);
        }
        else {
          prompt << "\nPlease enter a valid BAS account no";
        }
      }
      else {
        prompt << "\nPlease enter the BAS account you want to book selected tagged amounts to (E.g., '-to_bas 1920'";
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-amount_trails") {
      using AmountTrailsMap = std::map<CentsAmount,TaggedAmounts>;
      AmountTrailsMap amount_trails_map{};
      for (auto const& ta : model->selected_date_ordered_tagged_amounts) {
        amount_trails_map[abs(ta.cents_amount())].push_back(ta);
      }
      std::vector<std::pair<CentsAmount,TaggedAmounts>> date_ordered_amount_trails_map{};
      std::ranges::copy(amount_trails_map,std::back_inserter(date_ordered_amount_trails_map));
      std::ranges::sort(date_ordered_amount_trails_map,[](auto const& e1,auto const& e2){
        if (e1.second.front().date() == e2.second.front().date()) return e1.first < e2.first;
        else return e1.second.front().date() < e2.second.front().date();
      });
      for (auto const& [cents_amount,tas] : date_ordered_amount_trails_map) {
        auto units_and_cents_amount = to_units_and_cents(cents_amount);
        prompt << "\n" << units_and_cents_amount;
        for (auto const& ta : tas) {
          prompt << "\n\t" << ta;
        }
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-aggregates") {
      // Reduce to aggregates
      auto is_aggregate = [](TaggedAmount const& ta) {
        return (ta.tags().contains("type") and ta.tags().at("type") == "aggregate");
      };
      TaggedAmounts reduced{};
      std::ranges::copy(model->selected_date_ordered_tagged_amounts | std::views::filter(is_aggregate),std::back_inserter(reduced));				
      model->selected_date_ordered_tagged_amounts = reduced;
      // List by bucketing on aggregates (listing orphan (non-aggregated) tagged amounts separatly)
      std::cout << "\n<AGGREGATES>" << std::flush;
      prompt << "\n<AGGREGATES>";
      for (auto const& ta : model->selected_date_ordered_tagged_amounts) {
        prompt << "\n" << ta;
        if (auto members_value = ta.tag_value("_members")) {
          auto members = Key::Path{*members_value};
          if (auto value_ids = to_value_ids(members)) {
            prompt << "\n\t<members>";
            if (auto tas = model->all_date_ordered_tagged_amounts.to_ta_ptrs(*value_ids)) {
              for (auto const& ta : *tas) {
                prompt << "\n\t" << ta;
              }
            }
          }
        }
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-to_hads") {
      prompt << "\nCreating Heading Amount Date entries (HAD:s) from selected Tagged Amounts";
      auto had_candidate_ta_ptrs = model->selected_date_ordered_tagged_amounts;
      // Filter out all tagged amounts that are SIE aggregates or member of an SIE aggregate (these are already in the books)
      for (auto const& ta : model->selected_date_ordered_tagged_amounts) {
        bool has_SIE_tag = ta.tag_value("SIE").has_value();
        if (auto members_value = ta.tag_value("_members");has_SIE_tag and members_value) {
          prompt << "\nDisregarded SIE aggregate " << ta;
          had_candidate_ta_ptrs.erase(to_value_id(ta));
          auto members = Key::Path{*members_value};
          if (auto value_ids = to_value_ids(members)) {
            if (auto tas = model->all_date_ordered_tagged_amounts.to_ta_ptrs(*value_ids)) {
              for (auto const& ta : *tas) {
                prompt << "\nDisregarded SIE aggregate member " << ta;
                had_candidate_ta_ptrs.erase(to_value_id(ta));            
              }
            }
          }
        }
      }
      for (auto const& ta : had_candidate_ta_ptrs) {
        if (auto o_had = to_had(ta)) {
          model->heading_amount_date_entries.push_back(*o_had);
        }
        else {
          prompt << "\nSORRY, failed to turn tagged amount into a heading amount date entry" << ta;
        }
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-todo") {
      // Filter out tagged amounts that are already in the books as an SIE entry / aggregate?
      prompt << "\nSorry, Identifying todos on tagged amounts not yet implemented";
    }
    else if (ast[0] == "-bas") {
// std::cout << " :)";
      if (ast.size() == 2) {
        // Import bas account plan csv file
        if (ast[1] == "?") {
          // List csv-files in the resources sub-folder
          std::filesystem::path resources_folder{"./resources"};
          std::filesystem::directory_iterator dir_iter{resources_folder};
          for (auto const& dir_entry : std::filesystem::directory_iterator{resources_folder}) {
            prompt << "\n" << dir_entry.path(); // TODO: Remove .path() when stdc++ library supports streaming of std::filesystem::directory_entry 
          }
        }
        else {
          // Import and parse file at provided file path
          std::filesystem::path csv_file_path{ast[1]};
          if (std::filesystem::exists(csv_file_path)) {
            std::ifstream in{csv_file_path};
            BAS::parse_bas_account_plan_csv(in,prompt);
          }
          else {
            prompt << "\nUse '-bas' to list available files to import (It seems I can't find the file " << csv_file_path;
          }
        }
      }
      else {
        // Use string literal with csv data
        std::istringstream in{BAS::bas_2022_account_plan_csv};
        BAS::parse_bas_account_plan_csv(in,prompt);
      }
    }
    else if (ast[0] == "-sie") {
      // Import sie and add as base of our environment
      if (ast.size()==1) {
        // List current sie environment
        prompt << model->sie["current"];				
        // std::cout << model->sie["current"];
      }
      else if (ast.size()==2) {
        if (ast[1]=="*") {
          // List unposted (staged) sie entries
          FilteredSIEEnvironment filtered_sie{model->sie["current"],BAS::filter::is_flagged_unposted{}};
          prompt << filtered_sie;
        }
        else if (model->sie.contains(ast[1])) {
          // List journal entries of a year index
          prompt << model->sie[ast[1]];
        }
        else if (auto bas_account_no = BAS::to_account_no(ast[1])) {
          // List journal entries filtered on an bas account number
          prompt << "\nFilter journal entries that has a transaction to account no " << *bas_account_no;
          prompt << "\nTIP: If you meant filter on amount please re-enter using '.00' to distinguish it from an account no.";
          FilteredSIEEnvironment filtered_sie{model->sie["current"],BAS::filter::HasTransactionToAccount{*bas_account_no}};
          prompt << filtered_sie;
        }
        else if (auto gross_amount = to_amount(ast[1])) {
          // List journal entries filtered on given amount
          prompt << "\nFilter journal entries that match gross amount " << *gross_amount;
          FilteredSIEEnvironment filtered_sie{model->sie["current"],BAS::filter::HasGrossAmount{*gross_amount}};
          prompt << filtered_sie;
        }
        else if (auto sie_file_path = path_to_existing_file(ast[1])) {
          prompt << "\nImporting SIE to current year from " << *sie_file_path;
          if (auto sie_env = from_sie_file(*sie_file_path)) {
            model->sie_file_path["current"] = *sie_file_path;
            model->sie["current"] = std::move(*sie_env);
            // Update the list of staged entries
            if (auto sse = from_sie_file(model->staged_sie_file_path)) {
              auto unstaged = model->sie["current"].stage(*sse);
              for (auto const& je : unstaged) {
                prompt << "\nnow posted " << je; 
              }
            }							
          }
          else {
            // failed to parse sie-file into an SIE Environment 
            prompt << "\nERROR - Failed to import sie file " << *sie_file_path;
          }
        }
        else if (ast[1] == "-types") {
          // Group on Type Topology
          // using TypedMetaEntryMap = std::map<BAS::kind::AccountTransactionTypeTopology,std::vector<BAS::TypedMetaEntry>>;
          // using MetaEntryTopologyMap = std::map<std::size_t,TypedMetaEntryMap>;
          // MetaEntryTopologyMap meta_entry_topology_map{};
          // auto h = [&meta_entry_topology_map](BAS::TypedMetaEntry const& tme){
          // 	auto types_topology = BAS::kind::to_types_topology(tme);
          // 	auto signature = BAS::kind::to_signature(types_topology);
          // 	meta_entry_topology_map[signature][types_topology].push_back(tme);							
          // };
          // for_each_typed_meta_entry(model->sie,h);
          auto meta_entry_topology_map = to_meta_entry_topology_map(model->sie);
          // Prepare to record journal entries we could not use as template for new entries
          std::vector<BAS::TypedMetaEntry> failed_tmes{};
          std::set<BAS::kind::AccountTransactionTypeTopology> detected_topologies{};
          // List grouped on type topology
          for (auto const& [signature,tme_map] : meta_entry_topology_map) {
            for (auto const& [type_topology,tmes] : tme_map) {
              prompt << "\n[" << type_topology << "] ";
              detected_topologies.insert(type_topology);
              // Group tmes on BAS Accounts topology
              auto accounts_topology_map = to_accounts_topology_map(tmes);
              // List grouped BAS Accounts topology
              for (auto const& [signature,bat_map] : accounts_topology_map) {
                for (auto const& [type_topology,tmes] : bat_map) {
                  prompt << "\n    [" << type_topology << "] ";
                  for (auto const& tme : tmes) {
                    prompt << "\n       VAT Type:" << to_vat_type(tme);
                    prompt << "\n      " << tme.meta << " " << std::quoted(tme.defacto.caption) << " " << tme.defacto.date;
                    prompt << IndentedOnNewLine{tme.defacto.account_transactions,10};
                    // TEST that we are able to operate on journal entries with this topology? 
                    auto test_result = test_typed_meta_entry(model->sie,tme);
                    prompt << "\n       TEST: " << test_result;
                    if (test_result.failed) failed_tmes.push_back(tme);											
                  }
                }
              }
            }
          }
          // LOG detected journal entry type topologies
          prompt << "\n<DETECTED TOPOLOGIES>";
          for (auto const& topology : detected_topologies) {
            prompt << "\n\t" << topology;
          }
          // LOG the 'tmes' (template meta entries) we failed to identify as templates for new journal entries
          prompt << "\n<DESIGN INSUFFICIENCY: FAILED TO IDENTIFY AND USE THESE ENTRIES AS TEMPLATE>";
          for (auto const& tme : failed_tmes) {
            auto types_topology = BAS::kind::to_types_topology(tme);
            prompt << "\n" << types_topology << " " << tme.meta << " " << tme.defacto.caption << " " << tme.defacto.date;
          }
        }
        else {
          // assume user search criteria on transaction heading and comments
          FilteredSIEEnvironment filtered_sie{model->sie["current"],BAS::filter::matches_user_search_criteria{ast[1]}};
          prompt << "\nNot '*', existing year id or existing file: " << std::quoted(ast[1]);
          prompt << "\nFilter current sie for " << std::quoted(ast[1]); 
          prompt << filtered_sie;
        }
      }
      else if (ast.size()==3) {
        auto year_key = ast[1];
        if (auto sie_file_path = path_to_existing_file(ast[2])) {
          prompt << "\nImporting SIE to realtive year " << year_key << " from " << *sie_file_path;
          if (auto sie_env = from_sie_file(*sie_file_path)) {
            model->sie_file_path[year_key] = *sie_file_path;
            model->sie[year_key] = std::move(*sie_env);
          }
          else {
            // failed to parse sie-file into an SIE Environment 
            prompt << "\nERROR - Failed to import sie file " << *sie_file_path;
          }
        }
        else {
          // assume user search criteria on transaction heading and comments
          if (model->sie.contains(year_key)) {
            FilteredSIEEnvironment filtered_sie{model->sie[year_key],BAS::filter::matches_user_search_criteria{ast[2]}};
            prompt << filtered_sie;
          }
          else {
            prompt << "\nYear identifier " << year_key << " is not associated with any data";
          }
        }
      }
      else {
        prompt << "\nSorry, failed to understand your request.";
        prompt << "\n\tI interpreted your input to contain " << ast.size() << " arguments:";
        for (auto const& token : ast) prompt << " " << std::quoted(token);
        prompt << "\n\tPlease enclose arguments (e.g., an SIE file path) that contains space(s) with \"\"";
        prompt << "\n\tPress <Enter> for additional help";
      }
    }
    else if (ast[0] == "-huvudbok") {
      // command "-huvudbok" defaults to "-huvudbok 0", that is the fiscal year of current date
      std::string year_id = (ast.size() == 2)?ast[1]:std::string{"current"};
      if (year_id == "0") year_id = "current";
      prompt << "\nHuvudbok for year id " << year_id;
      auto fiscal_year_date_range = model->to_fiscal_year_date_range(year_id);
      if (fiscal_year_date_range) {
        prompt << " " << *fiscal_year_date_range;
        TaggedAmounts tas{}; // journal entries
        auto is_journal_entry = [](TaggedAmount const& ta) {
          return (ta.tags().contains("parent_SIE") or ta.tags().contains("IB"));
        };        
        std::ranges::copy(model->all_date_ordered_tagged_amounts.in_date_range(*fiscal_year_date_range) | std::views::filter(is_journal_entry),std::back_inserter(tas));				
        // 2. Group tagged amounts into same BAS account and a list of parent_SIE
        std::map<BAS::AccountNo,TaggedAmounts> huvudbok{};
        std::map<BAS::AccountNo,CentsAmount> opening_balance{};
        
        for (auto const& ta : tas) {
          if (ta.tags().contains("BAS")) {
            auto opt_bas_account_no = BAS::to_account_no(ta.tags().at("BAS"));
            if (opt_bas_account_no) {
              if (ta.tags().contains("IB")) {
                opening_balance[*opt_bas_account_no] = ta.cents_amount();
              }
              else {
                huvudbok[*opt_bas_account_no].push_back(ta);
              }
            }
            else {
              std::cout << "\nDESIGN INSUFFICIENCY: Error, BAS tag contains value:" << ta.tags().at("BAS") << " that fails to translate to a BAS account no";
            }
          }
          else {
            std::cout << "\nDESIGN_INSUFFICIENCY: Error, Contains no BAS tag : " << ta;
          }
        }

        // 3. "print" the table grouping BAS accounts and date ordered SIE verifications (also showing a running accumultaion of the account balance)
        prompt << "\n<Journal Entries in year id " << year_id << ">";
        for (auto const& [bas_account_no,tas] : huvudbok) {
          CentsAmount acc{0};
          prompt << "\n" << bas_account_no;
          prompt << ":" << std::quoted(BAS::global_account_metas().at(bas_account_no).name);
          if (opening_balance.contains(bas_account_no)) {
            acc = opening_balance[bas_account_no];
            prompt << "\n\topening balance\t" << to_units_and_cents(acc);
          }
          for (auto const& ta : tas) {
            prompt << "\n\t" << ta;
            acc += ta.cents_amount();
            prompt << "\tsaldo:" << to_units_and_cents(acc); 
          }
          prompt << "\n\tclosing balance\t" << to_units_and_cents(acc);
        }
      }
      else {
        prompt << "\nSORRY, Failed to understand what fiscal year " << year_id << " refers to. Please enter a valid year id (0 or 'current', -1 = previous fiscal year...)";
      }
    }
    else if (ast[0] == "-balance") {
      // The user has requested to have us list account balances
      prompt << model->sie["current"].balances_at(to_today());
    }
    else if (ast[0] == "-hads") {
      auto hads = model->refreshed_hads();
      if (ast.size()==1) {
        prompt << to_had_listing_prompt(hads);
        model->prompt_state = PromptState::HADIndex;

      // 	// Expose current hads (Heading Amount Date transaction entries) to the user
      // 	auto& hads = model->heading_amount_date_entries;
      // 	unsigned int index{0};
      // 	std::vector<std::string> sHads{};
      // 	std::transform(hads.begin(),hads.end(),std::back_inserter(sHads),[&index](auto const& had){
      // 		std::stringstream os{};
      // 		os << index++ << " " << had;
      // 		return os.str();
      // 	});
      // 	prompt << "\n" << std::accumulate(sHads.begin(),sHads.end(),std::string{"Please select:"},[](auto acc,std::string const& entry) {
      // 		acc += "\n  " + entry;
      // 		return acc;
      // 	});
      // 	model->prompt_state = PromptState::HADIndex;
      }
      else if (ast.size()==2) {
        // Assume the user has entered text to macth against had Heading
        // Expose the hads (Heading Amount Date transaction entries) that matches user input
        // NOTE: Keep correct index for later retreiving any had selected by the user
        auto& hads = model->heading_amount_date_entries;
        unsigned int index{0};
        std::vector<std::string> sHads{};
        auto text = ast[1];
        std::transform(hads.begin(),hads.end(),std::back_inserter(sHads),[&index,&text](auto const& had){
          std::stringstream os{};
          if (strings_share_tokens(text,had.heading)) os << index << " " << had;
          ++index; // count even if not listed
          return os.str();
        });
        prompt << std::accumulate(sHads.begin(),sHads.end(),std::string{"Please select:"},[](auto acc,std::string const& entry) {
          if (entry.size()>0) acc += "\n  " + entry;
          return acc;
        });
        model->prompt_state = PromptState::HADIndex;
      }
      else {
        prompt << "\nPlease re-enter a valid input (It seems you entered to many arguments for me to understand)";
      }
    }
    else if (ast[0] == "-meta") {
      if (ast.size() > 1) {
        // Assume filter on provided text
        if (auto to_match_account_no = BAS::to_account_no(ast[1])) {
          // Assume match to BAS account or SRU account 
          // for (auto const& [account_no,am] : model->sie["current"].account_metas()) {
          // 	if ((*to_match_account_no == account_no) or (am.sru_code and (*to_match_account_no == *am.sru_code))) {
          // 		prompt << "\n  " << account_no << " " << std::quoted(am.name);
          // 		if (am.sru_code) prompt << " SRU:" << *am.sru_code;
          // 	}
          // }
          auto ams = matches_bas_or_sru_account_no(*to_match_account_no,model->sie["current"]);
          for (auto const& [account_no,am] : ams) {
            prompt << "\n  " << account_no << " " << std::quoted(am.name);
            if (am.sru_code) prompt << " SRU:" << *am.sru_code;
          }
        }
        else {
          // Assume match to account name
          // for (auto const& [account_no,am] : model->sie["current"].account_metas()) {
          // 	if (first_in_second_case_insensitive(ast[1],am.name)) {
          // 		prompt << "\n  " << account_no << " " << std::quoted(am.name);
          // 		if (am.sru_code) prompt << " SRU:" << *am.sru_code;
          // 	}
          // }
          auto ams = matches_bas_account_name(ast[1],model->sie["current"]);
          for (auto const& [account_no,am] : ams) {
            prompt << "\n  " << account_no << " " << std::quoted(am.name);
            if (am.sru_code) prompt << " SRU:" << *am.sru_code;
          }
        }
      }
      else {
        // list all metas
        for (auto const& [account_no,am] : model->sie["current"].account_metas()) {
          prompt << "\n  " << account_no << " " << std::quoted(am.name);
          if (am.sru_code) prompt << " SRU:" << *am.sru_code;
        }
      }
    }
    else if (ast[0] == "-sru") {
      if (ast.size() > 1) {
        if (ast[1] == "-bas") {
          // List SRU Accounts mapped to BAS Accounts
          auto const& account_metas = model->sie["current"].account_metas();
          auto sru_map = sru_to_bas_map(account_metas);
          for (auto const& [sru_account,bas_accounts] : sru_map) {
            prompt << "\nSRU:" << sru_account;
            for (auto const& bas_account_no : bas_accounts) {
              prompt << "\n  BAS:" << bas_account_no << " " << std::quoted(account_metas.at(bas_account_no).name);
            }
          }
        }
        else if (ast.size() > 2) {
          // Assume the user has provided a year-id and a path to a csv-file with SRU values for that year?
          auto year_id = ast[1];
          std::filesystem::path csv_file_path{ast[2]};
          if (std::filesystem::exists(csv_file_path)) {
            std::ifstream ifs{csv_file_path};
            encoding::UTF8::istream utf8_in{ifs}; // Assume UTF8 encoded file (should work for all-digits csv.file as ASCII 0..7F overlaps with UTF8 encoing anyhow?)
            if (auto const& field_rows = CSV::to_field_rows(utf8_in,';')) {
              for (auto const& field_row : *field_rows) {
                if (field_row.size()==2) {
                  if (auto const& sru_code = SKV::SRU::to_account_no(field_row[0])) {
                    model->sru[year_id].set(*sru_code,field_row[1]);
                  }
                }
              }
            }
            else {
              prompt << "\nSorry, seems to be unable to parse for SRU values in csv-file " << csv_file_path;
            }
          }
          else {
            prompt << "\nSorry, I seem to fail to find file " << csv_file_path;
          }
        }
        else {
          prompt << "\nPlease provide no arguments, argument '-bas' or arguments <year-id> <csv-file-path>";
        }
      }
      else {
        for (auto const& [year_id,sru_env] : model->sru) {
          prompt << "\nyear:id:" << year_id;
          prompt << "\n" << sru_env;
        }
      }
    }
    else if (ast[0] == "-gross") {
      auto ats = to_gross_account_transactions(model->sie);
      for (auto const& at : ats) {
        prompt << "\n" << at;
      }				
    }
    else if (ast[0] == "-net") {
      auto ats = to_net_account_transactions(model->sie);
      for (auto const& at : ats) {
        prompt << "\n" << at;
      }				
    }
    else if (ast[0] == "-vat") {
      auto vats = to_vat_account_transactions(model->sie);
      for (auto const& vat : vats) {
        prompt << "\n" << vat;
      }				
    }
    else if (ast[0] == "-t2") {
      auto t2s = t2_entries(model->sie);
      prompt << t2s;
    }
    else if (ast[0] == "-skv") {
      if (ast.size() == 1) {
        // List skv options
        prompt << "\n0: Arbetsgivardeklaration (TAX Returns)";
        prompt << "\n1: Momsrapport (VAT Returns)";
        prompt << "\n2: K10 (TAX Declaration Appendix Form)";
        prompt << "\n3: INK1 + K10 (Swedish Tax Agency private TAX Form + Dividend Form";
        prompt << "\n4: INK2 + INK2S + INK2R (Company Tax Returns form(s))";
        model->prompt_state = PromptState::SKVEntryIndex;
      }
    }
    else if (ast[0] == "-csv") {
      if (ast.size()==1) {
        // Command "-csv" (and no more)
        // ==> Process all *.csv in folder from_bank and *.skv in folder from_skv

      }
      else if ((ast.size()>2) and (ast[1] == "-had")) {
        // Command "-csv -had <path>"
        std::filesystem::path csv_file_path{ast[2]};
        if (std::filesystem::exists(csv_file_path)) {
          std::ifstream ifs{csv_file_path};
          CSV::NORDEA::istream in{ifs};
          BAS::OptionalAccountNo gross_bas_account_no{};
          if (ast.size()>3) {
            if (auto bas_account_no = BAS::to_account_no(ast[3])) {
              gross_bas_account_no = *bas_account_no;
            }
            else {
              prompt << "\nPlease enter a valid BAS account no for gross amount transaction. " << std::quoted(ast[3]) << " is not a valid BAS account no";
            }
          }
          auto hads = CSV::from_stream(in,gross_bas_account_no);
          // #X Filter entries in the read csv-file against already existing hads and sie-entries
          // #X match date and amount
          // 1) Don't add a had that is already in our models hads list
          std::erase_if(hads,[this](HeadingAmountDateTransEntry const& had) {
            auto iter = std::find_if(this->model->heading_amount_date_entries.begin(),this->model->heading_amount_date_entries.end(),[&had](HeadingAmountDateTransEntry const& other){
              auto result = (had.date == other.date) and (had.amount == other.amount) and (had.heading == other.heading);
              if (result) {
                std::cout << "\nHAD ALREADY AS HAD";
                std::cout << "\n\t" << had;
                std::cout << "\n\t" << other;
              }
              return result;
            });
            auto result = (iter != this->model->heading_amount_date_entries.end());
            return result;
          });
          // 2) Don't add a had that is already accounted for as an sie entry
          auto sie_hads_reducer = [&hads](BAS::MetaEntry const& me) {
            // Remove the had if it matches me 
            std::erase_if(hads,[&me](HeadingAmountDateTransEntry const& had) {
              bool result{false};
              if (me.defacto.date == had.date) {
                if (had.amount > 0) {
                  auto gross_amount = to_positive_gross_transaction_amount(me.defacto);
                  result = (BAS::to_cents_amount(had.amount) == BAS::to_cents_amount(gross_amount));
                }
                else {
                  auto gross_amount = to_negative_gross_transaction_amount(me.defacto);
                  result = (BAS::to_cents_amount(had.amount) == BAS::to_cents_amount(gross_amount));
                }
              }
              if (result) {
                std::cout << "\nHAD ALREADY AS SIE: ";
                std::cout << "\n\t" << had;
                std::cout << "\n\t" << me;
              }
              return result;
            });
          };
          for_each_meta_entry(model->sie["current"],sie_hads_reducer);
          std::copy(hads.begin(),hads.end(),std::back_inserter(model->heading_amount_date_entries));
        }
        else {
          prompt << "\nPlease provide a path to an existing file. Can't find " << csv_file_path;
        }
      }
      else if ((ast.size()>2) and (ast[1] == "-sru")) {
        // Command "-csv -sru <path>"
        std::filesystem::path csv_file_path{ast[2]};
        if (std::filesystem::exists(csv_file_path)) {
          std::ifstream ifs{csv_file_path};
          encoding::UTF8::istream utf8_in{ifs};
          if (auto field_rows = CSV::to_field_rows(utf8_in)) {
            for (auto const& field_row : *field_rows) {
              if (field_row.size()>0) prompt << "\n";
              for (int i=0;i<field_row.size();++i) {
                prompt << " [" << i << "]" << field_row[i];
              }
            }
            if (auto sru_value_map = SKV::SRU::to_sru_value_map(model,*field_rows)) {
              prompt << "\nSorry, Reading an input csv-file as base for SRU-file creation is not yet implemented.";
            }
            else {
              prompt << "\nSorry, failed to gather required sru values to create a valid sru-file";
            }
          }
          else {
            prompt << "\nSorry, failed to parse as csv the file " << csv_file_path;
          }
        }
        else {
          prompt << "\nPlease provide a path to an existing file. Can't find " << csv_file_path;
        }
      }
      else {
        prompt << "\nPlease provide '-had' or '-sru' followed by a path to a csv-file";
      }
    }
    else if (ast[0] == "-") {
      // model->prompt_state = PromptState::Root;
      auto new_state = model->to_previous_state(model->prompt_state);
      prompt << model->to_prompt_for_entering_state(new_state);
      model->prompt_state = new_state;      
    }
    else if (ast[0] == "-omslutning") {
      // Report yearly change for each BAS account
      std::string year_id = "current"; // default
      if (ast.size()>1) {
        if (model->sie.contains(ast[1])) {
          year_id = ast[1];
        }
        else {
          prompt << "\nSorry, I find no record of year " << std::quoted(ast[1]);
        }
      }

      // auto fiscal_year_date_range = model->sie[year_id].fiscal_year_date_range();
      auto fiscal_year_date_range = model->to_fiscal_year_date_range(year_id);

      if (fiscal_year_date_range) {
        auto fiscal_year_tagged_amounts_range = model->all_date_ordered_tagged_amounts.in_date_range(*fiscal_year_date_range); 
        auto bas_account_accs = tas::to_bas_omslutning(fiscal_year_tagged_amounts_range);

        std::map<BAS::AccountNo,Amount> opening_balances = model->sie[year_id].opening_balances();
        // Output Omslutning
        /*
        Omslutning 20230501...20240430 {
          <Konto>      <IN>  <period>     <OUT>
              1920  36147,89  -7420,27  28727,62
              1999    156,75   -156,75      0,00
              2098      0,00  84192,50  84192,50
              2099  84192,50 -84192,50      0,00
              2440  -5459,55      0,00  -5459,55
              2641   1331,46    156,75   1488,21
              2893  -2601,86   2420,27   -181,59
              2898  -5000,00   5000,00      0,00
              9000                0,00      0,00
        } // Omslutning
        */
        prompt << "\nOmslutning " << *fiscal_year_date_range << " {";
        prompt << "\n" << std::setfill(' ');
        auto w = 12;
        prompt << std::setw(w) << "<Konto>";
        prompt << "\t" << std::setw(w) << "<IN>";
        prompt << "\t" << std::setw(w) << "<period>";
        prompt << "\t" << std::setw(w) <<  "<OUT>";
        for (auto const& ta : bas_account_accs) {
          auto omslutning = to_units_and_cents(ta.cents_amount());
          std::string bas_account_string = ta.tags().at("BAS"); 
          prompt << "\n";
          prompt << std::setw(w) << std::string("") + bas_account_string;
          auto bas_account_no = *BAS::to_account_no(bas_account_string);
          if (opening_balances.contains(bas_account_no)) {
            auto ib = opening_balances.at(bas_account_no);
            auto ib_units_and_cents = to_units_and_cents(to_cents_amount(ib)); 
            prompt << "\t" << std::setw(w) << to_string(ib_units_and_cents);
            prompt << "\t" << std::setw(w) << to_string(omslutning);
            prompt << "\t" << std::setw(w) << to_string(to_units_and_cents(to_cents_amount(ib) + ta.cents_amount()));
            opening_balances.erase(bas_account_no); // reported
          }
          else {
            prompt << "\t" << std::setw(w) << "";
            prompt << "\t" << std::setw(w) << to_string(omslutning);
            prompt << "\t" << std::setw(w) << to_string(omslutning);
          }
        }
        // Add on BAS accounts that has an IB but not reported above (no omslutning / transactions)
        for (auto const& [bas_account_no,opening_balance] : opening_balances) {
          std::string bas_account_string = std::to_string(bas_account_no); 
          prompt << "\n";
          prompt << std::setw(w) << std::string("") + bas_account_string;
          auto ib_units_and_cents = to_units_and_cents(to_cents_amount(opening_balance)); 
          prompt << "\t" << std::setw(w) << to_string(ib_units_and_cents);
          prompt << "\t" << std::setw(w) << "0,00";
          prompt << "\t" << std::setw(w) << to_string(ib_units_and_cents);
        }
        prompt << "\n} // Omslutning";
      }
      else {
        prompt << "\nTry '-omslutning' with no argument for current year or enter a valid fiscal year id 'current','-1','-2',...";
      }
    }
    else if (ast[0] == "-plain_ar") {
      // Brute force an Annual Financial Statement as defined by Swedish Bolagsverket
      // Parse BAS::K2::bas_2022_mapping_to_k2_ar_text to get mapping of BAS account saldos
      // to Swedish Bolagsverket Annual Financial Statement ("Årsredovisning") according to K2 rules

      // So BAS::K2::bas_2022_mapping_to_k2_ar_text maps AR <--> BAS
      // So we can use it to accumulate saldos for AR fields from the specified BAS accounts over the fiscal year


      // Create tagged amounts that aggregates BAS Accounts to a saldo and AR=<AR Field ID> ARTEXT=<AR Field Heading> ARCOMMENT=<AR Field Description>
      // and the aggregates BAS accounts to accumulate for this AR Field Saldo - members=id;id;id;...

      auto ar_entries = BAS::K2::AR::parse(BAS::K2::AR::ar_online::bas_2022_mapping_to_k2_ar_text);
      auto fiscal_year_date_range = model->sie["-1"].fiscal_year_date_range();

      if (false and fiscal_year_date_range) {
        auto fiscal_year_tagged_amounts_range = model->all_date_ordered_tagged_amounts.in_date_range(*fiscal_year_date_range); 
        auto bas_account_accs = tas::to_bas_omslutning(fiscal_year_tagged_amounts_range);

        if (true) {
          // Log Omslutning
          std::cout << "\nOmslutning {";
          for (auto const& ta : bas_account_accs) {
            auto omslutning = to_units_and_cents(ta.cents_amount());
            std::string bas_account_string = ta.tags().at("BAS"); 
            std::cout << "\n\tkonto:" << bas_account_string;
            auto ib = model->sie["-1"].opening_balance_of(*BAS::to_account_no(bas_account_string));
            if (ib) {
              auto ib_units_and_cents = to_units_and_cents(to_cents_amount(*ib)); 
              std::cout << " IB:" << to_string(ib_units_and_cents);
              std::cout << " omslutning:" << to_string(omslutning);
              std::cout << " UB:" << to_string(to_units_and_cents(to_cents_amount(*ib) + ta.cents_amount()));
            }
            else {
              std::cout << " omslutning:" << to_string(omslutning);
            }
          }
          std::cout << "\n} // Omslutning";
        }

        auto not_accumulated = bas_account_accs;
        for (auto const& ta : bas_account_accs) {	
          for (auto& ar_entry : ar_entries) {
            if (ta.tags().contains("BAS")) {
              if (auto bas_account_no = BAS::to_account_no(ta.tags().at("BAS"))) {
                if (ar_entry.accumulate_this_bas_account(*bas_account_no,ta.cents_amount())) {
                  if (auto iter = std::find(not_accumulated.begin(),not_accumulated.end(),ta); iter != not_accumulated.end()) {
                    not_accumulated.erase(iter);
                  }
                }
              }
              else {
                std::cout << "\nDESIGN INSUFFICIENCY: A tagged amount has a BAS tag set to an invalid (Non BAS account no) value " << ta.tags().at("BAS");
              }
            }
            else {
              // skip, this tagged amount  is NOT a transaction to a BAS account
            }
          }
        }
        prompt << "\nÅrsredovisning {";
        for (auto& ar_entry : ar_entries) {
          if (ar_entry.m_amount != 0) {
            prompt << "\n\tfält:" << ar_entry.m_field_heading_text << " belopp:" << to_string(to_units_and_cents(ar_entry.m_amount));
            prompt << " " << std::quoted(ar_entry.m_bas_accounts_text) << " matched:";
            for (auto const& bas_account_no : ar_entry.m_bas_account_nos) {
              prompt << " " << bas_account_no;
            }
          }
        }
        prompt << "\n} // Årsredovisning";

        prompt << "\nNOT Accumulated {";
        for (auto const& ta : not_accumulated) {	
          prompt << "\n\t" << ta;
        }
        prompt << "\n} // NOT Accumulated";
      }
      else {
        prompt << "\nSORRY, I seem to have lost track of last fiscal year first and last dates?";
        prompt << "\nCheck that you have used the '-sie' command to import an sie file with that years data from another application?";
      }
    }
    else if (ast[0] == "-plain_ink2") {
      // SKV Tax return according to K2 rules (plain text)
      // 

      // Parse BAS::SRU::INK2_19_P1_intervall_vers_2_csv to get a mapping between SRU Codes and field designations on SKV TAX Return and BAS Account ranges
      // From https://www.bas.se/kontoplaner/sru/
      // Also in resources/INK2_19_P1-intervall-vers-2.csv

      // Create tagged amounts that aggregates BAS Accounts to a saldo and SRU=<SRU code> TAX_RETURN_ID=<SKV Tax Return form box id>
      // and the aggregates BAS accounts to accumulate for this Tax Return Field Saldo - members=id;id;id;... 

      BAS::SRU::INK2::parse(BAS::SRU::INK2::INK2_19_P1_intervall_vers_2_csv);

    }
    else if (ast[0] == "-doc_ar") {
      // Assume the user wants to generate an annual report
      // ==> The first document seems to be the  1) financial statements approval (fastställelseintyg) ?
      doc::Document annual_report_financial_statements_approval{};
      {
        annual_report_financial_statements_approval << doc::plain_text("Fastställelseintyg");
      }

      doc::Document annual_report{};
      // ==> The second document seems to be the 2) directors’ report  (förvaltningsberättelse)?
      auto annual_report_front_page = doc::separate_page();
      {
        *annual_report_front_page << doc::plain_text("Årsredovisning");
      }
      auto annual_report_directors_report = doc::separate_page();
      {
        *annual_report_directors_report << doc::plain_text("Förvaltningsberättelse");
      }
      // ==> The third document seems to be the 3)  profit and loss statement (resultaträkning)?
      auto annual_report_profit_and_loss_statement = doc::separate_page();
      {
        *annual_report_profit_and_loss_statement << doc::plain_text("Resultaträkning");
      }
      // ==> The fourth document seems to be the 4) balance sheet (balansräkning)?
      auto annual_report_balance_sheet = doc::separate_page();
      {
        *annual_report_balance_sheet << doc::plain_text("Balansräkning");
      }
      // ==> The fifth document seems to be the 5) notes (noter)?			
      auto annual_report_annual_report_notes = doc::separate_page();
      {
        *annual_report_annual_report_notes << doc::plain_text("Noter");
      }

      std::filesystem::path to_bolagsverket_file_folder{"to_bolagsverket"};

      {
        // Generate documents in RTF format
        auto annual_report_file_folder = to_bolagsverket_file_folder / "rtf";

        // Create one document for the financial statemnts approval (to accomodate the annual report copy sent to Swedish Bolagsverket)
        {
          auto annual_report_file_path = annual_report_file_folder / "financial_statement_approval.rtf";
          std::filesystem::create_directories(annual_report_file_path.parent_path());
          std::ofstream raw_annual_report_os{annual_report_file_path};

          RTF::OStream annual_report_os{raw_annual_report_os};

          annual_report_os << annual_report_financial_statements_approval;
        }
        // Create the annual report with all the other sections
        {
          auto annual_report_file_path = annual_report_file_folder / "annual_report.rtf";
          std::filesystem::create_directories(annual_report_file_path.parent_path());
          std::ofstream raw_annual_report_os{annual_report_file_path};

          RTF::OStream annual_report_os{raw_annual_report_os};

          annual_report_os << annual_report_front_page;
          annual_report_os << annual_report_directors_report;
          annual_report_os << annual_report_profit_and_loss_statement;
          annual_report_os << annual_report_balance_sheet;
          annual_report_os << annual_report_annual_report_notes;
        }
      }

      {
        // Generate documents in HTML format
        auto annual_report_file_folder = to_bolagsverket_file_folder / "html";

        // Create one document for the financial statemnts approval (to accomodate the annual report copy sent to Swedish Bolagsverket)
        {
          auto annual_report_file_path = annual_report_file_folder / "financial_statement_approval.html";
          std::filesystem::create_directories(annual_report_file_path.parent_path());
          std::ofstream raw_annual_report_os{annual_report_file_path};

          HTML::OStream annual_report_os{raw_annual_report_os};

          annual_report_os << annual_report_financial_statements_approval;
        }
        // Create the annual report with all the other sections
        {
          auto annual_report_file_path = annual_report_file_folder / "annual_report.html";
          std::filesystem::create_directories(annual_report_file_path.parent_path());
          std::ofstream raw_annual_report_os{annual_report_file_path};

          HTML::OStream annual_report_os{raw_annual_report_os};

          annual_report_os << annual_report_front_page;
          annual_report_os << annual_report_directors_report;
          annual_report_os << annual_report_profit_and_loss_statement;
          annual_report_os << annual_report_balance_sheet;
          annual_report_os << annual_report_annual_report_notes;
        }
      }

      {
        // Generate documents in LaTeX format and then transform them into pdf using OS installed pdflatex command
        auto annual_report_file_folder = to_bolagsverket_file_folder / "tex";
        std::filesystem::create_directories(annual_report_file_folder);

        {
          // Test code
          char const* test_tex{R"(\documentclass{article}

\begin{document}

\begin{center}
The ITfied AB
\par \vspace*{12pt} 556782-8172
\par \vspace{12pt} Årsredovisning för räkenskapsåret
\par \vspace{12pt} 2021-05-01 - 2022-04-40
\par \vspace{12pt} Styrelsen avger följande årsredovisning
\par \vspace{12pt} Samtliga belopp är angivna i hela kronor
\end{center}

\pagebreak
\section*{Förvaltningsberättelse}

\subsection*{Verksamheten}
\subsubsection*{Allmänt om verksamheten}
\subsection*{Flerårsöversikt}
\subsection*{Förändring i eget kapital}
\subsection*{Resultatdisposition}

\pagebreak
\section*{Resultaträkning}

\pagebreak
\section*{Balansräkning}

\pagebreak
\section*{Noter}

\pagebreak
\section*{Underskrifter}

\end{document})"};
          std::ofstream os{annual_report_file_folder / "test.tex"};
          os << test_tex;
          // std::filesystem::copy_file("test/test.tex",annual_report_file_folder / "test.tex",std::filesystem::copy_options::overwrite_existing);
        }
      }

      {
        // Transform LaTeX documents to pdf:s
        auto annual_report_file_folder = to_bolagsverket_file_folder / "pdf";
        std::filesystem::create_directories(annual_report_file_folder);

        // pdflatex --output-directory to_bolagsverket/pdf test/test.tex

        for (auto const& dir_entry : std::filesystem::directory_iterator{to_bolagsverket_file_folder / "tex"}) {
          // Test for installed pdflatex
          std::ostringstream command{};
          command << "pdflatex";
          command << " --output-directory " << annual_report_file_folder;
          command << " " << dir_entry.path();
          if (auto result = std::system(command.str().c_str());result == 0) {
            prompt << "\nCreated pdf-document " << (annual_report_file_folder / dir_entry.path().stem()) << ".pdf" << " OK";
          }
          else {
            prompt << "\nSorry, failed to create pdf-documents.";
            prompt << "\n==> Please ensure you have installed command line tool 'pdflatex' on your system?";
            prompt << "\nmacOS: brew install --cask mactex";
            prompt << "\nLinux: 'sudo apt-get install texlive-latex-base texlive-fonts-recommended texlive-fonts-extra texlive-latex-extra'";
          }
        }					


      }

    }
    /* ======================================================



              Act on on Words?


      ====================================================== */
    else {
      // std::cout << "\nAct on words";
      // Assume word based input
      if (model->prompt_state == PromptState::HADIndex) {
        if (do_assign) {
          prompt << "\nSorry, ASSIGN not yet implemented for your input " << std::quoted(command);
        }
      }
      else if ((model->prompt_state == PromptState::NetVATAccountInput) or (model->prompt_state == PromptState::GrossAccountInput))  {
        // Assume the user has enterd text to search for suitable accounts
        // Assume match to account name
        for (auto const& [account_no,am] : model->sie["current"].account_metas()) {
          if (first_in_second_case_insensitive(ast[1],am.name)) {
            prompt << "\n  " << account_no << " " << std::quoted(am.name);
            if (am.sru_code) prompt << " SRU:" << *am.sru_code;
          }
        }
      }
      else if (model->prompt_state == PromptState::EnterHA) {
        if (auto had_iter = model->selected_had()) {
          auto& had = *(*had_iter);
          if (!had.optional.current_candidate) std::cout << "\nNo had.optional.current_candidate";
          if (!had.optional.counter_ats_producer) std::cout << "\nNo had.optional.counter_ats_producer";
          if (had.optional.current_candidate and had.optional.counter_ats_producer) {
            auto gross_positive_amount = to_positive_gross_transaction_amount(had.optional.current_candidate->defacto);
            auto gross_negative_amount = to_negative_gross_transaction_amount(had.optional.current_candidate->defacto);
            auto gross_amounts_diff = gross_positive_amount + gross_negative_amount;
// std::cout << "\ngross_positive_amount:" << gross_positive_amount << " gross_negative_amount:" << gross_negative_amount << " gross_amounts_diff:" << gross_amounts_diff;

            switch (ast.size()) {
              case 0: {
                prompt << "\nPlease enter:";
                prompt << "\n\t Heading + Amount (to add a transaction aggregate with a caption)";
                prompt << "\n\t Heading          (to add a transaction aggregate with a caption and full remaining amount)";							
              } break;
              case 1: {
                if (auto amount = to_amount(ast[0])) {
                  prompt << "\nAMOUNT " << *amount;
                  prompt << "\nPlease enter Heading only (full remaining amount implied) or Heading + Amount";
                }
                else {
                  prompt << "\nHEADER " << ast[0];
                  auto ats = (*had.optional.counter_ats_producer)(abs(gross_amounts_diff),ast[0]);
                  std::copy(ats.begin(),ats.end(),std::back_inserter(had.optional.current_candidate->defacto.account_transactions));
                  prompt << "\nAdded transaction aggregate for REMAINING NET AMOUNT" << ats;;
                }
              } break;
              case 2: {
                if (auto amount = to_amount(ast[1])) {
                  prompt << "\nHEADER " << ast[0];
                  prompt << "\nAMOUNT " << *amount;
                  prompt << "\nWe will create a {net,vat} using this this header and amount";
                  if (gross_amounts_diff > 0) {
                    // We need to balance up with negative account transaction aggregates
                    auto ats = (*had.optional.counter_ats_producer)(abs(gross_amounts_diff),ast[0],amount);
                    std::copy(ats.begin(),ats.end(),std::back_inserter(had.optional.current_candidate->defacto.account_transactions));
                    prompt << "\nAdded negative transactions aggregate" << ats;
                  }
                  else if (gross_amounts_diff < 0) {
                    // We need to balance up with positive account transaction aggregates
                    auto ats = (*had.optional.counter_ats_producer)(abs(gross_amounts_diff),ast[0],amount);
                    std::copy(ats.begin(),ats.end(),std::back_inserter(had.optional.current_candidate->defacto.account_transactions));
                    prompt << "\nAdded positive transaction aggregate";
                  }
                  else if (abs(gross_amounts_diff) < 1.0) {
                    // Consider a cents rounding account transaction
                    prompt << "\nAdded cents rounding to account 3740";
                    auto cents_rounding_at = BAS::anonymous::AccountTransaction{
                      .account_no = 3740
                      ,.amount = -gross_amounts_diff
                    };
                    had.optional.current_candidate->defacto.account_transactions.push_back(cents_rounding_at);
                  }
                  else {
                    // The journal entry candidate balances. Consider to stage it
                    prompt << "\nTODO: Stage balancing journal entry";
                  }
                }
              } break;
            }
            prompt << "\ncandidate:" << *had.optional.current_candidate;
            gross_positive_amount = to_positive_gross_transaction_amount(had.optional.current_candidate->defacto);
            gross_negative_amount = to_negative_gross_transaction_amount(had.optional.current_candidate->defacto);
            gross_amounts_diff = gross_positive_amount + gross_negative_amount;
            prompt << "\n-------------------------------";
            prompt << "\ndiff:" << gross_amounts_diff;
            if (gross_amounts_diff == 0) {
              // Stage the journal entry
              auto me = model->sie["current"].stage(*had.optional.current_candidate);
              if (me) {
                prompt << "\n" << *me << " STAGED";
                model->heading_amount_date_entries.erase(*had_iter);
                model->prompt_state = PromptState::HADIndex;
              }
              else {
                prompt << "\nSORRY - Failed to stage entry";
                model->prompt_state = PromptState::Root;
              }
            }

          }
          else {
            prompt << "\nPlease re-select a valid HAD and template (Seems to have failed to identify a valid template for current situation)";
          }
        }
        else {
          prompt << "\nPlease re-select a valid had (seems to have lost previously selected had)";
        }

      }
      else if (    (model->prompt_state == PromptState::JEIndex)
                or (model->prompt_state == PromptState::JEAggregateOptionIndex)) {
        if (do_assign) {
          // Assume <text> = <text>
          if (auto had_iter = model->selected_had()) {
            auto& had = *(*had_iter);
            if (ast.size() >= 3 and ast[1] == "=") {
              // Assume at least three tokens 0:<token> 1:'=' 2:<Token>
              if (auto amount = to_amount(ast[2])) {
                BAS::AccountMetas ams{};
                auto transaction_amount = (abs(*amount) <= 1)?(to_double(*amount) * had.amount):(*amount); // quote of had amount or actual amount

                if (auto to_match_account_no = BAS::to_account_no(ast[0])) {
                  // The user entered <target> = a BAS Account or SRU account
                  ams = matches_bas_or_sru_account_no(*to_match_account_no,model->sie["current"]);
                }
                else {
                  // The user entered a search criteria for a BAS account name
                  ams = matches_bas_account_name(ast[0],model->sie["current"]);										
                }
                if (ams.size() == 0) {
                  prompt << "\nSorry, failed to match your input to any BAS or SRU account";
                }
                else if (ams.size() == 1) {
                  // Go ahead and use this account for an account transaction
                  if (had.optional.current_candidate) {
                    // extend current candidate
                    BAS::anonymous::AccountTransaction new_at{
                      .account_no = ams.begin()->first
                      ,.amount = transaction_amount
                    };
                    had.optional.current_candidate->defacto.account_transactions.push_back(new_at);
                    prompt << "\n" << *had.optional.current_candidate;
                  }
                  else {
                    // Create options from scratch
                    had.optional.gross_account_no = ams.begin()->first;
                    BAS::TypedMetaEntries template_candidates{};
                    std::string transtext = (ast.size() == 4)?ast[3]:"";
                    for (auto series : std::string{"ABCDEIM"}) {
                      BAS::MetaEntry me{
                        .meta = {
                          .series = series
                        }
                        ,.defacto = {
                          .caption = had.heading
                          ,.date = had.date
                        }
                      };
                      BAS::anonymous::AccountTransaction new_at{
                        .account_no = ams.begin()->first
                        ,.transtext = transtext
                        ,.amount = transaction_amount
                      };
                      me.defacto.account_transactions.push_back(new_at);
                      template_candidates.push_back(to_typed_meta_entry(me));
                    }
                    model->template_candidates.clear();
                    std::copy(template_candidates.begin(),template_candidates.end(),std::back_inserter(model->template_candidates));
                    unsigned ix = 0;
                    for (int i=0; i < model->template_candidates.size(); ++i) {
                      prompt << "\n    " << ix++ << " " << model->template_candidates[i];
                    }
                  }
                }
                else {
                  // List the options to inform the user there are more than one BAS account that matches the input
                  prompt << "\nDid you mean...";
                  for (auto const& [account_no,am] : ams) {
                    prompt << "\n\t" << std::quoted(am.name) << " " << account_no << " = " << transaction_amount;
                  }
                  prompt << "\n==> Please try again <target> = <Quote or amount>";
                }
              } // to_amount
              else {
                  prompt << "\nPlease provide a valid amount after '='. I failed to recognise your input " << std::quoted(ast[2]);
              }
            } // ast[1] == '='
            else {
              prompt << "Please provide a space on both sides of the '=' in your input " << std::quoted(command);
            }							
          }
          else {
            prompt << "\nSorry, I seem to have lost track of what had you selected.";
            prompt << "\nPlease try again with a new had.";														
          }
        }
        else {
          // Assume the user has entered a new search criteria for template candidates
          model->template_candidates = this->all_years_template_candidates([&command](BAS::anonymous::JournalEntry const& aje){
            return strings_share_tokens(command,aje.caption);
          });
          int ix{0};
          for (int i = 0; i < model->template_candidates.size(); ++i) {
            prompt << "\n    " << ix++ << " " << model->template_candidates[i];
          }
          // Consider the user may have entered the name of a gross account to journal the transaction amount
          auto gats = to_gross_account_transactions(model->sie);
          model->at_candidates.clear();
          std::copy_if(gats.begin(),gats.end(),std::back_inserter(model->at_candidates),[&command,this](BAS::anonymous::AccountTransaction const& at){
            bool result{false};
            if (at.transtext) result |= strings_share_tokens(command,*at.transtext);
            if (model->sie["current"].account_metas().contains(at.account_no)) {
              auto const& meta = model->sie["current"].account_metas().at(at.account_no);
              result |= strings_share_tokens(command,meta.name);
            }
            return result;
          });
          for (int i=0;i < model->at_candidates.size();++i) {
            prompt << "\n    " << ix++<< " " << model->at_candidates[i];
          }
        }
      }
      else if (model->prompt_state == PromptState::CounterAccountsEntry) {
        if (auto nha = to_name_heading_amount(ast)) {
          // List account candidates for the assumed "Name, Heading + Amount" entry by the user
          prompt << "\nAccount:" << std::quoted(nha->account_name);
          if (nha->trans_text) prompt << " text:" << std::quoted(*nha->trans_text);
          prompt << " amount:" << nha->amount;
        }
        else {
          prompt << "\nPlease enter an account, and optional transaction text and an amount";
        }
        // List the new current options
        if (auto had_iter = model->selected_had()) {
          if ((*had_iter)->optional.current_candidate) {
            unsigned int i{};
            prompt << "\n" << (*had_iter)->optional.current_candidate->defacto.caption << " " << (*had_iter)->optional.current_candidate->defacto.date;
            for (auto const& at : (*had_iter)->optional.current_candidate->defacto.account_transactions) {
              prompt << "\n  " << i++ << " " << at;
            }				
          }
          else {
            prompt << "\nPlease enter a valid Account Transaction Index";
          }
        }
        else {
          prompt << "\nPlease select a Heading Amount Date entry";
        }
      }
      else if (model->prompt_state == PromptState::EditAT) {
        // Handle user Edit of currently selected account transaction (at)
// std::cout << "\nPromptState::EditAT " << std::quoted(command);
        if (auto had_iter = model->selected_had()) {
          auto const& had = **had_iter;
      		if (auto at_iter = model->to_at_iter(had_iter,model->at_index)) {
            auto& at = **at_iter;
            prompt << "\n\tbefore:" << at;
            if (auto account_no = BAS::to_account_no(command)) {
              at.account_no = *account_no;
              prompt << "\nBAS Account: " << *account_no;
            }
            else if (auto amount = to_amount(command)) {
              prompt << "\nAmount " << *amount;
              at.amount = *amount;
            }
            else if (command[0] == 'x') {
              // ignore input
            }
            else {
              // Assume the user entered a new transtext
              prompt << "\nTranstext " << std::quoted(command);
              at.transtext = command;
            }
            unsigned int i{};
            std::for_each(had.optional.current_candidate->defacto.account_transactions.begin(),had.optional.current_candidate->defacto.account_transactions.end(),[&i,&prompt](auto const& at){
              prompt << "\n  " << i++ << " " << at;
            });
            model->prompt_state = PromptState::ATIndex;
          }
          else {
            prompt << "\nSORRY, I seems to have forgotten what account transaction you selected. Please try over again";
            model->prompt_state = PromptState::HADIndex;
          }
        }
        else {
          prompt << "\nSORRY, I seem to have forgotten what HAD you selected. Please select a Heading Amount Date entry";
          model->prompt_state = PromptState::HADIndex;
        }
      }
      else if (model->prompt_state == PromptState::EnterContact) {
        if (ast.size() == 3) {
          SKV::ContactPersonMeta cpm {
            .name = ast[0]
            ,.phone = ast[1]
            ,.e_mail = ast[2]
          };
          if (model->organisation_contacts.size() == 0) {
            model->organisation_contacts.push_back(cpm);
          }
          else {
            model->organisation_contacts[0] = cpm;
          }
          auto const& [delta_prompt,prompt_state] = this->transition_prompt_state(model->prompt_state,PromptState::SKVTaxReturnEntryIndex);
          prompt << delta_prompt;
          model->prompt_state = prompt_state;
        }
      }
      else if (model->prompt_state == PromptState::EnterEmployeeID) {
        if (ast.size() > 0) {
          if (model->employee_birth_ids.size()==0) {
            model->employee_birth_ids.push_back(ast[0]);
          }
          else {
            model->employee_birth_ids[0] = ast[0];
          }
          auto const& [delta_prompt,prompt_state] = this->transition_prompt_state(model->prompt_state,PromptState::SKVTaxReturnEntryIndex);
          prompt << delta_prompt;
          model->prompt_state = prompt_state;
        }
      }
      else if (model->prompt_state == PromptState::EnterIncome) {
        if (auto amount = to_amount(command)) {
          model->sru["0"].set(1000,std::to_string(SKV::to_tax(*amount)));

          Amount income = get_INK1_Income(model);
          prompt << "\n1) INK1 1.1 Lön, förmåner, sjukpenning m.m. = " << income;
          Amount dividend = get_K10_Dividend(model);
          prompt << "\n2) K10 1.6 Utdelning = " << dividend;
          prompt << "\n3) Continue (Create K10 and INK1)";
          model->prompt_state = PromptState::K10INK1EditOptions;								
        }
        else {
          prompt << "\nPlease enter a valid amount";
        }
      }
      else if (model->prompt_state == PromptState::EnterDividend) {
        if (auto amount = to_amount(command)) {
          model->sru["0"].set(4504,std::to_string(SKV::to_tax(*amount)));
          Amount income = get_INK1_Income(model);
          prompt << "\n1) INK1 1.1 Lön, förmåner, sjukpenning m.m. = " << income;
          Amount dividend = get_K10_Dividend(model);
          prompt << "\n2) K10 1.6 Utdelning = " << dividend;
          prompt << "\n3) Continue (Create K10 and INK1)";
          model->prompt_state = PromptState::K10INK1EditOptions;								
        }
        else {
          prompt << "\nPlease enter a valid amount";
        }
      }
      else if (auto me = find_meta_entry(model->sie["current"],ast)) {
        // The user has entered a search term for a specific journal entry (to edit)
        // Allow the user to edit individual account transactions
        if (auto had = to_had(*me)) {
          model->heading_amount_date_entries.push_back(*had);
          model->had_index = 0; // index zero is the "last" (newest) one
          unsigned int i{};
          std::for_each(had->optional.current_candidate->defacto.account_transactions.begin(),had->optional.current_candidate->defacto.account_transactions.end(),[&i,&prompt](auto const& at){
            prompt << "\n  " << i++ << " " << at;
          });
          model->prompt_state = PromptState::ATIndex;
        }
        else {
          prompt << "\nSorry, I failed to turn selected journal entry into a valid had (it seems I am not sure exactly why...)";
        }
      }
      else {
        // Assume Heading + Amount + Date (had) input
        auto tokens = tokenize::splits(command,tokenize::SplitOn::TextAmountAndDate);
        if (auto had = to_had(tokens)) {
          prompt << "\n" << *had;
          model->heading_amount_date_entries.push_back(*had);
          // Decided NOT to sort hads here to keep last listed had indexes intact (new -had command = sort and new indexes)
        }
        else {
          prompt << "\nERROR - Expected Heading + Amount + Date";
          prompt << "\nI Interpreted your input as,";
          for (auto const& token : tokens) prompt << "\n\ttoken: \"" << token << "\"";
          prompt << "\n\nPlease check that your input matches my expectations?";
          prompt << "\n\tHeading = any text (\"...\" enclosure allowed)";
          prompt << "\n\tAmount = any positive or negative amount with optional ',' or '.' decimal point with one or two decimal digits";
          prompt << "\n\tDate = YYYYMMDD or YYYY-MM-DD";
        }
      }
    }
  }
  if (prompt.str().size()>0) prompt << "\n"; 
  prompt << prompt_line(model->prompt_state);
  model->prompt = prompt.str();
  return {};
}
Cmd Updater::operator()(Quit const& quit) {
  // std::cout << "\noperator(Quit)";
  std::ostringstream os{};
  os << "\nBy for now :)";
  model->prompt = os.str();
  model->quit = true;
  return {};
}
Cmd Updater::operator()(Nop const& nop) {
  // std::cout << "\noperator(Nop)";
  return {};
}	
BAS::TypedMetaEntries Updater::all_years_template_candidates(auto const& matches) {
  BAS::TypedMetaEntries result{};
  auto meta_entry_topology_map = to_meta_entry_topology_map(model->sie);
  for (auto const& [signature,tme_map] : meta_entry_topology_map) {
    for (auto const& [topology,tmes] : tme_map) {
      auto accounts_topology_map = to_accounts_topology_map(tmes);
      for (auto const& [signature,bat_map] : accounts_topology_map) {
        for (auto const& [topology,tmes] : bat_map) {
          for (auto const& tme : tmes) {
            auto me = to_meta_entry(tme);
            if (matches(me.defacto)) result.push_back(tme);
          }
        }
      }
    }
  }
  return result;
}
std::pair<std::string,PromptState> Updater::transition_prompt_state(PromptState const& from_state,PromptState const& to_state) {
  std::ostringstream prompt{};
  switch (to_state) {
    case PromptState::SKVTaxReturnEntryIndex: {
      prompt << "\n1 Organisation Contact:" << std::quoted(model->organisation_contacts[0].name) << " " << std::quoted(model->organisation_contacts[0].phone) << " " << model->organisation_contacts[0].e_mail;
      prompt << "\n2 Employee birth no:" << model->employee_birth_ids[0];
      prompt << "\n3 <Period> Generate Tax Returns form (Period in form YYYYMM)";
    } break;
    default: {
      prompt << "\nPlease mail developer that transition_prompt_state detected a potential design insufficiency";
    } break;
  }
  return {prompt.str(),to_state};
}


class Cratchit {
public:
	Cratchit(std::filesystem::path const& p) 
		: cratchit_file_path{p} {}

	Model init(Command const& command) {
		std::ostringstream prompt{};
		prompt << "\nInit from ";
		prompt << cratchit_file_path;
		auto environment = environment_from_file(cratchit_file_path);
		auto model = this->model_from_environment(environment);
		model->prompt_state = PromptState::Root;
		prompt << "\n" << prompt_line(model->prompt_state);
		model->prompt += prompt.str();
		return model;
	}
	std::pair<Model,Cmd> update(Msg const& msg,Model&& model) {
		// std::cout << "\nupdate" << std::flush;
		Cmd cmd{};
		{
			// Scope to ensure correct move of model in and out of updater
			model->prompt.clear();
			Updater updater{std::move(model)};
			cmd = std::visit(updater,msg);
			model = std::move(updater.model);
		}
		// std::cout << "\nCratchit::update updater visit ok" << std::flush;
		if (model->quit) {
			// std::cout << "\nCratchit::update if (updater.model->quit) {" << std::flush;
			auto model_environment = environment_from_model(model);
			auto cratchit_environment = add_cratchit_environment(model_environment);
			this->environment_to_file(cratchit_environment,cratchit_file_path);
			unposted_to_sie_file(model->sie["current"],model->staged_sie_file_path);
			// Update/create the skv xml-file (employer monthly tax declaration)
// std::cout << R"(\nmodel->sie["current"].organisation_no.CIN=)" << model->sie["current"].organisation_no.CIN;
		}
		// std::cout << "\nCratchit::update before prompt update" << std::flush;
		// std::cout << "\nCratchit::update before return" << std::flush;
		return {std::move(model),cmd};
	}
	Ux view(Model const& model) {
		// std::cout << "\nview" << std::flush;
		Ux ux{};
		ux.push_back(model->prompt);
		return ux;
	}
private:
	std::filesystem::path cratchit_file_path{};
	std::vector<SKV::ContactPersonMeta> contacts_from_environment(Environment const& environment) {
		std::vector<SKV::ContactPersonMeta> result{};
		auto [begin,end] = environment.equal_range("contact");
		if (begin == end) {
			// Instantiate default values if required
			result.push_back({
					.name = "Ville Vessla"
					,.phone = "555-244454"
					,.e_mail = "ville.vessla@foretaget.se"
				});
		}
		else {
			std::transform(begin,end,std::back_inserter(result),[](auto const& entry){
				return *to_contact(entry.second); // Assume success
			});
		}
		return result;
	}

	DateOrderedTaggedAmountsContainer date_ordered_tagged_amounts_from_sie_environment(SIEEnvironment const& sie_env) {
		// std::cout << "\ndate_ordered_tagged_amounts_from_sie_environment" << std::flush;
		DateOrderedTaggedAmountsContainer result{};
    // Create / add opening balances for BAS accounts as tagged amounts
    auto fiscal_year_date_range = sie_env.fiscal_year_date_range();
    auto opening_saldo_date = fiscal_year_date_range->begin();
    // std::cout << "\nOpening Saldo Date:" << opening_saldo_date;
    for (auto const& [bas_account_no,saldo] : sie_env.opening_balances()) {
      auto saldo_cents_amount = to_cents_amount(saldo);
      TaggedAmount saldo_ta{opening_saldo_date,saldo_cents_amount};
      saldo_ta.tags()["BAS"] = std::to_string(bas_account_no);
      saldo_ta.tags()["IB"] = "True";
      result.insert(saldo_ta);
      // std::cout << "\n\tsaldo_ta : " << saldo_ta;
    }
		auto create_tagged_amounts_to_result = [&result](BAS::MetaEntry const& me) {
			auto tagged_amounts = to_tagged_amounts(me);
      // TODO #SIE: Consider to check here is we already have tagged amounts reflecting the same SIE transaction (This one in the SIE file is the one to use)
      //       Can we first delete any existing tagged amounts for the same SIE transaction (to ensure we do not get dublikates for SIE transactions edited externally?)
      // Hm...problem is that here we do not have access to the other tagged amounts already in the environment...
			result += tagged_amounts;
		};
		for_each_meta_entry(sie_env,create_tagged_amounts_to_result);
		return result;
	}

  struct SKVSpecsDummy {}; 

  SKVSpecsDummy skv_specs_mapping_from_csv_files(Environment const& environment) {
    // TODO 230420: Implement actual storage in model for these mappings (when usage is implemented)
    SKVSpecsDummy result{};
		auto skv_specs_path = this->cratchit_file_path.parent_path() /  "skv_specs";
		std::filesystem::create_directories(skv_specs_path); // Returns false both if already exists and if it fails (so useless to check...I think?)
		if (std::filesystem::exists(skv_specs_path) == true) {
			// Process all files in skv_specs_path
			std::cout << "\nBEGIN: Processing files in " << skv_specs_path;
			for (auto const& dir_entry : std::filesystem::directory_iterator{skv_specs_path}) {
				auto skv_specs_member_path = dir_entry.path();
        if (std::filesystem::is_directory(skv_specs_member_path)) {
          std::cout << "\n\nBEGIN Folder: " << skv_specs_member_path;
    			for (auto const& dir_entry : std::filesystem::directory_iterator{skv_specs_member_path}) {
    				auto fiscal_year_member_path = dir_entry.path();
            std::cout << "\n\tBEGIN " <<  fiscal_year_member_path;
            if (std::filesystem::is_regular_file(fiscal_year_member_path) and (fiscal_year_member_path.extension() == ".csv")) {
              auto ifs = std::ifstream{fiscal_year_member_path};
              encoding::UTF8::istream utf8_in{ifs}; // Assume the file is created in UTF8 character set encoding
              if (auto field_rows = CSV::to_field_rows(utf8_in,';')) {
                std::cout << "\n\tNo Operation implemented";
                // std::cout << "\n\t<Entries>";
                for (int i=0;i<field_rows->size();++i) {
                  auto field_row = field_rows->at(i);
                  // std::cout << "\n\t\t[" << i << "] : ";
                  for (int j=0;j<field_row.size();++j) {
                    // [14] :  [0]1.1 Årets gränsbelopp enligt förenklingsregeln [1]4511 [2]Numeriskt_B [3]N [4]+ [5]Regel_E
                    // index 1 = SRU Code
                    // Index 0 = Readable field name on actual human readable form
                    // The other fields defines representation and "rules"
                    // std::cout << " [" << j << "]" << field_row[j];
                  }
                }
              }
              else {
                std::cout << "\n\tFAILED PARSING FILE " <<  fiscal_year_member_path;
              }
            }
            else {
              std::cout << "\n\tSKIPPED UNUSED FILE" <<  fiscal_year_member_path;
            }
            std::cout << "\n\tEND " <<  fiscal_year_member_path;
          }
          std::cout << "\nEND Folder: " << skv_specs_member_path;
        }
        else {
          std::cout << "\n\tSKIPPED NON FOLDER PATH " << skv_specs_member_path;
        }
      }
			std::cout << "\nEND: Processing files in " << skv_specs_path;
    }
    return result;
  }

	DateOrderedTaggedAmountsContainer date_ordered_tagged_amounts_from_account_statement_files(Environment const& environment) {
    if (true) {
		  std::cout << "\ndate_ordered_tagged_amounts_from_account_statement_files" << std::flush;
    }
		DateOrderedTaggedAmountsContainer result{};
		// Ensure folder "from_bank_or_skv folder" exists
		auto from_bank_or_skv_path = this->cratchit_file_path.parent_path() /  "from_bank_or_skv";
		std::filesystem::create_directories(from_bank_or_skv_path); // Returns false both if already exists and if it fails (so useless to check...I think?)
		if (std::filesystem::exists(from_bank_or_skv_path) == true) {
			// Process all files in from_bank_or_skv_path
			std::cout << "\nBEGIN: Processing files in " << from_bank_or_skv_path << std::flush;
      // auto dir_entries = std::filesystem::directory_iterator{}; // Test empty iterator (does heap error disappear?) - YES!
      // TODO 240611 - Locate heap error detected here for g++ built with address sanitizer
      // NOTE: Seems to be all std::filesystem::directory_iterator usage in this code (Empty iterator here makes another loop cause heap error)
			for (auto const& dir_entry : std::filesystem::directory_iterator{from_bank_or_skv_path}) { 
        // 240612 Test if g++14 with sanitizer detects heap violation also for an empty std::filesystem::directory_iterator loop?
        //        YES - c++14 with "-fsanitize=address,undefined" reports runtime heap violation error on the for statement above!
        if (true) {
          auto path = dir_entry.path();
          std::cout << "\n\nBEGIN File: " << path << std::flush;
          if (dir_entry.is_directory()) {
            // skip directories (will process regular files and symlinks etc...)
          }
          // Process file
          else if (auto tagged_amounts = to_tagged_amounts(path)) {
            result += *tagged_amounts;
            std::cout << "\n\tValid entries count:" << tagged_amounts->size();
            auto consumed_files_path = from_bank_or_skv_path / "consumed";
            std::filesystem::create_directories(consumed_files_path); // Returns false both if already exists and if it fails (so useless to check...I think?)
            std::filesystem::rename(path,consumed_files_path / path.filename());
            std::cout << "\n\tConsumed account statement file moved to " << consumed_files_path / path.filename();
          }
          else {
            std::cout << "\n*Ignored* " << path << " (Failed to understand file content)";
          }
          std::cout << "\nEND File: " << path;
        }
			}
			std::cout << "\nEND: Processed Files in " << from_bank_or_skv_path;
		}
    if (true) {
  		std::cout << "\ndate_ordered_tagged_amounts_from_account_statement_files RETURNS " << result.tagged_amounts().size() << " entries";
    }
		return result;
	}

	DateOrderedTaggedAmountsContainer date_ordered_tagged_amounts_from_environment(Environment const& environment) {
    if (true) {
		  std::cout << "\ndate_ordered_tagged_amounts_from_environment" << std::flush;
    }
		DateOrderedTaggedAmountsContainer result{};
		auto [begin,end] = environment.equal_range("TaggedAmount");
		std::for_each(begin,end,[&result](auto const& entry){
			if (auto o_ta = to_tagged_amount(entry.second)) {
				result.insert(*o_ta);
			}
			else {
				std::cout << "\nDESIGN INSUFFICIENCY - Failed to convert environment value " << entry.second << " to a TaggedAmount";
			}
		});

		// Import any new account statements in dedicated "files from bank or skv" folder
		result += date_ordered_tagged_amounts_from_account_statement_files(environment);
		return result;
	}

  void synchronize_tagged_amounts_with_sie(DateOrderedTaggedAmountsContainer& all_date_ordered_tagged_amounts,SIEEnvironment const& sie_environment) {
    // Use a double pointer mechanism to step through provided all_date_ordered_tagged_amounts and date_ordered_tagged_amounts_from_sie_environment in date order.
    // 1) If an SIE entry is in tagged amount but NOT in sie environment --> erase it from tagged amounts
    // 2) If an SIE entry is in sie environment but NOT in tagged amounts --> insert it into tagged amounts
    // 3) If an SIE entry is in both tagged amounts and sie environment but with the wrong properties --> Erase in tagged amounts and insert from SIE
    // 4) else, if both in tagged amounts and SIE --> do nothing (all is in sync)

    std::cout << "\nSYNHRONIZE TAGGED AMOUNTS WITH SIE - BEGIN {";
    if (auto fiscal_year_date_range = sie_environment.fiscal_year_date_range()) {
      std::cout << "\n\tSIE fiscal year:" << *fiscal_year_date_range;
      auto date_ordered_tagged_amounts_from_sie_environment = this->date_ordered_tagged_amounts_from_sie_environment(sie_environment);
      auto target_iter = all_date_ordered_tagged_amounts.begin();
      auto source_iter = date_ordered_tagged_amounts_from_sie_environment.begin();
      if (target_iter != all_date_ordered_tagged_amounts.end() and source_iter != date_ordered_tagged_amounts_from_sie_environment.end()) {
        // Both are valid ranges
        // get the fiscal year of provided SIE entries
        if (target_iter->date()<fiscal_year_date_range->begin()) {
          // Skip tagged amounts before the fiscal year
          while (target_iter != all_date_ordered_tagged_amounts.end() and target_iter->date()<fiscal_year_date_range->begin()) {
            // std::cout << "\n\tSkipping Older TA" << *(*target_iter);
            ++target_iter;
          }
        }
        else while (source_iter != date_ordered_tagged_amounts_from_sie_environment.end() and source_iter->date() < target_iter->date()) {
          // Add new SIE entries not yet in (older than) "all"
          std::cout << "\n\tAdding Older SIE entry" << *source_iter;
          all_date_ordered_tagged_amounts.insert(*source_iter);
          ++source_iter;
        }
        // We are now Synchronized to the "same" start date.
        // Now ensure "all" contains only the provided SIE entries
        while (source_iter != date_ordered_tagged_amounts_from_sie_environment.end()) {
          if (source_iter->tag_value("IB")) {
            all_date_ordered_tagged_amounts.insert(*source_iter); // Ensure any opening balance is updated ok (sie file values take precedence)
            ++source_iter;
            continue; // skip further processing of added opening balance
          }

          // iterate to next SIE aggregate in source and target
          if ( not source_iter->tag_value("SIE")) {
            ++source_iter;
            continue; // skip non SIE aggregate
          }
          if  (target_iter != all_date_ordered_tagged_amounts.end() and !target_iter->tag_value("SIE")) {
            ++target_iter;
            continue; // skip non SIE aggregate
          }
          // Both source and target is an SIE aggregate ok
          if (target_iter != all_date_ordered_tagged_amounts.end()) {
            if (target_iter->date() < source_iter->date()) {
              // This SIE aggregate in "all" should not be there!
              std::cout << "\n\tShould be erased - Older TA not in imported SIE " << *target_iter;
              // all_date_ordered_tagged_amounts.erase(target_iter);
              ++target_iter;
            }
            else {
              // Now, the SIE in target must be in provided SIE entries or else it must go
              if (*target_iter == *source_iter) { 
                ++source_iter;
                ++target_iter;
              }
              else if (target_iter->date() > source_iter->date()) {
                // Provided SIE contains an entry not in "all"
                std::cout << "\n\tInserted to TA a source SIE with date > target SIE date " << *source_iter;
                all_date_ordered_tagged_amounts.insert(*source_iter);
                ++source_iter;
              }
              else {
                // We have a quirky situation here. Provided SIE entries are ordrered by Date
                // but may come out of order in sequence?!
                // So maybe we need to assemble all SIE entries with the same date and check these sets are equal?
                std::cout << "\n\tOut of order (or undeteced same?) - May require SET comparisson or updated operator==? {";
                std::cout << "\n\t\tSIE: " << *source_iter;
                std::cout << "\n\t\tTA: " << *target_iter;
                std::cout << "\n\t} Out of order?";
                ++target_iter;
              }
            }
          }
          else {
            // Add new SIE entries not yet in "all"
            std::cout << "\n\tADDING to TA the newer imported SIE" << *source_iter;
            all_date_ordered_tagged_amounts.insert(*source_iter);
            ++source_iter;
          }
        } // while source_iter
      }
    }
    else {
      std::cout << "\n\tERROR, synchronize_tagged_amounts_with_sie failed -  Provided SIE Environment has no fiscal year set";
    }
    std::cout << "\n} SYNHRONIZE TAGGED AMOUNTS WITH SIE - END";
  }

	SRUEnvironments srus_from_environment(Environment const& environment) {
		SRUEnvironments result{};
		auto [begin,end] = environment.equal_range("SRU:S");
		std::transform(begin,end,std::inserter(result,result.end()),[](auto const& entry){
			if (auto result_entry = to_sru_environments_entry(entry.second)) return *result_entry;
			return SRUEnvironments::value_type{"null",{}};
		});
		return result;
	}

	std::vector<std::string> employee_birth_ids_from_environment(Environment const& environment) {
		std::vector<std::string> result{};
		auto [begin,end] = environment.equal_range("employee");
		if (begin == end) {
			// Instantiate default values if required
			result.push_back({"198202252386"});
		}
		else {
			std::transform(begin,end,std::back_inserter(result),[](auto const& entry){
				return *to_employee(entry.second); // dereference optional = Assume success
			});
		}
		return result;
	}
	
	HeadingAmountDateTransEntries hads_from_environment(Environment const& environment) {
		HeadingAmountDateTransEntries result{};
		auto [begin,end] = environment.equal_range("HeadingAmountDateTransEntry");
		std::transform(begin,end,std::back_inserter(result),[](auto const& entry){
			return *to_had(entry.second); // Assume success
		});
		return result;
	}

	bool is_value_line(std::string const& line) {
		return (line.size()==0)?false:(line.substr(0,2) != R"(//)");
	}

	Model model_from_environment(Environment const& environment) {
		Model model = std::make_unique<ConcreteModel>();
		std::ostringstream prompt{};

		if (auto val_iter = environment.find("sie_file");val_iter != environment.end()) {
			for (auto const& [year_key,sie_file_name] : val_iter->second) {
				std::filesystem::path sie_file_path{sie_file_name};
				if (auto sie_environment = from_sie_file(sie_file_path)) {
					model->sie[year_key] = std::move(*sie_environment);
					model->sie_file_path[year_key] = {sie_file_name};
					prompt << "\nsie[" << year_key << "] from " << sie_file_path;
				}
        else {
					prompt << "\nsie[" << year_key << "] from " << sie_file_path << " - *FAILED*";
        }
			}
		}
		if (auto sse = from_sie_file(model->staged_sie_file_path)) {
			if (sse->journals().size()>0) {
				prompt << "\n<STAGED>";
				prompt << *sse;
			}
			auto unstaged = model->sie["current"].stage(*sse); // the call returns any now no longer staged entries
			for (auto const& je : unstaged) {
				prompt << "\nnow posted " << je; 
			}
		}
		model->heading_amount_date_entries = this->hads_from_environment(environment);
		model->organisation_contacts = this->contacts_from_environment(environment);
		model->employee_birth_ids = this->employee_birth_ids_from_environment(environment);
		model->sru = this->srus_from_environment(environment);

    // TODO 240216 #SIE: Consider a way to ensure the tagged amounts are in sync with the actual SIE file contents.
    // Note that we read the SIE files before reading in the existing tagged amounts.
    // This is important for how we should implement a way to ensure the SIE file content is the map of the world we MUST use!
    // Also note that the all_date_ordered_tagged_amounts type DateOrderedTaggedAmountsContainer has an overloaded operator+= that ensures no duplicates are added.
    // So given we first read the SIE file with the values to keep, we should (?) be safe to modify the "equals" operator to ensure special treatment for SIE aggregates and its members?
    /*
    240218 Consider to implement "last in wins" for SIE aggregates and its members?

    1) Read the SIE-files last to ensure the tagged amounts are in sync with the actual SIE file contents.
    2) Make inserting of SIE aggregates into tagged amounts detect SIE entries with the same series and sequence number within the same fiscal year.
       For any pre-existing SIE aggregate in tagged amounts -> delete the aggregate and its mebers before inserting the new SIE aggregate.
       
    */
    if (false) {
      // TODO 240219 - switch to this new implementation
      // 1) Read in tagged amounts from persistent storage
      model->all_date_ordered_tagged_amounts += this->date_ordered_tagged_amounts_from_environment(environment);
      // 2) Synchronize SIE tagged amounts with external SIE files (any edits and changes made externally)
      for (auto const& [key,sie_environment] : model->sie) {
        this->synchronize_tagged_amounts_with_sie(model->all_date_ordered_tagged_amounts,sie_environment);
      }
    }
    else {
      // TODO 240219 - Replace this old implementation with the new one above
      for (auto const& sie_environments_entry : model->sie) {
        model->all_date_ordered_tagged_amounts += this->date_ordered_tagged_amounts_from_sie_environment(sie_environments_entry.second);	
      }
      prompt << "\nDESIGN_UNSUFFICIENCY - No proper synchronization of tagged amounts with SIE files yet in place (dublicate SIE entries may remain in tagged amounts)";
      model->all_date_ordered_tagged_amounts += this->date_ordered_tagged_amounts_from_environment(environment);
    }

    // TODO: 240216: Is skv_specs_mapping_from_csv_files still of intereset to use for something?
    auto dummy = this->skv_specs_mapping_from_csv_files(environment);

		model->prompt = prompt.str();
		return model;
	}
	Environment environment_from_model(Model const& model) {
		Environment result{};
		auto tagged_amount_to_environment = [&result](TaggedAmount const& ta) {
      // TODO 240524 - Attend to this code when final implemenation of tagged amounts <--> SIE entries are in place
      //               Problem for now is that syncing between tagged amounts and SIE entries is flawed and insufficient (and also error prone)
      if (true) {
        if (ta.tag_value("BAS") or ta.tag_value("SIE")) {
          // std::cout << "\nDESIGN INSUFFICIENCY: No persistent storage yet for SIE or BAS TA:" << ta;
          return; // discard any tagged amounts relating to SIE entries (no persistent storage yet for these)
        }
      }
			result.insert({"TaggedAmount",to_environment_value(ta)});
		};
		model->all_date_ordered_tagged_amounts.for_each(tagged_amount_to_environment);
		for (auto const& entry :  model->heading_amount_date_entries) {
			result.insert({"HeadingAmountDateTransEntry",to_environment_value(entry)});
		}
		std::string sev = std::accumulate(model->sie_file_path.begin(),model->sie_file_path.end(),std::string{},[](auto acc,auto const& entry){
			std::ostringstream os{};
			if (acc.size()>0) os << acc << ";";
			os << entry.first << "=" << entry.second.string();
			return os.str();
		});
		// sev += std::string{";"} + "path" + "=" + model->sie_file_path["current"].string();
		result.insert({"sie_file",to_environment_value(sev)});
		for (auto const& entry : model->organisation_contacts) {
			result.insert({"contact",to_environment_value(entry)});
		}
		for (auto const& entry : model->employee_birth_ids) {
			result.insert({"employee",to_environment_value(std::string{"birth-id="} + entry)});
		}
		for (auto const& [year_id,sru_env] : model->sru) {
			std::ostringstream os{};
			OEnvironmentValueOStream en_val_os{os};
			en_val_os << "year_id=" << year_id << sru_env;
			result.insert({"SRU:S",to_environment_value(os.str())});
		}
		return result;
	}
	Environment add_cratchit_environment(Environment const& environment) {
		Environment result{environment};
		// Add cratchit environment values if/when there are any
		return result;
	}

	// HeadingAmountDateTransEntry "belopp=1389,50;datum=20221023;rubrik=Klarna"
	Environment environment_from_file(std::filesystem::path const& p) {
		Environment result{};
		try {
			std::ifstream in{p};
			std::string line{};
			while (std::getline(in,line)) {
				if (is_value_line(line)) {
					std::istringstream in{line};
					std::string key{},value{};
					in >> key >> std::quoted(value);
					result.insert({key,to_environment_value(value)});
				}
			}
		}
		catch (std::runtime_error const& e) {
			std::cout << "\nERROR - Read from " << p << " failed. Exception:" << e.what();
		}
		return result;
	}
	void environment_to_file(Environment const& environment, std::filesystem::path const& p) {
		try {
			std::ofstream out{p};		
			for (auto iter = environment.begin(); iter != environment.end();++iter) {
				if (iter == environment.begin()) out << to_string(*iter);
				else out << "\n" << to_string(*iter);
			}
		}
		catch (std::runtime_error const& e) {
			std::cout << "\nERROR - Write to " << p << " failed. Exception:" << e.what();
		}
	}
};

class REPL {
public:
    REPL(std::filesystem::path const& environment_file_path) : cratchit{environment_file_path} {}

	void run(Command const& command) {
    auto model = cratchit.init(command);
		auto ux = cratchit.view(model);
		render_ux(ux);
		while (true) {
			try {
				// process events (user input)
				if (in.size()>0) {
					auto msg = in.front();
					in.pop();
					// std::cout << "\nmsg[" << msg.index() << "]";
					// Turn Event (Msg) into updated model
					// std::cout << "\nREPL::run before cratchit.update " << std::flush;
					auto [updated_model,cmd] = cratchit.update(msg,std::move(model));
					// std::cout << "\nREPL::run cratchit.update ok" << std::flush;
					model = std::move(updated_model);
					// std::cout << "\nREPL::run model moved ok" << std::flush;
					if (cmd.msg) in.push(*cmd.msg);
					// std::cout << "\nREPL::run before view" << std::flush;
					auto ux = cratchit.view(model);
					// std::cout << "\nREPL::run before render" << std::flush;
					render_ux(ux);
					if (model->quit) break; // Done
				}
				else {
					// Get more buffered user input
					std::string user_input{};
					std::getline(std::cin,user_input);
					auto cmd = to_cmd(user_input);
					if (cmd.msg) this->in.push(*cmd.msg);
				}
			}
			catch (std::exception& e) {
				std::cout << "\nERROR: run() loop failed, exception=" << e.what() << std::flush;
			}
		}
		// std::cout << "\nREPL::run exit" << std::flush;
	}
private:
    Cratchit cratchit;
		void render_ux(Ux const& ux) {
			for (auto const&  row : ux) {
				if (row.size()>0) std::cout << row;
			}
		}
    std::queue<Msg> in{};
};

// void test_directory_iterator() {
//   // Code from https://en.cppreference.com/w/cpp/filesystem/directory_iterator 
//     const std::filesystem::path sandbox{"sandbox"};
//     std::filesystem::create_directories(sandbox/"dir1"/"dir2");
//     std::ofstream{sandbox/"file1.txt"};
//     std::ofstream{sandbox/"file2.txt"};
 
//     std::cout << "directory_iterator:\n";
//     // directory_iterator can be iterated using a range-for loop
//     for (auto const& dir_entry : std::filesystem::directory_iterator{sandbox}) {}
// }

int main(int argc, char *argv[])
{
  // Test ImmutableFileManager
  if (false) {
    test_immutable_file_manager();
  }
  if (false) {
    // test_directory_iterator();
    // exit(0);
  }
	if (true) {
		// Log current locale and test charachter encoding.
		// TODO: Activate to adjust for cross platform handling 
			std::cout << "\nDeafult (current) locale setting is " << std::locale().name().c_str();
			std::string sHello{"Hallå Åland! Ömsom ödmjuk. Ärligt äkta."}; // This source file expected to be in UTF-8
      std::cout << "\nUTF-8 sHello:" << std::quoted(sHello);
			// std::string sPATH{std::getenv("PATH")};
      // std::cout << "\nPATH=" << sPATH;
	}
  
  std::signal(SIGWINCH, handle_winch); // We need a signal handler to not confuse std::cin on console window resize
	std::string command{};
	for (int i=1;i < argc;i++) command+= std::string{argv[i]} + " ";
	auto current_path = std::filesystem::current_path();
	auto environment_file_path = current_path / "cratchit.env";
	REPL repl{environment_file_path};
	repl.run(command);
	// std::cout << "\nBye for now :)";
// std::cout << std::endl;
	return 0;
}

namespace SKV { // SKV
	namespace XML { // SKV::XML
		namespace VATReturns { // SKV::XML::VATReturns
// Mapping between BAS Account numbers and VAT Returns form designation codes (SRU Codes as "bonus")
char const* ACCOUNT_VAT_CSV = R"(KONTO;BENÄMNING;MOMSKOD (MOMSRUTA);SRU
3305;Försäljning tjänster till land utanför EU;ÖTEU (40);7410
3521;Fakturerade frakter, EU-land;VTEU (35);7410
3108;Försäljning varor till annat EU-land, momsfri;VTEU (35);7410
2634;Utgående moms omvänd skattskyldighet, 6 %;UTFU3 (32);7369
2624;Utgående moms omvänd skattskyldighet, 12 %;UTFU2 (31);7369
2614;Utgående moms omvänd skattskyldighet, 25 %;UOS1 (30);7369
2635;Utgående moms import av varor, 6 %;UI6 (62);7369
2615;Utgående moms import av varor, 25 %;UI25 (60);7369
2625;Utgående moms import av varor, 12 %;UI12 (61);7369
2636;Utgående moms VMB 6 %;U3 (12);7369
2631;Utgående moms på försäljning inom Sverige, 6 %;U3 (12);7369
2630;Utgående moms, 6 %;U3 (12);7369
2633;Utgående moms för uthyrning, 6 %;U3 (12);7369
2632;Utgående moms på egna uttag, 6 %;U3 (12);7369
2626;Utgående moms VMB 12 %;U2 (11);7369
2622;Utgående moms på egna uttag, 12 %;U2 (11);7369
2621;Utgående moms på försäljning inom Sverige, 12 %;U2 (11);7369
2620;Utgående moms, 12 %;U2 (11);7369
2623;Utgående moms för uthyrning, 12 %;U2 (11);7369
2612;Utgående moms på egna uttag, 25 %;U1 (10);7369
2610;Utgående moms, 25 %;U1 (10);7369
2611;Utgående moms på försäljning inom Sverige, 25 %;U1 (10);7369
2616;Utgående moms VMB 25 %;U1 (10);7369
2613;Utgående moms för uthyrning, 25 %;U1 (10);7369
2650;Redovisningskonto för moms;R2 (49);7369
1650;Momsfordran;R1 (49);7261
3231;Försäljning inom byggsektorn, omvänd skattskyldighet moms;OTTU (41);7410
3003;Försäljning inom Sverige, 6 % moms;MP3 (05);7410
3403;Egna uttag momspliktiga, 6 %;MP3 (05);7410
3402;Egna uttag momspliktiga, 12 %;MP2 (05);7410
3002;Försäljning inom Sverige, 12 % moms;MP2 (05);7410
3401;Egna uttag momspliktiga, 25 %;MP1 (05);7410
3510;Fakturerat emballage;MP1 (05);7410
3600;Rörelsens sidointäkter (gruppkonto);MP1 (05);7410
3530;Fakturerade tull- och speditionskostnader m.m.;MP1 (05);7410
3520;Fakturerade frakter;MP1 (05);7410
3001;Försäljning inom Sverige, 25 % moms;MP1 (05);7410
3540;Faktureringsavgifter;MP1 (05);7410
3106;Försäljning varor till annat EU-land, momspliktig;MP1 (05);7410
3990;Övriga ersättningar och intäkter;MF (42);7413
3404;Egna uttag, momsfria;MF (42);7410
3004;Försäljning inom Sverige, momsfri;MF (42);7410
3980;Erhållna offentliga stöd m.m.;MF (42);7413
4516;Inköp av varor från annat EU-land, 12 %;IVEU (20);7512
4515;Inköp av varor från annat EU-land, 25 %;IVEU (20);7512
9021;Varuvärde Inlöp annat EG-land (Momsrapport ruta 20);IVEU (20);
4517;Inköp av varor från annat EU-land, 6 %;IVEU (20);7512
4415;Inköpta varor i Sverige, omvänd skattskyldighet, 25 % moms;IV (23);7512
4531;Inköp av tjänster från ett land utanför EU, 25 % moms;ITGLOB (22);7512
4532;Inköp av tjänster från ett land utanför EU, 12 % moms;ITGLOB (22);7512
4533;Inköp av tjänster från ett land utanför EU, 6 % moms;ITGLOB (22);7512
4537;Inköp av tjänster från annat EU-land, 6 %;ITEU (21);7512
4536;Inköp av tjänster från annat EU-land, 12 %;ITEU (21);7512
4535;Inköp av tjänster från annat EU-land, 25 %;ITEU (21);7512
4427;Inköpta tjänster i Sverige, omvänd skattskyldighet, 6 %;IT (24);7512
4426;Inköpta tjänster i Sverige, omvänd skattskyldighet, 12 %;IT (24);7512
2642;Debiterad ingående moms i anslutning till frivillig skattskyldighet;I (48);7369
2640;Ingående moms;I (48);7369
2647;Ingående moms omvänd skattskyldighet varor och tjänster i Sverige;I (48);7369
2641;Debiterad ingående moms;I (48);7369
2649;Ingående moms, blandad verksamhet;I (48);7369
2646;Ingående moms på uthyrning;I (48);7369
2645;Beräknad ingående moms på förvärv från utlandet;I (48);7369
3913;Frivilligt momspliktiga hyresintäkter;HFS (08);7413
3541;Faktureringsavgifter, EU-land;FTEU (39);7410
3308;Försäljning tjänster till annat EU-land;FTEU (39);7410
3542;Faktureringsavgifter, export;E (36);7410
3522;Fakturerade frakter, export;E (36);7410
3105;Försäljning varor till land utanför EU;E (36);7410
3211;Försäljning positiv VMB 25 %;BVMB (07);7410
3212;Försäljning negativ VMB 25 %;BVMB (07);7410
9022;Beskattningsunderlag vid import (Momsrapport Ruta 50);BI (50);
4545;Import av varor, 25 % moms;BI (50);7512
4546;Import av varor, 12 % moms;BI (50);7512
4547;Import av varor, 6 % moms;BI (50);7512
3740;Öres- och kronutjämning;A;7410)";
		} // namespace VATReturns
	} // namespace XML
} // namespace SKV 

namespace charset {
	namespace CP437 {

		// from http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP437.TXT
		std::map<char,char16_t> cp437ToUnicodeMap{
		{0x0,0x0}
		,{0x1,0x1}
		,{0x2,0x2}
		,{0x3,0x3}
		,{0x4,0x4}
		,{0x5,0x5}
		,{0x6,0x6}
		,{0x7,0x7}
		,{0x8,0x8}
		,{0x9,0x9}
		,{0xA,0xA}
		,{0xB,0xB}
		,{0xC,0xC}
		,{0xD,0xD}
		,{0xE,0xE}
		,{0xF,0xF}
		,{0x10,0x10}
		,{0x11,0x11}
		,{0x12,0x12}
		,{0x13,0x13}
		,{0x14,0x14}
		,{0x15,0x15}
		,{0x16,0x16}
		,{0x17,0x17}
		,{0x18,0x18}
		,{0x19,0x19}
		,{0x1A,0x1A}
		,{0x1B,0x1B}
		,{0x1C,0x1C}
		,{0x1D,0x1D}
		,{0x1E,0x1E}
		,{0x1F,0x1F}
		,{0x20,0x20}
		,{0x21,0x21}
		,{0x22,0x22}
		,{0x23,0x23}
		,{0x24,0x24}
		,{0x25,0x25}
		,{0x26,0x26}
		,{0x27,0x27}
		,{0x28,0x28}
		,{0x29,0x29}
		,{0x2A,0x2A}
		,{0x2B,0x2B}
		,{0x2C,0x2C}
		,{0x2D,0x2D}
		,{0x2E,0x2E}
		,{0x2F,0x2F}
		,{0x30,0x30}
		,{0x31,0x31}
		,{0x32,0x32}
		,{0x33,0x33}
		,{0x34,0x34}
		,{0x35,0x35}
		,{0x36,0x36}
		,{0x37,0x37}
		,{0x38,0x38}
		,{0x39,0x39}
		,{0x3A,0x3A}
		,{0x3B,0x3B}
		,{0x3C,0x3C}
		,{0x3D,0x3D}
		,{0x3E,0x3E}
		,{0x3F,0x3F}
		,{0x40,0x40}
		,{0x41,0x41}
		,{0x42,0x42}
		,{0x43,0x43}
		,{0x44,0x44}
		,{0x45,0x45}
		,{0x46,0x46}
		,{0x47,0x47}
		,{0x48,0x48}
		,{0x49,0x49}
		,{0x4A,0x4A}
		,{0x4B,0x4B}
		,{0x4C,0x4C}
		,{0x4D,0x4D}
		,{0x4E,0x4E}
		,{0x4F,0x4F}
		,{0x50,0x50}
		,{0x51,0x51}
		,{0x52,0x52}
		,{0x53,0x53}
		,{0x54,0x54}
		,{0x55,0x55}
		,{0x56,0x56}
		,{0x57,0x57}
		,{0x58,0x58}
		,{0x59,0x59}
		,{0x5A,0x5A}
		,{0x5B,0x5B}
		,{0x5C,0x5C}
		,{0x5D,0x5D}
		,{0x5E,0x5E}
		,{0x5F,0x5F}
		,{0x60,0x60}
		,{0x61,0x61}
		,{0x62,0x62}
		,{0x63,0x63}
		,{0x64,0x64}
		,{0x65,0x65}
		,{0x66,0x66}
		,{0x67,0x67}
		,{0x68,0x68}
		,{0x69,0x69}
		,{0x6A,0x6A}
		,{0x6B,0x6B}
		,{0x6C,0x6C}
		,{0x6D,0x6D}
		,{0x6E,0x6E}
		,{0x6F,0x6F}
		,{0x70,0x70}
		,{0x71,0x71}
		,{0x72,0x72}
		,{0x73,0x73}
		,{0x74,0x74}
		,{0x75,0x75}
		,{0x76,0x76}
		,{0x77,0x77}
		,{0x78,0x78}
		,{0x79,0x79}
		,{0x7A,0x7A}
		,{0x7B,0x7B}
		,{0x7C,0x7C}
		,{0x7D,0x7D}
		,{0x7E,0x7E}
		,{0x7F,0x7F}
		,{0x80,0xC7}
		,{0x81,0xFC}
		,{0x82,0xE9}
		,{0x83,0xE2}
		,{0x84,0xE4}
		,{0x85,0xE0}
		,{0x86,0xE5}
		,{0x87,0xE7}
		,{0x88,0xEA}
		,{0x89,0xEB}
		,{0x8A,0xE8}
		,{0x8B,0xEF}
		,{0x8C,0xEE}
		,{0x8D,0xEC}
		,{0x8E,0xC4}
		,{0x8F,0xC5}
		,{0x90,0xC9}
		,{0x91,0xE6}
		,{0x92,0xC6}
		,{0x93,0xF4}
		,{0x94,0xF6}
		,{0x95,0xF2}
		,{0x96,0xFB}
		,{0x97,0xF9}
		,{0x98,0xFF}
		,{0x99,0xD6}
		,{0x9A,0xDC}
		,{0x9B,0xA2}
		,{0x9C,0xA3}
		,{0x9D,0xA5}
		,{0x9E,0x20A7}
		,{0x9F,0x192}
		,{0xA0,0xE1}
		,{0xA1,0xED}
		,{0xA2,0xF3}
		,{0xA3,0xFA}
		,{0xA4,0xF1}
		,{0xA5,0xD1}
		,{0xA6,0xAA}
		,{0xA7,0xBA}
		,{0xA8,0xBF}
		,{0xA9,0x2310}
		,{0xAA,0xAC}
		,{0xAB,0xBD}
		,{0xAC,0xBC}
		,{0xAD,0xA1}
		,{0xAE,0xAB}
		,{0xAF,0xBB}
		,{0xB0,0x2591}
		,{0xB1,0x2592}
		,{0xB2,0x2593}
		,{0xB3,0x2502}
		,{0xB4,0x2524}
		,{0xB5,0x2561}
		,{0xB6,0x2562}
		,{0xB7,0x2556}
		,{0xB8,0x2555}
		,{0xB9,0x2563}
		,{0xBA,0x2551}
		,{0xBB,0x2557}
		,{0xBC,0x255D}
		,{0xBD,0x255C}
		,{0xBE,0x255B}
		,{0xBF,0x2510}
		,{0xC0,0x2514}
		,{0xC1,0x2534}
		,{0xC2,0x252C}
		,{0xC3,0x251C}
		,{0xC4,0x2500}
		,{0xC5,0x253C}
		,{0xC6,0x255E}
		,{0xC7,0x255F}
		,{0xC8,0x255A}
		,{0xC9,0x2554}
		,{0xCA,0x2569}
		,{0xCB,0x2566}
		,{0xCC,0x2560}
		,{0xCD,0x2550}
		,{0xCE,0x256C}
		,{0xCF,0x2567}
		,{0xD0,0x2568}
		,{0xD1,0x2564}
		,{0xD2,0x2565}
		,{0xD3,0x2559}
		,{0xD4,0x2558}
		,{0xD5,0x2552}
		,{0xD6,0x2553}
		,{0xD7,0x256B}
		,{0xD8,0x256A}
		,{0xD9,0x2518}
		,{0xDA,0x250C}
		,{0xDB,0x2588}
		,{0xDC,0x2584}
		,{0xDD,0x258C}
		,{0xDE,0x2590}
		,{0xDF,0x2580}
		,{0xE0,0x3B1}
		,{0xE1,0xDF}
		,{0xE2,0x393}
		,{0xE3,0x3C0}
		,{0xE4,0x3A3}
		,{0xE5,0x3C3}
		,{0xE6,0xB5}
		,{0xE7,0x3C4}
		,{0xE8,0x3A6}
		,{0xE9,0x398}
		,{0xEA,0x3A9}
		,{0xEB,0x3B4}
		,{0xEC,0x221E}
		,{0xED,0x3C6}
		,{0xEE,0x3B5}
		,{0xEF,0x2229}
		,{0xF0,0x2261}
		,{0xF1,0xB1}
		,{0xF2,0x2265}
		,{0xF3,0x2264}
		,{0xF4,0x2320}
		,{0xF5,0x2321}
		,{0xF6,0xF7}
		,{0xF7,0x2248}
		,{0xF8,0xB0}
		,{0xF9,0x2219}
		,{0xFA,0xB7}
		,{0xFB,0x221A}
		,{0xFC,0x207F}
		,{0xFD,0xB2}
		,{0xFE,0x25A0}
		,{0xFF,0xA0}
		};
	} // namespace CP437
}

namespace SKV {
	namespace XML {

		namespace TAXReturns {
			SKV::XML::XMLMap tax_returns_template{
			{R"(Skatteverket^agd:Avsandare^agd:Programnamn)",R"(Programmakarna AB)"}
			,{R"(Skatteverket^agd:Avsandare^agd:Organisationsnummer)",R"(190002039006)"}
			,{R"(Skatteverket^agd:Avsandare^agd:TekniskKontaktperson^agd:Namn)",R"(Valle Vadman)"}
			,{R"(Skatteverket^agd:Avsandare^agd:TekniskKontaktperson^agd:Telefon)",R"(23-2-4-244454)"}
			,{R"(Skatteverket^agd:Avsandare^agd:TekniskKontaktperson^agd:Epostadress)",R"(valle.vadman@programmakarna.se)"}
			,{R"(Skatteverket^agd:Avsandare^agd:Skapad)",R"(2021-01-30T07:42:25)"}

			,{R"(Skatteverket^agd:Blankettgemensamt^agd:Arbetsgivare^agd:AgRegistreradId)",R"(165560269986)"}
			,{R"(Skatteverket^agd:Blankettgemensamt^agd:Arbetsgivare^agd:Kontaktperson^agd:Namn)",R"(Ville Vessla)"}
			,{R"(Skatteverket^agd:Blankettgemensamt^agd:Arbetsgivare^agd:Kontaktperson^agd:Telefon)",R"(555-244454)"}
			,{R"(Skatteverket^agd:Blankettgemensamt^agd:Arbetsgivare^agd:Kontaktperson^agd:Epostadress)",R"(ville.vessla@foretaget.se)"}

			,{R"(Skatteverket^agd:Kontaktperson^agd:Arendeinformation^agd:Arendeagare)",R"(165560269986)"}
			,{R"(Skatteverket^agd:Kontaktperson^agd:Arendeinformation^agd:Period)",R"(202101)"}

			,{R"(Skatteverket^agd:Kontaktperson^agd:Blankettinnehall^agd:HU^agd:ArbetsgivareHUGROUP^agd:AgRegistreradId faltkod="201")",R"(16556026998)"}
			,{R"(Skatteverket^agd:Kontaktperson^agd:Blankettinnehall^agd:HU^agd:RedovisningsPeriod faltkod="006")",R"(202101)"}
			,{R"(Skatteverket^agd:Kontaktperson^agd:Blankettinnehall^agd:HU^agd:SummaArbAvgSlf faltkod="487")",R"(0)"}
			,{R"(Skatteverket^agd:Kontaktperson^agd:Blankettinnehall^agd:HU^agd:SummaSkatteavdr faltkod="497")",R"(0)"}

			,{R"(Skatteverket^agd:Blankett^agd:Arendeinformation^agd:Arendeagare)",R"(165560269986)"}
			,{R"(Skatteverket^agd:Blankett^agd:Arendeinformation^agd:Period)",R"(202101)"}

			,{R"(Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:ArbetsgivareIUGROUP^agd:AgRegistreradId faltkod="201"))",R"(165560269986)"}
			,{R"(Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:BetalningsmottagareIUGROUP^agd:BetalningsmottagareIDChoice^agd:BetalningsmottagarId faltkod="215")",R"(198202252386)"}
			,{R"(Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:RedovisningsPeriod faltkod="006")",R"(202101)"}
			,{R"(Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:Specifikationsnummer faltkod="570")",R"(001)"}
			,{R"(Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:AvdrPrelSkatt faltkod="001")",R"(0)"}
			};
		} // namespace TAXReturns
	} // namespace XML

	namespace SRU {
		namespace INK1 {
      // csv-format of file "INK1_SKV2000-32-02-0022-02_SKV269.xls" in zip file "_Nyheter_from_beskattningsperiod_2022P4-3.zip" from web-location https://skatteverket.se/download/18.48cfd212185efbb440b65a8/1679495970044/_Nyheter_from_beskattningsperiod_2022P4-3.zip
      // Also see project folder "cratchit/skv_specs".
			const char* ink1_csv_to_sru_template = R"(Fältnamn på INK1_SKV2000-32-02-0022-02;;;;;
;;;;;
;;;;;
Attribut;Fältnamn;Datatyp;Obl.;*/+/-;Regel
Framställningsdatum;DatFramst;Datum_A;J;;
Framställningstid;TidFramst;Tid_A;J;;
Fältkodsnummer;FältKod;Numeriskt_E;N;;
Intern information för framställande program/system;SystemInfo;Str_250;N;;
Korrekt person-/samordningsnummer eller organisationsnummer som inleds med 161 (dödsbo) eller 163 (gemensamma distriktet);PersOrgNr;Orgnr_Id_PD;J;;
1.1 Lön, förmåner, sjukpenning m.m.;1000;Numeriskt_10;N;*;
1.2 Kostnadsersättningar;1001;Numeriskt_10;N;*;
1.3 Allmän pension och tjänstepension m.m.;1002;Numeriskt_10;N;*;
1.4 Privat pension och livränta;1003;Numeriskt_10;N;*;
1.5 Andra inkomster som inte är pensionsgrundande;1004;Numeriskt_10;N;*;
1.6 Inkomster, t.ex. hobby som du själv ska betala egenavgifter för;1005;Numeriskt_10;N;*;
1.7 Inkomst enligt bilaga K10, K10A och K13;1006;Numeriskt_10;N;*;
2.1 Resor till och från arbetet;1070;Numeriskt_10;N;*;
2.2 Tjänsteresor;1071;Numeriskt_10;N;*;
2.3 Tillfälligt arbete, dubbel bosättning och hemresor;1072;Numeriskt_10;N;*;
2.4 Övriga utgifter;1073;Numeriskt_10;N;*;
3.1 Socialförsäkringsavgifter enligt EU-förordning m.m.;1501;Numeriskt_10;N;*;
4.1 Rotarbete;1583;Numeriskt_10;N;*;
4.2 Rutarbete;1584;Numeriskt_10;N;*;
4.3 Förnybar el;1582;Numeriskt_10;N;*;
4.4 Gåva;1581;Numeriskt_10;N;*;
4.5 Installation av grön teknik;1585;Numeriskt_10;N;*;
5.1 Småhus/ägarlägenhet  0,75 %;80;Numeriskt_B;N;*;
6.1 Småhus/ägarlägenhet: tomtmark, byggnad under uppförande 1,0 %;84;Numeriskt_B;N;*;
7.1 Schablonintäkter;1106;Numeriskt_10;N;*;
7.2 Ränteinkomster, utdelningar m.m. Vinst enligt bilaga K4 avsnitt C m.m.;1100;Numeriskt_10;N;*;
7.3 Överskott vid uthyrning av privatbostad;1101;Numeriskt_10;N;*;
"7.4 Vinst fondandelar m.m. Vinst från bilaga K4 avsnitt A och B, K9 avsnitt B, 
K10, K10A, K11, K12 avsnitt B och K13.";1102;Numeriskt_10;N;*;
7.5 Vinst ej marknadsnoterade fondandelar m.m. Vinst från bilaga K4 avsnitt D, K9 avsnitt B, K12 avsnitt C och K15A/B m.m.;1103;Numeriskt_10;N;*;
7.6 Vinst från bilaga K5 och K6. Återfört uppskov från bilaga K2;1104;Numeriskt_10;N;*;
7.7 Vinst från bilaga K7 och K8;1105;Numeriskt_10;N;*;
8.1 Ränteutgifter m.m. Förlust enligt bilaga K4 avsnitt C m.m.;1170;Numeriskt_10;N;*;
"8.3 Förlust fondandelar m.m. Förlust från bilaga K4 avsnitt A, K9 avsnitt B, K10,
K12 avsnitt B och K13.";1172;Numeriskt_10;N;*;
"8.4 Förlust ej marknadsnoterade fondandelar. Förlust från bilaga K4 avsnitt D, K9 avsnitt B,  K10A,
K12 avsnitt Coch K15A/B.";1173;Numeriskt_10;N;*;
8.5 Förlust från bilaga K5 och K6;1174;Numeriskt_10;N;*;
8.6 Förlust enligt bilaga K7 och K8;1175;Numeriskt_10;N;*;
8.7 Investeraravdrag från bilaga K11;1176;Numeriskt_10;N;*;
9.1 Skatteunderlag för kapitalförsäkring;1550;Numeriskt_10;N;*;
9.2 Skatteunderlag för pensionsförsäkring;1551;Numeriskt_10;N;*;
10.1 Överskott av aktiv näringsverksamhet enskild;1200;Numeriskt_10;N;*;
10.1 Överskott av aktiv näringsverksamhet handelsbolag;1201;Numeriskt_10;N;*;
10.2 Underskott av aktiv näringsverksamhet enskild;1202;Numeriskt_10;N;*;
10.2 Underskott av aktiv näringsverksamhet handelsbolag;1203;Numeriskt_10;N;*;
10.3 Överskott av passiv näringsverksamhet enskild;1250;Numeriskt_10;N;*;
10.3 Överskott av passiv näringsverksamhet handelsbolag;1251;Numeriskt_10;N;*;
10.4 Underskott av passiv näringsverksamhet enskild;1252;Numeriskt_10;N;*;
10.4 Underskott av passiv näringsverksamhet handelsbolag;1253;Numeriskt_10;N;*;
10.5 Inkomster för vilka uppdragsgivare ska betala socialavgifter Brutto;1400;Numeriskt_10;N;*;
10.5 Inkomster för vilka uppdragsgivare ska betala socialavgifter Kostnader;1401;Numeriskt_10;N;*;
10.6 Underlag för särskild löneskatt på pensionskostnader eget;1301;Numeriskt_10;N;*;
10.6 Underlag för särskild löneskatt på pensionskostnader anställdas;1300;Numeriskt_10;N;*;
10.7 Underlag för avkastningsskatt på pensionskostnader;1305;Numeriskt_10;N;*;
11.1 Positiv räntefördelning;1570;Numeriskt_10;N;*;
11.2 Negativ räntefördelning;1571;Numeriskt_10;N;*;
12.1 Underlag för expansionsfondsskatt ökning;1310;Numeriskt_10;N;*;
12.2 Underlag för expansionsfondsskatt minskning;1311;Numeriskt_10;N;*;
13.1 Regionalt nedsättningsbelopp, endast näringsverksamhet i stödområde;1411;Numeriskt_10;N;*;
14.1 Allmänna avdrag underskott näringsverksamhet;1510;Numeriskt_10;N;*;
15.1 Hyreshus: bostäder 0,3 %;93;Numeriskt_B;N;*;
16.1 Hyreshus: tomtmark, bostäder under uppförande 0,4 %;86;Numeriskt_B;N;*;
16.2 Hyreshus: lokaler 1,0 %;95;Numeriskt_B;N;*;
16.3 Industri och elproduktionsenhet, värmekraftverk 0,5 %;96;Numeriskt_B;N;*;
16.4 Elproduktionsenhet: vattenkraftverk 0,5 %;97;Numeriskt_B;N;*;
16.5 Elproduktionsenhet: vindkraftverk 0,2 %;98;Numeriskt_B;N;*;
17.1 Inventarieinköp under 2021;1586;Numeriskt_B;N;*;
Kryssa här om din näringsverksamhet har upphört;92;Str_X;N;;
Kryssa här om du begär omfördelning av skattereduktion för rot/-rutavdrag eller installation av grön teknik.;8052;Str_X;N;;
Kryssa här om någon inkomstuppgift är felaktig/saknas;8051;Str_X;N;;
Kryssa här om du har haft inkomst från utlandet;8055;Str_X;N;;
Kryssa här om du begär avräkning av utländsk skatt.;8056;Str_X;N;;
Övrigt;8090;Str_1000;N;;

;;;;
;Dokumenthistorik;;;
;Datum;Version;Beskrivning;Signatur
;;;;
;;;;
;;;;
;;;;
;;;;
;Referenser;;;
;"Definition av format återfinns i SKV 269 ""Teknisk beskrivning Näringsuppgifter(SRU) Anstånd Tjänst Kapital""";;;)";
      // csv-format of file "K10_SKV2110-35-04-22-01.xls" in zip file "_Nyheter_from_beskattningsperiod_2022P4-3.zip" from web-location https://skatteverket.se/download/18.48cfd212185efbb440b65a8/1679495970044/_Nyheter_from_beskattningsperiod_2022P4-3.zip
      // Also see project folder "cratchit/skv_specs".
			const char* k10_csv_to_sru_template = R"(Fältnamn på K10_SKV2110-35-04-22-01;;;;;
;;;;;
;;;;;
Attribut;Fältnamn;Datatyp;Obl.;*/+/-;Regel
Framställningsdatum;DatFramst;Datum_A;J;;
Framställningstid;TidFramst;Tid_A;J;;
Fältkodsnummer;FältKod;Numeriskt_E;N;;
Intern information för framställande program/system;SystemInfo;Str_250;N;;
Korrekt person-/samordningsnummer eller organisationsnummer som inleds med 161 (dödsbo) eller 163 (gemensamma distriktet);PersOrgNr;Orgnr_Id_PD;J;;
Uppgiftslämnarens namn;Namn;Str_250;N;;
Företagets organisationsnummer;4530;Orgnr_Id_O;J;;Ingen kontroll av värdet sker om 7050 är ifyllt
Antal ägda andelar vid årets ingång;4531;Numeriskt_B;N;;
Totala antalet andelar i hela företaget vid årets ingång;4532;Numeriskt_B;N;;
Bolaget är ett utländskt bolag;7050;Str_X;N;;
1.1 Årets gränsbelopp enligt förenklingsregeln;4511;Numeriskt_B;N;+;Regel_E
1.2 Sparat utdelningsutrymme från föregående år x 103,23 %;4501;Numeriskt_B;N;+;Regel_E
1.3 Gränsbelopp enligt förenklingsregeln;4502;Numeriskt_B;N;*;Regel_E
1.4 Vid avyttring eller gåva innan utdelningstillfället...;4720;Numeriskt_B;N;-;Regel_E
1.5 Gränsbelopp att ytnyttja vid  p. 1.7 nedan;4503;Numeriskt_B;N;*;Regel_E
1.6 Utdelning;4504;Numeriskt_B;N;+;Regel_E
1.7 Gränsbeloppet enligt p. 1.5 ovan;4721;Numeriskt_B;N;-;Regel_E
1.8 Utdelning som beskattas i tjänst;4505;Numeriskt_B;N;+;Regel_E
1.9 Sparat utdelningsutrymme;4722;Numeriskt_B;N;-;Regel_E
1.10 Vid delavyttring eller gåva efter utdelningstillfället : sparat utdelningsutrymme som är hänförligt till de överlåtna andelarna enligt p. 3.7a;4723;Numeriskt_B;N;-;Regel_E
1.11 Sparat utdelningsutrymme till nästa år;4724;Numeriskt_B;N;*;Regel_E
1.12 Utdelning;4506;Numeriskt_B;N;+;Regel_E
1.13 Belopp som beskattas i tjänst enligt p. 1.8 ovan;4725;Numeriskt_B;N;-;Regel_E
1.14 Utdelning i kapital;4507;Numeriskt_B;N;*;Regel_E
1.15 Beloppet i p. 1.5 x 2/3;4508;Numeriskt_B;N;+;Regel_E
1.16 Resterande utdelning;4509;Numeriskt_B;N;+;Regel_E
1.17 Utdelning som ska tas upp i kapital;4515;Numeriskt_B;N;*;Regel_E
2.1 Omkostnadsbelopp vid årets ingång (alternativt omräknat omkostnadsbelopp) x 9,23 %;4520;Numeriskt_B;N;+;Regel_F
2.2 Lönebaserat utrymme enligt avsnitt D p. 6.11;4521;Numeriskt_B;N;+;Regel_F
2.3 Sparat utdelningsutrymme från föregående år x 103,23 %;4522;Numeriskt_B;N;+;Regel_F
2.4 Gränsbelopp enligt huvudregeln;4523;Numeriskt_B;N;*;Regel_F
2.5 Vid avyttring eller gåva..innanutdelningstillfället...;4730;Numeriskt_B;N;-;Regel_F
2.6 Gränsbelopp att utnyttja vid punkt 2.8 nedan;4524;Numeriskt_B;N;*;Regel_F
2.7 Utdelning;4525;Numeriskt_B;N;+;Regel_F
2.8 Gränsbeloppet enligt p. 2.6 ovan;4731;Numeriskt_B;N;-;Regel_F
2.9 Utdelning som beskattas i tjänst;4526;Numeriskt_B;N;+;Regel_F
2.10 Sparat utdelningsutrymme;4732;Numeriskt_B;N;-;Regel_F
2.11 Vid delavyttring eller gåva efter utdelningstillfället......;4733;Numeriskt_B;N;-;Regel_F
2.12 Sparat utdelningsutrymme till nästa år;4734;Numeriskt_B;N;*;Regel_F
2.13 Utdelning;4527;Numeriskt_B;N;+;Regel_F
2.14 Belopp som beskattas i tjänst enligt p. 2.9 ovan;4735;Numeriskt_B;N;-;Regel_F
2.15 Utdelning i kapital;4528;Numeriskt_B;N;*;Regel_F
2.16 Beloppet i p. 2.6 x 2/3. Om beloppet i p. ...;4529;Numeriskt_B;N;+;Regel_F
2.17 Resterande utdelning...;4540;Numeriskt_B;N;+;Regel_F
2.18 Utdelning som ska tas upp i kapital;4541;Numeriskt_B;N;*;Regel_F
Antal sålda andelar;4543;Numeriskt_B;N;*;
Försäljningsdatum;4544;Datum_D;N;;
3.1 Ersättning minus utgifter för avyttring;4545;Numeriskt_B;N;+;
3.2 Verkligt omkostnadsbelopp;4740;Numeriskt_B;N;-;
3.3 Vinst;4546;Numeriskt_B;N;+;
3.4a. Förlust;4741;Numeriskt_B;N;-;
3.4b. Förlust i p. 3.4a x 2/3;4742;Numeriskt_B;N;*;
3.5 Ersättning minus utgifter för avyttring...;4547;Numeriskt_B;N;+;
3.6 Omkostnadsbelopp (alternativt omräknat omkostnadsbelopp);4743;Numeriskt_B;N;-;
3.7a. Om utdelning erhållits under året...;4744;Numeriskt_B;N;-;
3.7b. Om utdelning erhållits efter delavyttring...;4745;Numeriskt_B;N;-;
3.8 Skattepliktig vinst som ska beskattas i tjänst. (Max 7 100 000 kr);4548;Numeriskt_B;N;*;
3.9 Vinst enligt p. 3.3 ovan;4549;Numeriskt_B;N;+;
3.10 Belopp som beskattas i tjänst enligt p. 3.8 (max 7 100 000 kr;4746;Numeriskt_B;N;-;
3.11 Vinst i inkomstslaget kapital;4550;Numeriskt_B;N;*;
3.12. Belopp antingen enligt p. 3.7a x 2/3 eller 3.7b x 2/3. Om beloppet i p. 3.7a eller 3.7b är större än vinsten i p. 3.11 tas istället 2/3 av vinsten i p. 3.11 upp.;4551;Numeriskt_B;N;+;
"3.13 Resterande vinst (p. 3.11 minus antingen p. 3.7a eller
p. 3.7b). Beloppet kan inte bli lägre än 0 kr.";4552;Numeriskt_B;N;+;
3.14 Vinst som ska tas upp i inkomstslaget kapital;4553;Numeriskt_B;N;*;
Den i avsnitt B redovisade överlåtelsen avser avyttring (ej efterföljande byte) av andelar som har förvärvats genom andelsbyte;4555;Str_X;N;;
4.1 Det omräknade omkostnadsbeloppet har beräknats enligt Indexregeln (andelar anskaffade före 1990);4556;Str_X;N;;
4.2 Det omräknade omkostnadsbeloppet har beräknats enligt Kapitalunderlagsregeln (andelar anskaffade före 1992);4557;Str_X;N;;
Lönekravet uppfylls av närstående. Personnummer;4560;Orgnr_Id_PD;N;;
5.1 Din kontanta ersättning från företaget och dess dotterföretag under 2021;4561;Numeriskt_B;N;*;
5.2 Sammanlagd kontant ersättning i företaget och dess dotterföretag under 2021;4562;Numeriskt_B;N;*;
5.3 (Punkt 5.2 x 5%) + 409 200 kr;4563;Numeriskt_B;N;*;
Om löneuttag enligt ovan gjorts i dotterföretag. Organisationsnummer;4564;Orgnr_Id_O;N;;Ingen kontroll av värdet sker om 7060 är ifyllt
Om löneuttag enligt ovan gjorts i dotterföretag. Organisationsnummer;4565;Orgnr_Id_O;N;;Ingen kontroll av värdet sker om 7060 är ifyllt
Om löneuttag enligt ovan gjorts i dotterföretag. Organisationsnummer;4566;Orgnr_Id_O;N;;Ingen kontroll av värdet sker om 7060 är ifyllt
Utländskt dotterföretag;7060;Str_X;N;;
6.1 Kontant ersättning till arbetstagare under 2021...;4570;Numeriskt_B;N;+;
6.2 Kontant ersättning under 2021...;4571;Numeriskt_B;N;+;
6.3 Löneunderlag;4572;Numeriskt_B;N;*;
6.4 Löneunderlag enligt p. 6.3 x 50 %;4573;Numeriskt_B;N;*;
6.5 Ditt lönebaserade utrymme för andelar ägda under hela 2021...;4576;Numeriskt_B;N;*;
6.6 Kontant ersättning till arbetstagare under...;4577;Numeriskt_B;N;+;
6.7 Kontant ersättning under...;4578;Numeriskt_B;N;+;
6.8 Löneunderlag avseende den tid under 2021 andelarna ägts;4579;Numeriskt_B;N;*;
6.9 Löneunderlag enligt p. 6.8 x 50 %;4580;Numeriskt_B;N;+;
6.10 Ditt lönebaserade utrymme för andelar som anskaffats under 2021.;4583;Numeriskt_B;N;*;
6.11 Totalt lönebaserat utrymme;4584;Numeriskt_B;N;*;)";			
		}

		// From https://www.bas.se/kontoplaner/sru/
		// Also in resources/INK2_19_P1-intervall-vers-2.csv
		const char* INK2_csv_to_sru_template = R"(Fältnamn på INK2_SKV2002-30-01-20-02;;;;;
;;;;;
;;;;;
Attribut;Fältnamn;Datatyp;Obl.;*/+/-;Regel
Framställningsdatum;DatFramst;Datum_A;J;;
Framställningstid;TidFramst;Tid_A;J;;
Fältkodsnummer;FältKod;Numeriskt_E;N;;
Intern information för framställande program/system;SystemInfo;Str_250;N;;
Korrekt organisationsnummer;PersOrgNr;Orgnr_Id_O;J;;
Räkenskapsårets början;7011;Datum_D;N;;
Räkenskapsårets slut;7012;Datum_D;N;;
1.1 Överskott av näringsverksamhet;7104;Numeriskt_B;N;*;
1.2 Underskott av näringsverksamhet;7114;Numeriskt_B;N;*;
1.4 Underlag för särskild löneskatt på pensionskostnader;7132;Numeriskt_B;N;*;Får ej förekomma om 7133 finns med och den <> 0.
1.5 Negativt underlag för särskild löneskatt på pensionskostnader;7133;Numeriskt_B;N;*;Får ej förekomma om 7132 finns med och den <> 0.
1.6 a Underlag för avkastningsskatt 15% Försäkringsföretag m.fl. samt Avsatt till pensioner;7153;Numeriskt_B;N;*;
1.6 b Underlag för avkastningsskatt 15 % Utländska pensionsförsäkringar;7154;Numeriskt_B;N;*;
1.7 a Underlag för avkastningsskatt 30% Försäkringsföretag m.fl.;7155;Numeriskt_B;N;*;
1.7 b Underlag för avkastningsskatt 30 % Utländska kapitalförsäkringar;7156;Numeriskt_B;N;*;
1.8 Småhus/ägarlägenhet hel avgift;80;Numeriskt_B;N;*;
1.8 Småhus/ägarlägenhet halv avgift;82;Numeriskt_B;N;*;
1.9 Hyreshus, bostäder hel avgift;93;Numeriskt_B;N;*;
1.9 Hyreshus, bostäder halv avgift;94;Numeriskt_B;N;*;
1.10 Småhus/ägarlägenhet: tomtmark, byggnad under uppförande;84;Numeriskt_B;N;*;
1.11 Hyreshus: tomtmark, bostäder under uppförande;86;Numeriskt_B;N;*;
1.12 Hyreshus: lokaler;95;Numeriskt_B;N;*;
1.13 Industri/elproduktionsenhet, värmekraftverk (utom vindkraftverk);96;Numeriskt_B;N;*;
1.14 Elproduktionsenhet, vattenkraftverk;97;Numeriskt_B;N;*;
1.15 Elproduktionsenhet, vindkraftverk;98;Numeriskt_B;N;*;
Övriga upplysningar på bilaga;90;Str_X;N;;
Kanal;Kanal;Str_250;N;;

;;;;
;Dokumenthistorik;;;
;Datum;Version;Beskrivning;Signatur
;;;;
;;;;
;;;;
;;;;
;;;;
;Referenser;;;
;"Definition av format återfinns i SKV 269 ""Teknisk beskrivning Näringsuppgifter(SRU) Anstånd Tjänst Kapital""";;;)";

		const char* INK2S_csv_to_sru_template = R"(Fältnamn på INK2S_SKV2002-30-01-20-02;;;;;
;;;;;
;;;;;
Attribut;Fältnamn;Datatyp;Obl.;*/+/-;Regel
Framställningsdatum;DatFramst;Datum_A;J;;
Framställningstid;TidFramst;Tid_A;J;;
Fältkodsnummer;FältKod;Numeriskt_E;N;;
Intern information för framställande program/system;SystemInfo;Str_250;N;;
Korrekt organisationsnummer;PersOrgNr;Orgnr_Id_O;J;;
Uppgiftslämnarens namn;Namn;Str_250;N;;
Räkenskapsårets början;7011;Datum_D;N;;
Räkenskapsårets slut;7012;Datum_D;N;;
4.1 Årets resultat, vinst;7650;Numeriskt_B;N;+;Får ej förekomma om 7750 finns med och den <> 0.
4.2 Årets resultat, förlust;7750;Numeriskt_B;N;-;Får ej förekomma om 7650 finns med och den <> 0.
4.3a. Bokförda kostnader som inte ska dras av: a. Skatt på årets resultat;7651;Numeriskt_A;N;+;
b. Bokförda kostnader som inte ska dras av: b. Nedskrivning av finansiella tillgångar;7652;Numeriskt_A;N;+;
c. Bokförda kostnader som inte ska dras av: c. Andra bokförda kostnader;7653;Numeriskt_A;N;+;
4.4a. Kostnader som ska dras av men som inte ingår i det redovisade resultatet: a. Lämnade koncernbidrag;7751;Numeriskt_A;N;-;
b. Kostnader som ska dras av men som inte ingår i det redovisade resultatet: b. Andra ej bokförda kostnader;7764;Numeriskt_A;N;-;
4.5a. Bokförda intäkter som inte ska tas upp: a. Ackordsvinster;7752;Numeriskt_A;N;-;
b. Bokförda intäkter som inte ska tas upp: b. Utdelning;7753;Numeriskt_A;N;-;
c. Bokförda intäkter som inte ska tas upp: c. Andra bokförda intäkter;7754;Numeriskt_A;N;-;
"4.6a. Intäkter som ska tas upp men som inte ingår i det redovisade resultatet:
a. Beräknad schablonintäkt på kvarvarande periodiseringsfonder vid beskattningsårets ingång";7654;Numeriskt_B;N;+;
b. Intäkter som ska tas upp men som inte ingår i det redovisade resultatet: b. Beräknad schablonintäkt på fondandelar ägda vid ingången av kalenderåret;7668;Numeriskt_A;N;+;
c. Intäkter som ska tas upp men som inte ingår i det redovisade resultatet: c. Mottagna koncernbidrag;7655;Numeriskt_A;N;+;
d. Intäkter som ska tas upp men som inte ingår i det redovisade resultatet: d. Uppräknat belopp vid återföring av periodiseringsfond;7673;Numeriskt_A;N;+;
e. Intäkter som ska tas upp men som inte ingår i det redovisade resultatet: e. Andra ej bokförda intäkter;7665;Numeriskt_A;N;+;
4.7a. Avyttring av delägarrätter: a. Bokförd vinst;7755;Numeriskt_B;N;-;
b. Avyttring av delägarrätter: b. Bokförd förlust;7656;Numeriskt_B;N;+;
c. Avyttring av delägarrätter: c. Uppskov med kapitalvinst enligt blankett N4;7756;Numeriskt_B;N;-;
d. Avyttring av delägarrätter: d. Återfört uppskov med kapitalvinst enligt blankett N4;7657;Numeriskt_B;N;+;
e. Avyttring av delägarrätter: e. Kapitalvinst för beskattningsåret;7658;Numeriskt_B;N;+;
f. Avyttring av delägarrätter: f. Kapitalförlust som ska dras av;7757;Numeriskt_B;N;-;
4.8a. Andel i handelsbolag (inkl. avyttring): a. Bokförd intäkt/vinst;7758;Numeriskt_B;N;-;
b. Andel i handelsbolag (inkl. avyttring): b. Skattemässigt överskott enligt N3B;7659;Numeriskt_B;N;+;
c. Andel i handelsbolag (inkl. avyttring): c. Bokförd kostnad/förlust;7660;Numeriskt_B;N;+;
d. Andel i handelsbolag (inkl. avyttring): d. Skattemässigt underskott enligt N3B;7759;Numeriskt_B;N;-;
4.9 Skattemässig justering av bokfört resultat för avskrivningar på byggnader och annan fast egendom samt restvärdesavskrivning på maskiner och inventarier (+);7666;Numeriskt_B;N;+;
4.9 Skattemässig justering av bokfört resultat för avskrivningar på byggnader och annan fast egendom samt restvärdesavskrivning på maskiner och inventarier (-);7765;Numeriskt_B;N;-;
4.10 Skattemässig korrigering av bokfört resultat vid avyttring av näringsfastighet och näringsbostadsrätt: +;7661;Numeriskt_B;N;+;
4.10 Skattemässig korrigering av bokfört resultat vid avyttring av näringsfastighet och näringsbostadsrätt: -;7760;Numeriskt_B;N;-;
4.11 Skogs-/substansminskningsavdrag (specificeras på blankett N8);7761;Numeriskt_B;N;-;
4.12 Återföringar vid avyttring av fastighet t.ex. värdeminskningsavdrag, skogsavdrag och substansminskningsavdrag...;7662;Numeriskt_A;N;+;
4.13 Andra skattemässiga justeringar av resultatet: +;7663;Numeriskt_B;N;+;
4.13 Andra skattemässiga justeringar av resultatet: -;7762;Numeriskt_B;N;-;
4.14a. Underskott: a. Outnyttjat underskott från föregående år;7763;Numeriskt_B;N;-;
4.14b. Reduktion av outnyttjat underskott med hänsyn till beloppsspärr, ackord eller konkurs;7671;Numeriskt_A;N;+;
4.14c. Reduktion av outnyttjat underskott med hänsyn till koncernbidragsspärr, fusionsspärr m.m.;7672;Numeriskt_A;N;+;
4.15 Överskott (flyttas till p. 1.1 på sid. 1);7670;Numeriskt_B;N;+;Får ej förekomma om 7770 finns med och 7770 <> 0.
4.16 Underskott (flyttas till p. 1.2 på sid. 1);7770;Numeriskt_B;N;-;Får ej förekomma om 7670 finns med och 7670 <> 0.
4.17 Årets begärda och tidigare års medgivna värdeminskningsavdrag som finns vid beskattningsårets utgång avseende byggnader.;8020;Numeriskt_A;N;*;
4.18 Årets begärda och tidigare års medgivna värdeminskningsavdrag  som finns vid beskattningsårets utgång avseende markanläggningar.;8021;Numeriskt_A;N;*;
"4.19 Vid restvärdesavskrivning:
återförda belopp för av- och nedskrivning,
försäljning, utrangering";8023;Numeriskt_B;N;*;
"4.20 Lån från aktieägare (fysisk person)
vid räkenskapsårets utgång";8026;Numeriskt_B;N;*;
4.21 Pensionskostnader (som ingår i p. 3.8);8022;Numeriskt_A;N;*;
4.22 Koncernbidrags-, fusionsspärrat underskott m.m.;8028;Numeriskt_B;N;*;
Uppdragstagare (t.ex. redovisningskonsult) har biträtt vid upprättandet av årsredovisningen: Ja;8040;Str_X;N;;Får ej förekomma om 8041 finns med.
Uppdragstagare (t.ex. redovisningskonsult) har biträtt vid upprättandet av årsredovisningen: Nej;8041;Str_X;N;;Får ej förekomma om 8040 finns med.
Årsredovisningen har varit föremål för revision: Ja;8044;Str_X;N;;Får ej förekomma om 8045 finns med.
Årsredovisningen har varit föremål för revision: Nej;8045;Str_X;N;;Får ej förekomma om 8044 finns med.)";
		const char* INK2R_csv_to_sru_template = R"(Fältnamn på INK2R_SKV2002-30-01-20-02;;;;;
;;;;;
;;;;;
Attribut;Fältnamn;Datatyp;Obl.;*/+/-;Regel
Framställningsdatum;DatFramst;Datum_A;J;;
Framställningstid;TidFramst;Tid_A;J;;
Fältkodsnummer;FältKod;Numeriskt_E;N;;
Intern information för framställande program/system;SystemInfo;Str_250;N;;
Korrekt organisationsnummer;PersOrgNr;Orgnr_Id_O;J;;
Uppgiftslämnarens namn;Namn;Str_250;N;;
Räkenskapsårets början;7011;Datum_D;N;;
Räkenskapsårets slut;7012;Datum_D;N;;
2.1 Immateriella anläggningstillgångar Koncessioner, patent, licenser, varumärken, hyresrätter, goodwill och liknande rättigheter;7201;Numeriskt_A;N;*;
2.2 Immateriella anläggningstillgångar Förskott avseende immateriella anläggningstillgångar;7202;Numeriskt_A;N;*;
2.3 Materiella anläggningstillgångar Byggnader och mark;7214;Numeriskt_A;N;*;
2.4 Materiella anläggningstillgångar Maskiner, inventarier och övriga materiella anläggningstillgångar;7215;Numeriskt_A;N;*;
2.5 Materiella anläggningstillgångar Förbättringsutgifter på annans fastighet;7216;Numeriskt_A;N;*;
2.6 Materiella anläggningstillgångar Pågående nyanläggningar och förskott avseende materiella anläggningstillgångar;7217;Numeriskt_A;N;*;
2.7 Finansiella anläggningstillgångar Andelar i koncernföretag;7230;Numeriskt_A;N;*;
2.8 Finansiella anläggningstillgångar Andelar i intresseföretag och gemensamt styrda företag;7231;Numeriskt_A;N;*;
2.9 Finansiella anläggningstillgångar Ägarintressen i övriga företag och Andra långfristiga värdepappersinnehav;7233;Numeriskt_A;N;*;
2.10 Finansiella anläggningstillgångar Fordringar hos koncern-, intresse- och  gemensamt styrda företag;7232;Numeriskt_A;N;*;
2.11 Finansiella anläggningstillgångar Lån till delägare eller närstående;7234;Numeriskt_A;N;*;
2.12 Finansiella anläggningstillgångar Fordringar hos övriga företag som det finns ett ägarintresse i och Andra långfristiga fordringar;7235;Numeriskt_A;N;*;
2.13 Varulager Råvaror och förnödenheter;7241;Numeriskt_A;N;*;
2.14 Varulager Varor under tillverkning;7242;Numeriskt_A;N;*;
2.15 Varulager Färdiga varor och handelsvaror;7243;Numeriskt_A;N;*;
2.16 Varulager Övriga lagertillgångar;7244;Numeriskt_A;N;*;
2.17 Varulager Pågående arbeten för annans räkning;7245;Numeriskt_A;N;*;
2.18 Varulager Förskott till leverantörer;7246;Numeriskt_A;N;*;
2.19 Kortfristiga fordringar Kundfordringar;7251;Numeriskt_A;N;*;
2.20 Kortfristiga fordringar Fordringar hos koncern-, intresse- och gemensamt styrda företag;7252;Numeriskt_A;N;*;
2.21 Kortfristiga fordringar Fordringar hos övriga företag som det finns ett ägarintresse i och Övriga fordringar;7261;Numeriskt_A;N;*;
2.22 Kortfristiga fordringar Upparbetad men ej fakturerad intäkt;7262;Numeriskt_A;N;*;
2.23 Kortfristiga fordringar Förutbetalda kostnader och upplupna intäkter;7263;Numeriskt_A;N;*;
2.24 Kortfristiga placeringar Andelar i koncernföretag;7270;Numeriskt_A;N;*;
2.25 Kortfristiga placeringar Övriga kortfristiga placeringar;7271;Numeriskt_A;N;*;
2.26 Kassa och bank Kassa, bank och redovisningsmedel;7281;Numeriskt_A;N;*;
2.27 Eget kapital Bundet eget kapital;7301;Numeriskt_A;N;*;
2.28 Eget kapital Fritt eget kapital;7302;Numeriskt_A;N;*;
2.29 Obeskattade reserver Periodiseringsfonder;7321;Numeriskt_A;N;*;
2.30 Obeskattade reserver Ackumulerade överavskrivningar;7322;Numeriskt_A;N;*;
2.31 Obeskattade reserver Övriga obeskattade reserver;7323;Numeriskt_A;N;*;
2.32 Avsättningar Avsättningar för pensioner och liknande förpliktelser enligt lag (1967:531)...;7331;Numeriskt_A;N;*;
2.33 Avsättningar Övriga avsättningar för pensioner och liknande förpliktelser;7332;Numeriskt_A;N;*;
2.34 Avsättningar Övriga avsättningar;7333;Numeriskt_A;N;*;
2.35 Långfristiga skulder Obligationslån;7350;Numeriskt_A;N;*;
2.36 Långfristiga skulder Checkräkningskredit;7351;Numeriskt_A;N;*;
2.37 Långfristiga skulder Övriga skulder till kreditinstitut;7352;Numeriskt_A;N;*;
2.38 Långfristiga skulder Skulder till koncern-, intresse- och gemensamt styrda företag;7353;Numeriskt_A;N;*;
2.39 Långfristiga skulder Skulder till övriga företag som det finns ett ägarintresse i och Övriga skulder;7354;Numeriskt_A;N;*;
2.40 Kortfristiga skulder Checkräkningskredit;7360;Numeriskt_A;N;*;
2.41 Kortfristiga skulder Övriga skulder till kreditinstitut;7361;Numeriskt_A;N;*;
2.42 Kortfristiga skulder Förskott från kunder;7362;Numeriskt_A;N;*;
2.43 Kortfristiga skulder Pågående arbeten för annans räkning;7363;Numeriskt_A;N;*;
2.44 Kortfristiga skulder Fakturerad men ej upparbetad intäkt;7364;Numeriskt_A;N;*;
2.45 Kortfristiga skulder Leverantörsskulder;7365;Numeriskt_A;N;*;
2.46 Kortfristiga skulder Växelskulder;7366;Numeriskt_A;N;*;
2.47 Kortfristiga skulder Skulder till koncern-, intresse- och gemensamt styrda företag;7367;Numeriskt_A;N;*;
2.48 Kortfristiga skulder Skulder till övriga företag som det finns ett ägarintresse i och Övriga skulder;7369;Numeriskt_A;N;*;
2.49 Kortfristiga skulder Skattesskulder;7368;Numeriskt_A;N;*;
2.50 Kortfristiga skulder Upplupna kostnader och förutbetalda intäkter;7370;Numeriskt_A;N;*;
3.1 Nettoomsättning;7410;Numeriskt_A;N;+;
3.2 Förändring av lager av produkter i arbete, färdiga varor och pågående arbete för annans räkning. +;7411;Numeriskt_A;N;+;
3.2 Förändring av lager av produkter i arbete, färdiga varor och pågående arbete för annans räkning. -;7510;Numeriskt_A;N;-;
3.3 Aktiverat arbete för egen räkning;7412;Numeriskt_A;N;+;
3.4 Övriga rörelseintäkter;7413;Numeriskt_A;N;+;
3.5 Råvaror och förnödenheter;7511;Numeriskt_A;N;-;
3.6 Handelsvaror;7512;Numeriskt_A;N;-;
3.7 Övriga externa kostnader;7513;Numeriskt_A;N;-;
3.8 Personalkostnader;7514;Numeriskt_A;N;-;
3.9 Av- och nedskrivningar av materiella och immateriella anläggningstillgångar;7515;Numeriskt_A;N;-;
3.10 Nedskrivningar av omsättningstillgångar utöver normala nedskrivningar;7516;Numeriskt_A;N;-;
3.11 Övriga rörelsekostnader;7517;Numeriskt_A;N;-;
3.12 Resultat från andelar i koncernföretag +;7414;Numeriskt_A;N;+;
3.12 Resultat från andelar i koncernföretag -;7518;Numeriskt_A;N;-;
3.13 Resultat från andelar i intresseföretag och gemensamt styrda företag +;7415;Numeriskt_A;N;+;
3.13 Resultat från andelar i intresseföretag och gemensamt styrda företag -;7519;Numeriskt_A;N;-;
3.14 Resultat från övriga företag som det finns ett ägarintresse i +;7423;Numeriskt_A;N;+;
3.14 Resultat från övriga företag som det finns ett ägarintresse i -;7530;Numeriskt_A;N;-;
3.15 Resultat från övriga finansiella anläggningstillgångar +;7416;Numeriskt_A;N;+;
3.15 Resultat från övriga finansiella anläggningstillgångar -;7520;Numeriskt_A;N;-;
3.16 Övriga ränteintäkter och liknande resultatposter;7417;Numeriskt_A;N;+;
3.17 Nedskrivning av finansiella anläggningstillgångar och kortfristiga placeringar;7521;Numeriskt_A;N;-;
3.18 Räntekostnader och liknande resultatposter;7522;Numeriskt_A;N;-;
3.19 Lämnade koncernbidrag;7524;Numeriskt_A;N;-;
3.20 Mottagna koncernbidrag;7419;Numeriskt_A;N;+;
3.21 Återföring av periodiseringsfond;7420;Numeriskt_B;N;+;
3.22 Avsättning till periodiseringsfond;7525;Numeriskt_B;N;-;
3.23 Förändring av överavskrivningar +;7421;Numeriskt_A;N;+;
3.23 Förändring av överavskrivningar -;7526;Numeriskt_A;N;-;
3.24 Övriga bokslutsdispositioner +;7422;Numeriskt_A;N;+;
3.24 Övriga bokslutsdispositioner -;7527;Numeriskt_A;N;-;
3.25 Skatt på årets resultat;7528;Numeriskt_A;N;-;
3.26 Årets resultat, vinst (flyttas till p. 4.1);7450;Numeriskt_B;N;+;Får ej förekomma om 7550 finns med och 7550 <> 0.
3.27 Årets resultat, förlust (flyttas till p. 4.2);7550;Numeriskt_B;N;-;Får ej förekomma om 7450 finns med och 7450 <> 0.)";

	} // namespace SRU
} // namespace SKV

namespace BAS { // BAS

	namespace SRU { // BAS::SRU

		//  From file resources/INK2_19_P1-intervall-vers-2.csv
		// 	URL: https://www.bas.se/kontoplaner/sru/
		// 	These are mapping between SKV SRU-codes and BAS accounts
		// 	for INK2 Income TAX Return.
		// 	The csv-files are exported from macOS Numbers from correspodning xls-file.
		// 	File resources/INK2_19_P1-intervall-vers-2.csv us easier to parse for an algorithm
		// 	as BAS accounts are givven in explicit intervals.

		namespace INK2 { // BAS::SRU::INK2
			char const* INK2_19_P1_intervall_vers_2_csv{R"(Blad1: Table 1
2018;;Inkomstdeklaration 2;2019 P1;;;;;
;;  ;;;;;;
;;;;;;;;
;;För deklarationer som lämnas fr.o.m. period 1 2018;Ändring i kopplingstabellen jämfört med föregående år.;;;;;
;;;;;;;;
Fält-kod;Rad Ink 2;Benämning;Konton i BAS 2017;;;;;
;;BALANSRÄKNING;;;;;;
;;Tillgångar;;;;;;
;;;;;;;;
;;Anläggningstillgångar;;;;;;
;;;;;;;;
;;Immateriella anläggningstillgångar;;;;;;
7201;2.1;Koncessioner, patent, licenser, varumärken, hyresrätter, goodwill och liknande rättigheter;1000-1087, 1089-1099;;;;;
7202;2.2;Förskott avseende immateriella anläggningstillgångar;1088;;;;;
;;;;;;;;
;;Materiella anläggningstillgångar;;;;;;
7214;2.3;Byggnader och mark;1100-1119, 1130-1179, 1190-1199;;;;;
7215;2.4 ;Maskiner, inventarier och övriga materiella anläggningstillgångar;1200-1279, 1290-1299;;;;;
7216;2.5;Förbättringsutgifter på annans fastighet;112x;;;;;
7217;2.6;Pågående nyanläggningar och förskott avseende materiella anläggningstillgångar;118x, 128x;;;;;
;;;;;;;;
;;Finansiella anläggningstillgångar;;;;;;
7230;2.7;Andelar i koncernföretag;131x;;;;;
7231;2.8;Andelar i intresseföretag och gemensamt styrda företag;1330-1335, 1338-1339;;;;;
7233;2.9;Ägarintresse i övriga företag och andra långfristiga värdepappersinnehav;135x, 1336, 1337;;;;;
7232;2.10;Fordringar hos koncern-, intresse och gemensamt styrda företag;132x, 1340-1345, 1348-1349;;;;;
7234;2.11;Lån till delägare eller närstående;136x;;;;;
7235;2.12;Fordringar hos övriga företag som det finns ett ett ägarintresse i och Andra långfristiga fordringar;137x, 138x, 1346, 1347;;;;;
;;;;;;;;
;;Omsättningstillgångar;;;;;;
;;;;;;;;
;;Varulager;;;;;;
7241;2.13;Råvaror och förnödenheter;141x, 142x;;;;;
7242;2.14;Varor under tillverkning;144x;;;;;
7243;2.15;Färdiga varor och handelsvaror;145x, 146x;;;;;
7244;2.16;Övriga lagertillgångar;149x;;;;;
7245;2.17;Pågående arbeten för annans räkning;147x;;;;;
7246;2.18;Förskott till leverantörer;148x;;;;;
;;;;;;;;
;;Kortfristiga fordringar;;;;;;
7251;2.19;Kundfordringar;151x–155x, 158x;;;;;
7252;2.20;Fordringar hos koncern-, intresse- och gemensamt styrda företag;156x, 1570-1572, 1574-1579, 166x, 1671-1672, 1674-1679;;;;;
7261;2.21;Fordringar hos övriga företag som det finns ett ägarintresse i och Övriga fordringar;161x, 163x-165x, 168x-169x, 1573, 1673;;;;;
7262;2.22;Upparbetad men ej fakturerad intäkt;162x;;;;;
7263;2.23;Förutbetalda kostnader och upplupna intäkter;17xx;;;;;
;;;;;;;;
;;Kortfristiga placeringar;;;;;;
7270;2.24;Andelar i koncernföretag;186x;;;;;
7271;2.25;Övriga kortfristiga placeringar;1800-1859, 1870-1899;;;;;
;;;;;;;;
;;Kassa och bank;;;;;;
7281;2.26;Kassa, bank och redovisningsmedel;19xx;;;;;
;;;;;;;;
;;Eget kapital och skulder;;;;;;
;;Eget kapital;;;;;;
;;;;;;;;
7301;2.27;Bundet eget kapital;208x;;;;;
7302;2.28;Fritt eget kapital;209x;;;;;
;;;;;;;;
;;Obeskattade reserver och avsättningar;;;;;;
;;;;;;;;
;;Obeskattade reserver;;;;;;
7321;2.29;Periodiseringsfonder;211x-213x;;;;;
7322;2.30;Ackumulerade överavskrivningar;215x;;;;;
7323;2.31;Övriga obeskattade reserver;216x-219x;;;;;
;;;;;;;;
;;Avsättningar;;;;;;
7331;2.32;Avsättningar för pensioner och liknande förpliktelser enligt lagen (1967:531) om tryggande av pensionsutfästelserr m.m.;221x;;;;;
7332;2.33;Övriga avsättningar för pensioner och liknande förpliktelser;223x;;;;;
7333;2.34;Övriga avsättningar;2220-2229, 2240-2299;;;;;
;;;;;;;;
;;Skulder;;;;;;
;;;;;;;;
;;Långfristiga skulder (förfaller senare än 12 månader från balansdagen);;;;;;
7350;2.35;Obligationslån;231x-232x;;;;;
7351;2.36;Checkräkningskredit;233x;;;;;
7352;2.37;Övriga skulder till kreditinstitut;234x-235x;;;;;
7353;2.38;Skulder till koncern-, intresse och gemensamt styrda företag;2360- 2372, 2374-2379;;;;;
7354;2.39;Skulder till övriga företag som det finns ett ägarintresse i och övriga skulder;238x-239x, 2373;;;;;
;;;;;;;;
;;Kortfristiga skulder (förfaller inom 12 månader från balansdagen);;;;;;
7360;2.40;Checkräkningskredit;248x;;;;;
7361;2.41;Övriga skulder till kreditinstitut;241x;;;;;
7362;2.42;Förskott från kunder;242x;;;;;
7363;2.43;Pågående arbeten för annans räkning;243x;;;;;
7364;2.44;Fakturerad men ej upparbetad intäkt;245x;;;;;
7365;2.45;Leverantörsskulder;244x;;;;;
7366;2.46;Växelskulder;2492;;;;;
7367;2.47;Skulder till koncern-, intresse och gemensamt styrda företag;2460-2472, 2474-2872, 2874-2879;;;;;
7369;2.48;Skulder till övriga företag som det finns ett ägarintresse i och Övriga skulder;2490-2491, 2493-2499, 2600-2859, 2880-2899;;;;;
7368;2.49;Skatteskulder;25xx;;;;;
7370;2.50;Upplupna kostnader och förutbetalda intäkter;29xx;;;;;
;;;;;;;;
;;;;;;;;
;;RESULTATRÄKNING;;;;;;
;;;;;;;;
7410;3.1;Nettoomsättning;30xx-37xx;;;;;
7411;3.2;Förändring av lager av produkter i arbete, färdiga varor och pågående arbete för annans räkning;`+ 4900-4909, 4930-4959, 4970-4979, 4990-4999;;;;;
7510;;;– 4900-4909, 4930-4959, 4970-4979, 4990-4999;;;;;
7412;3.3;Aktiverat arbete för egen räkning;38xx;;;;;
7413;3.4;Övriga rörelseintäkter;39xx;;;;;
7511;3.5;Råvaror och förnödenheter;40xx-47xx, 4910-4920;;;;;
7512;3.6;Handelsvaror;40xx-47xx, 496x, 498x;;;;;
7513;3.7;Övriga externa kostnader;50xx-69xx;;;;;
7514;3.8;Personalkostnader;70xx-76xx;;;;;
7515;3.9;Av- och nedskrivningar av materiella och immateriella anläggningstillgångar;7700-7739, 7750-7789, 7800-7899;;;;;
7516;3.10;Nedskrivningar av omsättningstillgångar utöver normala nedskrivningar;774x, 779x;;;;;
7517;3.11;Övriga rörelsekostnader;79xx;;;;;
7414;3.12;Resultat från andelar i koncernföretag; +  8000-8069, 8090-8099;;;;;
7518;;;– 8000-8069,8090-8099;;;;;
7415;3.13;Resultat från andelar i intresseföretag och gemensamt styrda företag; + 8100-8112, 8114-8117, 8119-8122, 8124-8132,8134-8169, 8190-8199;;;;;
7519;;;– 8100-8112, 8114-8117, 8119-8122, 8124-8132, 8134-8169, 8190-8199;;;;;
7423;3.14;Resultat från övriga företag som det finns ett ägarintresse i; + 8113, 8118, 8123, 8133;;;;;
7530;;; - 8113, 8118, 8123, 8133;;;;;
7416;3.15;Resultat från övriga anläggningstillgångar; + 8200-8269, 8290-8299;;;;;
7520;;;– 8200-8269, 8290-8299;;;;;
7417;3.16;Övriga ränteintäkter och liknande resultatposter;8300-8369, 8390-8399;;;;;
7521;3.17;Nedskrivningar av finansiella anläggningstillgångar och kortfristiga placeringar;807x, 808x, 817x, 818x, 827x, 828x, 837x, 838x;;;;;
7522;3.18;Räntekostnader och liknande resultatposter;84xx;;;;;
7524;3.19;Lämnade koncernbidrag;883x;;;;;
7419;3.20;Mottagna koncernbidrag;882x;;;;;
7420;3.21;Återföring av periodiseringsfond;8810, 8819;;;;;
7525;3.22;Avsättning till periodiseringsfond;8810, 8811;;;;;
7421;3.23;Förändring av överavskrivningar;+ 885x;;;;;
7526;;;– 885x;;;;;
7422;3.24;Övriga bokslutsdispositioner;+ 886x-889x;;;;;
7527;;;– 886x-889x, 884x;;;;;
7528;3.25;Skatt på årets resultat;8900-8989;;;;;
7450;3.26;Årets resultat, vinst (flyttas till p. 4.1)  (+);+ 899x;;;;;
7550;3.27;Årets resultat, förlust (flyttas till p. 4.2) (-);– 899x;;;;;

Blad2: Table 1
;;;;


Blad3: Table 1
;;;;
)"}; // char const* INK2_19_P1_intervall_vers_2_csv

			void parse(char const* INK2_19_P1_intervall_vers_2_csv) {
				std::cout << "\nTODO: Implement BAS::SRU::INK2::parse";
				std::istringstream in{INK2_19_P1_intervall_vers_2_csv};
				std::string row{};		
				while (std::getline(in,row)) {
					Key::Path tokens{row,';'};
					std::cout << "\n------------------";
					int index{};
					for (auto const& token : tokens) {
						std::cout << "\n\t" << index++ << ":" << std::quoted(token);
					}
				}
			}


		} // BAS::SRU::INK2

	} // BAS::SRU

	namespace K2 { // BAS::K2

		namespace AR { // BAS::K2::AR
			// A namespace for Swedish Bolagsverket "Årsredovisning" according to K2 rules

      namespace ar_online {

        // From https://www.arsredovisning-online.se/bas_kontoplan as of 221118
        // This text defines mapping between fields on the Swedish TAX Return form and ranges of BAS Accounts 
        char const* bas_2022_mapping_to_k2_ar_text{R"(Resultaträkning
Konto 3000-3799

Fält: Nettoomsättning
Beskrivning: Intäkter som genererats av företagets ordinarie verksamhet, t.ex. varuförsäljning och tjänsteintäkter.
Konto 3800-3899

Fält: Aktiverat arbete för egen räkning
Beskrivning: Kostnader för eget arbete där resultatet av arbetet tas upp som en tillgång i balansräkningen.
Konto 3900-3999

Fält: Övriga rörelseintäkter
Beskrivning: Intäkter genererade utanför företagets ordinarie verksamhet, t.ex. valutakursvinster eller realisationsvinster.
Konto 4000-4799 eller 4910-4931

Fält: Råvaror och förnödenheter
Beskrivning: Årets inköp av råvaror och förnödenheter +/- förändringar av lagerposten ”Råvaror och förnödenheter”. Även kostnader för legoarbeten och underentreprenader.
Konto 4900-4999 (förutom 4910-4931, 4960-4969 och 4980-4989)

Fält: Förändring av lager av produkter i arbete, färdiga varor och pågående arbete för annans räkning
Beskrivning: Årets förändring av värdet på produkter i arbete och färdiga egentillverkade varor samt förändring av värde på uppdrag som utförs till fast pris.
Konto 4960-4969 eller 4980-4989

Fält: Handelsvaror
Beskrivning: Årets inköp av handelsvaror +/- förändring av lagerposten ”Handelsvaror”.
Konto 5000-6999

Fält: Övriga externa kostnader
Beskrivning: Normala kostnader som inte passar någon annan stans, t.ex. lokalhyra, konsultarvoden, telefon, porto, reklam och nedskrivning av kortfristiga fordringar.
Konto 7000-7699

Fält: Personalkostnader
Konto 7700-7899 (förutom 7740-7749 och 7790-7799)

Fält: Av- och nedskrivningar av materiella och immateriella anläggningstillgångar
Konto 7740-7749 eller 7790-7799

Fält: Nedskrivningar av omsättningstillgångar utöver normala nedskrivningar
Beskrivning: Används mycket sällan. Ett exempel är om man gör ovanligt stora nedskrivningar av kundfordringar.
Konto 7900-7999

Fält: Övriga rörelsekostnader
Beskrivning: Kostnader som ligger utanför företagets normala verksamhet, t.ex. valutakursförluster och realisationsförlust vid försäljning av icke- finansiella anläggningstillgångar.
Konto 8000-8099 (förutom 8070-8089)

Fält: Resultat från andelar i koncernföretag
Beskrivning: Nettot av företagets finansiella intäkter och kostnader från koncernföretag med undantag av räntor, koncernbidrag och nedskrivningar, t.ex. erhållna utdelningar, andel i handelsbolags resultat och realisationsresultat.
Konto 8070-8089, 8170-8189, 8270-8289 eller 8370-8389

Fält: Nedskrivningar av finansiella anläggningstillgångar och kortfristiga placeringar
Beskrivning: Nedskrivningar av och återföring av nedskrivningar på finansiella anläggningstillgångar och kortfristiga placeringar
Konto 8100-8199 (förutom 8113, 8118, 8123, 8133 och 8170-8189)

Fält: Resultat från andelar i intresseföretag och gemensamt styrda företag
Beskrivning: Nettot av företagets finansiella intäkter och kostnader från intresseföretag och gemensamt styrda företag med undantag av räntor och nedskrivningar, t.ex. erhållna utdelningar, andel i handelsbolags resultat och realisationsresultat.
Konto 8113, 8118, 8123 eller 8133

Fält: Resultat från övriga företag som det finns ett ägarintresse i
Beskrivning: Nettot av företagets finansiella intäkter och kostnader från övriga företag som det finns ett ägarintresse i med undantag av räntor och nedskrivningar, t.ex. vissa erhållna vinstutdelningar, andel i handelsbolags resultat och realisationsresultat.
Konto 8200-8299 (förutom 8270-8289)

Fält: Resultat från övriga finansiella anläggningstillgångar
Beskrivning: Nettot av intäkter och kostnader från företagets övriga värdepapper och fordringar som är anläggningstillgångar, med undantag av nedskrivningar. T.ex. ränteintäkter (även på värdepapper avseende koncern- och intresseföretag), utdelningar, positiva och negativa valutakursdifferenser och realisationsresultat.
Konto 8300-8399 (förutom 8370-8389)

Fält: Övriga ränteintäkter och liknande resultatposter
Beskrivning: Resultat från finansiella omsättningstillgångar med undantag för nedskrivningar. T.ex. ränteintäkter (även dröjsmålsräntor på kundfordringar), utdelningar och positiva valutakursdifferenser.
Konto 8400-8499

Fält: Räntekostnader och liknande resultatposter
Beskrivning: Resultat från finansiella skulder, t.ex. räntor på lån, positiva och negativa valutakursdifferenser samt dröjsmåls-räntor på leverantörsskulder.
Konto 8710

Fält: Extraordinära intäkter
Beskrivning: Används mycket sällan. Får inte användas för räkenskapsår som börjar 2016-01-01 eller senare.
Konto 8750

Fält: Extraordinära kostnader
Beskrivning: Används mycket sällan. Får inte användas för räkenskapsår som börjar 2016-01-01 eller senare.
Konto 8810-8819

Fält: Förändring av periodiseringsfonder
Konto 8820-8829

Fält: Erhållna koncernbidrag
Konto 8830-8839

Fält: Lämnade koncernbidrag
Konto 8840-8849 eller 8860-8899

Fält: Övriga bokslutsdispositioner
Konto 8850-8859

Fält: Förändring av överavskrivningar
Konto 8900-8979

Fält: Skatt på årets resultat
Beskrivning: Skatt som belastar årets resultat. Här ingår beräknad skatt på årets resultat, men även t.ex. justeringar avseende tidigare års skatt.
Konto 8980-8989

Fält: Övriga skatter
Beskrivning: Används sällan.
Balansräkning
Konto 1020-1059 eller 1080-1089 (förutom 1088)

Fält: Koncessioner, patent, licenser, varumärken samt liknande rättigheter
Konto 1060-1069

Fält: Hyresrätter och liknande rättigheter
Konto 1070-1079

Fält: Goodwill
Konto 1088

Fält: Förskott avseende immateriella anläggningstillgångar
Beskrivning: Förskott i samband med förvärv, t.ex. handpenning och deposition.
Konto 1100-1199 (förutom 1120-1129 och 1180-1189)

Fält: Byggnader och mark
Beskrivning: Förutom byggnader och mark, även maskiner som är avsedda för byggnadens allmänna användning.
Konto 1120-1129

Fält: Förbättringsutgifter på annans fastighet
Konto 1180-1189 eller 1280-1289

Fält: Pågående nyanläggningar och förskott avseende materiella anläggningstillgångar
Konto 1210-1219

Fält: Maskiner och andra tekniska anläggningar
Beskrivning: Maskiner och tekniska anläggningar avsedda för produktionen.
Konto 1220-1279 (förutom 1260)

Fält: Inventarier, verktyg och installationer
Beskrivning: Om du fyller i detta fält måste du även fylla i motsvarande not i avsnittet "Noter".
Konto 1290-1299

Fält: Övriga materiella anläggningstillgångar
Beskrivning: T.ex. djur som klassificerats som anläggningstillgång.
Konto 1310-1319

Fält: Andelar i koncernföretag
Beskrivning: Aktier och andelar i koncernföretag.
Konto 1320-1329

Fält: Fordringar hos koncernföretag
Beskrivning: Fordringar på koncernföretag som förfaller till betalning senare än 12 månader från balansdagen.
Konto 1330-1339 (förutom 1336-1337)

Fält: Andelar i intresseföretag och gemensamt styrda företag
Beskrivning: Aktier och andelar i intresseföretag.
Konto 1336-1337

Fält: Ägarintressen i övriga företag
Beskrivning: Aktier och andelar i övriga företag som det redovisningsskyldiga företaget har ett ägarintresse i.
Konto 1340-1349 (förutom 1346-1347)

Fält: Fordringar hos intresseföretag och gemensamt styrda företag
Beskrivning: Fordringar på intresseföretag och gemensamt styrda företag, som förfaller till betalning senare än 12 månader från balansdagen.
Konto 1346-1347

Fält: Fordringar hos övriga företag som det finns ett ägarintresse i
Beskrivning: Fordringar på övriga företag som det finns ett ägarintresse i och som ska betalas senare än 12 månader från balansdagen.
Konto 1350-1359

Fält: Andra långfristiga värdepappersinnehav
Beskrivning: Långsiktigt innehav av värdepapper som inte avser koncern- eller intresseföretag.
Konto 1360-1369

Fält: Lån till delägare eller närstående
Beskrivning: Fordringar på delägare, och andra som står delägare nära, som förfaller till betalning senare än 12 månader från balansdagen.
Konto 1380-1389

Fält: Andra långfristiga fordringar
Beskrivning: Fordringar som förfaller till betalning senare än 12 månader från balansdagen.
Konto 1410-1429, 1430, 1431 eller 1438

Fält: Råvaror och förnödenheter
Beskrivning: Lager av råvaror eller förnödenheter som har köpts för att bearbetas eller för att vara komponenter i den egna tillverkningen.
Konto 1432-1449 (förutom 1438)

Fält: Varor under tillverkning
Beskrivning: Lager av varor där tillverkning har påbörjats.
Konto 1450-1469

Fält: Färdiga varor och handelsvaror
Beskrivning: Lager av färdiga egentillverkade varor eller varor som har köpts för vidareförsäljning (handelsvaror).
Konto 1470-1479

Fält: Pågående arbete för annans räkning
Beskrivning: Om du fyller i detta fält måste du även fylla i motsvarande not i avsnittet "Noter".
Konto 1480-1489

Fält: Förskott till leverantörer
Beskrivning: Betalningar och obetalda fakturor för varor och tjänster som redovisas som lager men där prestationen ännu inte erhållits.
Konto 1490-1499

Fält: Övriga lagertillgångar
Beskrivning: Lager av värdepapper (t.ex. lageraktier), lagerfastigheter och djur som klassificerats som omsättningstillgång.
Konto 1500-1559 eller 1580-1589

Fält: Kundfordringar
Konto 1560-1569 eller 1660-1669

Fält: Fordringar hos koncernföretag
Beskrivning: Fordringar på koncernföretag, inklusive kundfordringar.
Konto 1570-1579 (förutom 1573) eller 1670-1679 (förutom 1673)

Fält: Fordringar hos intresseföretag och gemensamt styrda företag
Beskrivning: Fordringar på intresseföretag och gemensamt styrda företag, inklusive kundfordringar.
Konto 1573 eller 1673

Fält: Fordringar hos övriga företag som det finns ett ägarintresse i
Beskrivning: Fordringar på övriga företag som det finns ett ägarintresse i, inklusive kundfordringar.
Konto 1590-1619, 1630-1659 eller 1680-1689

Fält: Övriga fordringar
Beskrivning: T.ex. aktuella skattefordringar.
Konto 1620-1629

Fält: Upparbetad men ej fakturerad intäkt
Beskrivning: Upparbetade men ej fakturerade intäkter från uppdrag på löpande räkning eller till fast pris enligt huvudregeln.
Konto 1690-1699

Fält: Tecknat men ej inbetalat kapital
Beskrivning: Fordringar på aktieägare före tecknat men ej inbetalt kapital. Används vid nyemission.
Konto 1700-1799

Fält: Förutbetalda kostnader och upplupna intäkter
Beskrivning: Förutbetalda kostnader (t.ex. förutbetalda hyror eller försäkringspremier) och upplupna intäkter (varor eller tjänster som är levererade men där kunden ännu inte betalat).
Konto 1800-1899 (förutom 1860-1869)

Fält: Övriga kortfristiga placeringar
Beskrivning: Innehav av värdepapper eller andra placeringar som inte är anläggningstillgångar och som inte redovisas i någon annan post under Omsättningstillgångar och som ni planerar att avyttra inom 12 månader från bokföringsårets slut.
Konto 1860-1869

Fält: Andelar i koncernföretag
Beskrivning: Här registrerar ni de andelar i koncernföretag som ni planerar att avyttra inom 12 månader från bokföringsårets slut.
Konto 1900-1989

Fält: Kassa och bank
Konto 1990-1999

Fält: Redovisningsmedel
Konto 2081, 2083 eller 2084

Fält: Aktiekapital
Beskrivning: Aktiekapitalet i ett aktiebolag ska vara minst 25 000 kr.
Konto 2082

Fält: Ej registrerat aktiekapital
Beskrivning: Beslutad ökning av aktiekapitalet genom fond- eller nyemission.
Konto 2085

Fält: Uppskrivningsfond
Konto 2086

Fält: Reservfond
Konto 2087

Fält: Bunden överkursfond
Konto 2090, 2091, 2093-2095 eller 2098

Fält: Balanserat resultat
Beskrivning: Summan av tidigare års vinster och förluster. Registrera balanserat resultat med minustecken om det balanserade resultatet är en balanserad förlust. Är det en balanserad vinst ska du inte använda minustecken.
Konto 2097

Fält: Fri överkursfond
Konto 2110-2149

Fält: Periodiseringsfonder
Beskrivning: Man kan avsätta upp till 25% av resultat efter finansiella poster till periodiseringsfonden. Det är ett sätt att skjuta upp bolagsskatten i upp till fem år. Avsättningen måste återföras till beskattning senast på det sjätte året efter det att avsättningen gjordes.
Konto 2150-2159

Fält: Ackumulerade överavskrivningar
Konto 2160-2199

Fält: Övriga obeskattade reserver
Konto 2210-2219

Fält: Avsättningar för pensioner och liknande förpliktelser enligt lagen (1967:531) om tryggande av pensionsutfästelse m.m.
Beskrivning: Åtaganden för pensioner enligt tryggandelagen.
Konto 2220-2229 eller 2250-2299

Fält: Övriga avsättningar
Beskrivning: Andra avsättningar än för pensioner, t.ex. garantiåtaganden.
Konto 2230-2239

Fält: Övriga avsättningar för pensioner och liknande förpliktelser
Beskrivning: Övriga pensionsåtaganden till nuvarande och tidigare anställda.
Konto 2310-2329

Fält: Obligationslån
Konto 2330-2339

Fält: Checkräkningskredit
Konto 2340-2359

Fält: Övriga skulder till kreditinstitut
Konto 2360-2369

Fält: Skulder till koncernföretag
Konto 2370-2379 (förutom 2373)

Fält: Skulder till intresseföretag och gemensamt styrda företag
Konto 2373

Fält: Skulder till övriga företag som det finns ett ägarintresse i
Konto 2390-2399

Fält: Övriga skulder
Konto 2410-2419

Fält: Övriga skulder till kreditinstitut
Konto 2420-2429

Fält: Förskott från kunder
Konto 2430-2439

Fält: Pågående arbete för annans räkning
Beskrivning: Om du fyller i detta fält måste du även fylla i motsvarande not i avsnittet "Noter".
Konto 2440-2449

Fält: Leverantörsskulder
Konto 2450-2459

Fält: Fakturerad men ej upparbetad intäkt
Konto 2460-2469 eller 2860-2869

Fält: Skulder till koncernföretag
Konto 2470-2479 (förutom 2473) eller 2870-2879 (förutom 2873)

Fält: Skulder till intresseföretag och gemensamt styrda företag
Konto 2473 eller 2873

Fält: Skulder till övriga företag som det finns ett ägarintresse i
Konto 2480-2489

Fält: Checkräkningskredit
Konto 2490-2499 (förutom 2492), 2600-2799, 2810-2859 eller 2880-2899

Fält: Övriga skulder
Konto 2492

Fält: Växelskulder
Konto 2500-2599

Fält: Skatteskulder
Konto 2900-2999

Fält: Upplupna kostnader och förutbetalda intäkter)"}; // bas_2022_mapping_to_k2_ar_text
      } // namespace ar_online
		} // namespace BAS::K2::AR
	} // namespace BAS::K2

	// The following string literal is the "raw" output of:
	// 1) In macOS Numbers opening excel file downloaded from https://www.bas.se/kontoplaner/
	// 2) Exported as csv-file
	// See project resource ./resources/Kontoplan-2022.csv
	char const* bas_2022_account_plan_csv{R"(;Kontoplan – BAS 2022;;;;;;;;
;;;;;;;;;
;;;;|;;= Ändring eller tillägg jämfört med föregående år. Mer information finns på BAS webbplats (bas.se).;;;
;;;;■;;= Kontot ingår i det urval av konton som för de flesta företag är tillräckligt för en grundläggande bokföring.;;;
;;;;[Ej K2];;= Kontot används inte av de företag som valt att tillämpa K2-regler.;;;
;;;;;;;;;
;;;Huvudkonton;;;Underkonton;;;
;;;;;;;;;
;;1;Tillgångar;;;;;;
;;10;Immateriella anläggningstillgångar;;;;;;
;[Ej K2];101;Utvecklingsutgifter;[Ej K2];1010;Utvecklingsutgifter;;;
;;;;[Ej K2];1011;Balanserade utgifter för forskning och utveckling;;;
;;;;[Ej K2];1012;Balanserade utgifter för programvaror;;;
;;;;[Ej K2];1018;Ackumulerade nedskrivningar på balanserade utgifter;;;
;;;;[Ej K2];1019;Ackumulerade avskrivningar på balanserade utgifter;;;
;;102;Koncessioner m.m.;;1020;Koncessioner m.m.;;;
;;;;;1028;Ackumulerade nedskrivningar på koncessioner m.m.;;;
;;;;;1029;Ackumulerade avskrivningar på koncessioner m.m.;;;
;■;103;Patent;■;1030;Patent;;;
;;;;;1038;Ackumulerade nedskrivningar på patent;;;
;;;;■;1039;Ackumulerade avskrivningar på patent;;;
;;104;Licenser;;1040;Licenser;;;
;;;;;1048;Ackumulerade nedskrivningar på licenser;;;
;;;;;1049;Ackumulerade avskrivningar på licenser;;;
;;105;Varumärken;;1050;Varumärken;;;
;;;;;1058;Ackumulerade nedskrivningar på varumärken;;;
;;;;;1059;Ackumulerade avskrivningar på varumärken;;;
;■;106;Hyresrätter, tomträtter och liknande;■;1060;Hyresrätter, tomträtter och liknande;;;
;;;;;1068;Ackumulerade nedskrivningar på hyresrätter, tomträtter och liknande;;;
;;;;■;1069;Ackumulerade avskrivningar på hyresrätter, tomträtter och liknande;;;
;;107;Goodwill;;1070;Goodwill;;;
;;;;;1078;Ackumulerade nedskrivningar på goodwill;;;
;;;;;1079;Ackumulerade avskrivningar på goodwill;;;
;;108;Förskott för immateriella anläggningstillgångar;;1080;Förskott för immateriella anläggningstillgångar;;;
;;;;[Ej K2];1081;Pågående projekt för immateriella anläggningstillgångar;;;
;;;;;1088;Förskott för immateriella anläggningstillgångar;;;
;;11;Byggnader och mark;;;;;;
;■;111;Byggnader;■;1110;Byggnader;;;
;;;;;1111;Byggnader på egen mark;;;
;;;;;1112;Byggnader på annans mark;;;
;;;;;1118;Ackumulerade nedskrivningar på byggnader;;;
;;;;■;1119;Ackumulerade avskrivningar på byggnader;;;
;;112;Förbättringsutgifter på annans fastighet;;1120;Förbättringsutgifter på annans fastighet;;;
;;;;;1129;Ackumulerade avskrivningar på förbättringsutgifter på annans fastighet;;;
;■;113;Mark;■;1130;Mark;;;
;;114;Tomter och obebyggda markområden;;1140;Tomter och obebyggda markområden;;;
;■;115;Markanläggningar;■;1150;Markanläggningar;;;
;;;;;1158;Ackumulerade nedskrivningar på markanläggningar;;;
;;;;■;1159;Ackumulerade avskrivningar på markanläggningar;;;
;;118;Pågående nyanläggningar och förskott för byggnader och mark;;1180;Pågående nyanläggningar och förskott för byggnader och mark;;;
;;;;;1181;Pågående ny-, till- och ombyggnad;;;
;;;;;1188;Förskott för byggnader och mark;;;
;;12;Maskiner och inventarier;;;;;;
;■;121;Maskiner och andra tekniska anläggningar;■;1210;Maskiner och andra tekniska anläggningar;;;
;;;;;1211;Maskiner;;;
;;;;;1213;Andra tekniska anläggningar;;;
;;;;;1218;Ackumulerade nedskrivningar på maskiner och andra tekniska anläggningar;;;
;;;;■;1219;Ackumulerade avskrivningar på maskiner och andra tekniska anläggningar;;;
;■;122;Inventarier och verktyg;■;1220;Inventarier och verktyg;;;
;;;;;1221;Inventarier;;;
;;;;;1222;Byggnadsinventarier;;;
;;;;;1223;Markinventarier;;;
;;;;;1225;Verktyg;;;
;;;;;1228;Ackumulerade nedskrivningar på inventarier och verktyg;;;
;;;;■;1229;Ackumulerade avskrivningar på inventarier och verktyg;;;
;;123;Installationer;;1230;Installationer;;;
;;;;;1231;Installationer på egen fastighet;;;
;;;;;1232;Installationer på annans fastig het;;;
;;;;;1238;Ackumulerade nedskrivningar på installationer;;;
;;;;;1239;Ackumulerade avskrivningar på installationer;;;
;■;124;Bilar och andra transportmedel;■;1240;Bilar och andra transportmedel;;;
;;;;;1241;Personbilar;;;
;;;;;1242;Lastbilar;;;
;;;;;1243;Truckar;;;
;;;;;1244;Arbetsmaskiner;;;
;;;;;1245;Traktorer;;;
;;;;;1246;Motorcyklar, mopeder och skotrar;;;
;;;;;1247;Båtar, flygplan och helikoptrar;;;
;;;;;1248;Ackumulerade nedskrivningar på bilar och andra transportmedel;;;
;;;;■;1249;Ackumulerade avskrivningar på bilar och andra transportmedel;;;
;■;125;Datorer;■;1250;Datorer;;;
;;;;;1251;Datorer, företaget;;;
;;;;;1257;Datorer, personal;;;
;;;;;1258;Ackumulerade nedskrivningar på datorer;;;
;;;;■;1259;Ackumulerade avskrivningar på datorer;;;
;[Ej K2];126;Leasade tillgångar;[Ej K2];1260;Leasade tillgångar;;;
;;;;[Ej K2];1269;Ackumulerade avskrivningar på leasade tillgångar;;;
;;128;Pågående nyanläggningar och förskott för maskiner och inventarier;;1280;Pågående nyanläggningar och förskott för maskiner och inventarier;;;
;;;;;1281;Pågående nyanläggningar, maskiner och inventarier;;;
;;;;;1288;Förskott för maskiner och inventarier;;;
;■;129;Övriga materiella anläggningstillgångar;■;1290;Övriga materiella anläggningstillgångar;;;
;;;;■;1291;Konst och liknande tillgångar;;;
;;;;;1292;Djur som klassificeras som anläggningstillgång;;;
;;;;;1298;Ackumulerade nedskrivningar på övriga materiella anläggningstillgångar;;;
;;;;■;1299;Ackumulerade avskrivningar på övriga materiella anläggningstillgångar;;;
;;13;Finansiella anläggningstillgångar;;;;;;
;;131;Andelar i koncernföretag;;1310;Andelar i koncernföretag;;;
;;;;;1311;Aktier i noterade svenska koncernföretag;;;
;;;;;1312;Aktier i onoterade svenska koncernföretag;;;
;;;;;1313;Aktier i noterade utländska koncernföretag;;;
;;;;;1314;Aktier i onoterade utländska koncernföretag;;;
;;;;;1316;Andra andelar i svenska koncernföretag;;;
;;;;;1317;Andra andelar i utländska koncernförertag;;;
;;;;;1318;Ackumulerade nedskrivningar av andelar i koncernföretag;;;
;;132;Långfristiga fordringar hos koncernföretag;;1320;Långfristiga fordringar hos koncernföretag;;;
;;;;;1321;Långfristiga fordringar hos moderföretag;;;
;;;;;1322;Långfristiga fordringar hos dotterföretag;;;
;;;;;1323;Långfristiga fordringar hos andra koncernföretag;;;
;;;;;1328;Ackumulerade nedskrivningar av långfristiga fordringar hos koncernföretag;;;
;;133;Andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;1330;Andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;1331;Andelar i intresseföretag;;;
;;;;;1332;Ackumulerade nedskrivningar av andelar i intresseföretag;;;
;;;;;1333;Andelar i gemensamt styrda företag;;;
;;;;;1334;Ackumulerade nedskrivningar av andelar i gemensamt styrda företag;;;
;;;;;1336;Andelar i övriga företag som det finns ett ägarintresse i;;;
;;;;;1337;Ackumulerade nedskrivningar av andelar i övriga företag som det finns ett ägarintresse i;;;
;;;;;1338;Ackumulerade nedskrivningar av andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;134;Långfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;1340;Långfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;1341;Långfristiga fordringar hos intresseföretag;;;
;;;;;1342;Ackumulerade nedskrivningar av långfristiga fordringar hos intresseföretag;;;
;;;;;1343;Långfristiga fordringar hos gemensamt styrda företag;;;
;;;;;1344;Ackumulerade nedskrivningar av långfristiga fordringar hos gemensamt styrda företag;;;
;;;;;1346;Långfristiga fordringar hos övriga företag som det finns ett ägarintresse i;;;
;;;;;1347;Ackumulerade nedskrivningar av långfristiga fordringar hos övriga företag som det finns ett ägarintresse i;;;
;;;;;1348;Ackumulerade nedskrivningar av långfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;■;135;Andelar och värdepapper i andra företag;■;1350;Andelar och värdepapper i andra företag;;;
;;;;;1351;Andelar i noterade företag;;;
;;;;;1352;Andra andelar;;;
;;;;;1353;Andelar i bostadsrättsföreningar;;;
;;;;;1354;Obligationer;;;
;;;;;1356;Andelar i ekonomiska föreningar, övriga företag;;;
;;;;;1357;Andelar i handelsbolag, andra företag;;;
;;;;;1358;Ackumulerade nedskrivningar av andra andelar och värdepapper;;;
;;136;Lån till delägare eller närstående enligt ABL, långfristig del;;1360;Lån till delägare eller närstående enligt ABL, långfristig del;;;
;;;;;1369;Ackumulerade nedskrivningar av lån till delägare eller närstående enligt ABL, långfristig del;;;
;[Ej K2];137;Uppskjuten skattefordran;[Ej K2];1370;Uppskjuten skattefordran;;;
;■;138;Andra långfristiga fordringar;■;1380;Andra långfristiga fordringar;;;
;;;;;1381;Långfristiga reversfordringar;;;
;;;;;1382;Långfristiga fordringar hos anställda;;;
;;;;;1383;Lämnade depositioner, långfristiga;;;
;;;;;1384;Derivat;;;
;;;;;1385;Kapitalförsäkring;|;;
;;;;;1387;Långfristiga kontraktsfordringar;;;
;;;;;1388;Långfristiga kundfordringar;;;
;;;;;1389;Ackumulerade nedskrivningar av andra långfristiga fordringar;;;
;;14;Lager, produkter i arbete och pågående arbeten;;;;;;
;■;141;Lager av råvaror;■;1410;Lager av råvaror;;;
;;;;■;1419;Förändring av lager av råvaror;;;
;;142;Lager av tillsatsmaterial och förnödenheter;;1420;Lager av tillsatsmaterial och förnödenheter;;;
;;;;;1429;Förändring av lager av tillsatsmaterial och förnödenheter;;;
;■;144;Produkter i arbete;■;1440;Produkter i arbete;;;
;;;;■;1449;Förändring av produkter i arbete;;;
;■;145;Lager av färdiga varor;■;1450;Lager av färdiga varor;;;
;;;;■;1459;Förändring av lager av färdiga varor;;;
;■;146;Lager av handelsvaror;■;1460;Lager av handelsvaror;;;
;;;;;1465;Lager av varor VMB;;;
;;;;;1466;Nedskrivning av varor VMB;;;
;;;;;1467;Lager av varor VMB förenklad;;;
;;;;■;1469;Förändring av lager av handelsvaror;;;
;■;147;Pågående arbeten;■;1470;Pågående arbeten;;;
;;;;;1471;Pågående arbeten, nedlagda kostnader;;;
;;;;;1478;Pågående arbeten, fakturering;;;
;;;;■;1479;Förändring av pågående arbeten;;;
;■;148;Förskott för varor och tjänster;■;1480;Förskott för varor och tjänster;;;
;;;;;1481;Remburser;;;
;;;;;1489;Övriga förskott till leverantörer;;;
;■;149;Övriga lagertillgångar;■;1490;Övriga lagertillgångar;;;
;;;;;1491;Lager av värdepapper;;;
;;;;;1492;Lager av fastigheter;;;
;;;;;1493;Djur som klassificeras som omsättningstillgång;;;
;;15;Kundfordringar;;;;;;
;■;151;Kundfordringar;■;1510;Kundfordringar;;;
;;;;;1511;Kundfordringar;;;
;;;;;1512;Belånade kundfordringar (factoring);;;
;;;;■;1513;Kundfordringar – delad faktura;;;
;;;;;1516;Tvistiga kundfordringar;;;
;;;;;1518;Ej reskontraförda kundfordringar;;;
;;;;■;1519;Nedskrivning av kundfordringar;;;
;;152;Växelfordringar;;1520;Växelfordringar;;;
;;;;;1525;Osäkra växelfordringar;;;
;;;;;1529;Nedskrivning av växelfordringar;;;
;;153;Kontraktsfordringar;;1530;Kontraktsfordringar;;;
;;;;;1531;Kontraktsfordringar;;;
;;;;;1532;Belånade kontraktsfordringar;;;
;;;;;1536;Tvistiga kontraktsfordringar;;;
;;;;;1539;Nedskrivning av kontraktsfordringar;;;
;;155;Konsignationsfordringar;;1550;Konsignationsfordringar;;;
;;156;Kundfordringar hos koncernföretag;;1560;Kundfordringar hos koncernföretag;;;
;;;;;1561;Kundfordringar hos moderföretag;;;
;;;;;1562;Kundfordringar hos dotterföretag;;;
;;;;;1563;Kundfordringar hos andra koncernföretag;;;
;;;;;1568;Ej reskontraförda kundfordringar hos koncernföretag;;;
;;;;;1569;Nedskrivning av kundfordringar hos koncernföretag;;;
;;157;Kundfordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;1570;Kundfordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;1571;Kundfordringar hos intresseföretag;;;
;;;;;1572;Kundfordringar hos gemensamt styrda företag;;;
;;;;;1573;Kundfordringar hos övriga företag som det finns ett ägarintresse i;;;
;■;158;Fordringar för kontokort och kuponger;■;1580;Fordringar för kontokort och kuponger;;;
;;16;Övriga kortfristiga fordringar;;;;;;
;■;161;Kortfristiga fordringar hos anställda;■;1610;Kortfristiga fordringar hos anställda;;;
;;;;;1611;Reseförskott;;;
;;;;;1612;Kassaförskott;;;
;;;;;1613;Övriga förskott;;;
;;;;;1614;Tillfälliga lån till anställda;;;
;;;;;1619;Övriga fordringar hos anställda;;;
;;162;Upparbetad men ej fakturerad intäkt;;1620;Upparbetad men ej fakturerad intäkt;;;
;■;163;Avräkning för skatter och avgifter (skattekonto);■;1630;Avräkning för skatter och avgifter (skattekonto);;;
;■;164;Skattefordringar;■;1640;Skattefordringar;;;
;■;165;Momsfordran;■;1650;Momsfordran;;;
;;166;Kortfristiga fordringar hos koncernföretag;;1660;Kortfristiga fordringar hos koncernföretag;;;
;;;;;1661;Kortfristiga fordringar hos moderföretag;;;
;;;;;1662;Kortfristiga fordringar hos dotterföretag;;;
;;;;;1663;Kortfristiga fordringar hos andra koncernföretag;;;
;;167;Kortfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;1670;Kortfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;1671;Kortfristiga fordringar hos intresseföretag;;;
;;;;;1672;Kortfristiga fordringar hos gemensamt styrda företag;;;
;;;;;1673;Kortfristiga fordringar hos övriga företag som det finns ett ägarintresse i;;;
;■;168;Andra kortfristiga fordringar;■;1680;Andra kortfristiga fordringar;;;
;;;;;1681;Utlägg för kunder;;;
;;;;;1682;Kortfristiga lånefordringar;;;
;;;;;1683;Derivat;;;
;;;;;1684;Kortfristiga fordringar hos leverantörer;;;
;;;;;1685;Kortfristiga fordringar hos delägare eller närstående;;;
;;;;;1687;Kortfristig del av långfristiga fordringar;;;
;;;;;1688;Fordran arbetsmarknadsförsäkringar;;;
;;;;;1689;Övriga kortfristiga fordringar;;;
;;169;Fordringar för tecknat men ej inbetalt aktiekapital;;1690;Fordringar för tecknat men ej inbetalt aktiekapital;;;
;;17;Förutbetalda kostnader och upplupna intäkter;;;;;;
;■;171;Förutbetalda hyreskostnader;■;1710;Förutbetalda hyreskostnader;;;
;■;172;Förutbetalda leasingavgifter;■;1720;Förutbetalda leasingavgifter;;;
;■;173;Förutbetalda försäkringspremier;■;1730;Förutbetalda försäkringspremier;;;
;■;174;Förutbetalda räntekostnader;■;1740;Förutbetalda räntekostnader;;;
;■;175;Upplupna hyresintäkter;■;1750;Upplupna hyresintäkter;;;
;■;176;Upplupna ränteintäkter;■;1760;Upplupna ränteintäkter;;;
;;177;Tillgångar av kostnadsnatur;;1770;Tillgångar av kostnadsnatur;;;
;;178;Upplupna avtalsintäkter;;1780;Upplupna avtalsintäkter;;;
;■;179;Övriga förutbetalda kostnader och upplupna intäkter;■;1790;Övriga förutbetalda kostnader och upplupna intäkter;;;
;;18;Kortfristiga placeringar;;;;;;
;■;181;Andelar i börsnoterade företag;■;1810;Andelar i börsnoterade företag;;;
;;182;Obligationer;;1820;Obligationer;;;
;;183;Konvertibla skuldebrev;;1830;Konvertibla skuldebrev;;;
;;186;Andelar i koncernföretag, kortfristigt;;1860;Andelar i koncernföretag, kortfristigt;;;
;■;188;Andra kortfristiga placeringar;■;1880;Andra kortfristiga placeringar;;;
;;;;;1886;Derivat;;;
;;;;;1889;Andelar i övriga företag;;;
;■;189;Nedskrivning av kortfristiga placeringar;■;1890;Nedskrivning av kortfristiga placeringar;;;
;;19;Kassa och bank;;;;;;
;■;191;Kassa;■;1910;Kassa;;;
;;;;;1911;Huvudkassa;;;
;;;;;1912;Kassa 2;;;
;;;;;1913;Kassa 3;;;
;■;192;PlusGiro;■;1920;PlusGiro;;;
;■;193;Företagskonto/checkkonto/affärskonto;■;1930;Företagskonto/checkkonto/affärskonto;;;
;■;194;Övriga bankkonton;■;1940;Övriga bankkonton;;;
;;195;Bankcertifikat;;1950;Bankcertifikat;;;
;;196;Koncernkonto moderföretag;;1960;Koncernkonto moderföretag;;;
;;197;Särskilda bankkonton;;1970;Särskilda bankkonton;;;
;;;;;1972;Upphovsmannakonto;;;
;;;;;1973;Skogskonto;;;)"
R"(;;;;; 1974; Spärrade bankmedel;;;
;;;;;1979;Övriga särskilda bankkonton;;;
;;198;Valutakonton;;1980;Valutakonton;;;
;;199;Redovisningsmedel;;1990;Redovisningsmedel;;;
;;2;Eget kapital och skulder;;;;;;
;;20;Eget kapital;;;;;;
;;201;Eget kapital (enskild firma);■;2010;Eget kapital;;;
;;;;■;2011;Egna varuuttag;;;
;;;;■;2013;Övriga egna uttag;;;
;;;;■;2017;Årets kapitaltillskott;;;
;;;;■;2018;Övriga egna insättningar;;;
;;;;■;2019;Årets resultat;;;
;;201;Eget kapital, delägare 1;■;2010;Eget kapital;;;
;;;;■;2011;Egna varuuttag;;;
;;;;■;2013;Övriga egna uttag;;;
;;;;■;2017;Årets kapitaltillskott;;;
;;;;■;2018;Övriga egna insättningar;;;
;;;;■;2019;Årets resultat, delägare 1;;;
;;202;Eget kapital, delägare 2;■;2020;Eget kapital;;;
;;;;■;2021;Egna varuuttag;;;
;;;;■;2023;Övriga egna uttag;;;
;;;;■;2027;Årets kapitaltillskott;;;
;;;;■;2028;Övriga egna insättningar;;;
;;;;■;2029;Årets resultat, delägare 2;;;
;;203;Eget kapital, delägare 3;■;2030;Eget kapital;;;
;;;;■;2031;Egna varuuttag;;;
;;;;■;2033;Övriga egna uttag;;;
;;;;■;2037;Årets kapitaltillskott;;;
;;;;■;2038;Övriga egna insättningar;;;
;;;;■;2039;Årets resultat, delägare 3;;;
;;204;Eget kapital, delägare 4;■;2040;Eget kapital;;;
;;;;■;2041;Egna varuuttag;;;
;;;;■;2043;Övriga egna uttag;;;
;;;;■;2047;Årets kapitaltillskott;;;
;;;;■;2048;Övriga egna insättningar;;;
;;;;■;2049;Årets resultat, delägare 4;;;
;;205;Avsättning till expansionsfond;;2050;Avsättning till expansionsfond;;;
;■;206;Eget kapital i ideella föreningar, stiftelser och registrerade trossamfund;■;2060;Eget kapital i ideella föreningar, stiftelser och registrerade trossamfund;;;
;;;;;2061;Eget kapital/stiftelsekapital/grundkapital;;;
;;;;;2065;Förändring i fond för verkligt värde;;;
;;;;;2066;Värdesäkringsfond;;;
;;;;;2067;Balanserad vinst eller förlust/balanserat kapital;;;
;;;;;2068;Vinst eller förlust från föregående år;;;
;;;;;2069;Årets resultat;;;
;■;207;Ändamålsbestämda medel;■;2070;Ändamålsbestämda medel;;;
;;;;;2071;Ändamål 1;;;
;;;;;2072;Ändamål 2;;;
;;208;Bundet eget kapital;;2080;Bundet eget kapital;;;
;;;;■;2081;Aktiekapital;;;
;;;;;2082;Ej registrerat aktiekapital;;;
;;;;■;2083;Medlemsinsatser;;;
;;;;;2084;Förlagsinsatser;;;
;;;;;2085;Uppskrivningsfond;;;
;;;;■;2086;Reservfond;;;
;;;;;2087;Insatsemission;;;
;;;;;2087;Bunden överkursfond;;;
;;;;;2088;Fond för yttre underhåll;;;
;;;;[Ej K2];2089;Fond för utvecklingsutgifter;;;
;■;209;Fritt eget kapital;■;2090;Fritt eget kapital;;;
;;;;■;2091;Balanserad vinst eller förlust;;;
;;;;[Ej K2];2092;Mottagna/lämnade koncernbidrag;;;
;;;;;2093;Erhållna aktieägartillskott;;;
;;;;;2094;Egna aktier;;;
;;;;;2095;Fusionsresultat;;;
;;;;[Ej K2];2096;Fond för verkligt värde;;;
;;;;;2097;Fri överkursfond;;;
;;;;■;2098;Vinst eller förlust från föregående år;;;
;;;;■;2099;Årets resultat;;;
;;21;Obeskattade reserver;;;;;;
;;211;Periodiseringsfonder;;2110;Periodiseringsfonder;;;
;■;212;Periodiseringsfond 2020;■;2120;Periodiseringsfond 2020;;;
;;;;■;2121;Periodiseringsfond 2021;;;
;;;;■;2122;Periodiseringsfond 2022;;;
;;;;■;2123;Periodiseringsfond 2023;;;
;;;;■;2125;Periodiseringsfond 2015;;;
;;;;■;2126;Periodiseringsfond 2016;;;
;;;;■;2127;Periodiseringsfond 2017;;;
;;;;■;2128;Periodiseringsfond 2018;;;
;;;;■;2129;Periodiseringsfond 2019;;;
;;213;Periodiseringsfond 2020 – nr 2;;2130;Periodiseringsfond 2020 – nr 2;;;
;;;;;2131;Periodiseringsfond 2021 – nr 2;;;
;;;;;2132;Periodiseringsfond 2022 – nr 2;;;
;;;;;2133;Periodiseringsfond 2023 – nr 2;;;
;;;;;2134;Periodiseringsfond 2024 – nr 2;;;
;;;;;2135;Periodiseringsfond 2015 – nr 2;;;
;;;;;2136;Periodiseringsfond 2016 – nr 2;;;
;;;;;2137;Periodiseringsfond 2017 – nr 2;;;
;;;;;2138;Periodiseringsfond 2018 – nr 2;;;
;;;;;2139;Periodiseringsfond 2019 – nr 2;;;
;■;215;Ackumulerade överavskrivningar;■;2150;Ackumulerade överavskrivningar;;;
;;;;;2151;Ackumulerade överavskrivningar på immateriella anläggningstillgångar;;;
;;;;;2152;Ackumulerade överavskrivningar på byggnader och markanläggningar;;;
;;;;;2153;Ackumulerade överavskrivningar på maskiner och inventarier;;;
;;216;Ersättningsfond;;2160;Ersättningsfond;;;
;;;;;2161;Ersättningsfond maskiner och inventarier;;;
;;;;;2162;Ersättningsfond byggnader och markanläggningar;;;
;;;;;2164;Ersättningsfond för djurlager i jordbruk och renskötsel;;;
;;219;Övriga obeskattade reserver;;2190;Övriga obeskattade reserver;;;
;;;;;2196;Lagerreserv;;;
;;;;;2199;Övriga obeskattade reserver;;;
;;22;Avsättningar;;;;;;
;■;221;Avsättningar för pensioner enligt tryggandelagen;■;2210;Avsättningar för pensioner enligt tryggandelagen;;;
;■;222;Avsättningar för garantier;■;2220;Avsättningar för garantier;;;
;;223;Övriga avsättningar för pensioner och liknande förpliktelser;;2230;Övriga avsättningar för pensioner och liknande förpliktelser;;;
;[Ej K2];224;Avsättningar för uppskjutna skatter;[Ej K2];2240;Avsättningar för uppskjutna skatter;;;
;;225;Övriga avsättningar för skatter;;2250;Övriga avsättningar för skatter;;;
;;;;;2252;Avsättningar för tvistiga skatter;;;
;;;;;2253;Avsättningar särskild löneskatt, deklarationspost;;;
;■;229;Övriga avsättningar;■;2290;Övriga avsättningar;;;
;;23;Långfristiga skulder;;;;;;
;;231;Obligations- och förlagslån;;2310;Obligations- och förlagslån;;;
;;232;Konvertibla lån och liknande;;2320;Konvertibla lån och liknande;;;
;;;;;2321;Konvertibla lån;;;
;;;;;2322;Lån förenade med optionsrätt;;;
;;;;;2323;Vinstandelslån;;;
;;;;;2324;Kapitalandelslån;;;
;■;233;Checkräkningskredit;■;2330;Checkräkningskredit;;;
;;;;;2331;Checkräkningskredit 1;;;
;;;;;2332;Checkräkningskredit 2;;;
;;234;Byggnadskreditiv;;2340;Byggnadskreditiv;;;
;■;235;Andra långfristiga skulder till kreditinstitut;■;2350;Andra långfristiga skulder till kreditinstitut;;;
;;;;;2351;Fastighetslån, långfristig del;;;
;;;;;2355;Långfristiga lån i utländsk valuta från kreditinstitut;;;
;;;;;2359;Övriga långfristiga lån från kreditinstitut;;;
;;236;Långfristiga skulder till koncernföretag;;2360;Långfristiga skulder till koncernföretag;;;
;;;;;2361;Långfristiga skulder till moderföretag;;;
;;;;;2362;Långfristiga skulder till dotterföretag;;;
;;;;;2363;Långfristiga skulder till andra koncernföretag;;;
;;237;Långfristiga skulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;2370;Långfristiga skulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;2371;Långfristiga skulder till intresseföretag;;;
;;;;;2372;Långfristiga skulder till gemensamt styrda företag;;;
;;;;;2373;Långfristiga skulder till övriga företag som det finns ett ägarintresse i;;;
;■;239;Övriga långfristiga skulder;■;2390;Övriga långfristiga skulder;;;
;;;;;2391;Avbetalningskontrakt, långfristig del;;;
;;;;;2392;Villkorliga långfristiga skulder;;;
;;;;■;2393;Lån från närstående personer, långfristig del;;;
;;;;;2394;Långfristiga leverantörskrediter;;;
;;;;;2395;Andra långfristiga lån i utländsk valuta;;;
;;;;;2396;Derivat;;;
;;;;;2397;Mottagna depositioner, långfristiga;;;
;;;;;2399;Övriga långfristiga skulder;;;
;;24;Kortfristiga skulder till kreditinstitut, kunder och leverantörer;;;;;;
;■;241;Andra kortfristiga låneskulder till kreditinstitut;■;2410;Andra kortfristiga låneskulder till kreditinstitut;;;
;;;;;2411;Kortfristiga lån från kreditinstitut;;;
;;;;;2412;Byggnadskreditiv, kortfristig del;;;
;;;;;2417;Kortfristig del av långfristiga skulder till kreditinstitut;;;
;;;;;2419;Övriga kortfristiga skulder till kreditinstitut;;;
;■;242;Förskott från kunder;■;2420;Förskott från kunder;;;
;;;;;2421;Ej inlösta presentkort;;;
;;;;;2429;Övriga förskott från kunder;;;
;;243;Pågående arbeten;;2430;Pågående arbeten;;;
;;;;;2431;Pågående arbeten, fakturering;;;
;;;;;2438;Pågående arbeten, nedlagda kostnader;;;
;;;;;2439;Beräknad förändring av pågående arbeten;;;
;■;244;Leverantörsskulder;■;2440;Leverantörsskulder;;;
;;;;;2441;Leverantörsskulder;;;
;;;;;2443;Konsignationsskulder;;;
;;;;;2445;Tvistiga leverantörsskulder;;;
;;;;;2448;Ej reskontraförda leverantörsskulder;;;
;;245;Fakturerad men ej upparbetad intäkt;;2450;Fakturerad men ej upparbetad intäkt;;;
;;246;Leverantörsskulder till koncernföretag;;2460;Leverantörsskulder till koncernföretag;;;
;;;;;2461;Leverantörsskulder till moderföretag;;;
;;;;;2462;Leverantörsskulder till dotterföretag;;;
;;;;;2463;Leverantörsskulder till andra koncernföretag;;;
;;247;Leverantörsskulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;2470;Leverantörsskulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;2471;Leverantörsskulder till intresseföretag;;;
;;;;;2472;Leverantörsskulder till gemensamt styrda företag;;;
;;;;;2473;Leverantörsskulder till övriga företag som det finns ett ägarintresse i;;;
;■;248;Checkräkningskredit, kortfristig;■;2480;Checkräkningskredit, kortfristig;;;
;■;249;Övriga kortfristiga skulder till kreditinstitut, kunder och leverantörer;■;2490;Övriga kortfristiga skulder till kreditinstitut, kunder och leverantörer;;;
;;;;;2491;Avräkning spelarrangörer;;;
;;;;;2492;Växelskulder;;;
;;;;;2499;Andra övriga kortfristiga skulder;;;
;;25;Skatteskulder;;;;;;
;■;251;Skatteskulder;■;2510;Skatteskulder;;;
;;;;;2512;Beräknad inkomstskatt;;;
;;;;;2513;Beräknad fastighetsskatt/fastighetsavgift;;;
;;;;;2514;Beräknad särskild löneskatt på pensionskostnader;;;
;;;;;2515;Beräknad avkastningsskatt;;;
;;;;;2517;Beräknad utländsk skatt;;;
;;;;;2518;Betald F-skatt;;;
;;26;Moms och särskilda punktskatter;;;;;;
;■;261;Utgående moms, 25 %;■;2610;Utgående moms, 25 %;;;
;;;;■;2611;Utgående moms på försäljning inom Sverige, 25 %;;;
;;;;■;2612;Utgående moms på egna uttag, 25 %;;;
;;;;■;2613;Utgående moms för uthyrning, 25 %;;;
;;;;■;2614;Utgående moms omvänd skattskyldighet, 25 %;;;
;;;;■;2615;Utgående moms import av varor, 25 %;;;
;;;;■;2616;Utgående moms VMB 25 %;;;
;;;;;2618;Vilande utgående moms, 25 %;;;
;■;262;Utgående moms, 12 %;■;2620;Utgående moms, 12 %;;;
;;;;■;2621;Utgående moms på försäljning inom Sverige, 12 %;;;
;;;;■;2622;Utgående moms på egna uttag, 12 %;;;
;;;;■;2623;Utgående moms för uthyrning, 12 %;;;
;;;;■;2624;Utgående moms omvänd skattskyldighet, 12 %;;;
;;;;■;2625;Utgående moms import av varor, 12 %;;;
;;;;■;2626;Utgående moms VMB 12 %;;;
;;;;;2628;Vilande utgående moms, 12 %;;;
;■;263;Utgående moms, 6 %;■;2630;Utgående moms, 6 %;;;
;;;;■;2631;Utgående moms på försäljning inom Sverige, 6 %;;;
;;;;■;2632;Utgående moms på egna uttag, 6 %;;;
;;;;■;2633;Utgående moms för uthyrning, 6 %;;;
;;;;■;2634;Utgående moms omvänd skattskyldighet, 6 %;;;
;;;;■;2635;Utgående moms import av varor, 6 %;;;
;;;;■;2636;Utgående moms VMB 6 %;;;
;;;;;2638;Vilande utgående moms, 6 %;;;
;■;264;Ingående moms;■;2640;Ingående moms;;;
;;;;■;2641;Debiterad ingående moms;;;
;;;;■;2642;Debiterad ingående moms i anslutning till frivillig skattskyldighet;;;
;;;;■;2645;Beräknad ingående moms på förvärv från utlandet;;;
;;;;■;2646;Ingående moms på uthyrning;;;
;;;;■;2647;Ingående moms omvänd skattskyldighet varor och tjänster i Sverige;;;
;;;;■;2648;Vilande ingående moms;;;
;;;;■;2649;Ingående moms, blandad verksamhet;;;
;■;265;Redovisningskonto för moms;■;2650;Redovisningskonto för moms;;;
;;266;Särskilda punktskatter;;2660;Särskilda punktskatter;;;
;;;;;2661;Reklamskatt;;;
;;;;;2669;Övriga punktskatter;;;
;;27;Personalens skatter, avgifter och löneavdrag;;;;;;
;■;271;Personalskatt;■;2710;Personalskatt;;;
;■;273;Lagstadgade sociala avgifter och särskild löneskatt;■;2730;Lagstadgade sociala avgifter och särskild löneskatt;;;
;;;;;2731;Avräkning lagstadgade sociala avgifter;;;
;;;;;2732;Avräkning särskild löneskatt;;;
;■;274;Avtalade sociala avgifter;■;2740;Avtalade sociala avgifter;;;
;;275;Utmätning i lön m.m.;;2750;Utmätning i lön m.m.;;;
;;276;Semestermedel;;2760;Semestermedel;;;
;;;;;2761;Avräkning semesterlöner;;;
;;;;;2762;Semesterlönekassa;;;
;■;279;Övriga löneavdrag;■;2790;Övriga löneavdrag;;;
;;;;;2791;Personalens intressekonto;;;
;;;;;2792;Lönsparande;;;
;;;;;2793;Gruppförsäkringspremier;;;
;;;;;2794;Fackföreningsavgifter;;;
;;;;;2795;Mätnings- och granskningsarvoden;;;
;;;;;2799;Övriga löneavdrag;;;
;;28;Övriga kortfristiga skulder;;;;;;
;;281;Avräkning för factoring och belånade kontraktsfordringar;;2810;Avräkning för factoring och belånade kontraktsfordringar;;;
;;;;;2811;Avräkning för factoring;;;
;;;;;2812;Avräkning för belånade kontraktsfordringar;;;
;■;282;Kortfristiga skulder till anställda;■;2820;Kortfristiga skulder till anställda;;;
;;;;;2821;Löneskulder;;;
;;;;;2822;Reseräkningar;;;
;;;;;2823;Tantiem, gratifikationer;;;
;;;;;2829;Övriga kortfristiga skulder till anställda;;;
;;283;Avräkning för annans räkning;;2830;Avräkning för annans räkning;;;
;■;284;Kortfristiga låneskulder;■;2840;Kortfristiga låneskulder;;;
;;;;;2841;Kortfristig del av långfristiga skulder;;;
;;;;;2849;Övriga kortfristiga låneskulder;;;
;;285;Avräkning för skatter och avgifter (skattekonto);;2850;Avräkning för skatter och avgifter (skattekonto);;;
;;;;;2852;Anståndsbelopp för moms, arbetsgivaravgifter och personalskatt;;;
;;286;Kortfristiga skulder till koncernföretag;;2860;Kortfristiga skulder till koncernföretag;;;
;;;;;2861;Kortfristiga skulder till moderföretag;;;
;;;;;2862;Kortfristiga skulder till dotterföretag;;;
;;;;;2863;Kortfristiga skulder till andra koncernföretag;;;
;;287;Kortfristiga skulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;2870;Kortfristiga skulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;2871;Kortfristiga skulder till intresseföretag;;;
;;;;;2872;Kortfristiga skulder till gemensamt styrda företag;;;
;;;;;2873;Kortfristiga skulder till övriga företag som det finns ett ägarintresse i;;;
;;288;Skuld erhållna bidrag;;2880;Skuld erhållna bidrag;;;
;■;289;Övriga kortfristiga skulder;■;2890;Övriga kortfristiga skulder;;;
;;;;;2891;Skulder under indrivning;;;
;;;;;2892;Inre reparationsfond/underhållsfond;;;
;;;;;2893;Skulder till närstående personer, kortfristig del;;;
;;;;;2895;Derivat (kortfristiga skulder);;;
;;;;;2897;Mottagna depositioner, kortfristiga;;;
;;;;;2898;Outtagen vinstutdelning;;;
;;;;;2899;Övriga kortfristiga skulder;;;
;;29;Upplupna kostnader och förutbetalda intäkter;;;;;;
;■;291;Upplupna löner;■;2910;Upplupna löner;;;
;;;;;2911;Löneskulder;;;
;;;;;2912;Ackordsöverskott;;;
;;;;;2919;Övriga upplupna löner;;;
;■;292;Upplupna semesterlöner;■;2920;Upplupna semesterlöner;;;
;;293;Upplupna pensionskostnader;;2930;Upplupna pensionskostnader;;;
;;;;;2931;Upplupna pensionsutbetalningar;;;
;■;294;Upplupna lagstadgade sociala och andra avgifter;■;2940;Upplupna lagstadgade sociala och andra avgifter;;;
;;;;;2941;Beräknade upplupna lagstadgade sociala avgifter;;;
;;;;;2942;Beräknad upplupen särskild löneskatt;;;
;;;;;2943;Beräknad upplupen särskild löneskatt på pensionskostnader, deklarationspost;;;
;;;;;2944;Beräknad upplupen avkastningsskatt på pensionskostnader;;;
;■;295;Upplupna avtalade sociala avgifter;■;2950;Upplupna avtalade sociala avgifter;;;
;;;;;2951;Upplupna avtalade arbetsmarknadsförsäkringar;;;
;;;;;2959;Upplupna avtalade pensionsförsäkringsavgifter, deklarationspost;;;
;■;296;Upplupna räntekostnader;■;2960;Upplupna räntekostnader;;;
;■;297;Förutbetalda intäkter;■;2970;Förutbetalda intäkter;;;
;;;;;2971;Förutbetalda hyresintäkter;;;
;;;;;2972;Förutbetalda medlemsavgifter;;;
;;;;;2979;Övriga förutbetalda intäkter;;;
;;298;Upplupna avtalskostnader;;2980;Upplupna avtalskostnader;;;)"
R"(; ■; 299; Övriga upplupna kostnader och förutbetalda intäkter; ■; 2990; Övriga upplupna kostnader och förutbetalda intäkter;;;
;;;;;2991;Beräknat arvode för bokslut;;;
;;;;;2992;Beräknat arvode för revision;;;
;;;;;2993;Ospecificerad skuld till leverantörer;;;
;;;;;2998;Övriga upplupna kostnader och förutbetalda intäkter;;;
;;;;■;2999;OBS-konto;;;
;;3;Rörelsens inkomster/intäkter;;;;;;
;;30;Huvudintäkter;;;;;;
;■;300;Försäljning inom Sverige;■;3000;Försäljning inom Sverige;;;
;;;;■;3001;Försäljning inom Sverige, 25 % moms;;;
;;;;■;3002;Försäljning inom Sverige, 12 % moms;;;
;;;;■;3003;Försäljning inom Sverige, 6 % moms;;;
;;;;■;3004;Försäljning inom Sverige, momsfri;;;
;;31;Huvudintäkter;;;;;;
;■;310;Försäljning av varor utanför Sverige;■;3100;Försäljning av varor utanför Sverige;;;
;;;;■;3105;Försäljning varor till land utanför EU;;;
;;;;■;3106;Försäljning varor till annat EU-land, momspliktig;;;
;;;;■;3108;Försäljning varor till annat EU-land, momsfri;;;
;;32;Huvudintäkter;;;;;;
;■;320;Försäljning VMB och omvänd moms;■;3200;Försäljning VMB och omvänd moms;;;
;■;321;Försäljning positiv VMB 25 %;■;3211;Försäljning positiv VMB 25 %;;;
;;;;■;3212;Försäljning negativ VMB 25 %;;;
;■;323;Försäljning inom byggsektorn, omvänd skattskyldighet moms;■;3231;Försäljning inom byggsektorn, omvänd skattskyldighet moms;;;
;;33;Huvudintäkter;;;;;;
;■;330;Försäljning av tjänster utanför Sverige;■;3300;Försäljning av tjänster utanför Sverige;;;
;;;;■;3305;Försäljning tjänster till land utanför EU;;;
;;;;■;3308;Försäljning tjänster till annat EU-land;;;
;;34;Huvudintäkter;;;;;;
;■;340;Försäljning, egna uttag;■;3400;Försäljning, egna uttag;;;
;;;;■;3401;Egna uttag momspliktiga, 25 %;;;
;;;;■;3402;Egna uttag momspliktiga, 12 %;;;
;;;;■;3403;Egna uttag momspliktiga, 6 %;;;
;;;;■;3404;Egna uttag, momsfria;;;
;;35;Fakturerade kostnader;;;;;;
;■;350;Fakturerade kostnader (gruppkonto);■;3500;Fakturerade kostnader (gruppkonto);;;
;■;351;Fakturerat emballage;■;3510;Fakturerat emballage;;;
;;;;;3511;Fakturerat emballage;;;
;;;;;3518;Returnerat emballage;;;
;■;352;Fakturerade frakter;■;3520;Fakturerade frakter;;;
;;;;■;3521;Fakturerade frakter, EU-land;;;
;;;;■;3522;Fakturerade frakter, export;;;
;■;353;Fakturerade tull- och speditionskostnader m.m.;■;3530;Fakturerade tull- och speditionskostnader m.m.;;;
;■;354;Faktureringsavgifter;■;3540;Faktureringsavgifter;;;
;;;;■;3541;Faktureringsavgifter, EU-land;;;
;;;;■;3542;Faktureringsavgifter, export;;;
;;355;Fakturerade resekostnader;;3550;Fakturerade resekostnader;;;
;;356;Fakturerade kostnader till koncernföretag;;3560;Fakturerade kostnader till koncernföretag;;;
;;;;;3561;Fakturerade kostnader till moderföretag;;;
;;;;;3562;Fakturerade kostnader till dotterföretag;;;
;;;;;3563;Fakturerade kostnader till andra koncernföretag;;;
;;357;Fakturerade kostnader till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;3570;Fakturerade kostnader till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;359;Övriga fakturerade kostnader;;3590;Övriga fakturerade kostnader;;;
;;36;Rörelsens sidointäkter;;;;;;
;■;360;Rörelsens sidointäkter (gruppkonto);■;3600;Rörelsens sidointäkter (gruppkonto);;;
;;361;Försäljning av material;;3610;Försäljning av material;;;
;;;;;3611;Försäljning av råmaterial;;;
;;;;;3612;Försäljning av skrot;;;
;;;;;3613;Försäljning av förbrukningsmaterial;;;
;;;;;3619;Försäljning av övrigt material;;;
;;362;Tillfällig uthyrning av personal;;3620;Tillfällig uthyrning av personal;;;
;;363;Tillfällig uthyrning av transportmedel;;3630;Tillfällig uthyrning av transportmedel;;;
;;367;Intäkter från värdepapper;;3670;Intäkter från värdepapper;;;
;;;;;3671;Försäljning av värdepapper;;;
;;;;;3672;Utdelning från värdepapper;;;
;;;;;3679;Övriga intäkter från värdepapper;;;
;;368;Management fees;;3680;Management fees;;;
;;369;Övriga sidointäkter;;3690;Övriga sidointäkter;;;
;;37;Intäktskorrigeringar;;;;;;
;;370;Intäktskorrigeringar (gruppkonto);;3700;Intäktskorrigeringar (gruppkonto);;;
;;371;Ofördelade intäktsreduktioner;;3710;Ofördelade intäktsreduktioner;;;
;■;373;Lämnade rabatter;■;3730;Lämnade rabatter;;;
;;;;;3731;Lämnade kassarabatter;;;
;;;;;3732;Lämnade mängdrabatter;;;
;■;374;Öres- och kronutjämning;■;3740;Öres- och kronutjämning;;;
;;375;Punktskatter;;3750;Punktskatter;;;
;;;;;3751;Intäktsförda punktskatter (kreditkonto);;;
;;;;;3752;Skuldförda punktskatter (debetkonto);;;
;;379;Övriga intäktskorrigeringar;;3790;Övriga intäktskorrigeringar;;;
;;38;Aktiverat arbete för egen räkning;;;;;;
;■;380;Aktiverat arbete för egen räkning (gruppkonto);■;3800;Aktiverat arbete för egen räkning (gruppkonto);;;
;;384;Aktiverat arbete (material);;3840;Aktiverat arbete (material);;;
;;385;Aktiverat arbete (omkostnader);;3850;Aktiverat arbete (omkostnader);;;
;;387;Aktiverat arbete (personal);;3870;Aktiverat arbete (personal);;;
;;39;Övriga rörelseintäkter;;;;;;
;■;390;Övriga rörelseintäkter (gruppkonto);■;3900;Övriga rörelseintäkter (gruppkonto);;;
;;391;Hyres- och arrendeintäkter;;3910;Hyres- och arrendeintäkter;;;
;;;;;3911;Hyresintäkter;;;
;;;;;3912;Arrendeintäkter;;;
;;;;■;3913;Frivilligt momspliktiga hyresintäkter;;;
;;;;;3914;Övriga momspliktiga hyresintäkter;;;
;;392;Provisionsintäkter, licensintäkter och royalties;;3920;Provisionsintäkter, licensintäkter och royalties;;;
;;;;;3921;Provisionsintäkter;;;
;;;;;3922;Licensintäkter och royalties;;;
;;;;;3925;Franchiseintäkter;;;
;[Ej K2];394;Orealiserade negativa/positiva värdeförändringar på säkringsinstrument;[Ej K2];3940;Orealiserade negativa/positiva värdeförändringar på säkringsinstrument;;;
;;395;Återvunna, tidigare avskrivna kundfordringar;;3950;Återvunna, tidigare avskrivna kundfordringar;;;
;■;396;Valutakursvinster på fordringar och skulder av rörelsekaraktär;■;3960;Valutakursvinster på fordringar och skulder av rörelsekaraktär;;;
;■;397;Vinst vid avyttring av immateriella och materiella anläggningstillgångar;■;3970;Vinst vid avyttring av immateriella och materiella anläggningstillgångar;;;
;;;;;3971;Vinst vid avyttring av immateriella anläggningstillgångar;;;
;;;;;3972;Vinst vid avyttring av byggnader och mark;;;
;;;;;3973;Vinst vid avyttring av maskiner och inventarier;;;
;■;398;Erhållna offentliga bidrag;■;3980;Erhållna offentliga bidrag;|;;
;;;;;3981;Erhållna EU-bidrag;;;
;;;;;3985;Erhållna statliga bidrag;|;;
;;;;;3987;Erhållna kommunala bidrag;;;
;;;;;3988;Erhållna offentliga bidrag för personal;|;;
;;;;;3989;Övriga erhållna offentliga bidrag;|;;
;;399;Övriga ersättningar, bidrag och intäkter;;3990;Övriga ersättningar, bidrag och intäkter;|;;
;;;;;3991;Konfliktersättning;;;
;;;;;3992;Erhållna skadestånd;;;
;;;;;3993;Erhållna donationer och gåvor;;;
;;;;;3994;Försäkringsersättningar;;;
;;;;;3995;Erhållet ackord på skulder av rörelsekaraktär;;;
;;;;;3996;Erhållna reklambidrag;;;
;;;;;3997;Sjuklöneersättning;|;;
;;;;;3998;Återbäring av överskott från försäkringsföretag;|;;
;;;;;3999;Övriga rörelseintäkter;;;
;;4;Utgifter/kostnader för varor, material och vissa köpta tjänster;;;;;;
;;40;Inköp av varor och material;;;;;;
;■;400;Inköp av varor från Sverige;■;4000;Inköp av varor från Sverige;;;
;;41;Inköp av varor och material;;;;;;
;;42;Inköp av varor och material;;;;;;
;■;420;Sålda varor VMB;■;4200;Sålda varor VMB;;;
;■;421;Sålda varor positiv VMB 25 %;■;4211;Sålda varor positiv VMB 25 %;;;
;;;;■;4212;Sålda varor negativ VMB 25 %;;;
;;43;Inköp av varor och material;;;;;;
;;44;Inköp av varor och material;;;;;;
;■;440;Momspliktiga inköp i Sverige;■;4400;Momspliktiga inköp i Sverige;;;
;■;441;Inköpta varor i Sverige, omvänd skattskyldighet, 25 % moms;■;4415;Inköpta varor i Sverige, omvänd skattskyldighet, 25 % moms;;;
;;;;;4416;Inköpta varor i Sverige, omvänd skattskyldighet, 12 % moms;;;
;;;;;4417;Inköpta varor i Sverige, omvänd skattskyldighet, 6 % moms;;;
;;442;Inköpta tjänster i Sverige, omvänd skattskyldighet, 25 % moms;;4425;Inköpta tjänster i Sverige, omvänd skattskyldighet, 25 % moms;;;
;;;;■;4426;Inköpta tjänster i Sverige, omvänd skattskyldighet, 12 %;;;
;;;;■;4427;Inköpta tjänster i Sverige, omvänd skattskyldighet, 6 %;;;
;;45;Inköp av varor och material;;;;;;
;■;450;Övriga momspliktiga inköp;■;4500;Övriga momspliktiga inköp;;;
;■;451;Inköp av varor från annat EU-land, 25 %;■;4515;Inköp av varor från annat EU-land, 25 %;;;
;;;;■;4516;Inköp av varor från annat EU-land, 12 %;;;
;;;;■;4517;Inköp av varor från annat EU-land, 6 %;;;
;;;;■;4518;Inköp av varor från annat EU-land, momsfri;;;
;■;453;Inköp av tjänster från ett land utanför EU, 25 % moms;■;4531;Inköp av tjänster från ett land utanför EU, 25 % moms;;;
;;;;■;4532;Inköp av tjänster från ett land utanför EU, 12 % moms;;;
;;;;■;4533;Inköp av tjänster från ett land utanför EU, 6 % moms;;;
;;;;■;4535;Inköp av tjänster från annat EU-land, 25 %;;;
;;;;■;4536;Inköp av tjänster från annat EU-land, 12 %;;;
;;;;■;4537;Inköp av tjänster från annat EU-land, 6 %;;;
;;;;■;4538;Inköp av tjänster från annat EU-land, momsfri;;;
;■;454;Import av varor, 25 % moms;■;4545;Import av varor, 25 % moms;;;
;;;;■;4546;Import av varor, 12 % moms;;;
;;;;■;4547;Import av varor, 6 % moms;;;
;;46;Legoarbeten, underentreprenader;;;;;;
;■;460;Legoarbeten och underentreprenader (gruppkonto);■;4600;Legoarbeten och underentreprenader (gruppkonto);;;
;;47;Reduktion av inköpspriser;;;;;;
;■;470;Reduktion av inköpspriser (gruppkonto);■;4700;Reduktion av inköpspriser (gruppkonto);;;
;;473;Erhållna rabatter;;4730;Erhållna rabatter;;;
;;;;;4731;Erhållna kassarabatter;;;
;;;;;4732;Erhållna mängdrabatter (inkl. bonus);;;
;;;;;4733;Erhållet aktivitetsstöd;;;
;;479;Övriga reduktioner av inköpspriser;;4790;Övriga reduktioner av inköpspriser;;;
;;48;(Fri kontogrupp);;;;;;
;;49;Förändring av lager, produkter i arbete och pågående arbeten;;;;;;
;■;490;Förändring av lager (gruppkonto);■;4900;Förändring av lager (gruppkonto);;;
;■;491;Förändring av lager av råvaror;■;4910;Förändring av lager av råvaror;;;
;■;492;Förändring av lager av tillsatsmaterial och förnödenheter;■;4920;Förändring av lager av tillsatsmaterial och förnödenheter;;;
;■;494;Förändring av produkter i arbete;■;4940;Förändring av produkter i arbete;;;
;;;;;4944;Förändring av produkter i arbete, material och utlägg;;;
;;;;;4945;Förändring av produkter i arbete, omkostnader;;;
;;;;;4947;Förändring av produkter i arbete, personalkostnader;;;
;■;495;Förändring av lager av färdiga varor;■;4950;Förändring av lager av färdiga varor;;;
;■;496;Förändring av lager av handelsvaror;■;4960;Förändring av lager av handelsvaror;;;
;■;497;Förändring av pågående arbeten, nedlagda kostnader;■;4970;Förändring av pågående arbeten, nedlagda kostnader;;;
;;;;;4974;Förändring av pågående arbeten, material och utlägg;;;
;;;;;4975;Förändring av pågående arbeten, omkostnader;;;
;;;;;4977;Förändring av pågående arbeten, personalkostnader;;;
;;498;Förändring av lager av värdepapper;;4980;Förändring av lager av värdepapper;;;
;;;;;4981;Sålda värdepappers anskaffningsvärde;;;
;;;;;4987;Nedskrivning av värdepapper;;;
;;;;;4988;Återföring av nedskrivning av värdepapper;;;
;;5;Övriga externa rörelseutgifter/ kostnader;;;;;;
;;50;Lokalkostnader;;;;;;
;;500;Lokalkostnader (gruppkonto);;5000;Lokalkostnader (gruppkonto);;;
;■;501;Lokalhyra;■;5010;Lokalhyra;;;
;;;;;5011;Hyra för kontorslokaler;;;
;;;;;5012;Hyra för garage;;;
;;;;;5013;Hyra för lagerlokaler;;;
;■;502;El för belysning;■;5020;El för belysning;;;
;■;503;Värme;■;5030;Värme;;;
;■;504;Vatten och avlopp;■;5040;Vatten och avlopp;;;
;;505;Lokaltillbehör;;5050;Lokaltillbehör;;;
;■;506;Städning och renhållning;■;5060;Städning och renhållning;;;
;;;;;5061;Städning;;;
;;;;;5062;Sophämtning;;;
;;;;;5063;Hyra för sopcontainer;;;
;;;;;5064;Snöröjning;;;
;;;;;5065;Trädgårdsskötsel;;;
;■;507;Reparation och underhåll av lokaler;■;5070;Reparation och underhåll av lokaler;;;
;;509;Övriga lokalkostnader;;5090;Övriga lokalkostnader;;;
;;;;;5098;Övriga lokalkostnader, avdragsgilla;;;
;;;;;5099;Övriga lokalkostnader, ej avdragsgilla;;;
;;51;Fastighetskostnader;;;;;;
;;510;Fastighetskostnader (gruppkonto);;5100;Fastighetskostnader (gruppkonto);;;
;;511;Tomträttsavgäld/arrende;;5110;Tomträttsavgäld/arrende;;;
;■;512;El för belysning;■;5120;El för belysning;;;
;■;513;Värme;■;5130;Värme;;;
;;;;;5131;Uppvärmning;;;
;;;;;5132;Sotning;;;
;■;514;Vatten och avlopp;■;5140;Vatten och avlopp;;;
;■;516;Städning och renhållning;■;5160;Städning och renhållning;;;
;;;;;5161;Städning;;;
;;;;;5162;Sophämtning;;;
;;;;;5163;Hyra för sopcontainer;;;
;;;;;5164;Snöröjning;;;
;;;;;5165;Trädgårdsskötsel;;;
;■;517;Reparation och underhåll av fastighet;■;5170;Reparation och underhåll av fastighet;;;
;;519;Övriga fastighetskostnader;;5190;Övriga fastighetskostnader;;;
;;;;;5191;Fastighetsskatt/fastighetsavgift;;;
;;;;;5192;Fastighetsförsäkringspremier;;;
;;;;;5193;Fastighetsskötsel och förvaltning;;;
;;;;;5198;Övriga fastighetskostnader, avdragsgilla;;;
;;;;;5199;Övriga fastighetskostnader, ej avdragsgilla;;;
;;52;Hyra av anläggningstillgångar;;;;;;
;■;520;Hyra av anläggningstillgångar (gruppkonto);■;5200;Hyra av anläggningstillgångar (gruppkonto);;;
;;521;Hyra av maskiner och andra tekniska anläggningar;;5210;Hyra av maskiner och andra tekniska anläggningar;;;
;;;;;5211;Korttidshyra av maskiner och andra tekniska anläggningar;;;
;;;;;5212;Leasing av maskiner och andra tekniska anläggningar;;;
;;522;Hyra av inventarier och verktyg;;5220;Hyra av inventarier och verktyg;;;
;;;;;5221;Korttidshyra av inventarier och verktyg;;;
;;;;;5222;Leasing av inventarier och verktyg;;;
;;525;Hyra av datorer;;5250;Hyra av datorer;;;
;;;;;5251;Korttidshyra av datorer;;;
;;;;;5252;Leasing av datorer;;;
;;529;Övriga hyreskostnader för anläggningstillgångar;;5290;Övriga hyreskostnader för anläggningstillgångar;;;
;;53;Energikostnader;;;;;;
;■;530;Energikostnader (gruppkonto);■;5300;Energikostnader (gruppkonto);;;
;;531;El för drift;;5310;El för drift;;;
;;532;Gas;;5320;Gas;;;
;;533;Eldningsolja;;5330;Eldningsolja;;;
;;534;Stenkol och koks;;5340;Stenkol och koks;;;
;;535;Torv, träkol, ved och annat träbränsle;;5350;Torv, träkol, ved och annat träbränsle;;;
;;536;Bensin, fotogen och motorbrännolja;;5360;Bensin, fotogen och motorbrännolja;;;
;;537;Fjärrvärme, kyla och ånga;;5370;Fjärrvärme, kyla och ånga;;;
;;538;Vatten;;5380;Vatten;;;
;;539;Övriga energikostnader;;5390;Övriga energikostnader;;;
;;54;Förbrukningsinventarier och förbrukningsmaterial;;;;;;
;;540;Förbrukningsinventarier och förbrukningsmaterial (gruppkonto);;5400;Förbrukningsinventarier och förbrukningsmaterial (gruppkonto);;;
;■;541;Förbrukningsinventarier;■;5410;Förbrukningsinventarier;;;
;;;;;5411;Förbrukningsinventarier med en livslängd på mer än ett år;;;
;;;;;5412;Förbrukningsinventarier med en livslängd på ett år eller mindre;;;
;■;542;Programvaror;■;5420;Programvaror;;;
;;543;Transportinventarier;;5430;Transportinventarier;;;
;;544;Förbrukningsemballage;;5440;Förbrukningsemballage;;;
;■;546;Förbrukningsmaterial;■;5460;Förbrukningsmaterial;;;
;;548;Arbetskläder och skyddsmaterial;;5480;Arbetskläder och skyddsmaterial;;;
;;549;Övriga förbrukningsinventarier och förbrukningsmaterial;;5490;Övriga förbrukningsinventarier och förbrukningsmaterial;;;
;;;;;5491;Övriga förbrukningsinventarier med en livslängd på mer än ett år;;;
;;;;;5492;Övriga förbrukningsinventarier med en livslängd på ett år eller mindre;;;
;;;;;5493;Övrigt förbrukningsmaterial;;;
;;55;Reparation och underhåll;;;;;;)"
R"(; ■; 550; Reparation och underhåll(gruppkonto); ■; 5500; Reparation och underhåll(gruppkonto);;;
;;551;Reparation och underhåll av maskiner och andra tekniska anläggningar;;5510;Reparation och underhåll av maskiner och andra tekniska anläggningar;;;
;;552;Reparation och underhåll av inventarier, verktyg och datorer m.m.;;5520;Reparation och underhåll av inventarier, verktyg och datorer m.m.;;;
;;553;Reparation och underhåll av installationer;;5530;Reparation och underhåll av installationer;;;
;;555;Reparation och underhåll av förbrukningsinventarier;;5550;Reparation och underhåll av förbrukningsinventarier;;;
;;558;Underhåll och tvätt av arbetskläder;;5580;Underhåll och tvätt av arbetskläder;;;
;;559;Övriga kostnader för reparation och underhåll;;5590;Övriga kostnader för reparation och underhåll;;;
;;56;Kostnader för transportmedel;;;;;;
;■;560;Kostnader för transportmedel (gruppkonto);■;5600;Kostnader för transportmedel (gruppkonto);;;
;;561;Personbilskostnader;;5610;Personbilskostnader;;;
;;;;■;5611;Drivmedel för personbilar;;;
;;;;■;5612;Försäkring och skatt för personbilar;;;
;;;;■;5613;Reparation och underhåll av personbilar;;;
;;;;■;5615;Leasing av personbilar;;;
;;;;;5616;Trängselskatt, avdragsgill;;;
;;;;;5619;Övriga personbilskostnader;;;
;;562;Lastbilskostnader;;5620;Lastbilskostnader;;;
;;563;Truckkostnader;;5630;Truckkostnader;;;
;;564;Kostnader för arbetsmaskiner;;5640;Kostnader för arbetsmaskiner;;;
;;565;Traktorkostnader;;5650;Traktorkostnader;;;
;;566;Motorcykel-, moped- och skoterkostnader;;5660;Motorcykel-, moped- och skoterkostnader;;;
;;567;Båt-, flygplans- och helikopterkostnader;;5670;Båt-, flygplans- och helikopterkostnader;;;
;;569;Övriga kostnader för transportmedel;;5690;Övriga kostnader för transportmedel;;;
;;57;Frakter och transporter;;;;;;
;■;570;Frakter och transporter (gruppkonto);■;5700;Frakter och transporter (gruppkonto);;;
;;571;Frakter, transporter och försäkringar vid varudistribution;;5710;Frakter, transporter och försäkringar vid varudistribution;;;
;;572;Tull- och speditionskostnader m.m.;;5720;Tull- och speditionskostnader m.m.;;;
;;573;Arbetstransporter;;5730;Arbetstransporter;;;
;;579;Övriga kostnader för frakter och transporter;;5790;Övriga kostnader för frakter och transporter;;;
;;58;Resekostnader;;;;;;
;■;580;Resekostnader (gruppkonto);■;5800;Resekostnader (gruppkonto);;;
;■;581;Biljetter;■;5810;Biljetter;;;
;■;582;Hyrbilskostnader;■;5820;Hyrbilskostnader;;;
;;583;Kost och logi;;5830;Kost och logi;;;
;;;;■;5831;Kost och logi i Sverige;;;
;;;;■;5832;Kost och logi i utlandet;;;
;;589;Övriga resekostnader;;5890;Övriga resekostnader;;;
;;59;Reklam och PR;;;;;;
;■;590;Reklam och PR (gruppkonto);■;5900;Reklam och PR (gruppkonto);;;
;;591;Annonsering;;5910;Annonsering;;;
;;592;Utomhus- och trafikreklam;;5920;Utomhus- och trafikreklam;;;
;;593;Reklamtrycksaker och direktreklam;;5930;Reklamtrycksaker och direktreklam;;;
;;594;Utställningar och mässor;;5940;Utställningar och mässor;;;
;;595;Butiksreklam och återförsäljarreklam;;5950;Butiksreklam och återförsäljarreklam;;;
;;596;Varuprover, reklamgåvor, presentreklam och tävlingar;;5960;Varuprover, reklamgåvor, presentreklam och tävlingar;;;
;;597;Film-, radio-, TV- och Internetreklam;;5970;Film-, radio-, TV- och Internetreklam;;;
;;598;PR, institutionell reklam och sponsring;;5980;PR, institutionell reklam och sponsring;;;
;;599;Övriga kostnader för reklam och PR;;5990;Övriga kostnader för reklam och PR;;;
;;6;Övriga externa rörelseutgifter/ kostnader;;;;;;
;;60;Övriga försäljningskostnader;;;;;;
;;600;Övriga försäljningskostnader (gruppkonto);;6000;Övriga försäljningskostnader (gruppkonto);;;
;;601;Kataloger, prislistor m.m.;;6010;Kataloger, prislistor m.m.;;;
;;602;Egna facktidskrifter;;6020;Egna facktidskrifter;;;
;;603;Speciella orderkostnader;;6030;Speciella orderkostnader;;;
;;604;Kontokortsavgifter;;6040;Kontokortsavgifter;;;
;;605;Försäljningsprovisioner;;6050;Försäljningsprovisioner;;;
;;;;;6055;Franchisekostnader o.dyl.;;;
;;606;Kreditförsäljningskostnader;;6060;Kreditförsäljningskostnader;;;
;;;;;6061;Kreditupplysning;;;
;;;;;6062;Inkasso och KFM-avgifter;;;
;;;;;6063;Kreditförsäkringspremier;;;
;;;;;6064;Factoringavgifter;;;
;;;;;6069;Övriga kreditförsäljningskostnader;;;
;;607;Representation;;6070;Representation;;;
;;;;■;6071;Representation, avdragsgill;;;
;;;;■;6072;Representation, ej avdragsgill;;;
;;608;Bankgarantier;;6080;Bankgarantier;;;
;■;609;Övriga försäljningskostnader;■;6090;Övriga försäljningskostnader;;;
;;61;Kontorsmateriel och trycksaker;;;;;;
;■;610;Kontorsmateriel och trycksaker (gruppkonto);■;6100;Kontorsmateriel och trycksaker (gruppkonto);;;
;;611;Kontorsmateriel;;6110;Kontorsmateriel;;;
;;615;Trycksaker;;6150;Trycksaker;;;
;;62;Tele och post;;;;;;
;;620;Tele och post (gruppkonto);;6200;Tele och post (gruppkonto);;;
;■;621;Telekommunikation;■;6210;Telekommunikation;;;
;;;;;6211;Fast telefoni;;;
;;;;;6212;Mobiltelefon;;;
;;;;;6213;Mobilsökning;;;
;;;;;6214;Fax;;;
;;;;;6215;Telex;;;
;;623;Datakommunikation;;6230;Datakommunikation;;;
;■;625;Postbefordran;■;6250;Postbefordran;;;
;;63;Företagsförsäkringar och övriga riskkostnader;;;;;;
;;630;Företagsförsäkringar och övriga riskkostnader (gruppkonto);;6300;Företagsförsäkringar och övriga riskkostnader (gruppkonto);;;
;■;631;Företagsförsäkringar;■;6310;Företagsförsäkringar;;;
;;632;Självrisker vid skada;;6320;Självrisker vid skada;;;
;;633;Förluster i pågående arbeten;;6330;Förluster i pågående arbeten;;;
;;634;Lämnade skadestånd;;6340;Lämnade skadestånd;;;
;;;;;6341;Lämnade skadestånd, avdragsgilla;;;
;;;;;6342;Lämnade skadestånd, ej avdragsgilla;;;
;■;635;Förluster på kundfordringar;■;6350;Förluster på kundfordringar;;;
;;;;;6351;Konstaterade förluster på kundfordringar;;;
;;;;;6352;Befarade förluster på kundfordringar;;;
;;636;Garantikostnader;;6360;Garantikostnader;;;
;;;;;6361;Förändring av garantiavsättning;;;
;;;;;6362;Faktiska garantikostnader;;;
;;637;Kostnader för bevakning och larm;;6370;Kostnader för bevakning och larm;;;
;;638;Förluster på övriga kortfristiga fordringar;;6380;Förluster på övriga kortfristiga fordringar;;;
;■;639;Övriga riskkostnader;■;6390;Övriga riskkostnader;;;
;;64;Förvaltningskostnader;;;;;;
;;640;Förvaltningskostnader (gruppkonto);;6400;Förvaltningskostnader (gruppkonto);;;
;■;641;Styrelsearvoden som inte är lön;■;6410;Styrelsearvoden som inte är lön;;;
;■;642;Ersättningar till revisor;■;6420;Ersättningar till revisor;;;
;;;;;6421;Revision;;;
;;;;;6422;Revisonsverksamhet utöver revision;;;
;;;;;6423;Skatterådgivning – revisor;;;
;;;;;6424;Övriga tjänster – revisor;;;
;;643;Management fees;;6430;Management fees;;;
;;644;Årsredovisning och delårsrapporter;;6440;Årsredovisning och delårsrapporter;;;
;;645;Bolagsstämma/års- eller föreningsstämma;;6450;Bolagsstämma/års- eller föreningsstämma;;;
;;649;Övriga förvaltningskostnader;;6490;Övriga förvaltningskostnader;;;
;;65;Övriga externa tjänster;;;;;;
;;650;Övriga externa tjänster (gruppkonto);;6500;Övriga externa tjänster (gruppkonto);;;
;;651;Mätningskostnader;;6510;Mätningskostnader;;;
;;652;Ritnings- och kopieringskostnader;;6520;Ritnings- och kopieringskostnader;;;
;■;653;Redovisningstjänster;■;6530;Redovisningstjänster;;;
;■;654;IT-tjänster;■;6540;IT-tjänster;;;
;■;655;Konsultarvoden;■;6550;Konsultarvoden;;;
;;;;;6551;Arkitekttjänster;|;;
;;;;;6552;Teknisk provning och analys;|;;
;;;;;6553;Tekniska konsulttjänster;|;;
;;;;;6554;Finansiell- och övrig ekonomisk rådgivning;|;;
;;;;;6555;Skatterådgivning inkl. insolvens- och konkursförvaltning;|;;
;;;;;6556;Köpta tjänster avseende forskning och utveckling;|;;
;;;;;6559;Övrig konsultverksamhet;|;;
;■;656;Serviceavgifter till branschorganisationer;■;6560;Serviceavgifter till branschorganisationer;;;
;■;657;Bankkostnader;■;6570;Bankkostnader;;;
;;658;Advokat- och rättegångskostnader;;6580;Advokat- och rättegångskostnader;;;
;■;659;Övriga externa tjänster;■;6590;Övriga externa tjänster;;;
;;66;(Fri kontogrupp);;;;;;
;;67;(Fri kontogrupp);;;;;;
;;68;Inhyrd personal;;;;;;
;■;680;Inhyrd personal (gruppkonto);■;6800;Inhyrd personal (gruppkonto);;;
;;681;Inhyrd produktionspersonal;;6810;Inhyrd produktionspersonal;;;
;;682;Inhyrd lagerpersonal;;6820;Inhyrd lagerpersonal;;;
;;683;Inhyrd transportpersonal;;6830;Inhyrd transportpersonal;;;
;;684;Inhyrd kontors- och ekonomipersonal;;6840;Inhyrd kontors- och ekonomipersonal;;;
;;685;Inhyrd IT-personal;;6850;Inhyrd IT-personal;;;
;;686;Inhyrd marknads- och försäljningspersonal;;6860;Inhyrd marknads- och försäljningspersonal;;;
;;687;Inhyrd restaurang- och butikspersonal;;6870;Inhyrd restaurang- och butikspersonal;;;
;;688;Inhyrda företagsledare;;6880;Inhyrda företagsledare;;;
;;689;Övrig inhyrd personal;;6890;Övrig inhyrd personal;;;
;;69;Övriga externa kostnader;;;;;;
;;690;Övriga externa kostnader (gruppkonto);;6900;Övriga externa kostnader (gruppkonto);;;
;;691;Licensavgifter och royalties;;6910;Licensavgifter och royalties;;;
;;692;Kostnader för egna patent;;6920;Kostnader för egna patent;;;
;;693;Kostnader för varumärken m.m.;;6930;Kostnader för varumärken m.m.;;;
;;694;Kontroll-, provnings- och stämpelavgifter;;6940;Kontroll-, provnings- och stämpelavgifter;;;
;;695;Tillsynsavgifter myndigheter;;6950;Tillsynsavgifter myndigheter;;;
;■;697;Tidningar, tidskrifter och facklitteratur;■;6970;Tidningar, tidskrifter och facklitteratur;;;
;■;698;Föreningsavgifter;■;6980;Föreningsavgifter;;;
;;;;;6981;Föreningsavgifter, avdragsgilla;;;
;;;;;6982;Föreningsavgifter, ej avdragsgilla;;;
;;699;Övriga externa kostnader;;6990;Övriga externa kostnader;;;
;;;;■;6991;Övriga externa kostnader, avdragsgilla;;;
;;;;■;6992;Övriga externa kostnader, ej avdragsgilla;;;
;;;;;6993;Lämnade bidrag och gåvor;;;
;;;;;6996;Betald utländsk inkomstskatt;;;
;;;;;6997;Obetald utländsk inkomstskatt;;;
;;;;;6998;Utländsk moms;;;
;;;;;6999;Ingående moms, blandad verksamhet;;;
;;7;Utgifter/kostnader för personal, avskrivningar m.m.;;;;;;
;;70;Löner till kollektivanställda;;;;;;
;;700;Löner till kollektivanställda (gruppkonto);;7000;Löner till kollektivanställda (gruppkonto);;;
;■;701;Löner till kollektivanställda;■;7010;Löner till kollektivanställda;;;
;;;;;7011;Löner till kollektivanställda;;;
;;;;;7012;Vinstandelar till kollektivanställda;;;
;;;;;7013;Lön växa-stöd kollektivanställda 10,21 %;;;
;;;;;7017;Avgångsvederlag till kollektivanställda;;;
;;;;;7018;Bruttolöneavdrag, kollektivanställda;;;
;;;;;7019;Upplupna löner och vinstandelar till kollektivanställda;;;
;;703;Löner till kollektivanställda (utlandsanställda);;7030;Löner till kollektivanställda (utlandsanställda);;;
;;;;;7031;Löner till kollektivanställda (utlandsanställda);;;
;;;;;7032;Vinstandelar till kollektivanställda (utlandsanställda);;;
;;;;;7037;Avgångsvederlag till kollektivanställda (utlandsanställda);;;
;;;;;7038;Bruttolöneavdrag, kollektivanställda (utlandsanställda);;;
;;;;;7039;Upplupna löner och vinstandelar till kollektivanställda (utlandsanställda);;;
;;708;Löner till kollektivanställda för ej arbetad tid;;7080;Löner till kollektivanställda för ej arbetad tid;;;
;;;;;7081;Sjuklöner till kollektivanställda;;;
;;;;;7082;Semesterlöner till kollektivanställda;;;
;;;;;7083;Föräldraersättning till kollektivanställda;;;
;;;;;7089;Övriga löner till kollektivanställda för ej arbetad tid;;;
;■;709;Förändring av semesterlöneskuld;■;7090;Förändring av semesterlöneskuld;;;
;;71;(Fri kontogrupp);;;;;;
;;72;Löner till tjänstemän och företagsledare;;;;;;
;;720;Löner till tjänstemän och företagsledare (gruppkonto);;7200;Löner till tjänstemän och företagsledare (gruppkonto);;;
;■;721;Löner till tjänstemän;■;7210;Löner till tjänstemän;;;
;;;;;7211;Löner till tjänstemän;;;
;;;;;7212;Vinstandelar till tjänstemän;;;
;;;;;7213;Lön växa-stöd tjänstemän 10,21 %;;;
;;;;;7217;Avgångsvederlag till tjänstemän;;;
;;;;;7218;Bruttolöneavdrag, tjänstemän;;;
;;;;;7219;Upplupna löner och vinstandelar till tjänstemän;;;
;■;722;Löner till företagsledare;■;7220;Löner till företagsledare;;;
;;;;;7221;Löner till företagsledare;;;
;;;;;7222;Tantiem till företagsledare;;;
;;;;;7227;Avgångsvederlag till företagsledare;;;
;;;;;7228;Bruttolöneavdrag, företagsledare;;;
;;;;;7229;Upplupna löner och tantiem till företagsledare;;;
;;723;Löner till tjänstemän och ftgsledare (utlandsanställda);;7230;Löner till tjänstemän och ftgsledare (utlandsanställda);;;
;;;;;7231;Löner till tjänstemän och ftgsledare (utlandsanställda);;;
;;;;;7232;Vinstandelar till tjänstemän och ftgsledare (utlandsanställda);;;
;;;;;7237;Avgångsvederlag till tjänstemän och ftgsledare (utlandsanställda);;;
;;;;;7238;Bruttolöneavdrag, tjänstemän och ftgsledare (utlandsanställda);;;
;;;;;7239;Upplupna löner och vinstandelar till tjänstemän och ftgsledare (utlandsanställda);;;
;■;724;Styrelsearvoden;■;7240;Styrelsearvoden;;;
;;728;Löner till tjänstemän och företagsledare för ej arbetad tid;;7280;Löner till tjänstemän och företagsledare för ej arbetad tid;;;
;;;;;7281;Sjuklöner till tjänstemän;;;
;;;;;7282;Sjuklöner till företagsledare;;;
;;;;;7283;Föräldraersättning till tjänstemän;;;
;;;;;7284;Föräldraersättning till företagsledare;;;
;;;;;7285;Semesterlöner till tjänstemän;;;
;;;;;7286;Semesterlöner till företagsledare;;;
;;;;;7288;Övriga löner till tjänstemän för ej arbetad tid;;;
;;;;;7289;Övriga löner till företagsledare för ej arbetad tid;;;
;■;729;Förändring av semesterlöneskuld;■;7290;Förändring av semesterlöneskuld;;;
;;;;;7291;Förändring av semesterlöneskuld till tjänstemän;;;
;;;;;7292;Förändring av semesterlöneskuld till företagsledare;;;
;;73;Kostnadsersättningar och förmåner;;;;;;
;;730;Kostnadsersättningar och förmåner (gruppkonto);;7300;Kostnadsersättningar och förmåner (gruppkonto);;;
;■;731;Kontanta extraersättningar;■;7310;Kontanta extraersättningar;;;
;;;;;7311;Ersättningar för sammanträden m.m.;;;
;;;;;7312;Ersättningar för förslagsverksamhet och uppfinningar;;;
;;;;;7313;Ersättningar för/bidrag till bostadskostnader;;;
;;;;;7314;Ersättningar för/bidrag till måltidskostnader;;;
;;;;;7315;Ersättningar för/bidrag till resor till och från arbetsplatsen;;;
;;;;;7316;Ersättningar för/bidrag till arbetskläder;;;
;;;;;7317;Ersättningar för/bidrag till arbetsmaterial och arbetsverktyg;;;
;;;;;7318;Felräkningspengar;;;
;;;;;7319;Övriga kontanta extraersättningar;;;
;;732;Traktamenten vid tjänsteresa;;7320;Traktamenten vid tjänsteresa;;;
;;;;■;7321;Skattefria traktamenten, Sverige;;;
;;;;■;7322;Skattepliktiga traktamenten, Sverige;;;
;;;;■;7323;Skattefria traktamenten, utlandet;;;
;;;;■;7324;Skattepliktiga traktamenten, utlandet;;;
;;733;Bilersättningar;;7330;Bilersättningar;;;
;;;;■;7331;Skattefria bilersättningar;;;
;;;;■;7332;Skattepliktiga bilersättningar;;;
;;;;;7333;Ersättning för trängselskatt, skattefri;;;
;;735;Ersättningar för föreskrivna arbetskläder;;7350;Ersättningar för föreskrivna arbetskläder;;;
;;737;Representationsersättningar;;7370;Representationsersättningar;;;
;■;738;Kostnader för förmåner till anställda;■;7380;Kostnader för förmåner till anställda;;;
;;;;;7381;Kostnader för fri bostad;;;
;;;;;7382;Kostnader för fria eller subventionerade måltider;;;
;;;;;7383;Kostnader för fria resor till och från arbetsplatsen;;;
;;;;;7384;Kostnader för fria eller subventionerade arbetskläder;;;
;;;;■;7385;Kostnader för fri bil;;;
;;;;;7386;Subventionerad ränta;;;
;;;;;7387;Kostnader för lånedatorer;;;
;;;;;7388;Anställdas ersättning för erhållna förmåner;;;
;;;;;7389;Övriga kostnader för förmåner;;;
;■;739;Övriga kostnadsersättningar och förmåner;■;7390;Övriga kostnadsersättningar och förmåner;;;
;;;;;7391;Kostnad för trängselskatteförmån;;;
;;;;;7392;Kostnad för förmån av hushållsnära tjänster;;;
;;74;Pensionskostnader;;;;;;)"
R"(;; 740; Pensionskostnader(gruppkonto);; 7400; Pensionskostnader(gruppkonto);;;
; ■; 741; Pensionsförsäkringspremier; ■; 7410; Pensionsförsäkringspremier;;;
;;;;;7411;Premier för kollektiva pensionsförsäkringar;;;
;;;;;7412;Premier för individuella pensionsförsäkringar;;;
;;742;Förändring av pensionsskuld;;7420;Förändring av pensionsskuld;;;
;;743;Avdrag för räntedel i pensionskostnad;;7430;Avdrag för räntedel i pensionskostnad;;;
;;744;Förändring av pensionsstiftelsekapital;;7440;Förändring av pensionsstiftelsekapital;;;
;;;;;7441;Överföring av medel till pensionsstiftelse;|;;
;;;;;7448;Gottgörelse från pensionsstiftelse;;;
;;746;Pensionsutbetalningar;;7460;Pensionsutbetalningar;;;
;;;;;7461;Pensionsutbetalningar till f.d. kollektivanställda;;;
;;;;;7462;Pensionsutbetalningar till f.d. tjänstemän;;;
;;;;;7463;Pensionsutbetalningar till f.d. företagsledare;;;
;;747;Förvaltnings- och kreditförsäkringsavgifter;;7470;Förvaltnings- och kreditförsäkringsavgifter;;;
;■;749;Övriga pensionskostnader;■;7490;Övriga pensionskostnader;;;
;;75;Sociala och andra avgifter enligt lag och avtal;;;;;;
;;750;Sociala och andra avgifter enligt lag och avtal (gruppkonto);;7500;Sociala och andra avgifter enligt lag och avtal (gruppkonto);;;
;;751;Arbetsgivaravgifter 31,42 %;;7510;Arbetsgivaravgifter 31,42 %;;;
;;;;■;7511;Arbetsgivaravgifter för löner och ersättningar;;;
;;;;■;7512;Arbetsgivaravgifter för förmånsvärden;;;
;;;;;7515;Arbetsgivaravgifter på skattepliktiga kostnadsersättningar;;;
;;;;;7516;Arbetsgivaravgifter på arvoden;;;
;;;;;7518;Arbetsgivaravgifter på bruttolöneavdrag m.m.;;;
;;;;■;7519;Arbetsgivaravgifter för semester- och löneskulder;;;
;■;753;Särskild löneskatt;■;7530;Särskild löneskatt;;;
;;;;;7531;Särskild löneskatt för vissa försäkringsersättningar m.m.;;;
;;;;;7532;Särskild löneskatt pensionskostnader, deklarationspost;;;
;;;;;7533;Särskild löneskatt för pensionskostnader;;;
;■;755;Avkastningsskatt på pensionsmedel;■;7550;Avkastningsskatt på pensionsmedel;;;
;;;;;7551;Avkastningsskatt 15 % försäkringsföretag m.fl. samt avsatt till pensioner;;;
;;;;;7552;Avkastningsskatt 15 % utländska pensionsförsäkringar;;;
;;;;;7553;Avkastningsskatt 30 % utländska försäkringsföretag m.fl.;;;
;;;;;7554;Avkastningsskatt 30 % utländska kapitalförsäkringar;;;
;■;757;Premier för arbetsmarknadsförsäkringar;■;7570;Premier för arbetsmarknadsförsäkringar;;;
;;;;;7571;Arbetsmarknadsförsäkringar;;;
;;;;;7572;Arbetsmarknadsförsäkringar pensionsförsäkringspremier, deklarationspost;;;
;■;758;Gruppförsäkringspremier;■;7580;Gruppförsäkringspremier;;;
;;;;;7581;Grupplivförsäkringspremier;;;
;;;;;7582;Gruppsjukförsäkringspremier;;;
;;;;;7583;Gruppolycksfallsförsäkringspremier;;;
;;;;;7589;Övriga gruppförsäkringspremier;;;
;■;759;Övriga sociala och andra avgifter enligt lag och avtal;■;7590;Övriga sociala och andra avgifter enligt lag och avtal;;;
;;76;Övriga personalkostnader;;;;;;
;■;760;Övriga personalkostnader (gruppkonto);■;7600;Övriga personalkostnader (gruppkonto);;;
;■;761;Utbildning;■;7610;Utbildning;;;
;;762;Sjuk- och hälsovård;;7620;Sjuk- och hälsovård;;;
;;;;■;7621;Sjuk- och hälsovård, avdragsgill;;;
;;;;■;7622;Sjuk- och hälsovård, ej avdragsgill;;;
;;;;;7623;Sjukvårdsförsäkring, ej avdragsgill;;;
;;763;Personalrepresentation;;7630;Personalrepresentation;;;
;;;;■;7631;Personalrepresentation, avdragsgill;;;
;;;;■;7632;Personalrepresentation, ej avdragsgill;;;
;;765;Sjuklöneförsäkring;;7650;Sjuklöneförsäkring;;;
;;767;Förändring av personalstiftelsekapital;;7670;Förändring av personalstiftelsekapital;;;
;;;;;7671;Avsättning till personalstiftelse;;;
;;;;;7678;Gottgörelse från personalstiftelse;;;
;;769;Övriga personalkostnader;;7690;Övriga personalkostnader;;;
;;;;;7691;Personalrekrytering;;;
;;;;;7692;Begravningshjälp;;;
;;;;;7693;Fritidsverksamhet;;;
;;;;;7699;Övriga personalkostnader;;;
;;77;Nedskrivningar och återföring av nedskrivningar;;;;;;
;;771;Nedskrivningar av immateriella anläggningstillgångar;;7710;Nedskrivningar av immateriella anläggningstillgångar;;;
;■;772;Nedskrivningar av byggnader och mark;■;7720;Nedskrivningar av byggnader och mark;;;
;■;773;Nedskrivningar av maskiner och inventarier;■;7730;Nedskrivningar av maskiner och inventarier;;;
;;774;Nedskrivningar av vissa omsättningstillgångar;;7740;Nedskrivningar av vissa omsättningstillgångar;;;
;;776;Återföring av nedskrivningar av immateriella anläggningstillgångar;;7760;Återföring av nedskrivningar av immateriella anläggningstillgångar;;;
;;777;Återföring av nedskrivningar av byggnader och mark;;7770;Återföring av nedskrivningar av byggnader och mark;;;
;;778;Återföring av nedskrivningar av maskiner och inventarier;;7780;Återföring av nedskrivningar av maskiner och inventarier;;;
;;779;Återföring av nedskrivningar av vissa omsättningstillgångar;;7790;Återföring av nedskrivningar av vissa omsättningstillgångar;;;
;;78;Avskrivningar enligt plan;;;;;;
;■;781;Avskrivningar på immateriella anläggningstillgångar;■;7810;Avskrivningar på immateriella anläggningstillgångar;;;
;;;;;7811;Avskrivningar på balanserade utgifter;;;
;;;;;7812;Avskrivningar på koncessioner m.m.;;;
;;;;;7813;Avskrivningar på patent;;;
;;;;;7814;Avskrivningar på licenser;;;
;;;;;7815;Avskrivningar på varumärken;;;
;;;;;7816;Avskrivningar på hyresrätter;;;
;;;;;7817;Avskrivningar på goodwill;;;
;;;;;7819;Avskrivningar på övriga immateriella anläggningstillgångar;;;
;■;782;Avskrivningar på byggnader och markanläggningar;■;7820;Avskrivningar på byggnader och markanläggningar;;;
;;;;;7821;Avskrivningar på byggnader;;;
;;;;;7824;Avskrivningar på markanläggningar;;;
;;;;;7829;Avskrivningar på övriga byggnader;;;
;■;783;Avskrivningar på maskiner och inventarier;■;7830;Avskrivningar på maskiner och inventarier;;;
;;;;;7831;Avskrivningar på maskiner och andra tekniska anläggningar;;;
;;;;;7832;Avskrivningar på inventarier och verktyg;;;
;;;;;7833;Avskrivningar på installationer;;;
;;;;;7834;Avskrivningar på bilar och andra transportmedel;;;
;;;;;7835;Avskrivningar på datorer;;;
;;;;;7836;Avskrivningar på leasade tillgångar;;;
;;;;;7839;Avskrivningar på övriga maskiner och inventarier;;;
;;784;Avskrivningar på förbättringsutgifter på annans fastighet;;7840;Avskrivningar på förbättringsutgifter på annans fastighet;;;
;;79;Övriga rörelsekostnader;;;;;;
;[Ej K2];794;Orealiserade positiva/negativa värdeförändringar på säkringsinstrument;[Ej K2];7940;Orealiserade positiva/negativa värdeförändringar på säkringsinstrument;;;
;;796;Valutakursförluster på fordringar och skulder av rörelsekaraktär;;7960;Valutakursförluster på fordringar och skulder av rörelsekaraktär;;;
;■;797;Förlust vid avyttring av immateriella och materiella anläggningstillgångar;■;7970;Förlust vid avyttring av immateriella och materiella anläggningstillgångar;;;
;;;;;7971;Förlust vid avyttring av immateriella anläggningstillgångar;;;
;;;;;7972;Förlust vid avyttring av byggnader och mark;;;
;;;;;7973;Förlust vid avyttring av maskiner och inventarier;;;
;■;799;Övriga rörelsekostnader;■;7990;Övriga rörelsekostnader;;;
;;8;Finansiella och andra inkomster/ intäkter och utgifter/kostnader;;;;;;
;;80;Resultat från andelar i koncernföretag;;;;;;
;;801;Utdelning på andelar i koncernföretag;;8010;Utdelning på andelar i koncernföretag;;;
;;;;;8012;Utdelning på andelar i dotterföretag;;;
;;;;;8016;Emissionsinsats, koncernföretag;;;
;;802;Resultat vid försäljning av andelar i koncernföretag;;8020;Resultat vid försäljning av andelar i koncernföretag;;;
;;;;;8022;Resultat vid försäljning av andelar i dotterföretag;;;
;;803;Resultatandelar från handelsbolag (dotterföretag);;8030;Resultatandelar från handelsbolag (dotterföretag);;;
;;807;Nedskrivningar av andelar i och långfristiga fordringar hos koncernföretag;;8070;Nedskrivningar av andelar i och långfristiga fordringar hos koncernföretag;;;
;;;;;8072;Nedskrivningar av andelar i dotterföretag;;;
;;;;;8076;Nedskrivningar av långfristiga fordringar hos moderföretag;;;
;;;;;8077;Nedskrivningar av långfristiga fordringar hos dotterföretag;;;
;;808;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos koncernföretag;;8080;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos koncernföretag;;;
;;;;;8082;Återföringar av nedskrivningar av andelar i dotterföretag;;;
;;;;;8086;Återföringar av nedskrivningar av långfristiga fordringar hos moderföretag;;;
;;;;;8087;Återföringar av nedskrivningar av långfristiga fordringar hos dotterföretag;;;
;;81;Resultat från andelar i intresseföretag;;;;;;
;;811;Utdelningar på andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;8110;Utdelningar på andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;8111;Utdelningar på andelar i intresseföretag;;;
;;;;;8112;Utdelningar på andelar i gemensamt styrda företag;;;
;;;;;8113;Utdelningar på andelar i övriga företag som det finns ett ägarintresse i;;;
;;;;;8116;Emissionsinsats, intresseföretag;;;
;;;;;8117;Emissionsinsats, gemensamt styrda företag;;;
;;;;;8118;Emissionsinsats, övriga företag som det finns ett ägarintresse i;;;
;;812;Resultat vid försäljning av andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;8120;Resultat vid försäljning av andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;8121;Resultat vid försäljning av andelar i intresseföretag;;;
;;;;;8122;Resultat vid försäljning av andelar i gemensamt styrda företag;;;
;;;;;8123;Resultat vid försäljning av andelar i övriga företag som det finns ett ägarintresse i;;;
;;813;Resultatandelar från handelsbolag (intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i);;8130;Resultatandelar från handelsbolag (intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i);;;
;;;;;8131;Resultatandelar från handelsbolag (intresseföretag);;;
;;;;;8132;Resultatandelar från handelsbolag (gemensamt styrda företag);;;
;;;;;8133;Resultatandelar från handelsbolag (övriga företag som det finns ett ägarintresse i);;;
;;817;Nedskrivningar av andelar i och långfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;8170;Nedskrivningar av andelar i och långfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;8171;Nedskrivningar av andelar i intresseföretag;;;
;;;;;8172;Nedskrivningar av långfristiga fordringar hos intresseföretag;;;
;;;;;8173;Nedskrivningar av andelar i gemensamt styrda företag;;;
;;;;;8174;Nedskrivningar av långfristiga fordringar hos gemensamt styrda företag;;;
;;;;;8176;Nedskrivningar av andelar i övriga företag som det finns ett ägarintresse i;;;
;;;;;8177;Nedskrivningar av långfristiga fordringar hos övriga företag som det finns ett ägarintresse i;;;
;;818;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos intresseföretag;;8180;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos intresseföretag;;;
;;;;;8181;Återföringar av nedskrivningar av andelar i intresseföretag;;;
;;;;;8182;Återföringar av nedskrivningar av långfristiga fordringar hos intresseföretag;;;
;;;;;8183;Återföringar av nedskrivningar av andelar i gemensamt styrda företag;;;
;;;;;8184;Återföringar av nedskrivningar av långfristiga fordringar hos gemensamt styrda företag;;;
;;;;;8186;Återföringar av nedskrivningar av andelar i övriga företag som det finns ett ägarintresse i;;;
;;;;;8187;Återföringar av nedskrivningar av långfristiga fordringar hos övriga företag som det finns ett ägarintresse i;;;
;;82;Resultat från övriga värdepapper och långfristiga fordringar (anläggningstillgångar);;;;;;
;■;821;Utdelningar på andelar i andra företag;■;8210;Utdelningar på andelar i andra företag;;;
;;;;;8212;Utdelningar, övriga företag;;;
;;;;;8216;Insatsemissioner, övriga företag;;;
;■;822;Resultat vid försäljning av värdepapper i och långfristiga fordringar hos andra företag;■;8220;Resultat vid försäljning av värdepapper i och långfristiga fordringar hos andra företag;;;
;;;;;8221;Resultat vid försäljning av andelar i andra företag;;;
;;;;;8222;Resultat vid försäljning av långfristiga fordringar hos andra företag;;;
;;;;;8223;Resultat vid försäljning av derivat (långfristiga värdepappersinnehav);;;
;;823;Valutakursdifferenser på långfristiga fordringar;;8230;Valutakursdifferenser på långfristiga fordringar;;;
;;;;;8231;Valutakursvinster på långfristiga fordringar;;;
;;;;;8236;Valutakursförluster på långfristiga fordringar;;;
;;824;Resultatandelar från handelsbolag (andra företag);;8240;Resultatandelar från handelsbolag (andra företag);;;
;■;825;Ränteintäkter från långfristiga fordringar hos och värdepapper i andra företag;■;8250;Ränteintäkter från långfristiga fordringar hos och värdepapper i andra företag;;;
;;;;;8251;Ränteintäkter från långfristiga fordringar;;;
;;;;;8252;Ränteintäkter från övriga värdepapper;;;
;;;;;8254;Skattefria ränteintäkter, långfristiga tillgångar;;;
;;;;;8255;Avkastningsskatt kapitalplacering;;;
;;826;Ränteintäkter från långfristiga fordringar hos koncernföretag;;8260;Ränteintäkter från långfristiga fordringar hos koncernföretag;;;
;;;;;8261;Ränteintäkter från långfristiga fordringar hos moderföretag;;;
;;;;;8262;Ränteintäkter från långfristiga fordringar hos dotterföretag;;;
;;;;;8263;Ränteintäkter från långfristiga fordringar hos andra koncernföretag;;;
;■;827;Nedskrivningar av innehav av andelar i och långfristiga fordringar hos andra företag;■;8270;Nedskrivningar av innehav av andelar i och långfristiga fordringar hos andra företag;;;
;;;;;8271;Nedskrivningar av andelar i andra företag;;;
;;;;;8272;Nedskrivningar av långfristiga fordringar hos andra företag;;;
;;;;;8273;Nedskrivningar av övriga värdepapper hos andra företag;;;
;;828;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos andra företag;;8280;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos andra företag;;;
;;;;;8281;Återföringar av nedskrivningar av andelar i andra företag;;;
;;;;;8282;Återföringar av nedskrivningar av långfristiga fordringar hos andra företag;;;
;;;;;8283;Återföringar av nedskrivningar av övriga värdepapper i andra företag;;;
;[Ej K2];829;Värdering till verkligt värde, anläggningstillgångar;[Ej K2];8290;Värdering till verkligt värde, anläggningstillgångar;;;
;;;;[Ej K2];8291;Orealiserade värdeförändringar på anläggningstillgångar;;;
;;;;[Ej K2];8295;Orealiserade värdeförändringar på derivatinstrument;;;
;;83;Övriga ränteintäkter och liknande resultatposter;;;;;;
;■;831;Ränteintäkter från omsättningstillgångar;■;8310;Ränteintäkter från omsättningstillgångar;;;
;;;;;8311;Ränteintäkter från bank;;;
;;;;;8312;Ränteintäkter från kortfristiga placeringar;;;
;;;;;8313;Ränteintäkter från kortfristiga fordringar;;;
;;;;■;8314;Skattefria ränteintäkter;;;
;;;;;8317;Ränteintäkter för dold räntekompensation;;;
;;;;;8319;Övriga ränteintäkter från omsättningstillgångar;;;
;[Ej K2];832;Värdering till verkligt värde, omsättningstillgångar;[Ej K2];8320;Värdering till verkligt värde, omsättningstillgångar;;;
;;;;[Ej K2];8321;Orealiserade värdeförändringar på omsättningstillgångar;;;
;;;;[Ej K2];8325;Orealiserade värdeförändringar på derivatinstrument (oms.-tillg.);;;
;■;833;Valutakursdifferenser på kortfristiga fordringar och placeringar;■;8330;Valutakursdifferenser på kortfristiga fordringar och placeringar;;;
;;;;;8331;Valutakursvinster på kortfristiga fordringar och placeringar;;;)"
R"(;;;;; 8336; Valutakursförluster på kortfristiga fordringar och placeringar;;;
;■;834;Utdelningar på kortfristiga placeringar;■;8340;Utdelningar på kortfristiga placeringar;;;
;■;835;Resultat vid försäljning av kortfristiga placeringar;■;8350;Resultat vid försäljning av kortfristiga placeringar;;;
;;836;Övriga ränteintäkter från koncernföretag;;8360;Övriga ränteintäkter från koncernföretag;;;
;;;;;8361;Övriga ränteintäkter från moderföretag;;;
;;;;;8362;Övriga ränteintäkter från dotterföretag;;;
;;;;;8363;Övriga ränteintäkter från andra koncernföretag;;;
;;837;Nedskrivningar av kortfristiga placeringar;;8370;Nedskrivningar av kortfristiga placeringar;;;
;;838;Återföringar av nedskrivningar av kortfristiga placeringar;;8380;Återföringar av nedskrivningar av kortfristiga placeringar;;;
;■;839;Övriga finansiella intäkter;■;8390;Övriga finansiella intäkter;;;
;;84;Räntekostnader och liknande resultatposter;;;;;;
;;840;Räntekostnader (gruppkonto);;8400;Räntekostnader (gruppkonto);;;
;■;841;Räntekostnader för långfristiga skulder;■;8410;Räntekostnader för långfristiga skulder;;;
;;;;;8411;Räntekostnader för obligations-, förlags- och konvertibla lån;;;
;;;;;8412;Räntedel i årets pensionskostnad;;;
;;;;;8413;Räntekostnader för checkräkningskredit;;;
;;;;;8415;Räntekostnader för andra skulder till kreditinstitut;;;
;;;;;8417;Räntekostnader för dold räntekompensation m.m.;;;
;;;;;8418;Avdragspost för räntesubventioner;;;
;;;;;8419;Övriga räntekostnader för långfristiga skulder;;;
;■;842;Räntekostnader för kortfristiga skulder;■;8420;Räntekostnader för kortfristiga skulder;;;
;;;;;8421;Räntekostnader till kreditinstitut;;;
;;;;■;8422;Dröjsmålsräntor för leverantörsskulder;;;
;;;;■;8423;Räntekostnader för skatter och avgifter;;;
;;;;;8424;Räntekostnader byggnadskreditiv;;;
;;;;;8429;Övriga räntekostnader för kortfristiga skulder;;;
;■;843;Valutakursdifferenser på skulder;■;8430;Valutakursdifferenser på skulder;;;
;;;;;8431;Valutakursvinster på skulder;;;
;;;;;8436;Valutakursförluster på skulder;;;
;;844;Erhållna räntebidrag;;8440;Erhållna räntebidrag;;;
;[Ej K2];845;Orealiserade värdeförändringar på skulder;[Ej K2];8450;Orealiserade värdeförändringar på skulder;;;
;;;;[Ej K2];8451;Orealiserade värdeförändringar på skulder;;;
;;;;[Ej K2];8455;Orealiserade värdeförändringar på säkringsinstrument;;;
;;846;Räntekostnader till koncernföretag;;8460;Räntekostnader till koncernföretag;;;
;;;;;8461;Räntekostnader till moderföretag;;;
;;;;;8462;Räntekostnader till dotterföretag;;;
;;;;;8463;Räntekostnader till andra koncernföretag;;;
;[Ej K2];848;Aktiverade ränteutgifter;[Ej K2];8480;Aktiverade ränteutgifter;;;
;;849;Övriga skuldrelaterade poster;;8490;Övriga skuldrelaterade poster;;;
;;;;;8491;Erhållet ackord på skulder till kreditinstitut m.m.;;;
;;85;(Fri kontogrupp);;;;;;
;;86;(Fri kontogrupp);;;;;;
;;87;(Fri kontogrupp);;;;;;
;;88;Bokslutsdispositioner;;;;;;
;;881;Förändring av periodiseringsfond;;8810;Förändring av periodiseringsfond;;;
;;;;■;8811;Avsättning till periodiseringsfond;;;
;;;;■;8819;Återföring från periodiseringsfond;;;
;;882;Mottagna koncernbidrag;;8820;Mottagna koncernbidrag;;;
;;883;Lämnade koncernbidrag;;8830;Lämnade koncernbidrag;;;
;;884;Lämnade gottgörelser;;8840;Lämnade gottgörelser;;;
;■;885;Förändring av överavskrivningar;■;8850;Förändring av överavskrivningar;;;
;;;;;8851;Förändring av överavskrivningar, immateriella anläggningstillgångar;;;
;;;;;8852;Förändring av överavskrivningar, byggnader och markanläggningar;;;
;;;;;8853;Förändring av överavskrivningar, maskiner och inventarier;;;
;;886;Förändring av ersättningsfond;;8860;Förändring av ersättningsfond;;;
;;;;;8861;Avsättning till ersättningsfond för inventarier;;;
;;;;;8862;Avsättning till ersättningsfond för byggnader och markanläggningar;;;
;;;;;8864;Avsättning till ersättningsfond för djurlager i jordbruk och renskötsel;;;
;;;;;8865;Ianspråktagande av ersättningsfond för avskrivningar;;;
;;;;;8866;Ianspråktagande av ersättningsfond för annat än avskrivningar;;;
;;;;;8869;Återföring från ersättningsfond;;;
;;889;Övriga bokslutsdispositioner;;8890;Övriga bokslutsdispositioner;;;
;;;;;8892;Nedskrivningar av konsolideringskaraktär av anläggningstillgångar;;;
;;;;;8896;Förändring av lagerreserv;;;
;;;;;8899;Övriga bokslutsdispositioner;;;
;;89;Skatter och årets resultat;;;;;;
;■;891;Skatt som belastar årets resultat;■;8910;Skatt som belastar årets resultat;;;
;;892;Skatt på grund av ändrad beskattning;;8920;Skatt på grund av ändrad beskattning;;;
;;893;Restituerad skatt;;8930;Restituerad skatt;;;
;[Ej K2];894;Uppskjuten skatt;[Ej K2];8940;Uppskjuten skatt;;;
;;898;Övriga skatter;;8980;Övriga skatter;;;
;■;899;Resultat;■;8990;Resultat;;;
;;;;■;8999;Årets resultat;;;
	)"};
}
