#pragma once

#include <map>
#include <unordered_map>
#include <functional> // std::function
#include <ranges>
#include <limits>  // for std::numeric_limits

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

    using Map = std::unordered_map<Key,Hasher>;

    using Keyes = std::vector<Key>;    
    using Values = std::vector<Value>;

    // using const_iterator = Values::const_iterator;

    // Key based const iterator
    class const_iterator {
    public:
      using value_type = Value;
      // using reference = Value const&;
      // using pointer = Value const*;
      using difference_type = std::ptrdiff_t;
      using iterator_category = std::forward_iterator_tag;

      const_iterator() = default;
      const_iterator(Map const& map,Keyes const& keyes,size_t ix)
        :  m_map{&map}
          ,m_keyes{&keyes}
          ,m_ix{ix} {}

      Value const& operator*() {
        if (m_map==nullptr or m_keyes == nullptr or m_ix > m_keyes->size()) {
          throw std::runtime_error(std::format("ordered_composite::const_iterator: Can't dereference map:{} keyes:{} ix:{}",m_map,m_keyes,m_ix));
        }
        return m_map->at(m_keyes->at(m_ix));
      }

      const_iterator& operator++() { 
        ++m_ix; 
        return *this; 
      }

      const_iterator operator++(int) {
        const_iterator tmp = *this;
        ++(*this);
        return tmp;
      }

      bool operator==(const const_iterator& other) const noexcept {
        return m_map == other.m_map && m_keyes == other.m_keyes && m_ix == other.m_ix;
      }

      bool operator!=(const const_iterator& other) const noexcept {
        return !(*this == other);
      }

    private:
      size_t m_ix{std::numeric_limits<size_t>::max()};
      Keyes const* m_keyes{nullptr};
      Map const* m_map{nullptr};
    };

    using const_subrange = std::ranges::subrange<const_iterator, const_iterator>;

    const_iterator begin() const { return const_iterator(m_map,m_ordered_keyes,0);}
    const_iterator end() const { return const_iterator(m_map,m_ordered_keyes,m_ordered_keyes.size());}

  private:
    Hasher m_hasher;
    Map m_map{};
    Keyes m_ordered_keyes{};
    PrevOf m_prev_of;
  public:
    ordered_composite(PrevOf const& prev_of,Hasher hasher = Hasher{}) 
      :  m_prev_of{prev_of}
        ,m_hasher{hasher} {}

    ordered_composite& clear() {
      m_map.clear();
      return *this;
    }

    bool contains(Value const& value) const {
      return m_map.contains(m_hasher(value));
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
