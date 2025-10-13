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

namespace detail {

  using EnvironmentValue = std::map<std::string,std::string>; // vector of name-value pairs
  using EnvironmentValueName = std::string;
  using EnvironmentValueId = std::size_t;
  using EnvironmentValues_cas_repository = cas::repository<EnvironmentValueId,EnvironmentValue>;
  using EnvironmentIdValuePair = EnvironmentValues_cas_repository::value_type; // mutable id-value pair
  using EnvironmentIdValuePairs = std::vector<EnvironmentIdValuePair>; // To model the order in persistent file
} // detail

// TODO: Refactor into proper indexed based composite.
//       For now this is the original Environment just cloned
using IndexedEnvironment = std::map<detail::EnvironmentValueName,detail::EnvironmentIdValuePairs>; // Note: Uses a vector of cas repository entries <id,Node> to keep ordering to-and-from file
