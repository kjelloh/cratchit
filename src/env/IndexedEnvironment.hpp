#pragma once

/**
  An Indexed Environment is one using integer index 0..N to model
  ordered and aggregated values of an Environment.

  This is better suited to stream to persistent text file to protect against hash value creep.
  The file is consistent based on plain uniqie integer indexes for inter-value references.

  Also see CASEnvironment.
 */

#include "cas/cas.hpp"
#include <map>
#include <string>

class IndexedEnvironment {
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
private:
  Container m_container{};
};