#pragma once

#include "FiscalPeriod.hpp" // Date,
#include "fiscal/BASFramework.hpp"
#include "text/encoding.hpp"
#include <string>
#include <variant>

namespace SIE {

  struct Tag {
    std::string const expected;
  };

  // #RAR 0 20210501 20220430
  // #RAR -1 20200501 20210430	
  struct Rar {
    std::string tag;
    int year_id;
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

  namespace io {
    // Parses SIE entries assumed to be in UTF8 encoding.
    // But SIE files are specfied to be in CP437.
    // So cratchit applies a transformation pipeline CP437 -> UTF8 when reading SIE files
    // See encoding::CP437::istream::getline mechanism in from_sie_file

    struct AnonymousLine {std::string str{};};
    using SIEFileEntry = std::variant<OrgNr,FNamn,Adress,Rar,Ib,Konto,Sru,Ver,Trans,AnonymousLine>;
    using SIEParseResult = std::optional<SIEFileEntry>;

    std::istream& operator>>(std::istream& utf8_in,Tag const& tag);

    struct optional_YYYYMMdd_parser {
      std::optional<Date>& date;
    };
    struct YYYYMMdd_parser {
      Date& date;
    };
    std::istream& operator>>(std::istream& utf8_in,optional_YYYYMMdd_parser& p);
    std::istream& operator>>(std::istream& utf8_in,YYYYMMdd_parser& p);

    struct optional_Text_parser {
      std::optional<std::string>& text;
    };
    std::istream& operator>>(std::istream& utf8_in,optional_Text_parser& p);
    struct Scraps {};
    std::istream& operator>>(std::istream& utf8_in,Scraps& p);

    SIEParseResult parse_RAR(std::istream& utf8_in,std::string const& sie_field_tag);

    SIEParseResult parse_IB(std::istream& utf8_in,std::string const& sie_field_tag);
    SIEParseResult parse_ORGNR(std::istream& utf8_in,std::string const& sie_field_tag);
    SIEParseResult parse_FNAMN(std::istream& utf8_in,std::string const& sie_field_tag);
    SIEParseResult parse_ADRESS(std::istream& utf8_in,std::string const& sie_field_tag);
    SIEParseResult parse_KONTO(std::istream& utf8_in,std::string const& sie_field_tag);
    SIEParseResult parse_SRU(std::istream& utf8_in,std::string const& sru_tag);
    SIEParseResult parse_TRANS(std::istream& utf8_in,std::string const& trans_tag);
    SIEParseResult parse_Tag(std::istream& utf8_in,std::string const& tag);

    SIEParseResult parse_VER(std::istream& utf8_in);
    SIEParseResult parse_any_line(std::istream& utf8_in);

    // ===============================================================
    // BEGIN operator<< framework for SIE::T stream to text stream in SIE file representation
    // ===============================================================

    /**
    * NOTE ABOUT UTF-8 TO Code Page 437 used as the character set of an SIE file
    * 
    * The convertion is made in overloaded operator<<(SIE::io::OStream& sieos,char ch) called by operator<<(SIE::io::OStream& sieos,std::string s).
    * 
    * But I have not yet overloaded operator<<(SIE::io::OStream& for things like Amount, account_no etc.
    * SO - basically it is mainly transtext and vertext that is fed to the operator<<(SIE::io::OStream& sieos,std::string s).
    * TAKE CARE to not mess this up or you will get UTF-8 encoded text into the SIE file that will mess things up quite a lot...
    */

    struct OStream {
      std::ostream& os;
      encoding::UTF8::ToUnicodeBuffer to_unicode_buffer{};
    };

    SIE::io::OStream& operator<<(SIE::io::OStream& sieos,char ch);
    SIE::io::OStream& operator<<(SIE::io::OStream& sieos,std::string s);
    SIE::io::OStream& operator<<(SIE::io::OStream& sieos,SIE::Trans const& trans);
    SIE::io::OStream& operator<<(SIE::io::OStream& sieos,SIE::Ver const& ver);

    // ===============================================================
    // END operator<< framework for SIE::T stream to text stream in SIE file representation
    // ===============================================================
  } // io

} // namespace SIE

SIE::Trans to_sie_t(BAS::anonymous::AccountTransaction const& trans);
SIE::Ver to_sie_t(BAS::MDJournalEntry const& me);