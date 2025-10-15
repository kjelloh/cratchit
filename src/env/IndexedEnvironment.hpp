#pragma once

/**
  An Indexed Environment is one using integer index 0..N to model
  ordered and aggregated values of an Environment.

  This is better suited to stream to persistent text file to protect against hash value creep.
  The file is consistent based on plain uniqie integer indexes for inter-value references.

  Also see CASEnvironment.
 */

#include "cas.hpp"
#include <map>
#include <string>

class IndexedEnvironment {
public:

  using EnvironmentValue = std::map<std::string,std::string>; // vector of name-value pairs
  using EnvironmentValueName = std::string;
  using EnvironmentValueId = std::size_t;
  using EnvironmentValues_cas_repository = cas::repository<EnvironmentValueId,EnvironmentValue>;
  using EnvironmentIdValuePair = EnvironmentValues_cas_repository::value_type; // mutable id-value pair
  using EnvironmentIdValuePairs = std::vector<EnvironmentIdValuePair>; // To model the order in persistent file
  using Environment = std::map<EnvironmentValueName,EnvironmentIdValuePairs>; // Note: Uses a vector of cas repository entries <id,Node> to keep ordering to-and-from file

  using value_type = Environment::value_type;
  auto& operator[](EnvironmentValueName const& section) {
    return m_env[section];
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