#pragma once

#include "FiscalPeriod.hpp"
#include "cas/cas.hpp"
// #include "Environment.hpp"
// #include "CASEnvironment.hpp"
#include <string>
#include <vector>
#include <map>
#include <filesystem>

class Environment {
public:

  using ValueName = std::string;
  using Value = std::map<std::string,std::string>; // vector of name-value pairs
  using ValueId = std::size_t;
  using Values_cas_repository = cas::repository<ValueId,Value>;
  using IdValuePair = Values_cas_repository::value_type; // mutable id-value pair
  using IdValuePairs = std::vector<IdValuePair>; // To model the order in persistent file
  using Container = std::map<ValueName,IdValuePairs>; // Note: Uses a vector of cas repository entries <id,Node> to keep ordering to-and-from file

  using value_type = Container::value_type;
  auto& operator[](ValueName const& section) {
    return m_container[section];
  }
  auto begin() const {
    return m_container.begin();
  }
  auto end() const {
    return m_container.end();
  }

  bool contains(ValueName const& section) const {
    return m_container.contains(section);
  }
  auto const& at(ValueName const& section) const {
    return m_container.at(section);
  }

  auto operator<=>(const Environment&) const = default;

  auto size() const {
    return m_container.size();
  }

private:
  Container m_container{};
};

// parsing environment in -> Environment
namespace in {
  bool is_comment_line(std::string const& line);
  bool is_value_line(std::string const& line);
  std::pair<std::string, std::optional<Environment::ValueId>> to_name_and_id(std::string const& s);
  Environment::Value to_environment_value(std::string const& s);
  // Environment indexed_environment_from_file(std::filesystem::path const &p);
}
Environment environment_from_file(std::filesystem::path const &p);

// Processing environment -> out
namespace out {
  std::ostream& operator<<(std::ostream& os,Environment::Value const& ev);
  std::ostream& operator<<(std::ostream& os,Environment::value_type const& entry);
  std::ostream& operator<<(std::ostream& os,Environment const& env);
  std::string to_string(Environment::Value const& ev);
  std::string to_string(Environment::value_type const& entry);
  // void indexed_environment_to_file(Environment const &environment,std::filesystem::path const &p);
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