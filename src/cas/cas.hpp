#pragma once

#include "logger/log.hpp"
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

  /**
      Design considerations for a cas (content adressable storage).

      0.  The prerequisite must be that each value is immutabe and unique.

          Thus, when 'mutating' a value we in fact insert a new value (leaving the previous value in place)

      1.  Each value is stored and referenced by a 'value id' that is unique for the value.

          Any two values with the same 'value id' is to be terated as the 'same' value.

      ...

  */
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
    // KeyValueMap &the_map() {
    //   return m_map;
    // }

    auto insert(value_type const& entry) {
      return m_map.insert(entry);
    }

    auto erase(Key key) {
      return m_map.erase(key);
    }
  };

  /**
      Design considerations for an ordered composite cas.

      0.  The prerequisite must be that each value is immutabe and unique.

          Thus, when 'mutating' a value we in fact insert a new value (leaving the previous value in place)

      2.  Now, Should each member encode its relations to other values?

          That is, does a cas value that participates in a cas value ordering encode its relation to 
          the previous and/or next value? Also, does an aggregate cas value 
          encode references to its aggregated member values?

      3. Or should ordering and aggregation be a separate meta-data mechanism on top of a plain cas?

      4. Should replacing a value with a new one also replace the value in an ordering and aggregation?

          That is, if we 'mutate' a value, should is also 'mutate' the value in an ordering and/or aggregation.

          It seems 'mutation' of a value that is in an aggregation value should also result in a new
          aggregation value referencing the new value?

          And it seems 'mutation' of a value that is member of an ordering should also result in a new
          ordering where all the values are the same but the new value is used instead of the 'mutated' value?

      ...

   */

  // A Content Adressable Storage (CAS) container that preserves ordering 
  // and models value aggregation
  template <typename ValueT,class Hasher = std::hash<ValueT>>
  class ordered_composite {
  public:
    using Value = ValueT;
    using Key = decltype(std::declval<Hasher>()(std::declval<Value>()));
    using MaybeKey = std::optional<Key>;
    using MaybeValue = std::optional<Value>;
    using PrevOf = std::function<MaybeValue(Value const& value,ordered_composite const& container)>;

    using Map = std::unordered_map<Key,Hasher>;

    using Keyes = std::vector<Key>;    
    using Values = std::vector<Value>;

    // using const_iterator = Values::const_iterator;

    // Key based const iterator
    class const_iterator {
    public:

      // BEGIN Iterator 

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

      // END Iterator

      auto ix() const {
        return m_ix;
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

    const_iterator insert(const_iterator const& iter,Value const& value) {
      if (iter == this->end()) {
        // No previous
        if (this->m_ordered_keyes.size() == 0) {
          auto key = this->m_hasher(value);
          this->m_ordered_keyes.push_back(key);
          this->m_map.insert({key,value});
        }
        else {
          logger::design_insufficiency("const_iterator::insert: Can't insert at end() for non-empty m_keyes");
        }
      }
      else {
        auto key = this->m_hasher(value);
        auto pos = std::advance(m_ordered_keyes.begin(),iter.ix());
        this->m_ordered_keyes.insert(pos,key);
        this->m_map.insert({key,value});
      }
    }

    std::pair<const_iterator,bool> insert(Value const& value) {
      if (!this->contains(value)) {
        if (auto maybe_prev = this->m_prev_of(value,*this)) {
          auto iter = std::ranges::find(*this,maybe_prev.value());
          return this->insert(iter,value);
        }
        else {
          // No prev = first
          return this->insert(this->end(),value);
        }
      }
      else {
        // value already exists
        auto iter = std::ranges::find(*this,value);
        return {iter,false};
      }

    }

  };

} // namespace cas
