#pragma once

#include "SIEEnvironment.hpp"
#include "persistent/in/MDInFramework.hpp"
#include "logger/log.hpp"
#include <istream>
#include <filesystem>
#include <expected>

using MaybeSIEInStream = persistent::in::MaybeIStream;
MaybeSIEInStream to_maybe_sie_istream(std::filesystem::path sie_file_path);

namespace sie {
  using RelativeYearKey = std::string;
}

// struct MaybeSIEStreamMeta {
//   sie::RelativeYearKey m_year_id;
//   std::filesystem::path m_file_path;
// };
// using MDMaybeSIEIStream = MetaDefacto<MaybeSIEStreamMeta,MaybeSIEInStream>;
// using MDMaybeSIEIStreams = std::vector<MDMaybeSIEIStream>;

BAS::MDJournalEntry to_md_entry(SIE::Ver const& ver);
OptionalSIEEnvironment sie_from_stream(std::istream& is);

// using MDMaybeSIEEnvironment = MetaDefacto<MaybeSIEStreamMeta,OptionalSIEEnvironment>;
// MDMaybeSIEEnvironment to_md_sie_env(MDMaybeSIEIStream& md_posted_sie_istream);

// Replaced by SIEEnvironmentsMap::update_posted_from_file
// OptionalSIEEnvironment sie_from_sie_file(std::filesystem::path const& sie_file_path);

class SIEEnvironmentsMap {
public:
  // TODO: Consider to design a domain using an actual FiscalYear as key?
  //       SIE files are based on relative key 0,-1,-2.
  //       Cratchit may benefit from operating in FiscalYear defigned domain?
  using ActualYearKey = sie::RelativeYearKey;
  using map_type = std::map<ActualYearKey,SIEEnvironment>;

  SIEEnvironmentsMap() = default;

  // Moved to Model
  // struct Meta {
  //   std::map<RelativeYearKey,std::filesystem::path> posted_sie_files{};
  // };
  // Meta const& meta() const {return m_meta;}

  using UpdateFromPostedResult = std::optional<SIEEnvironment::EnvironmentChangeResults>;

  UpdateFromPostedResult update_from_posted_and_staged_sie_env(
     sie::RelativeYearKey year_id
    ,SIEEnvironment const& posted_sie_env
    ,SIEEnvironment const& staged_sie_env) {

    logger::scope_logger log_raii{
       logger::development_trace
      ,std::format(
         "update_posted_from_sie_env: year_id:{}, posted sie year:{} staged sie year:{}"
        ,year_id
        ,posted_sie_env.fiscal_year().to_string()
        ,staged_sie_env.fiscal_year().to_string())
    };

    UpdateFromPostedResult result{};

    auto const& [iter,was_inserted] = this->m_sie_envs_map.insert_or_assign(year_id,std::move(posted_sie_env));

    result = iter->second.stage(staged_sie_env); // insert_or_assign never fails (true=inserted, false = assigned)

    if (was_inserted) {
      logger::development_trace(
        "update_from_posted_and_staged_sie_env: Update to *NEW* posted id: {}"
        ,year_id);
    }
    else {
      // Was assigned
      logger::development_trace(
        "update_from_posted_and_staged_sie_env: Update to EXISTING posted id: {}"
        ,year_id);
    }

    return result;
  }

  // Redlaced with update_from_posted_and_staged_sie_env
  // UpdateFromPostedResult update_posted_from_md_istream(
  //    sie::RelativeYearKey year_id
  //   ,persistent::in::MDMaybeIFStream& md_maybe_istream) {

  //   logger::scope_logger log_raii{
  //      logger::development_trace
  //     ,std::format(
  //        "update_posted_from_md_istream: year_id:{}, meta.file_path:{}"
  //       ,year_id
  //       ,md_maybe_istream.meta.file_path.string())
  //   };

  //   auto const& sie_file_path = md_maybe_istream.meta.file_path;

  //   UpdateFromPostedResult result{};

  //   auto& maybe_posted_sie_istream = md_maybe_istream.defacto;
  //   if (maybe_posted_sie_istream) {
  //     if (auto maybe_posted_sie_env = sie_from_stream(maybe_posted_sie_istream.value())) {
  //       auto const& posted_sie_env = maybe_posted_sie_env.value();

  //       if (auto maybe_staged_sie_stream = to_maybe_sie_istream(posted_sie_env.staged_sie_file_path())) {
  //         if (auto maybe_staged_sie = sie_from_stream(maybe_staged_sie_stream.value())) {
  //           result = this->update_from_posted_and_staged_sie_env(year_id,posted_sie_env,maybe_staged_sie.value());
  //         }
  //         else {
  //           result = this->update_from_posted_and_staged_sie_env(year_id,posted_sie_env,SIEEnvironment{posted_sie_env.fiscal_year()});
  //         }
  //       }
  //       else {
  //           result = this->update_from_posted_and_staged_sie_env(year_id,posted_sie_env,SIEEnvironment{posted_sie_env.fiscal_year()});
  //       }
  //     }
  //   }
  //   else {
  //     // Should not happen
  //   }
  //   return result;
  // }

  auto begin() const {return m_sie_envs_map.begin();}
  auto end() const {return m_sie_envs_map.end();}
  auto contains(sie::RelativeYearKey key) const {return m_sie_envs_map.contains(key);}

  auto& operator[](sie::RelativeYearKey key) {
    auto iter = m_sie_envs_map.find(key);
    if (iter != m_sie_envs_map.end()) {return iter->second;}

    // No environment in map for key!
    static SIEEnvironment dummy{FiscalYear{Date{}.year(),Date{}.month()}};
    // TODO: Consider a design to not default construct SIEEnvironment as
    logger::design_insufficiency(
      "SIEEnvironmentsMap:operator['{}'] does not exist - Returns dummy / empty for fiscal year:{}"
      ,dummy.fiscal_year().to_string()
      ,key);
    return dummy;
  }

	SIEEnvironment::EnvironmentChangeResult stage(BAS::MDJournalEntry const& mdje) {
    SIEEnvironment::EnvironmentChangeResult result{mdje,SIEEnvironment::EnvironmentChangeResult::Status::Undefined};

    // TODO: Refctor this 'mess' *sigh* (to many optionals...)

    if (this->m_sie_envs_map.contains("current")) {
      if (auto financial_year = (*this)["current"].financial_year_date_range()) {
        if (financial_year->contains(mdje.defacto.date)) {
          return (*this)["current"].stage(mdje);
        }
      }
    }

    if (this->m_sie_envs_map.contains("-1")) {
      if (auto financial_year = (*this)["-1"].financial_year_date_range()) {
        if (financial_year->contains(mdje.defacto.date)) {
          return (*this)["-1"].stage(mdje);
        }
      }
    }
    return result;
  }

private:
  // Replaced with posted sie files map in Model
  // Meta m_meta{};

  map_type m_sie_envs_map;

  std::expected<ActualYearKey, std::string> to_actual_year_key(
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

};
