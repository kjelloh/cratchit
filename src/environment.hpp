#pragma once

#include <string>
#include <vector>
#include <map>
#include <filesystem>

// Content Addressable Storage namespace
namespace cas {

  template <typename Key, typename Value> class repository {
  private:
    using KeyValueMap = std::map<Key, Value>;
    KeyValueMap m_map{};
    using Keys = std::vector<Key>;
    using AdjacencyList = std::map<Key, Keys>;
    AdjacencyList m_adj{};

  public:
    using value_type = KeyValueMap::value_type;
    bool contains(Key const &key) const { return m_map.contains(key); }
    Value const &at(Key const &key) const { return m_map.at(key); }
    void clear() { return m_map.clear(); }
    KeyValueMap &the_map() {
      // std::cout << "\nDESIGN_INSUFFICIENCY: called cas::repository::the_map()
      // to gain access to aggregated map (Developer should extend repository
      // services to protect internal map handling.)";
      return m_map;
    }
    AdjacencyList &the_adjacency_list() { return m_adj; }
  };
} // namespace cas

using EnvironmentValue = std::map<std::string,std::string>; // vector of name-value-pairs
using EnvironmentValueName = std::string;
using EnvironmentValueId = std::size_t;
using EnvironmentValues_cas_repository = cas::repository<EnvironmentValueId,EnvironmentValue>;
using EnvironmentCasEntryVector = std::vector<EnvironmentValues_cas_repository::value_type>;
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
