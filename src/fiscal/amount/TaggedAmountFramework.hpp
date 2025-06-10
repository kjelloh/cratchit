#pragma once

#include "fiscal/BAS.hpp" // BAS::AccountNo,
#include "AmountFramework.hpp"
#include "environment.hpp" // namespace cas,
#include <ostream>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <limits> // std::numeric_limits

class TaggedAmount {
public:
  friend std::ostream &operator<<(std::ostream &os, TaggedAmount const &ta);
  using OptionalTagValue = std::optional<std::string>;
  using Tags = std::map<std::string, std::string>;
  using ValueId = std::size_t;
  using OptionalValueId = std::optional<ValueId>;
  using ValueIds = std::vector<ValueId>;
  using OptionalValueIds = std::optional<ValueIds>;

  TaggedAmount(Date const &date, CentsAmount const &cents_amount,Tags &&tags = Tags{});

  // Getters
  Date const &date() const { return m_date; }
  CentsAmount const &cents_amount() const { return m_cents_amount; }
  Tags const &tags() const { return m_tags; }
  Tags &tags() { return m_tags; }

  // Map key to optional value
  OptionalTagValue tag_value(std::string const &key) const {
    OptionalTagValue result{};
    if (m_tags.contains(key)) {
      result = m_tags.at(key);
    }
    return result;
  }

  bool operator==(TaggedAmount const &other) const;

  // tagged_amount::to_string ensures it does not override
  // std::to_string(integral type) or any local one
  static std::string to_string(TaggedAmount::ValueId value_id);

private:
  Date m_date;
  CentsAmount m_cents_amount;
  Tags m_tags;
}; // class TaggedAmount

using TaggedAmounts = std::vector<TaggedAmount>;
using OptionalTaggedAmount = std::optional<TaggedAmount>;
using OptionalTaggedAmounts = std::optional<TaggedAmounts>;

template <class T>
__attribute__((no_sanitize("undefined"))) inline void
hash_combine(std::size_t &seed, const T &v) {
  constexpr auto shift_left_count = 1;
  constexpr std::size_t max_size_t = std::numeric_limits<std::size_t>::max();
  constexpr std::size_t mask = max_size_t >> shift_left_count;
  std::hash<T> hasher;
  // Note: I decided to NOT use boost::hash_combine code as it will cause
  // integer overflow and thus undefined behaviour.
  //       And I need the hash I produce to be consistent so I can use it in
  //       persistent storage (the environment text file for Cratchit) Now,
  //       maybe the risk of getting different values on different hardware or
  //       runtimes (macOS, Linus, Windows etc) is non existent in practice. But
  //       hey, better safe than sorry, right?
  // seed ^= hasher(v) + 0x9e3779b9 + ((seed & mask) <<6) + (seed>>2); //
  // *magic* dustribution as defined by boost::hash_combine
  seed ^= (hasher(v) & mask)
          << shift_left_count; // Simple shift left distribution and no addition
}

namespace std {
  template <> struct hash<TaggedAmount> {
    std::size_t operator()(TaggedAmount const &ta) const noexcept {
      std::size_t result{};
      auto yyyymmdd = ta.date();
      hash_combine(result, static_cast<int>(yyyymmdd.year()));
      hash_combine(result, static_cast<unsigned>(yyyymmdd.month()));
      hash_combine(result, static_cast<unsigned>(yyyymmdd.day()));
      hash_combine(result, ta.cents_amount());
      for (auto const &[key, value] : ta.tags()) {
        hash_combine(result, key);
        hash_combine(result, value);
      }
      return result;
    }
  };
} // namespace std

TaggedAmount::ValueId to_value_id(TaggedAmount const &ta);
std::ostream &operator<<(std::ostream &os, TaggedAmount const &ta);
TaggedAmount::OptionalValueId to_value_id(std::string const &s);
TaggedAmount::OptionalValueIds to_value_ids(Key::Path const &sids);

// using TaggedAmountValueIdMap = std::map<TaggedAmount::ValueId,TaggedAmount>;
// using TaggedAmountValueIdMap = cas::repository<TaggedAmount::ValueId,TaggedAmount>;
using TaggedAmountsCasRepository = cas::repository<TaggedAmount::ValueId, TaggedAmount>;

// Behaves more or less as a vector of tagged amounts in date order.
// But uses a map <Key,Value> as the mechanism to look up a value based on its
// value_id. A of 240622 the behind-the-scenes map is a CAS (Content Addressable
// Storage) with a key being a hash of its 'value'
class DateOrderedTaggedAmountsContainer {
public:
  using OptionalTagValue = TaggedAmount::OptionalTagValue;
  using Tags = TaggedAmount::Tags;
  using ValueId = TaggedAmount::ValueId;
  using OptionalValueId = TaggedAmount::OptionalValueId;
  using ValueIds = TaggedAmount::ValueIds;
  using OptionalValueIds = TaggedAmount::OptionalValueIds;

  using iterator = TaggedAmounts::iterator;
  using const_iterator = TaggedAmounts::const_iterator;
  using const_subrange = std::ranges::subrange<const_iterator, const_iterator>;

  TaggedAmounts const &tagged_amounts() {
    return m_date_ordered_tagged_amounts;
  }
  std::size_t size() const { return m_date_ordered_tagged_amounts.size(); }
  iterator begin() { return m_date_ordered_tagged_amounts.begin(); }
  iterator end() { return m_date_ordered_tagged_amounts.end(); }
  const_iterator begin() const { return m_date_ordered_tagged_amounts.begin(); }
  const_iterator end() const { return m_date_ordered_tagged_amounts.end(); }
  const_subrange in_date_range(DateRange const &date_period);
  OptionalTaggedAmount at(ValueId const &value_id);
  OptionalTaggedAmount operator[](ValueId const &value_id);
  OptionalTaggedAmounts to_tagged_amounts(ValueIds const &value_ids);

  DateOrderedTaggedAmountsContainer &clear() {
    m_tagged_amount_cas_repository.clear();
    m_date_ordered_tagged_amounts.clear();
    return *this;
  }

  DateOrderedTaggedAmountsContainer& operator=(DateOrderedTaggedAmountsContainer const &other) {
    this->m_date_ordered_tagged_amounts = other.m_date_ordered_tagged_amounts;
    this->m_tagged_amount_cas_repository = other.m_tagged_amount_cas_repository;
    return *this;
  }

  // Insert the value and return the iterator to the vector of tas
  // (internal CAS map is hidden from client)
  // But the internally used key (the ValueId) is returned for environment vs
  // tagged amounts key transformation purposes (to and from Environment)
  std::pair<ValueId, iterator> insert(TaggedAmount const &ta);

  DateOrderedTaggedAmountsContainer &erase(ValueId const &value_id);

  DateOrderedTaggedAmountsContainer const &for_each(auto f) const {
    for (auto const &ta : m_date_ordered_tagged_amounts) {
      f(ta);
    }
    return *this;
  }

  DateOrderedTaggedAmountsContainer& operator+=(DateOrderedTaggedAmountsContainer const &other) {
    other.for_each([this](TaggedAmount const &ta) {
      // TODO 240217: Consider a way to ensure that SIE entries in SIE file has
      // preceedence (overwrite any existing tagged amounts reflecting the same
      // events) Hm...Maybe this is NOT the convenient place to do this?
      this->insert(ta);
    });
    return *this;
  }
  DateOrderedTaggedAmountsContainer &operator+=(TaggedAmounts const &tas) {
    for (auto const &ta : tas)
      this->insert(ta);
    return *this;
  }

  DateOrderedTaggedAmountsContainer &operator=(TaggedAmounts const &tas) {
    this->clear();
    *this += tas;
    return *this;
  }

private:
  // Note: Each tagged amount pointer instance is stored twice. Once in a
  // mapping between value_id and tagged amount pointer and once in a vector
  // ordered by date.
  TaggedAmountsCasRepository m_tagged_amount_cas_repository{};  // map <instance id> -> <tagged amount>
                                                                // as content addressable storage
                                                                // repository
  TaggedAmounts m_date_ordered_tagged_amounts{}; // vector of tagged amount ptrs
                                                 // ordered by date
}; // class DateOrderedTaggedAmountsContainer

// namespace BAS with 'forwards' now in BAS.hpp

namespace tas {
  // namespace for processing that produces tagged amounts and tagged amounts

  // Generic for parsing a range or container of tagged amount pointers into a
  // vector of saldo tagged amounts (tagged with 'BAS' for each accumulated bas
  // account)
  TaggedAmounts to_bas_omslutning(DateOrderedTaggedAmountsContainer::const_subrange const& tas);

} // namespace tas
