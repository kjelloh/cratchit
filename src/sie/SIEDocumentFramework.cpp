#include "SIEDocumentFramework.hpp"
#include "logger/log.hpp"
#include <fstream> // std::ifstream,...
#include <vector>

BAS::MDJournalEntry to_md_entry(sie::io::Ver const& ver) {
  if (false) {
    logger::cout_proxy << "\nto_entry(ver:" << ver.series << std::dec << ver.verno << " " << ver.verdate << " member count:" << ver.transactions.size()  << ")";
  }
	BAS::MDJournalEntry result{
		.meta = {
			.series = ver.series
			,.verno = ver.verno
		}
	};
	result.defacto.caption = ver.vertext;
	result.defacto.date = ver.verdate;
	for (auto const& trans : ver.transactions) {
		result.defacto.account_postings.push_back(BAS::anonymous::AccountPosting{
			 .account_no = trans.account_no
			,.transtext = trans.transtext
			,.amount = trans.amount
		});
	}
	return result;
}

std::optional<SIEDocument> sie_from_utf8_sv(std::string_view utf8_sv) {
  // logger::scope_logger log_raii{logger::development_trace,"sie_from_utf8_sv",logger::LogToConsole::ON};

  std::optional<SIEDocument> result{};
  std::istringstream utf8_in{std::string{utf8_sv}};

  // Phase 2: Parse all SIE entries into memory
  std::vector<sie::io::SIEFileEntry> parsed_elements;

  while (true) {
    if (auto e = sie::io::parse_ORGNR(utf8_in,"#ORGNR")) parsed_elements.push_back(*e);
    else if (auto e = sie::io::parse_FNAMN(utf8_in,"#FNAMN")) parsed_elements.push_back(*e);
    else if (auto e = sie::io::parse_ADRESS(utf8_in,"#ADRESS")) parsed_elements.push_back(*e);
    else if (auto e = sie::io::parse_RAR(utf8_in,"#RAR")) parsed_elements.push_back(*e);
    else if (auto e = sie::io::parse_KONTO(utf8_in,"#KONTO")) parsed_elements.push_back(*e);
    else if (auto e = sie::io::parse_SRU(utf8_in,"#SRU")) parsed_elements.push_back(*e);
    else if (auto e = sie::io::parse_IB(utf8_in,"#IB")) parsed_elements.push_back(*e);
    else if (auto e = sie::io::parse_VER(utf8_in)) parsed_elements.push_back(*e);
    else if (auto e = sie::io::parse_any_line(utf8_in)) parsed_elements.push_back(*e);
    else break;
  }

  if (parsed_elements.empty()) {
    logger::development_trace("No SIE elements found");
    return result;
  }

  // Phase 3: Find RAR (year_no == 0)
  std::optional<FiscalYear> fiscal_year;
  for (auto& entry : parsed_elements) {
    if (std::holds_alternative<sie::io::Rar>(entry)) {
      const auto& rar = std::get<sie::io::Rar>(entry);
      if (rar.year_id == 0) {
        auto date_range = to_date_range(rar.first_day_yyyymmdd, rar.last_day_yyyymmdd);
        if (date_range.has_value()) {
          auto candidate = FiscalYear{date_range->start().year(),date_range->start().month()};
          if (candidate.is_valid() and (candidate.last() == rar.last_day_yyyymmdd)) {
            fiscal_year = candidate;
          }
        }
        break;
      }
    }
  }

  if (!fiscal_year) {
    logger::cout_proxy << "\nNo valid #RAR entry found (returns nullopt SIE Archive)";
    return result;
  }

  // Phase 4: Construct environment and apply all parsed data
  SIEDocument sie_doc{*fiscal_year};

  for (auto& entry : parsed_elements) {
    if (std::holds_alternative<sie::io::OrgNr>(entry))
      sie_doc.organisation_no = std::get<sie::io::OrgNr>(entry);
    else if (std::holds_alternative<sie::io::FNamn>(entry))
      sie_doc.organisation_name = std::get<sie::io::FNamn>(entry);
    else if (std::holds_alternative<sie::io::Adress>(entry))
      sie_doc.organisation_address = std::get<sie::io::Adress>(entry);
    else if (std::holds_alternative<sie::io::Konto>(entry)) {
      auto& konto = std::get<sie::io::Konto>(entry);
      sie_doc.set_account_name(konto.account_no, konto.name);
    }
    else if (std::holds_alternative<sie::io::Sru>(entry)) {
      auto& sru = std::get<sie::io::Sru>(entry);
      sie_doc.set_account_SRU(sru.bas_account_no, sru.sru_account_no);
    }
    else if (std::holds_alternative<sie::io::Ib>(entry)) {
      auto& ib = std::get<sie::io::Ib>(entry);
      if (ib.year_no == 0) sie_doc.set_opening_balance(ib.account_no, ib.opening_balance);
    }
    else if (std::holds_alternative<sie::io::Ver>(entry)) {
      sie_doc.post_(to_md_entry(std::get<sie::io::Ver>(entry)));
    }
  }

  result = std::move(sie_doc);

  return result;
}

std::optional<SIEDocument> sie_from_cp437_stream(persistent::in::CP437::istream& cp437_in) {

  // scope Log
  logger::scope_logger log_raii{logger::development_trace,"sie_from_cp437_stream"};

  std::optional<SIEDocument> result{};

  if (!cp437_in) {
    logger::cout_proxy << "\nFailed to open stream ";
    return result;
  }

  // Phase 1: Read and transcode entire file to UTF-8
  std::string s_utf8;
  while (auto entry = cp437_in.getline(text::encoding::unicode::to_utf8{})) {
    s_utf8 += *entry;
    s_utf8 += "\n";
  }

  return sie_from_utf8_sv(s_utf8);

}

MaybeSIEInStream to_maybe_sie_istream(std::filesystem::path sie_file_path) {
  return persistent::in::text::to_maybe_istream(sie_file_path);
}
