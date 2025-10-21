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

  class Hasher {
  public:

    ValueId operator()(Value const& key_value_map) const {
      std::size_t result{};

      // Hash combine for Environment::Value
      // TODO: Consider to consolidate 'hashing' for 'Value Id' to somehow
      //       E.g., Also see hash_combine for TaggedAmount...
      auto hash_combine = [](std::size_t &seed, auto const& v) {
        constexpr auto shift_left_count = 1;
        constexpr std::size_t max_size_t = std::numeric_limits<std::size_t>::max();
        constexpr std::size_t mask = max_size_t >> shift_left_count;
        using ValueType = std::remove_cvref_t<decltype(v)>;
        std::hash<ValueType> hasher;
        // Note: I decided to NOT use boost::hash_combine code as it will cause
        // integer overflow and thus undefined behaviour.
        // seed ^= hasher(v) + 0x9e3779b9 + ((seed & mask) <<6) + (seed>>2); //
        // *magic* dustribution as defined by boost::hash_combine
        seed ^= (hasher(v) & mask)
                << shift_left_count; // Simple shift left distribution and no addition
      };

      for (auto const &[key, value] : key_value_map) {
        hash_combine(result, key);
        hash_combine(result, value);
      }
      return result;
    }
  };

  using Values_cas_repository = cas::repository<ValueId,Value,Hasher>;
  using MutableIdValuePair = Values_cas_repository::mutable_cid_value_type;
  using MutableIdValuePairs = std::vector<MutableIdValuePair>; // To model the order in persistent file
  using Container = std::map<ValueName,MutableIdValuePairs>; // Note: Uses a vector of cas repository entries <id,Node> to keep ordering to-and-from file

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