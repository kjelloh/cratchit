#pragma once

#include "AmountFramework.hpp"

class TaggedAmount {
public:
  friend std::ostream &operator<<(std::ostream &os, TaggedAmount const &ta);
  using OptionalTagValue = std::optional<std::string>;
  using Tags = std::map<std::string, std::string>;
  using ValueId = std::size_t;
  using OptionalValueId = std::optional<ValueId>;
  using ValueIds = std::vector<ValueId>;
  using OptionalValueIds = std::optional<ValueIds>;
  TaggedAmount(Date const &date, CentsAmount const &cents_amount,
               Tags &&tags = Tags{})
      : m_date{date}, m_cents_amount{cents_amount}, m_tags{tags} {}

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
  static std::string to_string(TaggedAmount::ValueId value_id) {
    std::ostringstream os{};
    os << std::setw(sizeof(std::size_t) * 2) << std::setfill('0') << std::hex
       << value_id << std::dec;
    return os.str();
  }

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

TaggedAmount::ValueId to_value_id(TaggedAmount const &ta) {
  return std::hash<TaggedAmount>{}(ta);
}

bool TaggedAmount::operator==(TaggedAmount const &other) const {
  auto result =
      this->date() == other.date() and
      this->cents_amount() == other.cents_amount() and
      std::all_of(m_tags.begin(), m_tags.end(),
                  [&other](Tags::value_type const &entry) {
                    return ((entry.first.starts_with("_")) or
                            (other.tags().contains(entry.first) and
                             other.tags().at(entry.first) == entry.second));
                  });

  // std::cout << "\nTaggedAmountClass::operator== ";
  // if (result) std::cout << "TRUE"; else std::cout << "FALSE";
  return result;
}

std::ostream &operator<<(std::ostream &os, TaggedAmount const &ta) {
  // os << TaggedAmount::to_string(to_value_id(ta));
  os << " " << ::to_string(ta.date());
  os << " " << ::to_string(to_units_and_cents(ta.cents_amount()));
  for (auto const &tag : ta.tags()) {
    os << "\n\t|--> \"" << tag.first << "=" << tag.second << "\"";
  }
  return os;
}

TaggedAmount::OptionalValueId to_value_id(std::string const &s) {
  // std::cout << "\nto_value_id()" << std::flush;
  TaggedAmount::OptionalValueId result{};
  TaggedAmount::ValueId value_id{};
  std::istringstream is{s};
  try {
    is >> std::hex >> value_id;
    result = value_id;
  } catch (...) {
    std::cout << "\nto_value_id(" << std::quoted(s)
              << ") failed. General Exception caught." << std::flush;
  }
  return result;
}

TaggedAmount::OptionalValueIds to_value_ids(Key::Path const &sids) {
  // std::cout << "\nto_value_ids()" << std::flush;
  TaggedAmount::OptionalValueIds result{};
  TaggedAmount::ValueIds value_ids{};
  for (auto const &sid : sids) {
    if (auto value_id = to_value_id(sid)) {
      // std::cout << "\n\tA valid instance id sid=" << std::quoted(sid);
      value_ids.push_back(*value_id);
    } else {
      std::cout << "\nDESIGN_INSUFFICIENCY: to_value_ids: Not a valid instance "
                   "id string sid="
                << std::quoted(sid) << std::flush;
    }
  }
  if (value_ids.size() == sids.size()) {
    result = value_ids;
  } else {
    std::cout << "\nDESIGN_INSUFFICIENCY: to_value_ids(Key::Path const& "
              << sids.to_string() << ") Failed. Created" << value_ids.size()
              << " out of " << sids.size() << " possible.";
  }
  return result;
}

// using TaggedAmountValueIdMap = std::map<TaggedAmount::ValueId,TaggedAmount>;
// using TaggedAmountValueIdMap =
// cas::repository<TaggedAmount::ValueId,TaggedAmount>;
using TaggedAmountsCasRepository =
    cas::repository<TaggedAmount::ValueId, TaggedAmount>;

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
  TaggedAmounts const &tagged_amounts() {
    return m_date_ordered_tagged_amounts;
  }
  std::size_t size() const { return m_date_ordered_tagged_amounts.size(); }
  iterator begin() { return m_date_ordered_tagged_amounts.begin(); }
  iterator end() { return m_date_ordered_tagged_amounts.end(); }
  const_iterator begin() const { return m_date_ordered_tagged_amounts.begin(); }
  const_iterator end() const { return m_date_ordered_tagged_amounts.end(); }
  auto in_date_range(DateRange const &date_period) {
    auto first = std::find_if(this->begin(), this->end(),
                              [&date_period](auto const &ta) {
                                return (ta.date() >= date_period.begin());
                              });
    auto last = std::find_if(this->begin(), this->end(),
                             [&date_period](auto const &ta) {
                               return (ta.date() > date_period.end());
                             });
    return std::ranges::subrange(first, last);
  }

  OptionalTaggedAmount at(ValueId const &value_id) {
    std::cout << "\nDateOrderedTaggedAmountsContainer::at("
              << TaggedAmount::to_string(value_id) << ")" << std::flush;
    OptionalTaggedAmount result{};
    if (m_tagged_amount_cas_repository.contains(value_id)) {
      result = m_tagged_amount_cas_repository.at(value_id);
    } else {
      std::cout << "\nDateOrderedTaggedAmountsContainer::at could not find a "
                   "mapping to value_id="
                << TaggedAmount::to_string(value_id) << std::flush;
    }
    return result;
  }

  OptionalTaggedAmount operator[](ValueId const &value_id) {
    std::cout << "\nDateOrderedTaggedAmountsContainer::operator[]("
              << TaggedAmount::to_string(value_id) << ")" << std::flush;
    OptionalTaggedAmount result{};
    if (auto o_ptr = this->at(value_id)) {
      result = o_ptr;
    } else {
      std::cout << "\nDateOrderedTaggedAmountsContainer::operator[] could not "
                   "find a mapping to value_id="
                << TaggedAmount::to_string(value_id) << std::flush;
    }
    return result;
  }

  OptionalTaggedAmounts to_tagged_amounts(ValueIds const &value_ids) {
    std::cout << "\nDateOrderedTaggedAmountsContainer::to_tagged_amounts()"
              << std::flush;
    OptionalTaggedAmounts result{};
    TaggedAmounts tas{};
    for (auto const &value_id : value_ids) {
      if (auto o_ta = (*this)[value_id]) {
        tas.push_back(*o_ta);
      } else {
        std::cout << "\nDateOrderedTaggedAmountsContainer::to_tagged_amounts() "
                     "failed. No instance found for value_id="
                  << TaggedAmount::to_string(value_id) << std::flush;
      }
    }
    if (tas.size() == value_ids.size()) {
      result = tas;
    } else {
      std::cout << "\nto_tagged_amounts() Failed. tas.size() = " << tas.size()
                << " IS NOT provided value_ids.size() = " << value_ids.size()
                << std::flush;
    }
    return result;
  }

  DateOrderedTaggedAmountsContainer &clear() {
    m_tagged_amount_cas_repository.clear();
    m_date_ordered_tagged_amounts.clear();
    return *this;
  }

  DateOrderedTaggedAmountsContainer &
  operator=(DateOrderedTaggedAmountsContainer const &other) {
    this->m_date_ordered_tagged_amounts = other.m_date_ordered_tagged_amounts;
    this->m_tagged_amount_cas_repository = other.m_tagged_amount_cas_repository;
    return *this;
  }

  // Insert the value and return the iterator to the vector of tas
  // (internal CAS map is hidden from client)
  // But the internally used key (the ValueId) is returned for environment vs
  // tagged amounts key transformation purposes (to and from Environment)
  std::pair<ValueId, iterator> insert(TaggedAmount const &ta) {
    auto result = m_date_ordered_tagged_amounts.end();
    auto value_id = to_value_id(ta);
    if (m_tagged_amount_cas_repository.contains(value_id) == false) {
      if (false) {
        std::cout << "\nthis:" << this << " Inserted new " << ta;
      }
      // Find the last element with a date less than the date of ta
      auto end = std::upper_bound(
          m_date_ordered_tagged_amounts.begin(),
          m_date_ordered_tagged_amounts.end(), ta,
          [](TaggedAmount const &ta1, TaggedAmount const &ta2) {
            return ta1.date() < ta2.date();
          });

      m_tagged_amount_cas_repository.the_map().insert(
          {value_id, ta}); // id -> ta
      result = m_date_ordered_tagged_amounts.insert(
          end, ta); // place after all with date less than the one of ta
    } else {
      // std::cout << "\nthis:" << this;
      // std::cout << "\n\tDESIGN_INSUFFICIENCY: Error, Skipped new[" <<
      // TaggedAmount::to_string(value_id) << "] " << ta; std::cout << "\n\t
      // same as old[" <<
      // TaggedAmount::TaggedAmount::to_string(to_value_id(m_tagged_amount_cas_repository.at(value_id)))
      // << "] " << m_tagged_amount_cas_repository.at(value_id);
    }
    return {value_id, result};
  }

  DateOrderedTaggedAmountsContainer &erase(ValueId const &value_id) {
    if (auto o_ptr = this->at(value_id)) {
      m_tagged_amount_cas_repository.the_map().erase(value_id);
      auto iter = std::ranges::find(m_date_ordered_tagged_amounts, *o_ptr);
      if (iter != m_date_ordered_tagged_amounts.end()) {
        m_date_ordered_tagged_amounts.erase(iter);
      } else {
        std::cout << "\nDESIGN INSUFFICIENCY: Failed to erase tagged amount in "
                     "map but not in date-ordered-vector, value_id "
                  << value_id;
      }
    } else {
      std::cout
          << "nDESIGN INSUFFICIENCY: DateOrderedTaggedAmountsContainer::at "
             "failed to find value_id "
          << value_id;
    }
    return *this;
  }

  DateOrderedTaggedAmountsContainer const &for_each(auto f) const {
    for (auto const &ta : m_date_ordered_tagged_amounts) {
      f(ta);
    }
    return *this;
  }

  DateOrderedTaggedAmountsContainer &
  operator+=(DateOrderedTaggedAmountsContainer const &other) {
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
  TaggedAmountsCasRepository
      m_tagged_amount_cas_repository{}; // map <instance id> -> <tagged amount>
                                        // as content addressable storage
                                        // repository
  TaggedAmounts m_date_ordered_tagged_amounts{}; // vector of tagged amount ptrs
                                                 // ordered by date
}; // class DateOrderedTaggedAmountsContainer

// Forward declaration of data and members of namespaces
namespace BAS {
  using AccountNo = unsigned int;
  using OptionalAccountNo = std::optional<AccountNo>;
  using AccountNos = std::vector<AccountNo>;
  using OptionalAccountNos = std::optional<AccountNos>;

  OptionalAccountNo to_account_no(std::string const &s);
} // namespace BAS

namespace tas {
  // namespace for processing that produces tagged amounts and tagged amounts

  // Generic for parsing a range or container of tagged amount pointers into a
  // vector of saldo tagged amounts (tagged with 'BAS' for each accumulated bas
  // account)
  TaggedAmounts to_bas_omslutning(auto const &tas) {
    TaggedAmounts result{};
    using BASBuckets = std::map<BAS::AccountNo, TaggedAmounts>;
    BASBuckets bas_buckets{};
    auto is_valid_bas_account_transaction = [](TaggedAmount const &ta) {
      if (ta.tags().contains("BAS") and
          !(BAS::to_account_no(ta.tags().at("BAS")))) {
        // Whine about invalid tagging of 'BAS' tag!
        // It is vital we do NOT have any badly tagged BAS account transactions
        // as this will screw up the saldo calculation!
        std::cout << "\nDESIGN_INSUFFICIENCY: tas::to_bas_omslutning failed to "
                     "create a valid BAS account no from tag 'BAS' with value "
                  << std::quoted(ta.tags().at("BAS"));
        return false;
      } else
        return ((ta.tags().contains("BAS")) and
                (BAS::to_account_no(ta.tags().at("BAS"))) and
                (((ta.tags().contains("type") == false)) or
                 ((ta.tags().contains("type") == true) and
                  (ta.tags().at("type") != "saldo"))));
    };
    for (auto const &ta : tas) {
      if (is_valid_bas_account_transaction(ta)) {
        bas_buckets[*BAS::to_account_no(ta.tags().at("BAS"))].push_back(ta);
      }
    }
    for (auto const &[bas_account_no, tas] : bas_buckets) {
      Date period_end_date{};
      std::cout << "\n" << std::dec << bas_account_no;
      auto cents_saldo = std::accumulate(
          tas.begin(), tas.end(), CentsAmount{0},
          [&period_end_date](auto acc, auto const &ta) {
            period_end_date =
                std::max(period_end_date,
                         ta.date()); // Ensure we keep the latest date. NOTE: We
                                     // expect they are in growing date order.
                                     // But just in case...
            acc += ta.cents_amount();
            std::cout << "\n\t" << period_end_date << " "
                      << to_string(to_units_and_cents(ta.cents_amount()))
                      << " ackumulerat:" << to_string(to_units_and_cents(acc));
            return acc;
          });

      TaggedAmount saldo_ta{period_end_date, cents_saldo};
      saldo_ta.tags()["BAS"] = std::to_string(bas_account_no);
      saldo_ta.tags()["type"] = "saldo";
      result.push_back(saldo_ta);
    }
    return result;
  }
} // namespace tas
