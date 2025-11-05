#pragma once

// main.hpp
// A refactoring step to break out a 'header part'
// for the zeroth/main.cpp.

float const VERSION = 0.5;

#include "tokenize.hpp"
#include "env/environment.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"
#include "MetaDefacto.hpp" // MetaDefacto<Meta,Defacto>
#include "fiscal/amount/HADFramework.hpp"
#include "fiscal/BASFramework.hpp"
#include "fiscal/SKVFramework.hpp"
#include "sie/SIEEnvironmentFramework.hpp"
#include "PersistentFile.hpp"
#include "text/charset.hpp"
#include "text/encoding.hpp"
#include "sie/sie.hpp"
#include "csv/csv.hpp"
#include "csv/projections.hpp"
#include "text/functional.hpp" // functional::text::filtered
#include "text/format.hpp" // text::format::to_hex_string
#include <iostream>
#include <locale>
#include <string>
#include <sstream>
#include <queue>
#include <variant>
#include <vector>
#include <optional>
#include <string_view>
#include <filesystem>
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
#include <coroutine> // Requires C++23 support
#include <csignal>
#include <format>
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include <unicode/regex.h>
#include <unicode/unistr.h>

// Define a signal handler function that does nothing
inline void handle_winch(int sig) {
    // Do nothing
}


// Helpers to stream this pointer for logging purposes
template<typename T>
concept PointerToClassType = std::is_pointer_v<T> && std::is_class_v<std::remove_pointer_t<T>>;
template<PointerToClassType T>
inline std::ostream& operator<<(std::ostream& os, T ptr) {
    os << " memory[" << std::hex << std::showbase << reinterpret_cast<std::uintptr_t>(ptr) << "]" << std::dec;
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

// namespace charset now in charset.hpp/cpp

// namespace std_overload now in std_overload unit

// namespace encoding now in encoding unit

// namespace Key now in AmountsFramework unit

// functional::text::filtered now in AmountFramework unit

// WrappedCentsAmount, IntCentsAmount etc.  now in AmountFramework unit

// Date and DateRange related code now in AmountFramework unit

namespace sie {

  auto to_financial_year_range = [](std::chrono::year financial_year_start_year, std::chrono::month financial_year_start_month) -> zeroth::DateRange {
      auto start_date = std::chrono::year_month_day{financial_year_start_year, financial_year_start_month, std::chrono::day{1}};
      auto next_start_date = std::chrono::year_month_day{financial_year_start_year + std::chrono::years{1}, financial_year_start_month, std::chrono::day{1}};
      auto end_date_sys_days = std::chrono::sys_days(next_start_date) - std::chrono::days{1};
      auto end_date = std::chrono::year_month_day{end_date_sys_days};
      return {start_date, end_date};
  };

  // SIE defines a relative index 0,-1,-2 for financial years relative the financial year of current date 
  using YearIndex2DateRangeMap = std::map<std::string,zeroth::DateRange>;
  inline YearIndex2DateRangeMap current_date_to_year_id_map(std::chrono::month financial_year_start_month,int index_count) {
    YearIndex2DateRangeMap result;
    
    auto today = std::chrono::year_month_day{floor<std::chrono::days>(std::chrono::system_clock::now())};
    auto current_year = today.year();

    for (int i = 0; i < index_count; ++i) {
        auto financial_year_start_year = current_year - std::chrono::years{i};
        auto range = to_financial_year_range(financial_year_start_year, financial_year_start_month);
        result.emplace(std::to_string(-i),range); // emplace insert to avoid default constructing a zeroth::DateRange
    }

    if (true) {
      // TODO 240630 - Fails to link with clang++18 on macOS 12.7 for streaming std::chrono::month financial_year_start_month (stream the index for now)
      //               G++ 14 is able to link code that streams the month type.
      std::cout << "\ncurrent_date_to_year_id_map(financial_year_start_month:" << static_cast<unsigned>(financial_year_start_month) << ",index_count:" << index_count << ")";
      for (auto const& [index,date_range] : result) {
        std::cout << "\n\tindex:" << index << " range:" << date_range;
      } 
    }

    return result;    
  }
}

// Content Addressable Storage namespace
// namespace cas now in environment unit

// Tagged Amounts related code is now in TaggedAmountFramework unit

namespace SKV::SRU::INK1 {
	extern const char* ink1_csv_to_sru_template;
	extern const char* k10_csv_to_sru_template;
}
namespace SKV::SRU::INK2 {
  namespace Y_2024 {
    extern const char* INK2_csv_to_sru_template;
    extern const char* INK2S_csv_to_sru_template;
    extern const char* INK2R_csv_to_sru_template;
    extern const char* info_ink2_template;
    extern const char* blanketter_ink2_ink2s_ink2r_template;
  }
}

namespace BAS::SRU::INK2 {
	extern char const* INK2_19_P1_intervall_vers_2_csv;
	void parse(char const* INK2_19_P1_intervall_vers_2_csv);
}

namespace BAS::K2::AR {

  namespace ar_online {
    // Code to operate on open data from URL:https://www.arsredovisning-online.se/bas_kontoplan
  	extern char const* bas_2024_mapping_to_k2_ar_text;

    namespace detail {

      using Lisp = std::string;

      struct State {
        bool success{};
        std::string parsed{};
        std::string msg{};
        std::size_t index{};
        std::string s;
      };

      inline std::ostream& operator<<(std::ostream& os,State const& state) {
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

    inline Predicate to_predicate(std::string const& field_heading_text, std::string const& bas_accounts_text) {
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
  inline Tokens tokenize(std::string const& bas_accounts_text) {
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

	// A test function to parse the bas_2024_mapping_to_k2_ar_text into AREntries
	inline AREntries parse(char const* bas_2024_mapping_to_k2_ar_text) {
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
		std::istringstream in{bas_2024_mapping_to_k2_ar_text};
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
			std::cout << "\n  From listing at URL https://www.arsredovisning-online.se/bas_kontoplan";
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
		std::istringstream words{bas_2024_mapping_to_k2_ar_text};
		auto count = std::ranges::count_if(std::ranges::istream_view<std::string>(words), [](std::string const& word) {return word == "Konto";});
		std::cout << "\nCount of 'Konto' in source text:" << count << " and parsed entry count is:" << result.size();
		if (count == result.size()) std::cout << " ==> OK!";
		else std::cout << " ** ERROR (must be equal = all must be parsed) **";
#else
		// Warn that we have no code to check the input / parsed result correctness
		std::cout << "\nWARNING: I Failed to check parse result of input bas_2024_mapping_to_k2_ar_text (Because this code is compiled with a compiler that does not support std::ranges)";
		std::cout << "\nWARNING: If I have failed to parse bas_2024_mapping_to_k2_ar_text, I may generate an incorrect Annual Financial Statement (Swedish Årsredovisning)";
		std::cout << "\nNOTE: I parse bas_2024_mapping_to_k2_ar_text to create a mapping between BAS accounts and fields on the Annual Financial Statement (Swedish Årsredovisning)";
#endif
		return result;
	}
}

// BAS::bas_2022_account_plan_csv now in BASFramework unit

namespace SKV::XML {
	using XMLMap = std::map<std::string,std::string>;
}
namespace SKV::XML::TAXReturns {
	extern SKV::XML::XMLMap tax_returns_template; // See bottom of this file
}

// SKV::XML::VatReturns::ACCOUNT_VAT_CSV now in SKVFramework unit

// Now in BASFramework unit:
// template <typename I>
// std::vector<std::pair<I,I>> to_ranges(std::vector<I> line_nos) ...
// operator<<(std::ostream& os,std::vector<std::pair<I,I>> const& ranges) ...

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
	inline Component& operator<<(Component& c,ComponentPtr const& p) {
		return c;
	}

	inline ComponentPtr plain_text(std::string const& s) {
		return std::make_shared<Component>(Component {
			.pDefacto = std::make_shared<Defacto>(std::make_shared<Leaf>(leaf::Text{s}))
		});
	}

	inline ComponentPtr separate_page() {
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

	inline Document& operator<<(Document& doc,ComponentPtr const& p) {
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

	inline OStream& operator<<(OStream& os,doc::ComponentPtr const& p) {
		if (p) {
			if (p->pMeta) std::visit(os.meta_state,*p->pMeta);
			DefactoStreamer ostreamer{os};
			if (p->pDefacto) std::visit(ostreamer,*p->pDefacto);
		}
		return os;
	}

	inline OStream& operator<<(OStream& os,doc::Document const& doc) {
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

	inline OStream& operator<<(OStream& os,doc::ComponentPtr const& cp) {
		os.os << "\nTODO: stream doc::ComponentPtr";
		return os;
	}

	inline OStream& operator<<(OStream& os,doc::Document const& doc) {
		os.os << "\nTODO: stream doc::Document";
		return os;
	}

} // namespace HTML


auto utf_ignore_to_upper_f = [](char ch) {
	if (ch <= 0x7F) return static_cast<char>(std::toupper(ch));
	return ch;
};

inline std::string utf_ignore_to_upper(std::string const& s) {
	std::string result{};
	std::transform(s.begin(),s.end(),std::back_inserter(result),utf_ignore_to_upper_f);
	return result;
}

inline std::vector<std::string> utf_ignore_to_upper(std::vector<std::string> const& tokens) {
	std::vector<std::string> result{};
	auto f = [](std::string s) {return utf_ignore_to_upper(s);};
	std::transform(tokens.begin(),tokens.end(),std::back_inserter(result),f);
	return result;
}

inline bool strings_share_tokens(std::string const& s1,std::string const& s2) {
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

inline bool first_in_second_case_insensitive(std::string const& s1, std::string const& s2) {
	auto upper_s1 = utf_ignore_to_upper(s1);
	auto upper_s2 = utf_ignore_to_upper(s2);
	return (upper_s2.find(upper_s1) != std::string::npos);
}

// to_four_digit_positive_int now in SKVFramework unit
// namespace SKV now in SKVFramework unit

// Now in csv-unit
// namespace CSV
// namespace CSV::NORDEA
// namespace CSV::SKV

// Now in tas::projection (TaggedAmountFramework unit)
// using OptionalDateOrderedTaggedAmounts = std::optional<DateOrderedTaggedAmountsContainer>;				
// OptionalDateOrderedTaggedAmounts to_tagged_amounts(std::filesystem::path const& path) {


inline unsigned first_digit(BAS::AccountNo account_no) {
	return account_no / 1000;
}


// MetaDefacto now in MetaDefacto.hpp
// namespace BAS now in BASFramework unit

// Now in BASFramework unit / 20251028
// Amount to_positive_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje);
// Amount to_negative_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje);
// void for_each_anonymous_account_transaction(BAS::anonymous::JournalEntry const& aje,auto& f);

namespace BAS {

    // Now in BASFramework unit
    // 	Amount mats_sum(BAS::MetaAccountTransactions const& mats) {

	using MatchesMetaEntry = std::function<bool(BAS::MDJournalEntry const& mdje)>;

	inline BAS::OptionalAccountNo to_account_no(std::string const& s) {
		return to_four_digit_positive_int(s);
	}

	inline OptionalJournalEntryMeta to_journal_meta(std::string const& s) {
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

	BAS::MDJournalEntry& sort(BAS::MDJournalEntry& mdje,auto& comp) {
		std::sort(mdje.defacto.account_transactions.begin(),mdje.defacto.account_transactions.end(),comp);
		return mdje;
	}

	namespace filter {
		struct is_series {
			BAS::Series required_series;
			bool operator()(MDJournalEntry const& mdje) {
				return (mdje.meta.series == required_series);
			}
		};

		class HasGrossAmount {
		public:
			HasGrossAmount(Amount gross_amount) : m_gross_amount(gross_amount) {}
			bool operator()(BAS::MDJournalEntry const& mdje) {
				if (m_gross_amount<0) {
					return (to_negative_gross_transaction_amount(mdje.defacto) == m_gross_amount);
				}
				else {
					return (to_positive_gross_transaction_amount(mdje.defacto) == m_gross_amount);
				}
			}
		private:
			Amount m_gross_amount;
		};

		class HasTransactionToAccount {
		public:
			HasTransactionToAccount(BAS::AccountNo bas_account_no) : m_bas_account_no(bas_account_no) {}
			bool operator()(BAS::MDJournalEntry const& mdje) {
				return std::any_of(mdje.defacto.account_transactions.begin(),mdje.defacto.account_transactions.end(),[this](BAS::anonymous::AccountTransaction const& at){
					return (at.account_no == this->m_bas_account_no);
				});
			}
		private:
			BAS::AccountNo m_bas_account_no;
		};

		struct is_flagged_unposted {
			bool operator()(MDJournalEntry const& mdje) {
				return (mdje.meta.unposted_flag and *mdje.meta.unposted_flag); // Rely on C++ short-circuit (https://en.cppreference.com/w/cpp/language/operator_logical)
			}
		};

		struct contains_account {
			BAS::AccountNo account_no;
			bool operator()(MDJournalEntry const& mdje) {
				return std::any_of(mdje.defacto.account_transactions.begin(),mdje.defacto.account_transactions.end(),[this](auto const& at){
					return (this->account_no == at.account_no);
				});
			}
		};

		struct matches_meta {
			JournalEntryMeta entry_meta;
			bool operator()(MDJournalEntry const& mdje) {
				return (mdje.meta == entry_meta);
			}
		};

		struct matches_amount {
			Amount amount;
			bool operator()(MDJournalEntry const& mdje) {
				return std::any_of(mdje.defacto.account_transactions.begin(),mdje.defacto.account_transactions.end(),[this](auto const& at){
					return (this->amount == at.amount);
				});
			}
		};

		struct matches_heading {
			std::string user_search_string;
			bool operator()(MDJournalEntry const& mdje) {
				bool result{false};
				result = strings_share_tokens(user_search_string,mdje.defacto.caption);
				if (!result) {
					result = std::any_of(mdje.defacto.account_transactions.begin(),mdje.defacto.account_transactions.end(),[this](auto const& at){
						if (at.transtext) return strings_share_tokens(user_search_string,*at.transtext);
						return false;
					});
				}
				return result;
			}
		};

		struct matches_user_search_criteria{
			std::string user_search_string;
			bool operator()(MDJournalEntry const& mdje) {
				bool result{false};
				if (!result and user_search_string.size()==1) {
					result = is_series{user_search_string[0]}(mdje);
				}
				if (auto account_no = to_account_no(user_search_string);!result and account_no) {
					result = contains_account{*account_no}(mdje);
				}
				if (auto meta = to_journal_meta(user_search_string);!result and meta) {
					result = matches_meta{*meta}(mdje);
				}
				if (auto amount = to_amount(user_search_string);!result and amount) {
					result = matches_amount{*amount}(mdje);
				}
				if (!result) {
					result = matches_heading{user_search_string}(mdje);
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
	} // anonymous

	using MDTypedJournalEntry = MetaDefacto<BAS::JournalEntryMeta,anonymous::TypedJournalEntry>;
	using TypedMetaEntries = std::vector<MDTypedJournalEntry>;

	inline void for_each_typed_account_transaction(BAS::MDTypedJournalEntry const& mdtje,auto& f) {
		for (auto const& tat : mdtje.defacto.account_transactions) {
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

		inline ATType to_at_type(std::string const& prop) {
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

		inline std::size_t to_at_types_order(BAS::kind::AccountTransactionTypeTopology const& topology) {
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

		inline std::vector<std::string> sorted(AccountTransactionTypeTopology const& topology) {
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

		inline BASAccountTopology to_accounts_topology(MDJournalEntry const& mdje) {
			BASAccountTopology result{};
			auto f = [&result](BAS::anonymous::AccountTransaction const& at) {
				result.insert(at.account_no);
			};
			for_each_anonymous_account_transaction(mdje.defacto,f);
			return result;
		}

		inline BASAccountTopology to_accounts_topology(MDTypedJournalEntry const& tme) {
			BASAccountTopology result{};
			auto f = [&result](BAS::anonymous::TypedAccountTransaction const& tat) {
				auto const& [at,props] = tat;
				result.insert(at.account_no);
			};
			for_each_typed_account_transaction(tme,f);
			return result;
		}

		inline AccountTransactionTypeTopology to_types_topology(MDTypedJournalEntry const& tme) {
			AccountTransactionTypeTopology result{};
			auto f = [&result](BAS::anonymous::TypedAccountTransaction const& tat) {
				auto const& [at,props] = tat;
				for (auto const& prop : props) result.insert(prop);
			};
			for_each_typed_account_transaction(tme,f);
			return result;
		}

		inline std::size_t to_signature(BASAccountTopology const& bat) {
			return detail::hash<BASAccountTopology>{}(bat);
		}

		inline std::size_t to_signature(AccountTransactionTypeTopology const& met) {
			return detail::hash<AccountTransactionTypeTopology>{}(met);
		}

	} // namespace kind

	namespace group {

	}
} // namespace BAS

using Sru2BasMap = std::map<SKV::SRU::AccountNo,BAS::AccountNos>;

inline Sru2BasMap sru_to_bas_map(BAS::AccountMetas const& metas) {
	Sru2BasMap result{};
	for (auto const& [bas_account_no,am] : metas) {
		if (am.sru_code) result[*am.sru_code].push_back(bas_account_no);
	}
	return result;
}

// now in SKVFramework unit:
    // namespace SKV::XML::VatReturns
    // to_vat_returns_form_bas_accounts
    // to_vat_accounts

inline auto is_any_of_accounts(BAS::MDAccountTransaction const mdat,BAS::AccountNos const& account_nos) {
	return std::any_of(account_nos.begin(),account_nos.end(),[&mdat](auto other){
		return other == mdat.defacto.account_no;
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

// Now in text/format unit
// std::ostream& operator<<(std::ostream& os,std::optional<std::string> const& opt_s) {

inline std::optional<std::filesystem::path> path_to_existing_file(std::string const& s) {
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

inline OptionalNameHeadingAmountAT to_name_heading_amount(std::vector<std::string> const& ast) {
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

inline BAS::anonymous::OptionalAccountTransaction to_bas_account_transaction(std::vector<std::string> const& ast) {
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

// Now in BASFramework
// std::ostream& operator<<(std::ostream& os,BAS::anonymous::AccountTransaction const& at) {
// 	if (BAS::global_account_metas().contains(at.account_no)) os << std::quoted(BAS::global_account_metas().at(at.account_no).name) << ":";
// 	os << at.account_no;
// 	os << " " << at.transtext;
// 	os << " " << to_string(at.amount); // When amount is double there will be no formatting of the amount to ensure two decimal cents digits
// 	return os;
// };

// std::string to_string(BAS::anonymous::AccountTransaction const& at) {
// 	std::ostringstream os{};
// 	os << at;
// 	return os.str();
// };

// std::ostream& operator<<(std::ostream& os,BAS::anonymous::AccountTransactions const& ats) {
// 	for (auto const& at : ats) {
// 		// os << "\n\t" << at;
// 		os << "\n  " << at;
// 	}
// 	return os;
// }

// std::ostream& operator<<(std::ostream& os,BAS::anonymous::JournalEntry const& aje) {
// 	os << std::quoted(aje.caption) << " " << aje.date;
// 	os << aje.account_transactions;
// 	return os;
// };

// std::ostream& operator<<(std::ostream& os,BAS::OptionalVerNo const& verno) {
// 	if (verno and *verno!=0) os << *verno;
// 	else os << " _ ";
// 	return os;
// }

// std::ostream& operator<<(std::ostream& os,std::optional<bool> flag) {
// 	auto ch = (flag)?((*flag)?'*':' '):' '; // '*' if known and set, else ' '
// 	os << ch;
// 	return os;
// }

// std::ostream& operator<<(std::ostream& os,BAS::JournalEntryMeta const& jem) {
// 	os << jem.unposted_flag << jem.series << jem.verno;
// 	return os;
// }

// std::ostream& operator<<(std::ostream& os,BAS::MetaAccountTransaction const& mat) {
// 	os << mat.meta.meta << " " << mat.defacto;
// 	return os;
// };

// std::ostream& operator<<(std::ostream& os,BAS::MetaEntry const& me) {
// 	os << me.meta << " " << me.defacto;
// 	return os;
// }

// std::ostream& operator<<(std::ostream& os,BAS::MetaEntries const& mes) {
// 	for (auto const& me : mes) {
// 		os << "\n" << me;
// 	}
// 	return os;
// };

// std::string to_string(BAS::anonymous::JournalEntry const& aje) {
// 	std::ostringstream os{};
// 	os << aje;
// 	return os.str();
// };

// std::string to_string(BAS::MetaEntry const& me) {
// 	std::ostringstream os{};
// 	os << me;
// 	return os.str();
// };

// TYPED ENTRY

inline std::ostream& operator<<(std::ostream& os,BAS::kind::BASAccountTopology const& accounts) {
	if (accounts.size()==0) os << " ?";
	else for (auto const& account : accounts) {
		os << " " << account;
	}
	return os;
}

inline std::ostream& operator<<(std::ostream& os,BAS::kind::AccountTransactionTypeTopology const& props) {
	auto sorted_props = BAS::kind::sorted(props); // props is a std::set, sorted_props is a vector
	if (sorted_props.size()==0) os << " ?";
	else for (auto const& prop : sorted_props) {
		os << " " << prop;
	}
	os << " = sort_code: 0x" << std::hex << BAS::kind::to_at_types_order(props) << std::dec;
	return os;
}

inline std::ostream& operator<<(std::ostream& os,BAS::anonymous::TypedAccountTransaction const& tat) {
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

inline std::ostream& operator<<(std::ostream& os,IndentedOnNewLine<BAS::anonymous::TypedAccountTransactions> const& indented) {
	for (auto const& at : indented.val) {
		os << "\n";
		for (int x = 0; x < indented.count; ++x) os << ' ';
		os << at;
	}
	return os;
}

inline std::ostream& operator<<(std::ostream& os,BAS::anonymous::TypedJournalEntry const& tje) {
	os << std::quoted(tje.caption) << " " << tje.date;
	for (auto const& tat : tje.account_transactions) {
		os << "\n\t" << tat;
	}
	return os;
}

inline std::ostream& operator<<(std::ostream& os,BAS::MDTypedJournalEntry const& tme) {
	os << tme.meta << " " << tme.defacto;
	return os;
}

// JOURNAL

// Now in BASFramework unit / 20251028
// using BASJournal = std::map<BAS::VerNo,BAS::anonymous::JournalEntry>;
// using BASJournalId = char; // The Id of a single BAS journal is a series character A,B,C,...
// using BASJournals = std::map<BASJournalId,BASJournal>; // Swedish BAS Journals named "Series" and labeled with "Id" A,B,C,...

inline TaggedAmount to_tagged_amount(Date const& date,BAS::anonymous::AccountTransaction const& at) {
	auto cents_amount = to_cents_amount(at.amount);
	TaggedAmount result{date,cents_amount};
	result.tags()["BAS"] = std::to_string(at.account_no);
	if (at.transtext and at.transtext->size() > 0) result.tags()["TRANSTEXT"] = *at.transtext;
	return result;
}

inline TaggedAmounts to_tagged_amounts(BAS::MDJournalEntry const& mdje) {
  if (false) {
    std::cout << "\nto_tagged_amounts(me)";
  }
	TaggedAmounts result{};
	auto journal_id = mdje.meta.series;
	auto verno = mdje.meta.verno;
	auto date = mdje.defacto.date;
	auto gross_cents_amount = to_cents_amount(to_positive_gross_transaction_amount(mdje.defacto));
	TaggedAmount::Tags tags{};
	tags["type"] = "aggregate";
	if (verno) tags["SIE"] = journal_id+std::to_string(*verno);
	tags["vertext"] = mdje.defacto.caption;
	TaggedAmount aggregate_ta{date,gross_cents_amount,std::move(tags)};
	Key::Sequence value_ids{};

	auto push_back_as_tagged_amount = [&value_ids,&date,&journal_id,&verno,&result](BAS::anonymous::AccountTransaction const& at){
		auto ta = to_tagged_amount(date,at);
    if (verno) ta.tags()["parent_SIE"] = journal_id+std::to_string(*verno);
    ta.tags()["Ix"]=std::to_string(result.size()); // index 0,1,2...
		result.push_back(ta);
		value_ids += text::format::to_hex_string(to_value_id(ta));
	};

	for_each_anonymous_account_transaction(mdje.defacto,push_back_as_tagged_amount);
	aggregate_ta.tags()["_members"] = value_ids.to_string();
	result.push_back(aggregate_ta);
	return result;
}

// class ToNetVatAccountTransactions now in BASFramework unit

// struct and using HeadingAmountDateTransEntry now in HADFramework unit
// operator<< for HeadingAmountDateTransEntry now in HADFramework unit


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
		inline CSV::NORDEA::istream& operator>>(CSV::NORDEA::istream& in,HeadingAmountDateTransEntry& had) {
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

	inline CSVParseResult parse_TRANS(auto& in) {
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

	inline HeadingAmountDateTransEntries from_stream(auto& in,BAS::OptionalAccountNo gross_bas_account_no = std::nullopt) {
		HeadingAmountDateTransEntries result{};
		parse_TRANS(in); // skip first line with field names
		while (auto had = parse_TRANS(in)) {
			if (gross_bas_account_no) {
// std::cout << "\nfrom_stream to gross_bas_account_no:" << *gross_bas_account_no;
				// Add a template with the gross amount transacted to provided bas account no
				BAS::MDJournalEntry mdje{
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
				mdje.defacto.account_transactions.push_back(gross_at);
				had->optional.current_candidate = mdje;
			}
			result.push_back(*had);
		}
		return result;
	}
} // namespace CSV

// Now in sie-unit
// namespace SIE
// to_sie_t(BAS::anonymous::AccountTransaction const& trans)
// to_sie_t(BAS::MetaEntry const& me) {

inline bool is_vat_returns_form_at(std::vector<SKV::XML::VATReturns::BoxNo> const& box_nos,BAS::anonymous::AccountTransaction const& at) {
	auto const& bas_account_nos = to_vat_returns_form_bas_accounts(box_nos);
	return bas_account_nos.contains(at.account_no);
}

inline bool is_vat_account(BAS::AccountNo account_no) {
	auto const& vat_accounts = to_vat_accounts();
	return vat_accounts.contains(account_no);
}

auto is_vat_account_at = [](BAS::anonymous::AccountTransaction const& at){
	return is_vat_account(at.account_no);
};

// Now in BASFramework  unit / 20251028
// bool does_balance(BAS::anonymous::JournalEntry const& aje) {

// Pick the negative or positive gross amount and return it without sign
inline OptionalAmount to_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje) {
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

inline BAS::anonymous::OptionalAccountTransaction gross_account_transaction(BAS::anonymous::JournalEntry const& aje) {
	BAS::anonymous::OptionalAccountTransaction result{};
	auto trans_amount = to_positive_gross_transaction_amount(aje);
	auto iter = std::find_if(aje.account_transactions.begin(),aje.account_transactions.end(),[&trans_amount](auto const& at){
		return abs(at.amount) == trans_amount;
	});
	if (iter != aje.account_transactions.end()) result = *iter;
	return result;
}

inline Amount to_account_transactions_sum(BAS::anonymous::AccountTransactions const& ats) {
	Amount result = std::accumulate(ats.begin(),ats.end(),Amount{},[](Amount acc,BAS::anonymous::AccountTransaction const& at){
		acc += at.amount;
		return acc;
	});
	return result;
}

inline bool have_opposite_signs(Amount a1,Amount a2) {
	return ((a1 > 0) and (a2 < 0)) or ((a1 < 0) and (a2 > 0)); // Note: false also for a1==a2==0
}

inline BAS::anonymous::AccountTransactions counter_account_transactions(BAS::anonymous::JournalEntry const& aje,BAS::anonymous::AccountTransaction const& gross_at) {
	BAS::anonymous::AccountTransactions result{};
	// Gather all ats with opposite sign and that sums upp to gross_at amount
	std::copy_if(aje.account_transactions.begin(),aje.account_transactions.end(),std::back_inserter(result),[&gross_at](auto const& at){
		return (have_opposite_signs(at.amount,gross_at.amount));
	});
	if (to_account_transactions_sum(result) != -gross_at.amount) result.clear();
	return result;
}

inline BAS::anonymous::OptionalAccountTransaction net_account_transaction(BAS::anonymous::JournalEntry const& aje) {
	BAS::anonymous::OptionalAccountTransaction result{};
	auto trans_amount = to_positive_gross_transaction_amount(aje);
	auto iter = std::find_if(aje.account_transactions.begin(),aje.account_transactions.end(),[&trans_amount](auto const& at){
		return (abs(at.amount) < trans_amount and not is_vat_account_at(at));
		// return abs(at.amount) == 0.8*trans_amount;
	});
	if (iter != aje.account_transactions.end()) result = *iter;
	return result;
}

inline BAS::anonymous::OptionalAccountTransaction vat_account_transaction(BAS::anonymous::JournalEntry const& aje) {
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

	JournalEntryTemplate(BAS::Series series,BAS::MDJournalEntry const& mdje) : m_series{series} {
		if (auto optional_gross_amount = to_gross_transaction_amount(mdje.defacto)) {
			auto gross_amount = *optional_gross_amount;
			if (gross_amount >= 0.01) {
				std::transform(mdje.defacto.account_transactions.begin(),mdje.defacto.account_transactions.end(),std::back_inserter(templates),[gross_amount](BAS::anonymous::AccountTransaction const& at){
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

inline OptionalJournalEntryTemplate to_template(BAS::MDJournalEntry const& mdje) {
	OptionalJournalEntryTemplate result({mdje.meta.series,mdje});
	return result;
}

inline BAS::MDJournalEntry to_md_entry(BAS::MDTypedJournalEntry const& tme) {
	BAS::MDJournalEntry result {
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

inline OptionalJournalEntryTemplate to_template(BAS::MDTypedJournalEntry const& tme) {
	return to_template(to_md_entry(tme));
}

inline BAS::MDJournalEntry to_md_journal_entry(HeadingAmountDateTransEntry const& had,JournalEntryTemplate const& jet) {
	BAS::MDJournalEntry result{};
	result.meta = {
		.series = jet.series()
	};
	result.defacto.caption = had.heading;
	result.defacto.date = had.date;
	result.defacto.account_transactions = jet(abs(had.amount)); // Ignore sign to have template apply its sign
	return result;
}

inline std::ostream& operator<<(std::ostream& os,AccountTransactionTemplate const& att) {
	os << "\n\t" << att.m_at.account_no << " " << att.m_percent;
	return os;
}

inline std::ostream& operator<<(std::ostream& os,JournalEntryTemplate const& entry) {
	os << "template: series " << entry.series();
	std::for_each(entry.templates.begin(),entry.templates.end(),[&os](AccountTransactionTemplate const& att){
		os << "\n\t" << att;
	});
	return os;
}

inline bool had_matches_trans(HeadingAmountDateTransEntry const& had,BAS::anonymous::JournalEntry const& aje) {
	return strings_share_tokens(had.heading,aje.caption);
}

// ==================================================================================

inline bool are_same_and_less_than_100_cents_apart(Amount const& a1,Amount const& a2) {
	bool result = (abs(abs(a1) - abs(a2)) < 1.0);
	return result;
}

inline BAS::MDJournalEntry to_swapped_ats_md_entry(BAS::MDJournalEntry const& mdje,BAS::anonymous::AccountTransaction const& target_at,BAS::anonymous::AccountTransaction const& new_at) {
	BAS::MDJournalEntry result{mdje};
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
inline BAS::MDJournalEntry to_updated_amounts_md_entry(BAS::MDJournalEntry const& mdje,BAS::anonymous::AccountTransaction const& at) {
// std::cout << "\nupdated_amounts_entry";
// std::cout << "\nme:" << me;
// std::cout << "\nat:" << at;

	BAS::MDJournalEntry result{mdje};
	BAS::sort(result,BAS::has_greater_abs_amount);
// std::cout << "\npre-result:" << result;

	auto iter = std::find_if(result.defacto.account_transactions.begin(),result.defacto.account_transactions.end(),[&at](auto const& entry){
		return (entry.account_no == at.account_no);
	});
	auto at_index = std::distance(result.defacto.account_transactions.begin(),iter);
// std::cout << "\nat_index = " << at_index;
	if (iter == result.defacto.account_transactions.end()) {
		result.defacto.account_transactions.push_back(at);
		result = to_updated_amounts_md_entry(result,at); // recurse with added entry
	}
	else if (mdje.defacto.account_transactions.size()==4) {
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
inline std::ostream& operator<<(std::ostream& os,SRUEnvironment const& sru_env) {
	for (auto const& [sru_code,value]: sru_env.m_sru_values) {
		os << "\n\tSRU:" << sru_code << " = ";
		if (value) os << std::quoted(*value);
		else os << "null";
	}
	return os;
}

inline OEnvironmentValueOStream& operator<<(OEnvironmentValueOStream& env_val_os,std::string const& s) {
	env_val_os.os << s;
	return env_val_os;
}

inline OEnvironmentValueOStream& operator<<(OEnvironmentValueOStream& env_val_os,SRUEnvironment const& sru_env) {
	for (auto const& [sru_code,value]: sru_env.m_sru_values) {
		if (value) env_val_os << ";" << std::to_string(sru_code) << "=" << *value;
	}
	return env_val_os;
}

using SRUEnvironments = std::map<std::string,SRUEnvironment>;

// Now in BASFramework unit / 20251028
// struct Balance {
// std::ostream& operator<<(std::ostream& os,Balance const& balance) {
// using Balances = std::vector<Balance>;
// using BalancesMap = std::map<Date,Balances>;
// std::ostream& operator<<(std::ostream& os,BalancesMap const& balances_map) {

// Now in SIEEnvironment unit / 20251028
// class SIEEnvironment {
// using OptionalSIEEnvironment = std::optional<SIEEnvironment>;


// Now in SIEEnvironmentFramework unit / 20251028
// class SIEEnvironmentsMap {

inline BAS::AccountMetas matches_bas_or_sru_account_no(BAS::AccountNo const& to_match_account_no,SIEEnvironment const& sie_env) {
	BAS::AccountMetas result{};
	for (auto const& [account_no,am] : sie_env.account_metas()) {
		if ((to_match_account_no == account_no) or (am.sru_code and (to_match_account_no == *am.sru_code))) {
			result[account_no] = am;
		}
	}
	return result;
}

inline BAS::AccountMetas matches_bas_account_name(std::string const& s,SIEEnvironment const& sie_env) {
	BAS::AccountMetas result{};
	for (auto const& [account_no,am] : sie_env.account_metas()) {
		if (first_in_second_case_insensitive(s,am.name)) {
			result[account_no] = am;
		}
	}
	return result;
}

inline void for_each_anonymous_journal_entry(SIEEnvironment const& sie_env,auto& f) {
	for (auto const& [journal_id,journal] : sie_env.journals()) {
		for (auto const& [verno,aje] : journal) {
			f(aje);
		}
	}
}

inline void for_each_anonymous_journal_entry(SIEEnvironmentsMap const& sie_envs_map,auto& f) {
	for (auto const& [financial_year_key,sie_env] : sie_envs_map) {
		for_each_anonymous_journal_entry(sie_env,f);
	}
}

inline void for_each_md_journal_entry(SIEEnvironment const& sie_env,auto& f) {
	for (auto const& [series,journal] : sie_env.journals()) {
		for (auto const& [verno,aje] : journal) {
			f(BAS::MDJournalEntry{.meta ={.series=series,.verno=verno,.unposted_flag=sie_env.is_unposted(series,verno)},.defacto=aje});
		}
	}
}

inline void for_each_md_journal_entry(SIEEnvironmentsMap const& sie_envs_map,auto& f) {
	for (auto const& [financial_year_key,sie_env] : sie_envs_map) {
		for_each_md_journal_entry(sie_env,f);
	}
}

inline void for_each_anonymous_account_transaction(SIEEnvironment const& sie_env,auto& f) {
	auto f_caller = [&f](BAS::anonymous::JournalEntry const& aje){for_each_anonymous_account_transaction(aje,f);};
	for_each_anonymous_journal_entry(sie_env,f_caller);
}

inline void for_each_md_account_transaction(BAS::MDJournalEntry const& mdje,auto& f) {
	for (auto const& at : mdje.defacto.account_transactions) {
		f(BAS::MDAccountTransaction{
			.meta = BAS::to_account_transaction_meta(mdje)
			,.defacto = at
		});
	}
}

inline void for_each_md_account_transaction(SIEEnvironment const& sie_env,auto& f) {
	auto f_caller = [&f](BAS::MDJournalEntry const& mdje){for_each_md_account_transaction(mdje,f);};
	for_each_md_journal_entry(sie_env,f_caller);
}

inline void for_each_md_account_transaction(SIEEnvironmentsMap const& sie_envs_map,auto& f) {
	auto f_caller = [&f](BAS::MDJournalEntry const& mdje){for_each_md_account_transaction(mdje,f);};
	for (auto const& [financial_year_key,sie_env] : sie_envs_map) {
		for_each_md_journal_entry(sie_env,f_caller);
	}
}

inline OptionalAmount account_sum(SIEEnvironment const& sie_env,BAS::AccountNo account_no) {
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

inline OptionalAmount to_ats_sum(SIEEnvironment const& sie_env,BAS::AccountNos const& bas_account_nos) {
	OptionalAmount result{};
	try {
		Amount amount{};
		auto f = [&amount,&bas_account_nos](BAS::MDAccountTransaction const& mdat) {
			if (std::any_of(bas_account_nos.begin(),bas_account_nos.end(),[&mdat](auto const&  bas_account_no){ return (mdat.defacto.account_no==bas_account_no);})) {
				amount += mdat.defacto.amount;
			}
		};
		for_each_md_account_transaction(sie_env,f);
		result = amount;
	}
	catch (std::exception const& e) {
		std::cout << "\nto_ats_sum failed. Excpetion=" << std::quoted(e.what());
	}
	return result;
}

inline OptionalAmount to_ats_sum(SIEEnvironmentsMap const& sie_envs_map,BAS::AccountNos const& bas_account_nos) {
	OptionalAmount result{};
	try {
		Amount amount{};
		auto f = [&amount,&bas_account_nos](BAS::MDAccountTransaction const& mdat) {
			if (std::any_of(bas_account_nos.begin(),bas_account_nos.end(),[&mdat](auto const&  bas_account_no){ return (mdat.defacto.account_no==bas_account_no);})) {
				amount += mdat.defacto.amount;
			}
		};
		for_each_md_account_transaction(sie_envs_map,f);
		result = amount;
	}
	catch (std::exception const& e) {
		std::cout << "\nto_ats_sum failed. Excpetion=" << std::quoted(e.what());
	}
	return result;
}

inline std::optional<std::string> to_ats_sum_string(SIEEnvironment const& sie_env,BAS::AccountNos const& bas_account_nos) {
	std::optional<std::string> result{};
	if (auto const& ats_sum = to_ats_sum(sie_env,bas_account_nos)) result = to_string(*ats_sum);
	return result;
}

inline std::optional<std::string> to_ats_sum_string(SIEEnvironmentsMap const& sie_envs_map,BAS::AccountNos const& bas_account_nos) {
	std::optional<std::string> result{};
	if (auto const& ats_sum = to_ats_sum(sie_envs_map,bas_account_nos)) result = to_string(*ats_sum);
	return result;
}

auto to_typed_md_entry = [](BAS::MDJournalEntry const& mdje) -> BAS::MDTypedJournalEntry {
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

	if (auto optional_gross_amount = to_gross_transaction_amount(mdje.defacto)) {
		auto gross_amount = *optional_gross_amount;
		// Direct type detection based on gross_amount and account meta data
		for (auto const& at : mdje.defacto.account_transactions) {
			if (round(abs(at.amount)) == round(gross_amount)) typed_ats[at].insert("gross");
			if (is_vat_account_at(at)) typed_ats[at].insert("vat");
			if (abs(at.amount) < 1) typed_ats[at].insert("cents");
			if (round(abs(at.amount)) == round(gross_amount / 2)) typed_ats[at].insert("transfer"); // 20240519 I no longer understand this? A transfer if half the gross? Strange?
		}

		// Ex vat amount Detection
		Amount ex_vat_amount{},vat_amount{};
		for (auto const& at : mdje.defacto.account_transactions) {
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
			for (auto const& at : mdje.defacto.account_transactions) {
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
	for (auto const& at : mdje.defacto.account_transactions) {
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
	for (auto const& at : mdje.defacto.account_transactions) {
		// Identify counter transactions to EU VAT and EU Purchase tagged accounts
		if (at.amount == -eu_vat_amount) typed_ats[at].insert("eu_vat"); // The counter trans for EU VAT
		if ((first_digit(at.account_no) == 4 or first_digit(at.account_no) == 9) and (at.amount == -eu_purchase_amount)) typed_ats[at].insert("eu_purchase"); // The counter trans for EU Purchase
	}
	// Mark gross accounts for EU VAT transaction journal entry
	for (auto const& at : mdje.defacto.account_transactions) {
		// We expect two accounts left unmarked and they are the gross accounts
		if (!typed_ats.contains(at) and (abs(at.amount) == abs(eu_purchase_amount))) {
			typed_ats[at].insert("gross");
		}
	}

	// Finally add any still untyped at with empty property set
	for (auto const& at : mdje.defacto.account_transactions) {
		if (!typed_ats.contains(at)) typed_ats.insert({at,{}});
	}

	BAS::MDTypedJournalEntry result{
		.meta = mdje.meta
		,.defacto = {
			.caption = mdje.defacto.caption
			,.date = mdje.defacto.date
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

inline std::ostream& operator<<(std::ostream& os,JournalEntryVATType const& vat_type) {
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

inline JournalEntryVATType to_vat_type(BAS::MDTypedJournalEntry const& tme) {
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
      return (at.account_no == 1630 or at.account_no == 2650 or at.account_no == 1650); // SKV account updated with VAT, i.e., cleared
	  // 1630 = SKV tax account, 1650 = SKV tax receivable, 2650 = SKV tax payable
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

inline void for_each_typed_md_entry(SIEEnvironmentsMap const& sie_envs_map,auto& f) {
	auto f_caller = [&f](BAS::MDJournalEntry const& mdje) {
		auto tme = to_typed_md_entry(mdje);
		f(tme);
	};
	for_each_md_journal_entry(sie_envs_map,f_caller);
}

using Kind2MDTypedJournalEntriesMap = std::map<BAS::kind::AccountTransactionTypeTopology,std::vector<BAS::MDTypedJournalEntry>>; // AccountTransactionTypeTopology -> TypedMetaEntry
using Kind2MDTypedJournalEntriesCAS = std::map<std::size_t,Kind2MDTypedJournalEntriesMap>; // hash -> TypeMetaEntry
// TODO: Consider to make MetaEntryTopologyMap an unordered_map (as it is already a map from hash -> TypedMetaEntry)
//       All we should have to do is to define std::hash for this type to make std::unordered_map find it?

inline Kind2MDTypedJournalEntriesCAS to_meta_entry_topology_map(SIEEnvironmentsMap const& sie_envs_map) {
	Kind2MDTypedJournalEntriesCAS result{};
	// Group on Type Topology
	Kind2MDTypedJournalEntriesCAS meta_entry_topology_map{};
	auto h = [&result](BAS::MDTypedJournalEntry const& tme){
		auto types_topology = BAS::kind::to_types_topology(tme);
		auto signature = BAS::kind::to_signature(types_topology);
		result[signature][types_topology].push_back(tme);
	};
	for_each_typed_md_entry(sie_envs_map,h);
	return result;
}

struct TestResult {
	std::ostringstream prompt{"null"};
	bool failed{true};
};

inline std::ostream& operator<<(std::ostream& os,TestResult const& tr) {
	os << tr.prompt.str();
	return os;
}

// A typed sub-meta-entry is a subset of transactions of provided typed meta entry
// that are all of the same "type" and that all sums to zero (do balance)
inline std::vector<BAS::MDTypedJournalEntry> to_typed_sub_meta_entries(BAS::MDTypedJournalEntry const& tme) {
	std::vector<BAS::MDTypedJournalEntry> result{};
	// TODO: When needed, identify sub-entries of typed account transactions that balance (sums to zero)
	result.push_back(tme); // For now, return input as the single sub-entry
	return result;
}

inline BAS::anonymous::TypedAccountTransactions to_alternative_tats(SIEEnvironmentsMap const& sie_envs_map,BAS::anonymous::TypedAccountTransaction const& tat) {
	BAS::anonymous::TypedAccountTransactions result{};
	result.insert(tat); // For now, return ourself as the only alternative
	return result;
}

inline bool operator==(BAS::MDTypedJournalEntry const& tme1,BAS::MDTypedJournalEntry const& tme2) {
	return (BAS::kind::to_types_topology(tme1) == BAS::kind::to_types_topology(tme2));
}

inline BAS::MDTypedJournalEntry to_tats_swapped_tme(BAS::MDTypedJournalEntry const& tme,BAS::anonymous::TypedAccountTransaction const& target_tat,BAS::anonymous::TypedAccountTransaction const& new_tat) {
	BAS::MDTypedJournalEntry result{tme};
	// TODO: Implement actual swap of tats
	return result;
}

inline BAS::OptionalMDJournalEntry to_meta_entry_candidate(BAS::MDTypedJournalEntry const& tme,Amount const& gross_amount) {
	BAS::OptionalMDJournalEntry result{};
	// TODO: Implement actual generation of a candidate using the provided typed meta entry and the gross amount
	auto order_code = BAS::kind::to_at_types_order(BAS::kind::to_types_topology(tme));
	BAS::MDJournalEntry mdje_candidate{
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
							mdje_candidate.defacto.account_transactions.push_back({
								.account_no = tat.first.account_no
								,.transtext = tat.first.transtext
								,.amount = gross_amount
							});
						}; break;
						case 0x4: {
							// net
							mdje_candidate.defacto.account_transactions.push_back({
								.account_no = tat.first.account_no
								,.transtext = tat.first.transtext
								,.amount = static_cast<Amount>(gross_amount*0.8) // NOTE: Hard coded 25% VAT
							});
						}; break;
						case 0x6: {
							// VAT
							mdje_candidate.defacto.account_transactions.push_back({
								.account_no = tat.first.account_no
								,.transtext = tat.first.transtext
								,.amount = static_cast<Amount>(gross_amount*0.2) // NOTE: Hard coded 25% VAT
							});
						}; break;
						case 0x7: {
							// Cents
							mdje_candidate.defacto.account_transactions.push_back({
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
				result = mdje_candidate;
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
							mdje_candidate.defacto.account_transactions.push_back({
								.account_no = tat.first.account_no
								,.transtext = tat.first.transtext
								,.amount = (tat.first.amount<0)?-abs(gross_amount):abs(gross_amount)
							});
						} break;
						case 0x3: {
							// gross +/-
							mdje_candidate.defacto.account_transactions.push_back({
								.account_no = tat.first.account_no
								,.transtext = tat.first.transtext
								,.amount = (tat.first.amount<0)?-abs(gross_amount):abs(gross_amount)
							});
						} break;
						case 0x5: {
							// eu_vat +/-
							auto vat_amount = static_cast<Amount>(((tat.first.amount<0)?-1.0:1.0) * 0.2 * abs(gross_amount));
							mdje_candidate.defacto.account_transactions.push_back({
								.account_no = tat.first.account_no
								,.transtext = tat.first.transtext
								,.amount = vat_amount
							});
						} break;
						// NOTE: case 0x6: vat will hit the same transaction as the eu_vat tagged account trasnactiopn is also tagged vat ;)
					} // switch
				} // for ats
				result = mdje_candidate;
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
							mdje_candidate.defacto.account_transactions.push_back({
								.account_no = tat.first.account_no
								,.transtext = tat.first.transtext
								,.amount = (tat.first.amount<0)?-abs(gross_amount):abs(gross_amount)
							});
						}; break;
					} // switch
				} // for ats
				result = mdje_candidate;
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

inline bool are_same_and_less_than_100_cents_apart(BAS::anonymous::AccountTransaction const& at1, BAS::anonymous::AccountTransaction const& at2) {
	return (     (at1.account_no == at2.account_no)
	         and (at1.transtext == at2.transtext)
					 and (are_same_and_less_than_100_cents_apart(at1.amount,at2.amount)));
}

inline bool are_same_and_less_than_100_cents_apart(BAS::anonymous::AccountTransactions const& ats1, BAS::anonymous::AccountTransactions const& ats2) {
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

inline bool are_same_and_less_than_100_cents_apart(BAS::MDJournalEntry const& me1, BAS::MDJournalEntry const& me2) {
	return (     	(me1.meta == me2.meta)
						and (me1.defacto.caption == me2.defacto.caption)
						and (me1.defacto.date == me2.defacto.date)
						and (are_same_and_less_than_100_cents_apart(me1.defacto.account_transactions,me2.defacto.account_transactions)));
}

inline TestResult test_typed_meta_entry(SIEEnvironmentsMap const& sie_envs_map,BAS::MDTypedJournalEntry const& tme) {
	TestResult result{};
	result.prompt << "test_typed_meta_entry=";
	auto sub_tmes = to_typed_sub_meta_entries(tme);
	for (auto const& sub_tme : sub_tmes) {
		for (auto const& tat : sub_tme.defacto.account_transactions) {
			auto alt_tats = to_alternative_tats(sie_envs_map,tat);
			for (auto const& alt_tat : alt_tats) {
				auto alt_tme = to_tats_swapped_tme(tme,tat,alt_tat);
				result.prompt << "\n\t\t" <<  "Swapped " << tat << " with " << alt_tat;
				// Test that we can do a roundtrip and get the alt_tme back
				auto gross_amount = std::accumulate(alt_tme.defacto.account_transactions.begin(),alt_tme.defacto.account_transactions.end(),Amount{0},[](auto acc, auto const& tat){
					if (tat.first.amount > 0) acc += tat.first.amount;
					return acc;
				});
				auto raw_alt_candidate = to_md_entry(alt_tme); // Raw conversion
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

inline AccountsTopologyMap to_accounts_topology_map(BAS::TypedMetaEntries const& tmes) {
	AccountsTopologyMap result{};
	auto g = [&result](BAS::MDTypedJournalEntry const& tme) {
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

inline BAS::anonymous::AccountTransactions to_gross_account_transactions(BAS::anonymous::JournalEntry const& aje) {
	GrossAccountTransactions ats{};
	ats(aje);
	return ats.result;
}

inline BAS::anonymous::AccountTransactions to_gross_account_transactions(SIEEnvironmentsMap const& sie_envs_map) {
	GrossAccountTransactions ats{};
	for_each_anonymous_journal_entry(sie_envs_map,ats);
	return ats.result;
}

inline BAS::anonymous::AccountTransactions to_net_account_transactions(SIEEnvironmentsMap const& sie_envs_map) {
	NetAccountTransactions ats{};
	for_each_anonymous_journal_entry(sie_envs_map,ats);
	return ats.result;
}

inline BAS::anonymous::AccountTransactions to_vat_account_transactions(SIEEnvironmentsMap const& sie_envs_map) {
	VatAccountTransactions ats{};
	for_each_anonymous_journal_entry(sie_envs_map,ats);
	return ats.result;
}

struct T2 {
	BAS::MDJournalEntry mdje;
	struct CounterTrans {
		BAS::AccountNo linking_account;
		BAS::MDJournalEntry mdje;
	};
	std::optional<CounterTrans> counter_trans{};
};
using T2s = std::vector<T2>;

using T2Entry = std::pair<BAS::MDJournalEntry,T2::CounterTrans>;
using T2Entries = std::vector<T2Entry>;

inline std::ostream& operator<<(std::ostream& os,T2Entry const& t2e) {
	os << t2e.first.meta;
	os << " <- " << t2e.second.linking_account << " ->";
	os << " " << t2e.second.mdje.meta;
	os << "\n 1:" << t2e.first;
	os << "\n 2:" << t2e.second.mdje;
	return os;
}

inline std::ostream& operator<<(std::ostream& os,T2Entries const& t2es) {
	for (auto const& t2e : t2es) os << "\n\n" << t2e;
	return os;
}

inline T2Entries to_t2_entries(T2s const& t2s) {
	T2Entries result{};
	for (auto const& t2 : t2s) {
		if (t2.counter_trans) result.push_back({t2.mdje,*t2.counter_trans});
	}
	return result;
}

struct CollectT2s {
	T2s t2s{};
	T2Entries result() const {
		return to_t2_entries(t2s);
	}
	void operator() (BAS::MDJournalEntry const& mdje) {
		auto t2_iter = t2s.begin();
		for (;t2_iter != t2s.end();++t2_iter) {
			if (!t2_iter->counter_trans) {
				// No counter trans found yet
				auto at_iter1 = std::find_if(mdje.defacto.account_transactions.begin(),mdje.defacto.account_transactions.end(),[&t2_iter](BAS::anonymous::AccountTransaction const& at1){
					auto  at_iter2 = std::find_if(t2_iter->mdje.defacto.account_transactions.begin(),t2_iter->mdje.defacto.account_transactions.end(),[&at1](BAS::anonymous::AccountTransaction const& at2){
						return (at1.account_no == at2.account_no) and (at1.amount == -at2.amount);
					});
					return (at_iter2 != t2_iter->mdje.defacto.account_transactions.end());
				});
				if (at_iter1 != mdje.defacto.account_transactions.end()) {
					// iter refers to an account transaction in mdje to the same account but a counter amount as in t2.je
					T2::CounterTrans counter_trans{.linking_account = at_iter1->account_no,.mdje = mdje};
					t2_iter->counter_trans = counter_trans;
					break;
				}
			}
		}
		if (t2_iter == t2s.end()) {
			t2s.push_back(T2{.mdje = mdje});
		}
	}
};

inline T2Entries t2_entries(SIEEnvironmentsMap const& sie_envs_map) {
	CollectT2s collect_t2s{};
	for_each_md_journal_entry(sie_envs_map,collect_t2s);
	return collect_t2s.result();
}

inline BAS::OptionalMDJournalEntry find_meta_entry(SIEEnvironment const& sie_env, std::vector<std::string> const& ast) {
	BAS::OptionalMDJournalEntry result{};
	try {
		if ((ast.size()==1) and (ast[0].size()>=2)) {
			// Assume A1,M13 etc as designation for the meta entry to find
			auto series = ast[0][0];
			auto s_verno = ast[0].substr(1);
			auto verno = std::stoi(s_verno);
			auto f = [&series,&verno,&result](BAS::MDJournalEntry const& mdje) {
				if (mdje.meta.series == series and mdje.meta.verno == verno) result = mdje;
			};
			for_each_md_journal_entry(sie_env,f);
		}
	}
	catch (std::exception const& e) {
		std::cout << "\nfind_meta_entry failed. Exception=" << std::quoted(e.what());
	}
	return result;
}

// SKV Electronic API (file formats for upload)

namespace SKV { // SKV

	inline int to_tax(Amount amount) {return to_double(trunc(amount));} // See https://www4.skatteverket.se/rattsligvagledning/2477.html?date=2014-01-01#section22-1
	inline int to_fee(Amount amount) {return to_double(trunc(amount));}

	inline zeroth::OptionalDateRange to_date_range(std::string const& period_id) {
		zeroth::OptionalDateRange result{};
		try {
			auto today = to_today();
			const std::regex is_year_quarter("^\\s*\\d\\d-Q[1-4]\\s*$"); // YY-Q1..YY-Q4
			const std::regex is_anonymous_quarter("Q[1-4]"); // Q1..Q4
			if (period_id.size()==0) {
				// default to current quarter
				result = zeroth::to_quarter_range(today);
			}
			else if (std::regex_match(period_id,is_year_quarter)) {
				// Create quarter range of given year YY-Qx
				auto current_century = static_cast<int>(today.year()) / 100;
				std::chrono::year year{current_century*100 + std::stoi(period_id.substr(0,2))};
				std::chrono::month end_month{3u * static_cast<unsigned>(period_id[4] - '0')};
				result = zeroth::to_quarter_range(year/end_month/std::chrono::last);
			}
			else if (std::regex_match(period_id,is_anonymous_quarter)) {
				// Create quarter range of current year Qx
				std::chrono::month end_month{3u * static_cast<unsigned>(period_id[1]-'0')};
				result = zeroth::to_quarter_range(today.year()/end_month/std::chrono::last);
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

    using sru_amount_value_type = int;

    inline sru_amount_value_type to_sru_amount(CentsAmount cents_amount) {

      // NOTE: It is a bit unclear if amounts should be rounded or truncated (or if it does not matter)?
      //       In my testing an online INK2 generator turned 0,70 SEK til 1 SEK (i.e., rounded)
      //       While 25024,84 was turned into 25024 (i.e., truncated)?
      //       For now, I round and hope this is in fact not an error either way?

      //       In "Skatteregler för aktie-och handelsbolag" (https://skatteverket.se/download/18.7d2cc99c18b24bd07c38e19/1708607420810/skatteregler-for-aktie-och-handelsbolag-skv294-utgava22.pdf)
      //       SKV states;

              /* Kronor och avrundning
                  Samtliga belopp i räkenskapsschemats balans- och
                  resultaträkning ska anges i hela kronor. Bortse från
                  eventuella avrundningsfel som kan uppstå.
              */

      // TODO: Clean up the 'amount strong type' mess?
      //       For now we have a lot (to many) of experimental strong-type-amounts?
      //       But keep currency amounts strongly typed while being able to
      //       apply correct rounding to integer amounts (possibly applying different roundings for different domains?).
      //       E.g., Swedish tax agencly may define one way to round to integer amounts? While BAS accounding, banks
      //       invoices etc. may apply (call for) roduing with truncation, floor, ceil etc?

      // For now, go through UnitsAndCents as the intemediate candidate type.
      // CentsAmount -> UnitAndCents -> Amount -> C++ double -> C++ int (applying std::round)

      auto units_and_cents = to_units_and_cents(cents_amount);
      auto amount = round(to_amount(units_and_cents));
      return static_cast<sru_amount_value_type>(to_double(amount));
    }

		struct OStream {
			std::ostream& os;
			encoding::UTF8::ToUnicodeBuffer to_unicode_buffer{};
			operator bool() {return static_cast<bool>(os);}
		};

		inline OStream& operator<<(OStream& sru_os,char ch) {
			// Assume ch is a byte in an UTF-8 stream and convert it to ISO8859_1 charachter set in file to SKV
			// std::cout << " " << std::hex << static_cast<unsigned int>(ch) << std::dec;
			if (auto unicode = sru_os.to_unicode_buffer.push(ch)) {
				auto iso8859_ch = charset::ISO_8859_1::UnicodeToISO8859(*unicode);
				// std::cout << ":" << std::hex << static_cast<unsigned int>(iso8859_ch) << std::dec;
				sru_os.os.put(iso8859_ch);
			}
			return sru_os;
		}

		inline OStream& operator<<(OStream& sru_os,std::string s) {
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

		inline SRUFileTagMap to_example_info_sru_file() {
			SRUFileTagMap result{};
			return result;
		}

		inline SRUFileTagMap to_example_blanketter_sru_file() {
			SRUFileTagMap result{};
			return result;
		}

		inline std::string sru_tag_value(std::string const& tag,SRUFileTagMap const& tag_map) {
// std::cout << "\nto_tag" << std::flush;
			std::ostringstream os{};
			os << tag << " ";
			if (tag_map.contains(tag)) os << tag_map.at(tag);
			else os << "?" << tag << "?";
			return os.str();
		}

		inline InfoOStream& operator<<(InfoOStream& os,FilesMapping const& fm) {

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
      if (fm.info.contains("#PROGRAM")) {
			  os.sru_os << "\n" << sru_tag_value("#PROGRAM",fm.info);
      }
      else {
			  os.sru_os << "\n" << "#PROGRAM" << " " << "CRATCHIT (Github Open Source)";
      }

				// #PROGRAM SRUDEKLARATION 1.4

			// 6. #FILNAMN (en post)
				// #FILNAMN blanketter.sru
      if (fm.info.contains("#FILNAMN")) {
			  os.sru_os << "\n" << sru_tag_value("#FILNAMN",fm.info);
      }
      else {
  			os.sru_os << "\n" << "#FILNAMN" << " " <<  "blanketter.sru";
      }

			// 7. #DATABESKRIVNING_SLUT
			os.sru_os << "\n" << "#DATABESKRIVNING_SLUT";

			// 8. #MEDIELEV_START
			os.sru_os << "\n" << "#MEDIELEV_START";

			// 9. #ORGNR
				// #ORGNR 191111111111
			// os.sru_os << "\n" << "#ORGNR" << " " << "191111111111";
			os.sru_os << "\n"  << sru_tag_value("#ORGNR",fm.info);


			// 10. #NAMN
				// #NAMN Databokföraren
			// os.sru_os << "\n" << "#NAMN" << " " << "Databokföraren";
			os.sru_os << "\n" << sru_tag_value("#NAMN",fm.info);

			// 11. #ADRESS (ej obligatorisk)
				// #ADRESS BOX 159

			// 12. #POSTNR
				// #POSTNR 12345
			// os.sru_os << "\n" << "#POSTNR" << " " << "12345";
			os.sru_os << "\n" << sru_tag_value("#POSTNR",fm.info);

			// 13. #POSTORT
				// #POSTORT SKATTSTAD
			// os.sru_os << "\n" << "#POSTORT" << " " << "SKATTSTAD";
			// os.sru_os << "\n" << "#POSTORT" << " " << "Järfälla";
			os.sru_os << "\n" << sru_tag_value("#POSTORT",fm.info);

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

		inline BlanketterOStream& operator<<(BlanketterOStream& os,FilesMapping const& fm) {

			for (int i=0;i<fm.blanketter.size();++i) {
				if (i>0) os.sru_os << "\n"; // NOTE: Empty lines not allowed (so no new-line for first entry)

				// Posterna i ett blankettblock måste förekomma i följande ordning:
				// 1. #BLANKETT
				// #BLANKETT N7-2013P1
				// os.sru_os << "#BLANKETT" << " " << "N7-2013P1";
				os.sru_os << sru_tag_value("#BLANKETT",fm.blanketter[i].first);

				// 2. #IDENTITET
				// #IDENTITET 193510250100 20130426 174557
				// os.sru_os << "\n" << "#IDENTITET" << " " << "193510250100 20130426 174557";
				os.sru_os << "\n" << sru_tag_value("#IDENTITET",fm.blanketter[i].first);

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

        // All #UPPGIFT entrires comes from non-zero values
        for (auto const& [account_no,value] : fm.blanketter[i].second) {
          if (value) {
            auto maybe_cents_amount = to_cents_amount(*value);
            if (maybe_cents_amount and (account_no != 7011) and (account_no != 7012)) {
              // A value and not a 'date' SRU
              // TODO: Refactor so that we do NOT have to trasnform 'cents amounts' to 'integer amounts' here
              //       It is too late and error prone (hacky)

              // Output as SRU amount (whole SEK)
              auto sru_amount = to_sru_amount(maybe_cents_amount.value());
              os.sru_os << "\n" << "#UPPGIFT" << " " << std::to_string(account_no) << " " << std::to_string(sru_amount);
            }
            else {
              // output as text
              os.sru_os << "\n" << "#UPPGIFT" << " " << std::to_string(account_no) << " " << *value;
            }
          }
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

			inline OptionalFilesMapping to_files_mapping() {
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

		inline std::string to_12_digit_orgno(std::string generic_org_no) {
			std::string result{};
			// The SKV XML IT-system requires 12 digit organisation numbers with digits only
			// E.g., SIE-file organisation XXXXXX-YYYY has to be transformed into 16XXXXXXYYYY
			// See https://sv.wikipedia.org/wiki/Organisationsnummer
			std::string sdigits = functional::text::filtered(generic_org_no,::isdigit);
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

		inline std::string to_value(XMLMap const& xml_map,std::string tag) {
			if (xml_map.contains(tag) and xml_map.at(tag).size() > 0) {
				return xml_map.at(tag);
			}
			else throw std::runtime_error(std::string{"to_string failed, no value for tag:"} + tag);
		}

		inline XMLMap::value_type to_entry(XMLMap const& xml_map,std::string tag) {
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

		inline EmployerDeclarationOStream& operator<<(EmployerDeclarationOStream& edos,std::string const& s) {
			auto& os = edos.os;
			os << s;
			return edos;
		}

		inline EmployerDeclarationOStream& operator<<(EmployerDeclarationOStream& edos,XMLMap::value_type const& entry) {
			Key::Path p{entry.first};
			std::string indent(p.size(),' ');
			edos << indent << "<" << p.back() << ">" << entry.second << "</" << p.back() << ">";
			return edos;
		}

		inline EmployerDeclarationOStream& operator<<(EmployerDeclarationOStream& edos,XMLMap const& xml_map) {
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

		inline bool to_employee_contributions_and_PAYE_tax_return_file(std::ostream& os,XMLMap const& xml_map) {
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
			static auto eskd_template_0 = R"(<?xml version="1.0" encoding="ISO-8859-1"?>
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
			static auto eskd_template_1 = R"(<?xml version="1.0" encoding="ISO-8859-1"?>
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
		static auto eskd_template_2 = R"(<?xml version="1.0" encoding="ISO-8859-1"?>
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
				zeroth::DateRange period;
				std::string period_to_declare; // VAT Returns period e.g., 202203 for Mars 2022 (also implying Q1 jan-mars)
			};
			using OptionalVatReturnsMeta = std::optional<VatReturnsMeta>;

			inline OptionalVatReturnsMeta to_vat_returns_meta(zeroth::DateRange const& period) {
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

			inline Amount to_form_sign(BoxNo box_no, Amount amount) {
				// All VAT Returns form amounts are without sign except box 49 where + means VAT to pay and - means VAT to "get back"
				switch (box_no) {
					// TODO: Consider it is in fact so that all amounts to SKV are to have the opposite sign of those in BAS?
					case 49: return -1 * amount; break; // The VAT Returns form must encode VAT to be paied as positive (opposite of how it is booked in BAS, negative meaning a debt to SKV)
					default: return abs(amount);
				}
			}

			inline std::ostream& operator<<(std::ostream& os,SKV::XML::VATReturns::FormBoxMap const& fbm) {
				for (auto const& [boxno,mats] : fbm) {
					auto mat_sum = BAS::to_mdats_sum(mats);
					os << "\nVAT returns Form[" << boxno << "] = " << to_tax(to_form_sign(boxno,mat_sum)) << " (from sum " <<  mat_sum << ")";
					Amount rolling_amount{};
					for (auto const& mat : mats) {
						rolling_amount += mat.defacto.amount;
						os << "\n\t" << to_string(mat.meta.date) << " " << mat << " " << rolling_amount;
					}
				}
				return os;
			}

			struct OStream {
				std::ostream& os;
				operator bool() {return static_cast<bool>(os);}
			};

			inline OStream& operator<<(OStream& os,std::string const& s) {
				os.os << s;
				return os;
			}

			inline OStream& operator<<(OStream& os,XMLMap::value_type const& entry) {
				Key::Path p{entry.first};
				std::string indent(p.size(),' ');
				os << indent << "<" << p.back() << ">" << entry.second << "</" << p.back() << ">";
				return os;
			}

			inline OStream& operator<<(OStream& os,SKV::XML::XMLMap const& xml_map) {
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

			static std::map<BoxNo,std::string> BOXNO_XML_TAG_MAP {
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

			inline std::string to_xml_tag(BoxNo const& box_no) {
				std::string result{"??"};
				if (BOXNO_XML_TAG_MAP.contains(box_no)) {
					auto const& tag = BOXNO_XML_TAG_MAP.at(box_no);
					if (tag.size() > 0) result = tag;
					else throw std::runtime_error{std::string{"ERROR: to_xml_tag failed. tag for box_no:"} + std::to_string(box_no) + " of zero length"};
				}
				else throw std::runtime_error{std::string{"ERROR: to_xml_tag failed. box_no:"} + std::to_string(box_no) + " not defined"};
				return result;
			}

			inline std::string to_11_digit_orgno(std::string generic_org_no) {
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

            // Now in SKVFramework unit
            // Key::Paths account_vat_form_mapping() {
            // BAS::AccountNos to_accounts(BoxNo box_no) {
			// std::set<BAS::AccountNo> to_accounts(BoxNos const& box_nos) {
            // std::set<BAS::AccountNo> to_vat_accounts() {

			inline BAS::MDAccountTransactions to_mats(SIEEnvironment const& sie_env,auto const& matches_mat) {
				BAS::MDAccountTransactions result{};
				auto x = [&matches_mat,&result](BAS::MDAccountTransaction const& mdat){
					if (matches_mat(mdat)) result.push_back(mdat);
				};
				for_each_md_account_transaction(sie_env,x);
				return result;
			}

			inline BAS::MDAccountTransactions to_mats(SIEEnvironmentsMap const& sie_envs_map,auto const& matches_mat) {
				BAS::MDAccountTransactions result{};
				auto x = [&matches_mat,&result](BAS::MDAccountTransaction const& mdat){
					if (matches_mat(mdat)) result.push_back(mdat);
				};
				for_each_md_account_transaction(sie_envs_map,x);
				return result;
			}

			inline std::optional<SKV::XML::XMLMap> to_xml_map(FormBoxMap const& vat_returns_form_box_map,SKV::OrganisationMeta const& org_meta,SKV::XML::DeclarationMeta const& form_meta) {
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
							xml_map[p+to_xml_tag(box_no)] = std::to_string(to_tax(to_form_sign(box_no,BAS::to_mdats_sum(mats))));
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

			inline BAS::MDAccountTransaction dummy_md_at(Amount amount) {
				return {
					.meta = {
						.jem = {
							.series = 'X'
						}
						,.caption = "Dummy..."
					}
					,.defacto = {
						.amount = amount
					}
				};
			}

			inline BAS::MDAccountTransactions to_vat_returns_mats(BoxNo box_no,SIEEnvironmentsMap const& sie_envs_map,auto mat_predicate) {
				auto account_nos = to_accounts(box_no);
				return to_mats(sie_envs_map,[&mat_predicate,&account_nos](BAS::MDAccountTransaction const& mdat) {
					return (mat_predicate(mdat) and is_any_of_accounts(mdat,account_nos));
				});
			}

            // to_box_49_amount now in SKVFramework unit

			inline std::optional<FormBoxMap> to_form_box_map(SIEEnvironmentsMap const& sie_envs_map,auto mat_predicate) {
				std::optional<FormBoxMap> result{};
				try {
					FormBoxMap box_map{};
					// Amount		VAT Return Box			XML Tag
					// 333200		05									"ForsMomsEjAnnan"
					box_map[5] = to_vat_returns_mats(5,sie_envs_map,mat_predicate);
					// box_map[5].push_back(dummy_mat(333200));
					// 83300		10									"MomsUtgHog"
					box_map[10] = to_vat_returns_mats(10,sie_envs_map,mat_predicate);
					// box_map[10].push_back(dummy_mat(83300));
					// 6616			20									"InkopVaruAnnatEg"
					box_map[20] = to_vat_returns_mats(20,sie_envs_map,mat_predicate);
					// box_map[20].push_back(dummy_mat(6616));
					// 1654			30									"MomsInkopUtgHog"
					box_map[30] = to_vat_returns_mats(30,sie_envs_map,mat_predicate);
					// box_map[30].push_back(dummy_mat(1654));
					// 957			39									"ForsTjSkskAnnatEg"
					box_map[39] = to_vat_returns_mats(39,sie_envs_map,mat_predicate);
					// box_map[39].push_back(dummy_mat(957));
					// 2688			48									"MomsIngAvdr"
					box_map[48] = to_vat_returns_mats(48,sie_envs_map,mat_predicate);
					// box_map[48].push_back(dummy_mat(2688));
					// 597			50									"MomsUlagImport"
					box_map[50] = to_vat_returns_mats(50,sie_envs_map,mat_predicate);
					// box_map[50].push_back(dummy_mat(597));
					// 149			60									"MomsImportUtgHog"
					box_map[60] = to_vat_returns_mats(60,sie_envs_map,mat_predicate);
					// box_map[60].push_back(dummy_mat(149));

					// NOTE: Box 49, vat designation id R1, R2 is a  t a r g e t  account, NOT a source.
					box_map[49].push_back(dummy_md_at(to_box_49_amount(box_map)));

					result = box_map;
				}
				catch (std::exception const& e) {
					std::cout << "\nERROR, to_form_box_map failed. Expection=" << std::quoted(e.what());
				}
				return result;
			}

			inline bool quarter_has_VAT_consilidation_entry(SIEEnvironmentsMap const& sie_envs_map,zeroth::DateRange const& period) {
				bool result{};
				auto f = [&period,&result](BAS::MDTypedJournalEntry const& tme) {
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
				for_each_typed_md_entry(sie_envs_map,f);
				return result;
			}

			inline HeadingAmountDateTransEntries to_vat_returns_hads(SIEEnvironmentsMap const& sie_envs_map) {
				HeadingAmountDateTransEntries result{};
				try {
					// Create a had for last quarter if there is no journal entry in the M series for it
					// Otherwise create a had for current quarter
					auto today = to_today();
					auto current_quarter = zeroth::to_quarter_range(today);
					auto previous_quarter = zeroth::to_three_months_earlier(current_quarter);
					auto vat_returns_range = zeroth::DateRange{previous_quarter.begin(),current_quarter.end()}; // previous and "current" two quarters
					// NOTE: By spanning previous and "current" quarters we can catch-up if we made any changes to prevuious quarter aftre having created the VAT returns consolidation
					// NOTE: making changes in a later VAT returns form for changes in previous one should be a low-crime offence?

					// Loop through quarters
					for (int i=0;i<3;++i) {
// std::cout << "\nto_vat_returns_hads, checking vat_returns_range " << vat_returns_range;
						// Check three quartes back for missing VAT consilidation journal entry
						if (quarter_has_VAT_consilidation_entry(sie_envs_map,current_quarter) == false) {
							auto vat_returns_meta = to_vat_returns_meta(vat_returns_range);
							auto is_vat_returns_range = [&vat_returns_meta](BAS::MDAccountTransaction const& mdat){
								return vat_returns_meta->period.contains(mdat.meta.date);
							};
							if (auto box_map = to_form_box_map(sie_envs_map,is_vat_returns_range)) {
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
										std::cout << "\n\t\t" << to_string(mat.meta.date) << " account_amounts[" << mat.defacto.account_no << "] += " << mat.defacto.amount << " saldo:" << account_amounts[mat.defacto.account_no];
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
						current_quarter = zeroth::to_three_months_earlier(current_quarter);
						vat_returns_range = zeroth::to_three_months_earlier(vat_returns_range);
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
			inline Amount to_eu_sales_list_amount(Amount amount) {
				return abs(round(amount)); // All amounts in the sales list form are defined to be positve (although sales in BAS are negative credits)
			}

			// Correct amount type for the form
			inline FormAmount to_form_amount(Amount amount) {
				return to_double(to_eu_sales_list_amount(amount));
			}

			inline std::string to_10_digit_orgno(std::string generic_org_no) {
				std::string result{};
				// The SKV CSV IT-system requires 10 digit organisation numbers with digits only
				// E.g., SIE-file organisation XXXXXX-YYYY has to be transformed into XXXXXXYYYY
				// See https://sv.wikipedia.org/wiki/Organisationsnummer
				std::string sdigits = functional::text::filtered(generic_org_no,::isdigit);
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

			inline OStream& operator<<(OStream& os,char ch) {
				os.os << ch;
				return os;
			}

			inline OStream& operator<<(OStream& os,std::string const& s) {
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

			inline OStream& operator<<(OStream& os,PeriodID const& period_id) {
				PeriodIDStreamer streamer{os};
				std::visit(streamer,period_id);
				return os;
			}

			inline OStream& operator<<(OStream& os,FirstRow const& row) {
				os.os << row.entry << ';';
				return os;
			}

			// Example: "556000016701;2001;Per Persson;0123-45690; post@filmkopia.se"
			inline OStream& operator<<(OStream& os,SecondRow const& row) {
				os << row.vat_registration_id.twenty_digits;
				os << ';' << row.period_id;
				os << ';' << row.name.name_max_35_characters;
				os << ';' << row.phone_number.swedish_format_no_space_max_17_chars;
				if (row.e_mail) os << ';' << *row.e_mail;
				return os;
			}

			inline OStream& operator<<(OStream& os,std::optional<FormAmount> const& ot) {
				os << ';';
				if (ot) os << std::to_string(*ot); // to_string to solve ambugiouty (TODO: Try to get rid of ambugiouty? and use os << *ot ?)
				return os;
			}

			// Example: FI01409351;16400;;;
			inline OStream& operator<<(OStream& os,RowN const& row) {
				os << row.vat_registration_id.with_country_code;
				os << row.goods_amount;
				os << row.three_part_business_amount;
				os << row.services_amount;
				return os;
			}

			inline OStream& operator<<(OStream& os,Form const& form) {
				os << form.first_row;
				os << '\n' << form.second_row;
				std::for_each(form.third_to_n_row.begin(),form.third_to_n_row.end(),[&os](auto row) {
				 	os << '\n' << row;
				});
				return os;
			}

			inline Quarter to_eu_list_quarter(Date const& date) {
				auto quarter_ix = to_quarter_index(date);
				std::ostringstream os{};
				os << (static_cast<int>(date.year()) % 100u) << "-" << quarter_ix.m_ix;
				return {os.str()};
			}

			inline EUVATRegistrationID to_eu_vat_id(SKV::XML::VATReturns::BoxNo const& box_no,BAS::MDAccountTransaction const& mdat) {
				std::ostringstream os{};
				if (!mdat.defacto.transtext) {
						os << "* transtext " << std::quoted("") << " for " << mdat << " does not define the EU VAT ID for this transaction *";
				}
				else {
					// See https://en.wikipedia.org/wiki/VAT_identification_number#European_Union_VAT_identification_numbers
					const std::regex eu_vat_id("^[A-Z]{2}\\w*"); // Must begin with two uppercase charachters for the country code
					if (std::regex_match(*mdat.defacto.transtext,eu_vat_id)) {
						os << *mdat.defacto.transtext;
					}
					else {
						os << "* transtext " << std::quoted(*mdat.defacto.transtext) << " for " << mdat << " does not define the EU VAT ID for this transaction *";
					}
				}
				return {os.str()};
			}

			inline std::vector<RowN> sie_to_eu_sales_list_rows(SKV::XML::VATReturns::FormBoxMap const& box_map) {
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
							auto x = [box_no=box_no,&vat_id_map](BAS::MDAccountTransaction const& mdat){
								auto eu_vat_id = to_eu_vat_id(box_no,mdat);
								if (!vat_id_map.contains(eu_vat_id)) vat_id_map[eu_vat_id].vat_registration_id = eu_vat_id;
								if (!vat_id_map[eu_vat_id].services_amount) vat_id_map[eu_vat_id].services_amount = 0;
								*vat_id_map[eu_vat_id].services_amount += to_form_amount(mdat.defacto.amount);
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

			inline SwedishVATRegistrationID to_swedish_vat_registration_id(SKV::OrganisationMeta const& org_meta) {
				std::ostringstream os{};
				os << "SE" << to_10_digit_orgno(org_meta.org_no) << "01";
				return {os.str()};
			}

			inline std::optional<Form> vat_returns_to_eu_sales_list_form(SKV::XML::VATReturns::FormBoxMap const& box_map,SKV::OrganisationMeta const& org_meta,zeroth::DateRange const& period) {
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


inline std::optional<SKV::XML::XMLMap> to_skv_xml_map(SKV::OrganisationMeta sender_meta,SKV::XML::DeclarationMeta declaration_meta,SKV::OrganisationMeta employer_meta,SKV::XML::TaxDeclarations tax_declarations) {
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
inline std::string to_skv_date_and_time(std::chrono::time_point<std::chrono::system_clock> const current) {
	std::ostringstream os{};
	auto now_timet = std::chrono::system_clock::to_time_t(current);
	auto now_local = localtime(&now_timet);
	os << std::put_time(now_local,"%Y-%m-%dT%H:%M:%S");
	return os.str();
}

inline std::optional<SKV::XML::XMLMap> cratchit_to_skv(SIEEnvironment const& sie_env,	std::vector<SKV::ContactPersonMeta> const& organisation_contacts, std::vector<std::string> const& employee_birth_ids) {
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

// Now in environment unit
// using EnvironmentValue = std::map<std::string,std::string>; // name-value-pairs
// using EnvironmentValueName = std::string;
// using EnvironmentValueId = std::size_t;
// using EnvironmentValues_cas_repository = cas::repository<EnvironmentValueId,EnvironmentValue>;
// using EnvironmentIdValuePairs = std::vector<EnvironmentValues_cas_repository::value_type>;
// using Environment = std::map<EnvironmentValueName,EnvironmentIdValuePairs>; // Note: Uses a vector of cas repository entries <id,Node> to keep ordering to-and-from file
// std::pair<std::string, std::optional<EnvironmentValueId>> to_name_and_id(std::string key);

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
    inline void loadFiles() {
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
            if (files_.size() == file_count_before) std::cout << " Key CONFLICT!!";
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

inline void test_immutable_file_manager() {
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


inline OptionalJournalEntryTemplate template_of(OptionalHeadingAmountDateTransEntry const& had,SIEEnvironment const& sie_environ) {
	OptionalJournalEntryTemplate result{};
	if (had) {
		BAS::MDJournalEntries candidates{};
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

inline OptionalHeadingAmountDateTransEntry to_had(BAS::MDJournalEntry const& me) {
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

inline OptionalHeadingAmountDateTransEntry to_had(TaggedAmount const& ta) {
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

// Now in HADFramework unit
// OptionalHeadingAmountDateTransEntry to_had(std::vector<std::string> const& tokens)

inline Environment::Value to_environment_value(SKV::ContactPersonMeta const& cpm) {
	Environment::Value ev{};
	ev["name"] = cpm.name;
	ev["phone"] = cpm.phone;
	ev["e-mail"] = cpm.e_mail;
	return ev;
}

// Now in TaggedAmountFramework unit
// to_environment_value(TaggedAmount)
// to_tagged_amount(EnvironmentValue)

inline std::optional<SRUEnvironments::value_type> to_sru_environments_entry(Environment::Value const& ev) {
	try {
// std::cout << "\nto_sru_environments_entry";
		// "4531=360000;4532=360000;relative_year_key=0"
		SRUEnvironment sru_env{};
		auto& relative_year_key = ev.at("relative_year_key");
		for (auto const& [key,value] : ev) {
// std::cout << "\nkey:" << key << " value:" << value;
			if (auto const& sru_code = SKV::SRU::to_account_no(key)) {
				sru_env.set(*sru_code,value);
			}
		}
		return SRUEnvironments::value_type{relative_year_key,sru_env};
	}
	catch (std::exception const& e) {
		std::cout << "\nto_sru_environments_entry failed. Exception=" << std::quoted(e.what());
	}
	return std::nullopt;
}

inline SKV::OptionalContactPersonMeta to_contact(Environment::Value const& ev) {
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

inline std::optional<std::string> to_employee(Environment::Value const& ev) {
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

inline std::ostream& operator<<(std::ostream& os,PromptOptionsList const& pol) {
	for (auto const& option : pol) {
		os << "\n" << option;
	}
	return os;
}

inline PromptOptionsList options_list_of_prompt_state(PromptState const& prompt_state) {
	std::vector<std::string> result{};
	result.push_back("Options:");
	switch (prompt_state) {
		case PromptState::Undefined: {result.push_back("DESIGN INSUFFICIENCY: options_list_of_prompt_state have no action for State PromptState::Undefined ");} break;
		case PromptState::Root: {
			result.push_back("<Heading> <Amount> <Date> : Entry of new Heading Amount Date (HAD) Transaction entry");
			result.push_back("-hads : lists current Heading Amount Date (HAD) entries");
			result.push_back("-sie <sie file path> : imports a new input sie file. Please enclose path with \"\" if it contains space.");
			result.push_back("-sie : lists transactions in input sie-file");
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

	SIEEnvironmentsMap sie_env_map{}; // 'Older' SIE envrionemtns map
  FiscalYear current_fiscal_year{FiscalYear::to_current_fiscal_year(std::chrono::month{5})}; // month hard coded for now

	SRUEnvironments sru{};
	HeadingAmountDateTransEntries heading_amount_date_entries{};
	DateOrderedTaggedAmountsContainer all_dotas{};
	DateOrderedTaggedAmountsContainer selected_dotas{};
	DateOrderedTaggedAmountsContainer new_dotas{};
	size_t ta_index{};
  std::string selected_year_index{"0"};

  // Now in SIEEnvironment
	// std::filesystem::path staged_sie_file_path{"cratchit.se"};

  // Removed/20251020 (Refactored to value semantics)
	// std::optional<DateOrderedTaggedAmountsContainer::const_iterator> to_ta_iter(std::size_t ix) {
	// 	std::optional<DateOrderedTaggedAmountsContainer::const_iterator> result{};
	// 	auto ta_iter = this->selected_dotas.begin();
	// 	auto end = this->selected_dotas.end();
	// 	if (ix < std::distance(ta_iter,end)) {
	// 		std::advance(ta_iter,ix); // zero based index
	// 		result = ta_iter;
	// 	}
	// 	return result;
	// }

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

	// std::optional<DateOrderedTaggedAmountsContainer::const_iterator> selected_ta() {
	// 	return to_ta_iter(this->ta_index);
	// }

	OptionalTaggedAmount selected_ta() {
    OptionalTaggedAmount result{};
    auto ordered_ids_view = this->selected_dotas.ordered_ids_view();
    if (ordered_ids_view.size() > this->ta_index) {
      result =  this->selected_dotas.at(ordered_ids_view[this->ta_index]);
    }
    return result;
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
		auto vat_returns_hads = SKV::XML::VATReturns::to_vat_returns_hads(this->sie_env_map);
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

  zeroth::OptionalDateRange to_financial_year_date_range(std::string const& relative_year_key) {
    zeroth::OptionalDateRange result{};
		if (this->sie_env_map.contains(relative_year_key)) {
      result = this->sie_env_map[relative_year_key].financial_year_date_range();
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

// Environment -> Model
std::vector<SKV::ContactPersonMeta> contacts_from_environment(Environment const& environment);
std::vector<std::string> employee_birth_ids_from_environment(Environment const& environment);
SRUEnvironments srus_from_environment(Environment const& environment);
void synchronize_tagged_amounts_with_sie(DateOrderedTaggedAmountsContainer& all_dotas,SIEEnvironment const& sie_environment);
DateOrderedTaggedAmountsContainer dotas_from_sie_environment(SIEEnvironment const& sie_env);

namespace SKV {
  struct SpecsDummy {};
}

using ConfiguredSIEFilePaths = std::vector<std::pair<std::string,std::filesystem::path>>;

struct CratchitFSMeta {
  std::filesystem::path m_root_path;
  ConfiguredSIEFilePaths m_configured_sie_file_paths{};
};
struct CratchitFSDefacto {
  struct MaybeSIEStreamMeta {
    SIEEnvironmentsMap::RelativeYearKey m_year_id;
    std::filesystem::path m_file_path;
  };
  using MDMaybeSIEIStream = MetaDefacto<MaybeSIEStreamMeta,MaybeSIEInStream>;
  using MDMaybeSIEIStreams = std::vector<MDMaybeSIEIStream>;
  MDMaybeSIEIStreams to_md_sie_istreams(ConfiguredSIEFilePaths const& configured_sie_file_paths) {
    return configured_sie_file_paths
      | std::views::transform([](auto const& configured_sie_file_path){
          return MDMaybeSIEIStream {
            .meta = {
               .m_year_id = configured_sie_file_path.first
              ,.m_file_path = configured_sie_file_path.second
            }
            ,.defacto = persistent::in::to_maybe_istream(configured_sie_file_path.second)
          };
        })
      | std::ranges::to<MDMaybeSIEIStreams>();
  }
};
using CratchitMDFileSystem = MetaDefacto<CratchitFSMeta,CratchitFSDefacto>;

// Environment + cratchit_environment_file_path -> Model
DateOrderedTaggedAmountsContainer dotas_from_environment_and_account_statement_files(std::filesystem::path cratchit_environment_file_path,Environment const& environment);
TaggedAmounts tas_sequence_from_consumed_account_statement_file(std::filesystem::path statement_file_path);
TaggedAmounts tas_sequence_from_consumed_account_statement_files(std::filesystem::path cratchit_environment_file_path);
SKV::SpecsDummy skv_specs_mapping_from_csv_files(std::filesystem::path cratchit_environment_file_path,Environment const& environment);

namespace zeroth {
  std::string to_user_cli_feedback(
     Model const& model
    ,SIEEnvironmentsMap::RelativeYearKey year_id
    ,SIEEnvironmentsMap::UpdateFromPostedResult const& change_results);
	Model model_from_environment(Environment const& environment);  
	Model model_from_environment_and_md_filesystem(Environment const& environment,CratchitMDFileSystem runtime);
	Model model_from_environment_and_files(std::filesystem::path cratchit_environment_file_path,Environment const& environment);

  Environment environment_from_model(Model const& model);
}
