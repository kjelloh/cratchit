#include "SIEEnvironmentFramework.hpp"
#include <fstream> // std::ifstream,...

BAS::MetaEntry to_entry(SIE::Ver const& ver) {
  if (false) {
    std::cout << "\nto_entry(ver:" << ver.series << std::dec << ver.verno << " " << ver.verdate << " member count:" << ver.transactions.size()  << ")";
  }
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

OptionalSIEEnvironment sie_from_sie_file(std::filesystem::path const& sie_file_path) {
  if (false) {
    std::cout << "\nsie_from_sie_file(" << sie_file_path << ")";
  }
	OptionalSIEEnvironment result{};
	std::ifstream ifs{sie_file_path};
  encoding::CP437::istream cp437_in{ifs};
	if (cp437_in) {
    // Read in the SIE file and transcode it to UTF8
    std::string s_utf8{};
    while (auto entry = cp437_in.getline(encoding::unicode::to_utf8{})) {
      s_utf8 += *entry;
      s_utf8 += "\n";
      if (false) {
        std::cout << "\nfrom cp437_in: " << std::quoted(*entry);
      }
    }
    // Create a stream with the UTF8 encoded SIE file entries for internal parsing
    std::istringstream utf8_in{s_utf8};
		SIEEnvironment sie_environment{};
		while (true) {
			// std::cout << "\nparse";
			if (auto opt_entry = SIE::io::parse_ORGNR(utf8_in,"#ORGNR")) {
				SIE::OrgNr orgnr = std::get<SIE::OrgNr>(*opt_entry);
				sie_environment.organisation_no = orgnr;
			}
			else if (auto opt_entry = SIE::io::parse_FNAMN(utf8_in,"#FNAMN")) {
				SIE::FNamn fnamn = std::get<SIE::FNamn>(*opt_entry);
				sie_environment.organisation_name = fnamn;
			}
			else if (auto opt_entry = SIE::io::parse_ADRESS(utf8_in,"#ADRESS")) {
				SIE::Adress adress = std::get<SIE::Adress>(*opt_entry);
				sie_environment.organisation_address = adress;
			}
			else if (auto opt_entry = SIE::io::parse_RAR(utf8_in,"#RAR")) {
				SIE::Rar rar = std::get<SIE::Rar>(*opt_entry);
				if (rar.year_no == 0) {
					// Process only "current" year in read sie file
					sie_environment.set_year_date_range(zeroth::DateRange{rar.first_day_yyyymmdd,rar.last_day_yyyymmdd});
				}
			}
			else if (auto opt_entry = SIE::io::parse_KONTO(utf8_in,"#KONTO")) {
				SIE::Konto konto = std::get<SIE::Konto>(*opt_entry);
				sie_environment.set_account_name(konto.account_no,konto.name);
			}
			else if (auto opt_entry = SIE::io::parse_SRU(utf8_in,"#SRU")) {
				SIE::Sru sru = std::get<SIE::Sru>(*opt_entry);
				sie_environment.set_account_SRU(sru.bas_account_no,sru.sru_account_no);
			}
			else if (auto opt_entry = SIE::io::parse_IB(utf8_in,"#IB")) {
				SIE::Ib ib = std::get<SIE::Ib>(*opt_entry);
				// std::cout << "\nIB " << ib.account_no << " = " << ib.opening_balance;
				if (ib.year_no == 0) sie_environment.set_opening_balance(ib.account_no,ib.opening_balance); // Only use "current" year opening balance
				// NOTE: The SIE-file defines a "year 0" that is "current year" as seen from the data in the file
				// See the #RAR tag that maps year_no 0,-1,... to actual date range (period / accounting year)
				// Example:
				// #RAR 0 20210501 20220430
				// #RAR -1 20200501 20210430				
			}
			else if (auto opt_entry = SIE::io::parse_VER(utf8_in)) {
				SIE::Ver ver = std::get<SIE::Ver>(*opt_entry);
				// std::cout << "\n\tVER!";
				auto me = to_entry(ver);
				sie_environment.post(me);
			}
			else if (auto opt_entry = SIE::io::parse_any_line(utf8_in)) {
				SIE::io::AnonymousLine al = std::get<SIE::io::AnonymousLine>(*opt_entry);
				// std::cout << "\n\tANY=" << al.str;
			}
			else {
        if (false) {
          std::cout << "\nsie_from_sie_file(" << sie_file_path << ") FILE PARSED";
        }
        break;
      }
		}
    // Inject source file path into environment
    // Note: Due to refactored from model map to SIEEnviuronment aggregate
    //       and SIEEnvironment has yet no constructor...
    sie_environment.sie_file_path = sie_file_path;
		result = std::move(sie_environment);
	}
  else {
    if (true) {
      std::cout << " NO SUCH FILE ";
    }
  }
	return result;
}
