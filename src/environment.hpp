#pragma once

#include "FiscalPeriod.hpp"
#include "cas.hpp"
#include <string>
#include <vector>
#include <map>
#include <filesystem>

// namespace cas now on cas unit

using EnvironmentValue = std::map<std::string,std::string>; // vector of name-value-pairs
using EnvironmentValueName = std::string;
using EnvironmentValueId = std::size_t;
using EnvironmentValues_cas_repository = cas::repository<EnvironmentValueId,EnvironmentValue>;
using EnvironmentIdValuePair = EnvironmentValues_cas_repository::value_type;
using EnvironmentCasEntryVector = std::vector<EnvironmentIdValuePair>;
using Environment = std::map<EnvironmentValueName,EnvironmentCasEntryVector>; // Note: Uses a vector of cas repository entries <id,Node> to keep ordering to-and-from file

// parsing environment in
bool is_value_line(std::string const& line);
std::pair<std::string, std::optional<EnvironmentValueId>> to_name_and_id(std::string key);
EnvironmentValue to_environment_value(std::string const s);
Environment environment_from_file(std::filesystem::path const &p);

// Processing environment out
std::ostream& operator<<(std::ostream& os,EnvironmentValue const& ev);
std::ostream& operator<<(std::ostream& os,Environment::value_type const& entry);
std::ostream& operator<<(std::ostream& os,Environment const& env);
std::string to_string(EnvironmentValue const& ev);
std::string to_string(Environment::value_type const& entry);
void environment_to_file(Environment const &environment,std::filesystem::path const &p);

struct EnvironmentPeriodSlice {
  Environment m_environment;
  FiscalPeriod m_period;
  EnvironmentPeriodSlice(Environment env,FiscalPeriod period) : m_environment(std::move(env)),m_period(period) {}
  FiscalPeriod const period() const { return m_period; }
  Environment const& environment() const { return m_environment; }
  Environment& environment() { return m_environment; }
}; // EnvironmentPeriodSlice