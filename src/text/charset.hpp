#pragma once

#include <string>
#include <map>
#include <algorithm>
#include <iostream>

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

		char16_t iso8859ToUnicode(char ch8859);
		char16_t_string iso8859ToUnicode(std::string s8859);
		uint8_t UnicodeToISO8859(char16_t unicode);

	}

	namespace CP437 {
		extern std::map<char,char16_t> cp437ToUnicodeMap;

		char16_t cp437ToUnicode(char ch437);
		uint8_t UnicodeToCP437(char16_t unicode);
		char16_t_string cp437ToUnicode(std::string s437);

	} // namespace CP437

} // namespace charset