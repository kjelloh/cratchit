#pragma once

/**
  An CAS (Content Adressable Storage) Environment is one using suitable value-hash as references for
  ordered and aggregated values of an Environment.

  This is better suited for internal immutable propcessing of environment values (E.g., Tagged Amounts).

  Also see IndexedEnvironment.
 */

#include "cas.hpp"
#include <map>
#include <string>

// TODO: Refactor into propoer CAS (Content Adressable Storage)
//       For now this is just a clone of original Environment that mixes index and hash based ID-Value mapping
//       also consumed from persistent storage text file.
class CASEnvironment {
public:

  using EnvironmentValue = std::map<std::string,std::string>; // vector of name-value pairs
  using EnvironmentValueName = std::string;
  using EnvironmentValueId = std::size_t;
  using EnvironmentValues_cas_repository = cas::repository<EnvironmentValueId,EnvironmentValue>;
  using EnvironmentIdValuePair = EnvironmentValues_cas_repository::value_type; // mutable id-value pair
  using EnvironmentIdValuePairs = std::vector<EnvironmentIdValuePair>; // To model the order in persistent file
  using Environment = std::map<EnvironmentValueName,EnvironmentIdValuePairs>; // Note: Uses a vector of cas repository entries <id,Node> to keep ordering to-and-from file

  bool contains(EnvironmentValueName const& section) const {
    return m_env.contains(section);
  }
  auto const& at(EnvironmentValueName const& section) const {
    return m_env.at(section);
  }
  auto& operator[](EnvironmentValueName const& section) {
    return m_env[section];
  }
  auto operator<=>(const CASEnvironment&) const = default;
  auto size() const {
    return m_env.size();
  }
  auto begin() const {
    return m_env.begin();
  }
  auto end() const {
    return m_env.end();
  }
private:
  Environment m_env{};
};
