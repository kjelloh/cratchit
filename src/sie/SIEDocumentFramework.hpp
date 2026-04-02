#pragma once

#include "SIEDocument.hpp"
#include "persistent/in/maybe.hpp"
#include "persistent/in/encoding_aware_read.hpp" // persistent::in::CP437::istream
#include "logger/log.hpp"
#include <istream>
#include <filesystem>
#include <expected>

using MaybeSIEInStream = persistent::in::text::MaybeIStream;
MaybeSIEInStream to_maybe_sie_istream(std::filesystem::path sie_file_path);

namespace sie {
  using RelativeYearKey = std::string;
}

BAS::MDJournalEntry to_md_entry(sie::io::Ver const& ver);
std::optional<SIEDocument> sie_from_utf8_sv(std::string_view utf8_sv);
std::optional<SIEDocument> sie_from_cp437_stream(persistent::in::CP437::istream& cp437_in);

using UpdateFromPostedResult = std::optional<SIEDocumentChangeResults>;
using MaybeSIEDocumentRef = cratchit::functional::memory::MaybeRef<SIEDocument>;
// TODO: Consider to design a domain using an actual FiscalYear as key?
//       SIE files are based on relative key 0,-1,-2.
//       Cratchit may benefit from operating in FiscalYear defigned domain?
using ActualYearKey = sie::RelativeYearKey;

class SIEArchive {
public:
  using map_type = std::map<ActualYearKey,SIEDocument>;

  SIEArchive() = default;

  std::pair<bool,std::string> remove(DatedJournalEntryMeta const& key);
  UpdateFromPostedResult update_from_posted_and_staged_sie_docs(
     sie::RelativeYearKey year_id
    ,SIEDocument const& posted_doc
    ,SIEDocument const& staged_doc);
  auto begin() const {return m_map.begin();}
  auto end() const {return m_map.end();}
  auto contains(sie::RelativeYearKey key) const {return m_map.contains(key);}  
  MaybeSIEDocumentRef at(sie::RelativeYearKey key);
  MaybeSIEDocumentRef at(Date date);
  BAS::MaybeJournalEntryRef at(DatedJournalEntryMeta key);
  SIEDocument& operator[](sie::RelativeYearKey key);
	SIEDocumentChangeResult stage(BAS::MDJournalEntry const& mdje);
private:
  std::expected<ActualYearKey, std::string> to_actual_year_key(
     sie::RelativeYearKey relative_year_key
    ,FiscalYear current_fiscal_year);

  map_type m_map;

};

inline void for_each_md_journal_entry(SIEDocument const& sie_doc,auto& f) {
	for (auto const& [series,journal] : sie_doc.journals()) {
		for (auto const& [verno,aje] : journal) {
			f(BAS::MDJournalEntry{.meta ={.series=series,.verno=verno,.unposted_flag=sie_doc.is_unposted(series,verno)},.defacto=aje});
		}
	}
}

inline void for_each_md_journal_entry(SIEArchive const& sie_archive,auto& f) {
	for (auto const& [financial_year_key,sie_doc] : sie_archive) {
		for_each_md_journal_entry(sie_doc,f);
	}
}

