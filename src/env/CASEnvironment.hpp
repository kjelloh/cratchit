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

namespace detail {

  using EnvironmentValue = std::map<std::string,std::string>; // vector of name-value pairs
  using EnvironmentValueName = std::string;
  using EnvironmentValueId = std::size_t;
  using EnvironmentValues_cas_repository = cas::repository<EnvironmentValueId,EnvironmentValue>;
  using EnvironmentIdValuePair = EnvironmentValues_cas_repository::value_type; // mutable id-value pair
  using EnvironmentIdValuePairs = std::vector<EnvironmentIdValuePair>; // To model the order in persistent file
  using Environment = std::map<EnvironmentValueName,EnvironmentIdValuePairs>; // Note: Uses a vector of cas repository entries <id,Node> to keep ordering to-and-from file
} // detail

using CASEnvironmentValueId = detail::EnvironmentValueId;
using CASEnvironmentValue = detail::EnvironmentValue;
using CASEnvironmentIdValuePair = detail::EnvironmentIdValuePair;
using CASEnvironmentIdValuePairs = detail::EnvironmentIdValuePairs;

// TODO: Refactor into propoer CAS (Content Adressable Storage)
//       For now this is just a clone of original Environment that mixes index and hash based ID-Value mapping
//       also consumed from persistent storage text file.
using CASEnvironment = detail::Environment;
