#include "sie.hpp"
#include <iostream>
#include <iomanip>

namespace SIE {

  namespace io {

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
      if (utf8_in >> Tag{sie_field_tag} >> rar.year_id >> first_day_parser >> last_day_parser) {
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
          std::cout << "\nparse_VER Ver: " << ver.series << " " << ver.verno << " " << ver.verdate << " " << ver.vertext;
        }
        /*
        From SIE Specification "SIE_filformat_ver_4B_ENGLISH.pdf"

        "#RTRANS Supplementary Transaction Item...
        ...This item [RTRANS] is a supplement in the SIE standard. 
        There is to always be a row of type #RTRANS immediately followed 
        by an identical row of type #TRANS 
        so that backward compatibility with the old SIE formats is maintained."      

        "If items of the type #RTRANS and #BTRANS are not handled, these rows are to be ignored when importing a SIE file."
        */      
        while (true) {
          if (auto entry = parse_TRANS(utf8_in,"#TRANS")) {
            // std::cout << "\nTRANS :)";	
            ver.transactions.push_back(std::get<Trans>(*entry));					
          }
          else if (auto entry = parse_TRANS(utf8_in,"#BTRANS")) {
            // Ignore
            // std::cout << "\n*Ignored* #BTRANS in " << ver.series << " " << ver.verno << " " << ver.verdate << " (as allowed by SIE specification SIE_filformat_ver_4B_ENGLISH.pdf)";
          }
          else if (auto entry = parse_TRANS(utf8_in,"#RTRANS")) {
            // Ignore
            // std::cout << "\n*Ignored* #RTRANS in " << ver.series << " " << ver.verno << " " << ver.verdate << " (as allowed by SIE specification SIE_filformat_ver_4B_ENGLISH.pdf)";
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
      else {
        // Failure to partse is NOT an error (it only indicates that the current entry in the parsed file is something else)
      }
      utf8_in.seekg(pos); // Reset position utf8_in case of failed parse
      utf8_in.clear(); // Reset failbit to allow try for other parse
      return result;
    }

    SIEParseResult parse_any_line(std::istream& utf8_in) {
      if (false) {
        std::cout << "\nparse_any_line";
        if (not utf8_in.good()) {
          std::cout << "\nparse_any_line utf8_in NOT GOOD";
          if (utf8_in.eof()) {
              std::cout << "\n\tError: End of File reached";
          }
          if (utf8_in.fail()) {
              std::cout << "\n\tError: Logical error on i/o operation (failbit)";
          }
          if (utf8_in.bad()) {
              std::cout << "\n\tError: Read/writing error on i/o operation (badbit)";
          }
        }
        else {
          std::cout << "\n\tpeek:" << std::to_string(utf8_in.peek());
        }
      }
      auto pos = utf8_in.tellg();
      std::string line{};
      if (std::getline(utf8_in,line)) {
        if (false) {
          // Log what is consumed as 'any' = not used
          std::cout << "\n\tany=" << line;
        }
        return AnonymousLine{.str=line};
      }
      else {
        if (false) {
          if (utf8_in.eof()) {
              std::cout << "\n\tError: End of File reached";
          }
          if (utf8_in.fail()) {
              std::cout << "\n\tError: Logical error on i/o operation (failbit)";
          }
          if (utf8_in.bad()) {
              std::cout << "\n\tError: Read/writing error on i/o operation (badbit)";
          }

          std::cout << "\n\tany null";
        }
        utf8_in.clear(); // Clear the error state to enable seekg to work
        utf8_in.seekg(pos);
        return {};
      }
    }

    SIE::io::OStream& operator<<(SIE::io::OStream& sieos,char ch) {
      // Assume ch is a byte in an UTF-8 stream and convert it to CP437 charachter set in file
      if (auto unicode = sieos.to_unicode_buffer.push(ch)) {
        auto cp437_ch = charset::CP437::UnicodeToCP437(*unicode);
        sieos.os.put(cp437_ch);
      }
      return sieos;
    }

    SIE::io::OStream& operator<<(SIE::io::OStream& sieos,std::string s) {
      if (false) {
        std::cout << "\nto SIE:" << std::quoted(s);
      }
      for (char ch : s) {
        sieos << ch; // Stream through operator<<(SIE::io::OStream& sieos,char ch) that will transform utf-8 encoded Unicode, to char encoded CP437
      }
      return sieos;
    }

    SIE::io::OStream& operator<<(SIE::io::OStream& sieos,SIE::Trans const& trans) {
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
    
    SIE::io::OStream& operator<<(SIE::io::OStream& sieos,SIE::Ver const& ver) {
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
  } // io

} // namespace SIE

SIE::Trans to_sie_t(BAS::anonymous::AccountTransaction const& trans) {
	SIE::Trans result{
		.account_no = trans.account_no
		,.amount = trans.amount
		,.transtext = trans.transtext // 240706 - encoded in runtime character set (cp437 encoding handled by output stream)
	};
	return result;
}

SIE::Ver to_sie_t(BAS::MDJournalEntry const& mdje) {
		/*
		Series series;
		BAS::VerNo verno;
		Date verdate;
		std::string vertext;
		*/

	SIE::Ver result{
		.series = mdje.meta.series
		,.verno = (mdje.meta.verno)?*mdje.meta.verno:0
		,.verdate = mdje.defacto.date
		,.vertext = mdje.defacto.caption};
	for (auto const& trans : mdje.defacto.account_transactions) {
		result.transactions.push_back(to_sie_t(trans));
	}
	return result;
}