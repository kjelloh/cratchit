#pragma once

#include "FiscalPeriod.hpp"
#include "cas.hpp"
#include <string>
#include <vector>
#include <map>
#include <filesystem>

// namespace cas now in cas unit

/*

  The Environment persistent file is a text file on the form:

  <type>:<index> <value>

  Where <index> is the reference used to encode structure (e.g. TaggedAmount aggregates SIE -> BAS members)
  And <value> is a tag-value sequence <tag>=<value>;<tag>=<value>;...

  E.g., 

  HeadingAmountDateTransEntry:0 "belopp=879.10;datum=20250930;rubrik=Momsrapport 2025-07-01...2025-09-30"
  ...
  TaggedAmount:665c9188f6246be "BAS=1920;Ix=0;cents_amount=-235900;parent_SIE=E2;yyyymmdd_date=20250616"
  ...
  contact:0 "e-mail=ville.vessla@foretaget.se;name=Ville Vessla;phone=555-244454"
  employee:0 "birth-id=198202252386"
  sie_file:0 "-1=sie_in/TheITfiedAB20250829_181631.se;-2=sie_in/TheITfiedAB20250828_143351.se;current=sie_in/TheITfiedAB20250812_145743.se"%                                                                              

*/

/*

  The internal model Environment is a map of each <type> into a section.
  Each section is a CAS (content addressable storage).

  Now parsing from the persistent storage to an Environment instance is done
  by first use the <index> in the persistent file as the 'instance id' for the CAS.
  Thus, when parsing the app trusts the index in the file and does not map the value to an id of its own.
  In this way the file is trusted to model the structure even if the index may not map to 
  current value->id function of the app.

*/

// TODO: Consider to refactor into clear 'PeristentEnvironment' vs 'Environment'
//       to more clearly see how the persistent file is parsed into the internal Environment representation? / 20251012
using EnvironmentValue = std::map<std::string,std::string>; // vector of name-value pairs
using EnvironmentValueName = std::string;
using EnvironmentValueId = std::size_t;
using EnvironmentValues_cas_repository = cas::repository<EnvironmentValueId,EnvironmentValue>;
using EnvironmentIdValuePair = EnvironmentValues_cas_repository::value_type; // mutable id-value pair
using EnvironmentCasEntryVector = std::vector<EnvironmentIdValuePair>; // To model the order in persistent file
using Environment = std::map<EnvironmentValueName,EnvironmentCasEntryVector>; // Note: Uses a vector of cas repository entries <id,Node> to keep ordering to-and-from file

// parsing environment in -> Environment
namespace in {
  bool is_comment_line(std::string const& line);
  bool is_value_line(std::string const& line);
  std::pair<std::string, std::optional<EnvironmentValueId>> to_name_and_id(std::string key);
  EnvironmentValue to_environment_value(std::string const s);
  Environment environment_from_file(std::filesystem::path const &p);
}
Environment environment_from_file(std::filesystem::path const &p);

// Processing environment -> out
namespace out {
  std::ostream& operator<<(std::ostream& os,EnvironmentValue const& ev);
  std::ostream& operator<<(std::ostream& os,Environment::value_type const& entry);
  std::ostream& operator<<(std::ostream& os,Environment const& env);
  std::string to_string(EnvironmentValue const& ev);
  std::string to_string(Environment::value_type const& entry);
  void environment_to_file(Environment const &environment,std::filesystem::path const &p);
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