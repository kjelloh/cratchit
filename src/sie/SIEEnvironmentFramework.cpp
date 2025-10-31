#include "SIEEnvironmentFramework.hpp"
#include "logger/log.hpp"
#include <fstream> // std::ifstream,...
#include <vector>

BAS::MDJournalEntry to_entry(SIE::Ver const& ver) {
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
		result.defacto.account_transactions.push_back(BAS::anonymous::AccountTransaction{
			.account_no = trans.account_no
			,.transtext = trans.transtext
			,.amount = trans.amount
		});
	}
	return result;
}

OptionalSIEEnvironment sie_from_stream(std::istream& cp437_is) {
  OptionalSIEEnvironment result{};

  encoding::CP437::istream cp437_in{cp437_is};
  if (!cp437_in) {
    logger::cout_proxy << "\nFailed to open stream ";
    return result;
  }

  // Phase 1: Read and transcode entire file to UTF-8
  std::string s_utf8;
  while (auto entry = cp437_in.getline(encoding::unicode::to_utf8{})) {
    s_utf8 += *entry;
    s_utf8 += "\n";
  }

  std::istringstream utf8_in{s_utf8};

  // Phase 2: Parse all SIE entries into memory
  std::vector<SIE::io::SIEFileEntry> parsed_elements;

  while (true) {
    if (auto e = SIE::io::parse_ORGNR(utf8_in,"#ORGNR")) parsed_elements.push_back(*e);
    else if (auto e = SIE::io::parse_FNAMN(utf8_in,"#FNAMN")) parsed_elements.push_back(*e);
    else if (auto e = SIE::io::parse_ADRESS(utf8_in,"#ADRESS")) parsed_elements.push_back(*e);
    else if (auto e = SIE::io::parse_RAR(utf8_in,"#RAR")) parsed_elements.push_back(*e);
    else if (auto e = SIE::io::parse_KONTO(utf8_in,"#KONTO")) parsed_elements.push_back(*e);
    else if (auto e = SIE::io::parse_SRU(utf8_in,"#SRU")) parsed_elements.push_back(*e);
    else if (auto e = SIE::io::parse_IB(utf8_in,"#IB")) parsed_elements.push_back(*e);
    else if (auto e = SIE::io::parse_VER(utf8_in)) parsed_elements.push_back(*e);
    else if (auto e = SIE::io::parse_any_line(utf8_in)) parsed_elements.push_back(*e);
    else break;
  }

  if (parsed_elements.empty()) {
    logger::cout_proxy << "\nNo SIE elements found";
    return result;
  }

  // Phase 3: Find RAR (year_no == 0)
  std::optional<FiscalYear> fiscal_year;
  for (auto& entry : parsed_elements) {
    if (std::holds_alternative<SIE::Rar>(entry)) {
      const auto& rar = std::get<SIE::Rar>(entry);
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
    logger::cout_proxy << "\nNo valid #RAR entry found (returns nullopt SIE Environment)";
    return result;
  }

  // Phase 4: Construct environment and apply all parsed data
  SIEEnvironment sie_environment{*fiscal_year};

  for (auto& entry : parsed_elements) {
    if (std::holds_alternative<SIE::OrgNr>(entry))
      sie_environment.organisation_no = std::get<SIE::OrgNr>(entry);
    else if (std::holds_alternative<SIE::FNamn>(entry))
      sie_environment.organisation_name = std::get<SIE::FNamn>(entry);
    else if (std::holds_alternative<SIE::Adress>(entry))
      sie_environment.organisation_address = std::get<SIE::Adress>(entry);
    else if (std::holds_alternative<SIE::Konto>(entry)) {
      auto& konto = std::get<SIE::Konto>(entry);
      sie_environment.set_account_name(konto.account_no, konto.name);
    }
    else if (std::holds_alternative<SIE::Sru>(entry)) {
      auto& sru = std::get<SIE::Sru>(entry);
      sie_environment.set_account_SRU(sru.bas_account_no, sru.sru_account_no);
    }
    else if (std::holds_alternative<SIE::Ib>(entry)) {
      auto& ib = std::get<SIE::Ib>(entry);
      if (ib.year_no == 0) sie_environment.set_opening_balance(ib.account_no, ib.opening_balance);
    }
    else if (std::holds_alternative<SIE::Ver>(entry)) {
      sie_environment.post(to_entry(std::get<SIE::Ver>(entry)));
    }
  }

  result = std::move(sie_environment);
  return result;
}

OptionalSIEEnvironment sie_from_sie_file(std::filesystem::path const& sie_file_path) {
  if (false) {
    logger::cout_proxy << "\nsie_from_sie_file(" << sie_file_path << ")";
  }
	std::ifstream is{sie_file_path};
  auto result = sie_from_stream(is);
  if (result) {
    // Inject source file path into environment
    // Note: Due to refactored from model map to SIEEnvironment aggregate
    result->set_source_sie_file_path(sie_file_path);
  }
  return result;
}
