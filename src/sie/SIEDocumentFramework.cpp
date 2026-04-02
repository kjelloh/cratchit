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
		result.defacto.account_transactions.push_back(BAS::anonymous::AccountTransaction{
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

// ------------------------------------------
// SIEArchive

std::pair<bool,std::string> SIEArchive::remove(DatedJournalEntryMeta const& key) {
  std::pair<bool,std::string> result{false,std::format("Could not find entry to remove")};

  // TODO: Consider to refactor the whole SIE storage API to use a key:{series,verno}.
  //       Then we can just look up ('A',11) plain and simple.
  //       We can then define 'views' in date order, series order or what have you
  //       using key vectors?
  //       For now, I suppose I have to live with the mess below (indirection hell...) 

  for (auto& [year_id,sie_env] : this->m_sie_envs_map) {
    if (sie_env.fiscal_year().contains(key.m_date) == false) continue;
    for (auto& [series,journal] : sie_env.journals()) {
      if (series != key.m_jem.series) continue;

      // TODO: Consider to rely on 'journal' storing entries in verno order low-to-high?
      //       In that case we can just use the rbegin() to access the last element...
      //       For now, the code both searches and exhaust all elements to
      //       also find largest verno to check if remove is allowed.
      BAS::VerNo max_verno{0};
      auto found_iter = journal.end(); 
      for (auto iter = journal.begin();iter != journal.end();++iter) {
        max_verno = std::max(max_verno,iter->first);
        if (key.m_jem.verno.value() == iter->first) found_iter = iter;
      }
      if (found_iter != journal.end()) {
        // Found it
        if (key.m_jem.verno.value() == max_verno) {
          // Ok to remove last in series
          journal.erase(found_iter);
          result.first = true;
        }
        else {
          result.second = std::format(
            "Failed to remove {}{}. Only last verno:{} can be removed."
            ,key.m_jem.series
            ,key.m_jem.verno.value()
            ,max_verno);
        }
      }

    }
  }

  return result;
}

UpdateFromPostedResult SIEArchive::update_from_posted_and_staged_sie_docs(
    sie::RelativeYearKey year_id
  ,SIEDocument const& posted_doc
  ,SIEDocument const& staged_doc) {

  logger::scope_logger log_raii{
      logger::development_trace
    ,std::format(
        "update_posted_from_sie_env: year_id:{}, posted sie year:{} staged sie year:{}"
      ,year_id
      ,posted_doc.fiscal_year().to_string()
      ,staged_doc.fiscal_year().to_string())
  };

  UpdateFromPostedResult result{};

  auto const& [iter,was_inserted] = this->m_sie_envs_map.insert_or_assign(year_id,std::move(posted_doc));

  result = iter->second.stage_sie_(staged_doc); // insert_or_assign never fails (true=inserted, false = assigned)

  if (was_inserted) {
    logger::development_trace(
      "update_from_posted_and_staged_sie_docs: Update to *NEW* posted id: {}"
      ,year_id);
  }
  else {
    // Was assigned
    logger::development_trace(
      "update_from_posted_and_staged_sie_docs: Update to EXISTING posted id: {}"
      ,year_id);
  }

  return result;
}

MybeSIEEnvironmentRef SIEArchive::at(sie::RelativeYearKey key) {
    auto it = m_sie_envs_map.find(key);
    if (it == m_sie_envs_map.end()) return {};
    return MybeSIEEnvironmentRef::from(it->second);
}

MybeSIEEnvironmentRef SIEArchive::at(Date date) {
  auto iter = std::ranges::find_if(this->m_sie_envs_map,[date](auto const& entry){
    return entry.second.fiscal_year().contains(date);
  });
  if (iter == this->m_sie_envs_map.end()) return {};
  return MybeSIEEnvironmentRef::from(iter->second);
}

BAS::MaybeJournalEntryRef SIEArchive::at(DatedJournalEntryMeta key) {
  return this->at(key.m_date)
    .and_then([&key](auto& sie_env){
      return sie_env.at(key);
    });
}

SIEDocument& SIEArchive::operator[](sie::RelativeYearKey key) {
  auto iter = m_sie_envs_map.find(key);
  if (iter != m_sie_envs_map.end()) {return iter->second;}

  // No environment in map for key!
  static SIEDocument dummy{FiscalYear{Date{}.year(),Date{}.month()}};
  // TODO: Consider a design to not default construct SIEDocument as
  logger::design_insufficiency(
    "SIEArchive:operator['{}'] does not exist - Returns dummy / empty for fiscal year:{}"
    ,dummy.fiscal_year().to_string()
    ,key);
  return dummy;
}

SIEEnvironmentChangeResult SIEArchive::stage(BAS::MDJournalEntry const& mdje) {
  SIEEnvironmentChangeResult result{mdje,SIEEnvironmentChangeResult::Status::Undefined};

  // TODO: Refctor this 'mess' *sigh* (to many optionals...)

  if (this->m_sie_envs_map.contains("current")) {
    if (auto financial_year = (*this)["current"].financial_year_date_range()) {
      if (financial_year->contains(mdje.defacto.date)) {
        return (*this)["current"].stage_entry_(mdje);
      }
    }
  }

  if (this->m_sie_envs_map.contains("-1")) {
    if (auto financial_year = (*this)["-1"].financial_year_date_range()) {
      if (financial_year->contains(mdje.defacto.date)) {
        return (*this)["-1"].stage_entry_(mdje);
      }
    }
  }
  return result;
}

// private

std::expected<ActualYearKey, std::string> SIEArchive::to_actual_year_key(
    sie::RelativeYearKey relative_year_key
  ,FiscalYear current_fiscal_year) {

  // Old mechanism: use relative key as-is
  return relative_year_key;

  // New mechanism - Use Date with the value of first day of financial year as key
  // try {
  //     auto relative_fiscal_year_index = std::stoi(relative_year_key);
  //     if (relative_fiscal_year_index <= 0 && relative_fiscal_year_index >= -10) {
  //         return current_fiscal_year.to_relative_fiscal_year(relative_fiscal_year_index).start();
  //     } else {
  //         return std::unexpected(std::format(
  //             "Relative year index {} is out of bounds (>0 or < -10)",
  //             relative_fiscal_year_index
  //         ));
  //     }
  // } catch (...) {
  //     return std::unexpected(std::format(
  //         "Failed to interpret relative year index {}", relative_year_key
  //     ));
  // }
}



// SIEArchive
// ------------------------------------------
