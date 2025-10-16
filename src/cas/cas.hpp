#pragma once

#include <map>
#include <unordered_map>
#include <functional> // std::function
#include <ranges>

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
    // Tricky! We need EnvironmentIdValuePairs to be assignable, so we cannot
    // use KeyValueMap::value_type directly as this would make the Key type const (not mutable).
    // using value_type = KeyValueMap::value_type;
    using value_type = std::pair<Key, Value>;
    bool contains(Key const &key) const { return m_map.contains(key); }
    Value const &at(Key const &key) const { return m_map.at(key); }
    void clear() { return m_map.clear(); }
    repository& operator=(const repository &other) {
      if (this != &other) {
        m_map = other.m_map; // OK: std::map is assignable
      }
      return *this;
    }
    KeyValueMap &the_map() {
      return m_map;
    }
  };

  // A Content Adressable Storage (CAS) container that preserves ordering 
  // and models value aggregation
  template <typename Value,class Hasher = std::hash<Value>>
  class ordered_composite {
  public:
    using Key = decltype(std::declval<Hasher>()(std::declval<Value>()));
    using MaybeKey = std::optional<Key>;
    using PrevOf = std::function<MaybeKey(Key key,ordered_composite const& container)>;
  private:
    Hasher m_hasher;
    std::unordered_map<Key,Hasher> m_map{};
    std::vector<Key> m_ordered_keyes{};
    PrevOf m_prev_of;
  public:
    ordered_composite(PrevOf const& prev_of,Hasher hasher = Hasher{}) 
      :  m_prev_of{prev_of}
        ,m_hasher{hasher} {}

    ordered_composite& clear() {
      m_map.clear();
      return *this;
    }

    bool contains(Key key) {
      return m_map.contains(key);
    }

    Value at(Key key) const {
      return m_map.at(key);
    }

    std::pair<Key,bool> insert(Value const& value) {
      auto key = m_hasher(value);
      auto [iter,inserted] = m_map.insert({key,value});
      if (inserted) {
        if (auto maybe_prev = m_prev_of(key)) {
          auto prev = maybe_prev.value();
          auto iter = std::ranges::find(m_ordered_keyes,prev);
          m_ordered_keyes.insert(iter,key);
        }
        else {
          // No previous = first or stand-alone
        }
      }
      return {key,inserted};
    }

  };

} // namespace cas
