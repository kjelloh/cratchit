#pragma once

#include "FiscalPeriod.hpp"
#include "IndexedEnvironment.hpp"
#include "CASEnvironment.hpp"
#include <string>
#include <vector>
#include <map>
#include <filesystem>

using Environment = CASEnvironment;

// parsing environment in -> Environment
namespace in {
  bool is_comment_line(std::string const& line);
  bool is_value_line(std::string const& line);
  std::pair<std::string, std::optional<IndexedEnvironment::ValueId>> to_name_and_id(std::string const& s);
  IndexedEnvironment::Value to_environment_value(std::string const& s);
  // IndexedEnvironment indexed_environment_from_file(std::filesystem::path const &p);
}
Environment environment_from_file(std::filesystem::path const &p);

// Processing environment -> out
namespace out {
  std::ostream& operator<<(std::ostream& os,IndexedEnvironment::Value const& ev);
  std::ostream& operator<<(std::ostream& os,IndexedEnvironment::value_type const& entry);
  std::ostream& operator<<(std::ostream& os,IndexedEnvironment const& env);
  std::string to_string(IndexedEnvironment::Value const& ev);
  std::string to_string(IndexedEnvironment::value_type const& entry);
  // void indexed_environment_to_file(IndexedEnvironment const &environment,std::filesystem::path const &p);
}
void environment_to_file(Environment const &environment,std::filesystem::path const &p);

struct EnvironmentPeriodSlice {
  Environment m_environment;
  FiscalPeriod m_period;
  EnvironmentPeriodSlice(Environment env,FiscalPeriod period) : m_environment(std::move(env)),m_period(period) {}
  FiscalPeriod const period() const { return m_period; }
  Environment const& environment() const { return m_environment; }
  Environment& environment() { return m_environment; }
}; // EnvironmentPeriodSlice