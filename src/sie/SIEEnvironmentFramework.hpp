#pragma once

#include "SIEEnvironment.hpp"
#include "persistent/in/MDInFramework.hpp"
#include "logger/log.hpp"
#include <istream>
#include <filesystem>
#include <expected>

BAS::MDJournalEntry to_md_entry(SIE::Ver const& ver);
OptionalSIEEnvironment sie_from_stream(std::istream& is);

using MaybeSIEInStream = persistent::in::MaybeIStream;
MaybeSIEInStream to_maybe_sie_istream(std::filesystem::path sie_file_path);

// Replaced by SIEEnvironmentsMap::update_posted_from_file
// OptionalSIEEnvironment sie_from_sie_file(std::filesystem::path const& sie_file_path);

class SIEEnvironmentsMap {
public:
  using RelativeYearKey = std::string;
  using ActualYearKey = RelativeYearKey; // Not refactored yet
  using map_type = std::map<ActualYearKey,SIEEnvironment>;
  SIEEnvironmentsMap() = default;

  struct Meta {
    std::map<RelativeYearKey,std::filesystem::path> posted_sie_files{};
  };

  Meta const& meta() const {return m_meta;}

  SIEEnvironment::EnvironmentChangeResults update_from_posted_and_staged_sie_env(
     RelativeYearKey year_id
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

    SIEEnvironment::EnvironmentChangeResults result{};

    auto const& [iter,was_inserted] = this->m_sie_envs_map.insert_or_assign(year_id,std::move(posted_sie_env));
    if (was_inserted) {
      // Inserted ok
      logger::development_trace("Inserted OK");
      // Consolidate 'staged' with the 'now posted'

      // #2 staged_sie_file_path is the path to SIE entries NOT in "current" import
      //    That is, asumed to be added or edited  by cratchit (and not yet known by external tool)
      result = iter->second.stage(staged_sie_env);
    }
    else {
      logger::development_trace("Insert posted FAILED");
    }

    return result;

  }


  SIEEnvironment::EnvironmentChangeResults update_posted_from_file(
     RelativeYearKey year_id
    ,std::filesystem::path sie_file_path) {

    logger::scope_logger log_raii{
       logger::development_trace
      ,std::format(
         "update_posted_from_file: year_id:{}, file{}"
        ,year_id
        ,sie_file_path.string())
    };

    SIEEnvironment::EnvironmentChangeResults result{};

    auto maybe_posted_sie_istream = to_maybe_sie_istream(sie_file_path);
    if (maybe_posted_sie_istream) {
      if (auto sie_env = sie_from_stream(maybe_posted_sie_istream.value())) {

        if (this->m_sie_envs_map.contains(year_id)) {
          logger::development_trace(
           "update_posted_from_file: Replaced existing sie for year id: {}"
          ,year_id);
        }

        // this->m_sie_envs_map[year_id] = std::move(*sie_env);
        auto const& [iter,was_inserted] = this->m_sie_envs_map.insert_or_assign(year_id,std::move(*sie_env));

        if (was_inserted) {
          // Inserted ok
          logger::development_trace("Inserted OK");
          this->m_meta.posted_sie_files[year_id] = sie_file_path;
          // Consolidate 'staged' with the 'now posted'
          auto maybe_staged_sie_stream = to_maybe_sie_istream(iter->second.staged_sie_file_path());
          if (auto sse = sie_from_stream(maybe_staged_sie_stream.value())) {
            // #2 staged_sie_file_path is the path to SIE entries NOT in "current" import
            //    That is, asumed to be added or edited  by cratchit (and not yet known by external tool)
            result = iter->second.stage(*sse);
          }
        }
        else {
          logger::development_trace("Insert FAILED");
        }
      }
    }
    else {
      // Should not happen
    }

    // sie_from_sie_file();

    return result;
  }

  auto begin() const {return m_sie_envs_map.begin();}
  auto end() const {return m_sie_envs_map.end();}
  auto contains(RelativeYearKey key) const {return m_sie_envs_map.contains(key);}

  auto& operator[](RelativeYearKey key) {
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
  Meta m_meta{};
  map_type m_sie_envs_map;

  std::expected<ActualYearKey, std::string> to_actual_year_key(
     RelativeYearKey relative_year_key
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
