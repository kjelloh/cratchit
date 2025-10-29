#pragma once

#include "SIEEnvironment.hpp"
#include "logger/log.hpp"
#include <istream>
#include <filesystem>
#include <expected>

BAS::MetaEntry to_entry(SIE::Ver const& ver);
OptionalSIEEnvironment sie_from_stream(std::istream& is);
OptionalSIEEnvironment sie_from_sie_file(std::filesystem::path const& sie_file_path);

class SIEEnvironmentsMap {
public:
  using RelativeYearKey = std::string;
  using ActualYearKey = RelativeYearKey; // Not refactored yet
  using map_type = std::map<ActualYearKey,SIEEnvironment>;
  SIEEnvironmentsMap() = default;

  auto begin() const {return m_sie_envs_map.begin();}
  auto end() const {return m_sie_envs_map.end();}
  auto contains(RelativeYearKey key) const {return m_sie_envs_map.contains(key);}
  // auto& operator[](RelativeYearKey key) {return m_sie_envs_map[key];}
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

	std::optional<BAS::MetaEntry> stage(BAS::MetaEntry const& me) {
    std::optional<BAS::MetaEntry> result{};

    // TODO: Refctor this 'mess' *sigh* (to many optionals...)

    if (this->m_sie_envs_map.contains("current")) {
      if (auto financial_year = (*this)["current"].financial_year_date_range()) {
        if (financial_year->contains(me.defacto.date)) {
          return (*this)["current"].stage(me);
        }
      }
    }

    if (this->m_sie_envs_map.contains("-1")) {
      if (auto financial_year = (*this)["-1"].financial_year_date_range()) {
        if (financial_year->contains(me.defacto.date)) {
          return (*this)["-1"].stage(me);
        }
      }
    }
    return result;
  }

private:
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
