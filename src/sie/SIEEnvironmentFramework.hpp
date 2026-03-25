#pragma once

#include "SIEEnvironment.hpp"
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

BAS::MDJournalEntry to_md_entry(SIE::Ver const& ver);
OptionalSIEEnvironment sie_from_utf8_sv(std::string_view utf8_sv);
OptionalSIEEnvironment sie_from_cp437_stream(persistent::in::CP437::istream& cp437_in);

using UpdateFromPostedResult = std::optional<SIEEnvironmentChangeResults>;
using MybeSIEEnvironmentRef = cratchit::functional::memory::MaybeRef<SIEEnvironment>;
// TODO: Consider to design a domain using an actual FiscalYear as key?
//       SIE files are based on relative key 0,-1,-2.
//       Cratchit may benefit from operating in FiscalYear defigned domain?
using ActualYearKey = sie::RelativeYearKey;

class SIEEnvironmentsMap {
public:
  using map_type = std::map<ActualYearKey,SIEEnvironment>;

  SIEEnvironmentsMap() = default;

  std::pair<bool,std::string> remove(DatedJournalEntryMeta const& key);
  UpdateFromPostedResult update_from_posted_and_staged_sie_env(
     sie::RelativeYearKey year_id
    ,SIEEnvironment const& posted_sie_env
    ,SIEEnvironment const& staged_sie_env);
  auto begin() const {return m_sie_envs_map.begin();}
  auto end() const {return m_sie_envs_map.end();}
  auto contains(sie::RelativeYearKey key) const {return m_sie_envs_map.contains(key);}  
  MybeSIEEnvironmentRef at(sie::RelativeYearKey key);
  MybeSIEEnvironmentRef at(Date date);
  BAS::MaybeJournalEntryRef at(DatedJournalEntryMeta key);
  SIEEnvironment& operator[](sie::RelativeYearKey key);
	SIEEnvironmentChangeResult stage(BAS::MDJournalEntry const& mdje);
private:
  std::expected<ActualYearKey, std::string> to_actual_year_key(
     sie::RelativeYearKey relative_year_key
    ,FiscalYear current_fiscal_year);

  map_type m_sie_envs_map;

};

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

