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

  using UpdateFromPostedResult = std::pair<SIEEnvironment::EnvironmentChangeResults,bool>;
  UpdateFromPostedResult update_from_posted_and_staged_sie_env(
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

    UpdateFromPostedResult result{};

    auto const& [iter,was_inserted] = this->m_sie_envs_map.insert_or_assign(year_id,std::move(posted_sie_env));
    result.second = was_inserted;

    if (was_inserted) {

      if (this->m_sie_envs_map.contains(year_id)) {
        logger::development_trace(
        "update_from_posted_and_staged_sie_env: Replaced existing sie for year id: {}"
        ,year_id);
      }

      // Consolidate 'staged' with the 'now posted'

      // #2 staged_sie_file_path is the path to SIE entries NOT in "current" import
      //    That is, asumed to be added or edited  by cratchit (and not yet known by external tool)
      result.first = iter->second.stage(staged_sie_env);
    }
    else {
      logger::development_trace("Insert posted FAILED");
    }

    return result;

  }

  UpdateFromPostedResult update_posted_from_md_istream(
     RelativeYearKey year_id
    ,persistent::in::MDMaybeIFStream& md_maybe_istream) {

    logger::scope_logger log_raii{
       logger::development_trace
      ,std::format(
         "update_posted_from_md_istream: year_id:{}, meta.file_path:{}"
        ,year_id
        ,md_maybe_istream.meta.file_path.string())
    };

    auto const& sie_file_path = md_maybe_istream.meta.file_path;

    UpdateFromPostedResult result{};

    auto& maybe_posted_sie_istream = md_maybe_istream.defacto;
    if (maybe_posted_sie_istream) {
      if (auto maybe_posted_sie_env = sie_from_stream(maybe_posted_sie_istream.value())) {
        auto const& posted_sie_env = maybe_posted_sie_env.value();

        // if (this->m_sie_envs_map.contains(year_id)) {
        //   logger::development_trace(
        //    "update_posted_from_file: Replaced existing sie for year id: {}"
        //   ,year_id);
        // }

        if (auto maybe_staged_sie_stream = to_maybe_sie_istream(posted_sie_env.staged_sie_file_path())) {
          if (auto maybe_staged_sie = sie_from_stream(maybe_staged_sie_stream.value())) {
            result = this->update_from_posted_and_staged_sie_env(year_id,posted_sie_env,maybe_staged_sie.value());
          }
          else {
            result = this->update_from_posted_and_staged_sie_env(year_id,posted_sie_env,SIEEnvironment{posted_sie_env.fiscal_year()});
          }
        }
        else {
            result = this->update_from_posted_and_staged_sie_env(year_id,posted_sie_env,SIEEnvironment{posted_sie_env.fiscal_year()});
        }

        if (result.second) {
          // inserty/update posted ok
          this->m_meta.posted_sie_files[year_id] = sie_file_path;
        }
        else {
          logger::design_insufficiency(
             "update_posted_from_file: Failed. m_meta.posted_sie_files[{}] left unchanged:{}"
            ,year_id
            ,this->m_meta.posted_sie_files.contains(year_id)
                ?this->m_meta.posted_sie_files.at(year_id).string()
                :"null");
        }
      }
    }
    else {
      // Should not happen
    }

    // sie_from_sie_file();

    return result;
  }


  UpdateFromPostedResult update_posted_from_file(
     RelativeYearKey year_id
    ,std::filesystem::path sie_file_path) {

    logger::scope_logger log_raii{
       logger::development_trace
      ,std::format(
         "update_posted_from_file: year_id:{}, file{}"
        ,year_id
        ,sie_file_path.string())
    };

    auto md_maybe_istream = persistent::in::to_md_maybe_istream(sie_file_path);
    return update_posted_from_md_istream(year_id,md_maybe_istream);

    // UpdateFromPostedResult result{};

    // auto maybe_posted_sie_istream = to_maybe_sie_istream(sie_file_path);
    // if (maybe_posted_sie_istream) {
    //   if (auto maybe_posted_sie_env = sie_from_stream(maybe_posted_sie_istream.value())) {
    //     auto const& posted_sie_env = maybe_posted_sie_env.value();

    //     // if (this->m_sie_envs_map.contains(year_id)) {
    //     //   logger::development_trace(
    //     //    "update_posted_from_file: Replaced existing sie for year id: {}"
    //     //   ,year_id);
    //     // }

    //     if (auto maybe_staged_sie_stream = to_maybe_sie_istream(posted_sie_env.staged_sie_file_path())) {
    //       if (auto maybe_staged_sie = sie_from_stream(maybe_staged_sie_stream.value())) {
    //         result = this->update_from_posted_and_staged_sie_env(year_id,posted_sie_env,maybe_staged_sie.value());
    //       }
    //       else {
    //         result = this->update_from_posted_and_staged_sie_env(year_id,posted_sie_env,SIEEnvironment{posted_sie_env.fiscal_year()});
    //       }
    //     }
    //     else {
    //         result = this->update_from_posted_and_staged_sie_env(year_id,posted_sie_env,SIEEnvironment{posted_sie_env.fiscal_year()});
    //     }

    //     if (result.second) {
    //       // inserty/update posted ok
    //       this->m_meta.posted_sie_files[year_id] = sie_file_path;
    //     }
    //     else {
    //       logger::design_insufficiency(
    //          "update_posted_from_file: Failed. m_meta.posted_sie_files[{}] left unchanged:{}"
    //         ,year_id
    //         ,this->m_meta.posted_sie_files.contains(year_id)
    //             ?this->m_meta.posted_sie_files.at(year_id).string()
    //             :"null");
    //     }

    //     // auto const& [iter,was_inserted] = this->m_sie_envs_map.insert_or_assign(year_id,std::move(*sie_env));

    //     // if (was_inserted) {
    //     //   // Inserted ok
    //     //   logger::development_trace("Inserted OK");
    //     //   this->m_meta.posted_sie_files[year_id] = sie_file_path;
    //     //   // Consolidate 'staged' with the 'now posted'
    //     //   auto maybe_staged_sie_stream = to_maybe_sie_istream(iter->second.staged_sie_file_path());
    //     //   if (auto sse = sie_from_stream(maybe_staged_sie_stream.value())) {
    //     //     // #2 staged_sie_file_path is the path to SIE entries NOT in "current" import
    //     //     //    That is, asumed to be added or edited  by cratchit (and not yet known by external tool)
    //     //     result = iter->second.stage(*sse);
    //     //   }
    //     // }
    //     // else {
    //     //   logger::development_trace("Insert FAILED");
    //     // }
    //   }
    // }
    // else {
    //   // Should not happen
    // }

    // // sie_from_sie_file();

    // return result;
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
