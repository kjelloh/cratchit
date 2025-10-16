#pragma once

/**
  An CAS (Content Adressable Storage) Environment is one using suitable value-hash as references for
  ordered and aggregated values of an Environment.

  This is better suited for internal immutable propcessing of environment values (E.g., Tagged Amounts).

  Also see IndexedEnvironment.
 */

#include "cas/cas.hpp"
#include <map>
#include <string>

// TODO: Refactor into propoer CAS (Content Adressable Storage)
//       For now this is just a clone of original Environment that mixes index and hash based ID-Value mapping
//       also consumed from persistent storage text file.
class CASEnvironment {
public:

  using ValueName = std::string;
  using Value = std::map<std::string,std::string>; // vector of name-value pairs
  using ValueId = std::size_t;
  using Values_cas_repository = cas::repository<ValueId,Value>;
  using IdValuePair = Values_cas_repository::value_type; // mutable id-value pair
  using IdValuePairs = std::vector<IdValuePair>; // To model the order in persistent file
  using Container = std::map<ValueName,IdValuePairs>; // Note: Uses a vector of cas repository entries <id,Node> to keep ordering to-and-from file

  bool contains(ValueName const& section) const {
    return m_container.contains(section);
  }
  auto const& at(ValueName const& section) const {
    return m_container.at(section);
  }
  auto& operator[](ValueName const& section) {
    return m_container[section];
  }
  auto operator<=>(const CASEnvironment&) const = default;

  auto size() const {
    return m_container.size();
  }
  auto begin() const {
    return m_container.begin();
  }
  auto end() const {
    return m_container.end();
  }
private:
  Container m_container{};
};
