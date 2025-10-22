#pragma once

#include "logger/log.hpp"
#include <unordered_map>
#include <functional> // std::function
#include <ranges>
#include <limits>  // for std::numeric_limits

// Content Addressable Storage namespace
namespace cas {

  /**
      Design considerations for a cas (content adressable storage).

      0.  The prerequisite must be that each value is immutabe and unique.

          Thus, when 'mutating' a value we in fact insert a new value (leaving the previous value in place)

      1.  Each value is stored and referenced by a 'value id' that is unique for the value.

          Any two values with the same 'value id' is to be terated as the 'same' value.

      ...

  */

  // CidT = Content ID type (Adress in CAS of Value)
  // ValueT = Content value type
  // ToCidT = Function: ValueT -> CidT (maps value to CAS address / CidT)
  template <typename CidT, typename ValueT,typename ToCidT>
  class repository {
  public:
    using Value = ValueT;
    using MaybeValue = std::optional<Value>;
    using ToCid = ToCidT;
    using Cid = decltype(std::declval<ToCid>()(std::declval<Value>())); // Return type of ToCid function

    // Refactoring error
    // TODO: Refactor code so that the template does not take CidT from the client.
    //       For now, have the compiler ensure we do not break the contract
    static_assert(
        std::is_same_v<Cid, CidT>,
        "ToCidT::operator()(ValueT) must return the same type as CidT"
    );    
    // Note: Cratchit uses a vector<value_type> to pass (assign) ordered values
    //       so we need value_type to be mutable.
    //       KeyValueMap::value_type wount do as then Cid is const (non mutable)
    //       TODO: Refactor this approach to make it work some other way?
    using mutable_cid_value_type = std::pair<Cid, Value>;
  private:
    using KeyValueMap = std::unordered_map<Cid, Value>;
    KeyValueMap m_map{};
    ToCid m_to_cid{};
  public:

    auto size() const {return m_map.size();}

    bool contains(Cid const &cid) const { return m_map.contains(cid); }

    MaybeValue cas_repository_get(Cid const &cid) const { 
      MaybeValue result{};

      auto iter = m_map.find(cid);
      if (iter != m_map.end()) {
        result = iter->second;
      }
      return result;
    }
    void clear() { return m_map.clear(); }
    repository& operator=(const repository &other) {
      if (this != &other) {
        m_map = other.m_map;
      }
      return *this;
    }

    std::pair<Cid,bool> try_cas_repository_put(Value const& value) const {
      auto cid = m_to_cid(value);
      auto is_new_value = m_map.contains(cid);
      return {cid,is_new_value};
    }

    std::pair<Cid,bool> cas_repository_put(Value const& value) {
      auto cid = m_to_cid(value);
      auto result = m_map.insert({cid,value});
      return {cid,result.second};
    }

    auto erase(Cid cid) {
      return m_map.erase(cid);
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
  template <typename ValueT,class ToCidT = std::hash<ValueT>>
  class ordered_composite {
  public:
    using Value = ValueT; // Expose template arg type
    using ToCid = ToCidT; // Expose template arg type

    using Cid = decltype(std::declval<ToCid>()(std::declval<Value>()));
    using MaybeKey = std::optional<Cid>;
    using MaybeValue = std::optional<Value>;
    using PrevOf = std::function<MaybeValue(Value const& value,ordered_composite const& container)>;

    using Map = std::unordered_map<Cid,ToCid>;

    using Keyes = std::vector<Cid>;    
    using Values = std::vector<Value>;

    // using const_iterator = Values::const_iterator;

    // Cid based const iterator
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
    ToCid m_hasher;
    Map m_map{};
    Keyes m_ordered_keyes{};
    PrevOf m_prev_of;
  public:
    ordered_composite(PrevOf const& prev_of,ToCid to_cid = ToCid{}) 
      :  m_prev_of{prev_of}
        ,m_hasher{to_cid} {}

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
          auto Cid = this->m_hasher(value);
          this->m_ordered_keyes.push_back(Cid);
          this->m_map.insert({Cid,value});
        }
        else {
          logger::design_insufficiency("const_iterator::insert: Can't insert at end() for non-empty m_keyes");
        }
      }
      else {
        auto Cid = this->m_hasher(value);
        auto pos = std::advance(m_ordered_keyes.begin(),iter.ix());
        this->m_ordered_keyes.insert(pos,Cid);
        this->m_map.insert({Cid,value});
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
