#pragma once

#include <map>

// Content Addressable Storage namespace
// TODO: Consider to make cas::repository more like std::unordrered_map in that
//       it inserts and retreives members based on known Key-function and provided member value?
//       That is, do NOT expose the_map?
namespace cas {

  template <typename Key, typename Value>
  class repository {
  private:
    using KeyValueMap = std::map<Key, Value>;
    KeyValueMap m_map{};
  public:
    // Tricky! We need EnvironmentCasEntryVector to be assignable, so we cannot
    // use KeyValueMap::value_type directly as this would make the Key type const (not mutable).
    // using value_type = KeyValueMap::value_type;
    using value_type = std::pair<Key, Value>;
    bool contains(Key const &key) const { return m_map.contains(key); }
    Value const &at(Key const &key) const { return m_map.at(key); }
    void clear() { return m_map.clear(); }
    KeyValueMap &the_map() {
      return m_map;
    }
    repository& operator=(const repository &other) {
      if (this != &other) {
        m_map = other.m_map; // OK: std::map is assignable
      }
      return *this;
    }
  };
} // namespace cas
