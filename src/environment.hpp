#pragma once

#include "FiscalPeriod.hpp"
#include <string>
#include <vector>
#include <map>
#include <filesystem>

// Content Addressable Storage namespace
namespace cas {

  template <typename Key, typename Value>
  class repository {
  private:
    using KeyValueMap = std::map<Key, Value>;
    KeyValueMap m_map{};
  public:
    // Tricky! We need EnvironmentCasEntryVector to be assignable, so we cannot
    // use KeyValueMap::value_type directly as this would make the Key type const...
    // using value_type = KeyValueMap::value_type;
    using value_type = std::pair<Key, Value>;
    bool contains(Key const &key) const { return m_map.contains(key); }
    Value const &at(Key const &key) const { return m_map.at(key); }
    void clear() { return m_map.clear(); }
    KeyValueMap &the_map() {
      return m_map;
    }
    repository& operator=(const repository &other) {
      if (this != &other) {
        m_map = other.m_map; // OK: std::map is assignable
      }
      return *this;
    }
  };
} // namespace cas

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