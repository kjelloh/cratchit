#pragma once
#include "sie.hpp"
#include "SIEDocument.hpp"
#include <expected>

using ActualYearKey = sie::RelativeYearKey;

using UpdateFromPostedResult = std::optional<JournalEntryChangeResults>;
using MaybeSIEDocumentRef = cratchit::functional::memory::MaybeRef<SIEDocument>;

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
	JournalEntryChangeResult stage(BAS::MDJournalEntry const& mdje);
private:
  std::expected<ActualYearKey, std::string> to_actual_year_key(
     sie::RelativeYearKey relative_year_key
    ,FiscalYear current_fiscal_year);

  map_type m_map;

};