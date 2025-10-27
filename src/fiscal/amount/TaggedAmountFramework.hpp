#pragma once

#include "fiscal/BASFramework.hpp" // BAS::AccountNo,
#include "AmountFramework.hpp"
#include "env/environment.hpp" // namespace cas,
#include "FiscalPeriod.hpp"
#include "csv/parse_csv.hpp" // CSV::project::HeadingId,...
#include <ostream>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <limits> // std::numeric_limits
#include <ranges>
#include <numeric> // std::accumulate,
#include <tuple>

class TaggedAmount {
public:
  friend std::ostream &operator<<(std::ostream &os, TaggedAmount const& ta);
  using OptionalTagValue = std::optional<std::string>;
  using Tags = std::map<std::string, std::string>;
  using ValueId = std::size_t;
  using OptionalValueId = std::optional<ValueId>;
  using ValueIds = std::vector<ValueId>;
  using OptionalValueIds = std::optional<ValueIds>;

  // TaggedAmount(Date const& date, CentsAmount const& cents_amount,Tags &&tags = Tags{});
  TaggedAmount(Date date, CentsAmount cents_amount,Tags tags = Tags{});

  // Getters
  Date const& date() const { return m_date; }
  CentsAmount const& cents_amount() const { return m_cents_amount; }
  Tags const& tags() const { return m_tags; }
  Tags &tags() { return m_tags; }

  // Map key to optional value
  OptionalTagValue tag_value(std::string const& key) const {
    OptionalTagValue result{};
    if (m_tags.contains(key)) {
      result = m_tags.at(key);
    }
    return result;
  }

  bool operator==(TaggedAmount const& other) const;
  bool operator<(TaggedAmount const& other) const;

  // Replaced with text::format::to_hex_string() / 20251022
  // tagged_amount::to_string ensures it does not override
  // std::to_string(integral type) or any local one
  // static std::string to_string(TaggedAmount::ValueId value_id);

private:
  Date m_date;
  CentsAmount m_cents_amount;
  Tags m_tags;
}; // class TaggedAmount

using TaggedAmounts = std::vector<TaggedAmount>;
using OptionalTaggedAmount = std::optional<TaggedAmount>;
using OptionalTaggedAmounts = std::optional<TaggedAmounts>;

namespace std {
  template <> struct hash<TaggedAmount> {
    std::size_t operator()(TaggedAmount const& ta) const noexcept {

    // Hash combine for TaggedAmount
    // TODO: Consider to consolidate 'hashing' for 'Value Id' to somehow
    //       E.g., Also see hash_combine for Environment...
    auto hash_combine = [](std::size_t &seed, auto const& v) {
      constexpr auto shift_left_count = 1;
      constexpr std::size_t max_size_t = std::numeric_limits<std::size_t>::max();
      constexpr std::size_t mask = max_size_t >> shift_left_count;
      using ValueType = std::remove_cvref_t<decltype(v)>;
      std::hash<ValueType> hasher;
      // Note: I decided to NOT use boost::hash_combine code as it will cause
      // integer overflow and thus undefined behaviour.
      // seed ^= hasher(v) + 0x9e3779b9 + ((seed & mask) <<6) + (seed>>2); //
      // *magic* dustribution as defined by boost::hash_combine
      seed ^= (hasher(v) & mask)
              << shift_left_count; // Simple shift left distribution and no addition
    };

      std::size_t result{};
      auto yyyymmdd = ta.date();
      hash_combine(result, static_cast<int>(yyyymmdd.year()));
      hash_combine(result, static_cast<unsigned>(yyyymmdd.month()));
      hash_combine(result, static_cast<unsigned>(yyyymmdd.day()));
      hash_combine(result, ta.cents_amount());
      for (auto const& [key, value] : ta.tags()) {

        // How the value is linked is not part of the 'value'
        // we care about.
        // In this way an SIE created by cratchit and SIE imported from        
        // sie-file will be 'the same' as long as it defines the same 
        // amount, date and tagging (including aggregation)
        // Note: So _members aggregation *is* part of the value!
        //       We can't just milli vanilly exclude any _xxx meta value...
        if (key == "_prev") continue;

        hash_combine(result, key);
        hash_combine(result, value);
      }
      return result;
    }
  };
} // namespace std

TaggedAmount::ValueId to_value_id(TaggedAmount const& ta);
std::ostream& operator<<(std::ostream &os, TaggedAmount const& ta);
TaggedAmount::OptionalValueId to_maybe_value_id(std::string const& sid);
TaggedAmount::OptionalValueIds to_maybe_value_ids(Key::Sequence const& sids);

// String conversion
std::string to_string(TaggedAmount const& ta);

// Environment conversions
Environment::Value to_environment_value(TaggedAmount const& ta);
OptionalTaggedAmount to_tagged_amount(Environment::Value const& ev);

// TaggedAmounts -> Id-Value pairs (map Environment value-Id to Environment Value)
inline auto id_value_pairs_from(TaggedAmounts const& tagged_amounts) {
  return tagged_amounts | std::views::transform(
    [](TaggedAmount const& ta) -> Environment::MutableIdValuePair {
      return {to_value_id(ta), to_environment_value(ta)};
    });
}

class TaggedAmountHasher {
public:
  TaggedAmount::ValueId operator()(TaggedAmount const& ta) const;
private:
};

namespace zeroth {

  // using TaggedAmountsCasRepository = cas::repository<TaggedAmount::ValueId, TaggedAmount>;
  using TaggedAmountsCasRepository = cas::repository<TaggedAmount::ValueId, TaggedAmount,TaggedAmountHasher>;

  // Behaves more or less as a vector of tagged amounts in date order.
  // But uses a map <Key,Value> as the mechanism to look up a value based on its
  // value_id. 
  // As of 240622 the behind-the-scenes map is a CAS (Content Addressable
  // Storage) with a key being a hash of its 'value'
  class DateOrderedTaggedAmountsContainer {
  public:
    using OptionalTagValue = TaggedAmount::OptionalTagValue;
    using Tags = TaggedAmount::Tags;
    using ValueId = TaggedAmount::ValueId;
    using OptionalValueId = TaggedAmount::OptionalValueId;
    using ValueIds = TaggedAmount::ValueIds;
    using OptionalValueIds = TaggedAmount::OptionalValueIds;
    using const_iterator = TaggedAmounts::const_iterator;
    // using const_subrange = std::ranges::subrange<const_iterator, const_iterator>;

    // Container
    /**
      * Insert provided value in date order.
      * If the insert location is between two other values, then 
      * the sequnce after the new value is re-linked.
      */
    std::pair<DateOrderedTaggedAmountsContainer::ValueId,bool> dotas_insert_auto_ordered_value(TaggedAmount const& ta);

    /**
      * Append provided value to provided prev.
      * Will fail if date ordering does not hold between prev and value
      */
    std::pair<DateOrderedTaggedAmountsContainer::ValueId,bool> dotas_append_value(
       DateOrderedTaggedAmountsContainer::OptionalValueId maybe_prev
      ,TaggedAmount const& ta
      ,bool auto_order_compability_mode = false);

    DateOrderedTaggedAmountsContainer& dotas_insert_auto_ordered_container(DateOrderedTaggedAmountsContainer const& other);
    DateOrderedTaggedAmountsContainer& dotas_insert_auto_ordered_sequence(TaggedAmounts const& tas);

    // Accessors
    bool contains(TaggedAmount const& ta) const;
    OptionalTaggedAmount at(ValueId const& value_id) const;
    TaggedAmountsCasRepository& cas();
    TaggedAmountsCasRepository const& cas() const;

    // Sequence
    std::size_t sequence_size() const;

    auto ordered_ids_view() const {
      return std::views::all(m_date_ordered_value_ids);
    }

    OptionalTaggedAmounts to_tagged_amounts(ValueIds const& value_ids) const;

    TaggedAmounts ordered_tagged_amounts() const;
    TaggedAmounts date_range_tagged_amounts(zeroth::DateRange const& date_period) const;

    // Mutation
    DateOrderedTaggedAmountsContainer& erase(ValueId const& value_id);
    DateOrderedTaggedAmountsContainer& reset(DateOrderedTaggedAmountsContainer const& other);
    DateOrderedTaggedAmountsContainer& reset(TaggedAmounts const& tas);
    DateOrderedTaggedAmountsContainer& clear();

  private:

    // Note: Each tagged amount is stored twice. Once in a
    // mapping between value_id and tagged amount and once in an iteratable sequence
    // ordered by date.
    TaggedAmountsCasRepository m_tagged_amount_cas_repository{};  // map <instance id> -> <tagged amount>
                                                                  // as content addressable storage
                                                                  // repository
    // TaggedAmounts m_dotas{}; // vector of tagged amount ordered by date
    ValueIds m_date_ordered_value_ids{};

    using PrevNextPair = std::pair<OptionalValueId,OptionalValueId>;

    // Encode _prev into provided ta as defined by provided prev arg
    static TaggedAmount to_linked_encoded_ta(OptionalValueId maybe_prev,TaggedAmount const& ta);

    auto ordered_id_value_pairs_view() const {
      return 
          ordered_ids_view()
        | std::views::transform([this](ValueId id)  {
            auto const& maybe = m_tagged_amount_cas_repository.cas_repository_get(id);
            // return id + value copy in optional (safe for now)
            return std::pair{id, maybe.value()}; // assumes present (will throw if not)
          });
    }

    auto ordered_tas_view() const {
      return
          ordered_id_value_pairs_view()
        | std::views::transform([this](auto const& pair) {
            auto const& maybe_ta = this->m_tagged_amount_cas_repository.cas_repository_get(pair.first);
            // return value copy in optional (safe for now)
            return maybe_ta.value();
          });
    }

    auto date_range_tas_view(zeroth::DateRange const& date_period) const {
      auto view = ordered_tas_view()
        | std::views::drop_while([&](auto const& ta) {
            return ta.date() < date_period.begin();
        })
        | std::views::take_while([&](auto const& ta) {
            return ta.date() <= date_period.end();
        });
      return view;
    }

    PrevNextPair to_prev_and_next(TaggedAmount const& ta);

    OptionalValueId to_prev(TaggedAmount const& ta);

    // std::pair<OptionalValueId,TaggedAmount> to_prev_and_transformed_ta(TaggedAmount const& ta);

    std::pair<PrevNextPair,TaggedAmount> to_prev_next_pair_and_transformed_ta(TaggedAmount const& ta);

  }; // class DateOrderedTaggedAmountsContainer
}

using DateOrderedTaggedAmountsContainer = zeroth::DateOrderedTaggedAmountsContainer;

// namespace BAS with 'forwards' now in BAS.hpp

// Date ordrered tagged amounts from environment
DateOrderedTaggedAmountsContainer dotas_from_environment(const Environment &env);

// Environment -> TaggedAmounts (filtered by fiscal period)
DateOrderedTaggedAmountsContainer to_period_dotas_container(FiscalPeriod period, const Environment &env);

namespace tas {
  // namespace for processing that produces tagged amounts

  // Generic for parsing a range or container of tagged amount pointers into a
  // vector of saldo tagged amounts (tagged with 'BAS' for each accumulated bas
  // account)
  TaggedAmounts to_bas_omslutning(auto ordered_tas_view) {

    logger::cout_proxy << "\nto_bas_omslutning";
    TaggedAmounts result{};
    using BASBuckets = std::map<BAS::AccountNo, TaggedAmounts>;
    BASBuckets bas_buckets{};

    auto is_valid_bas_account_transaction = [](TaggedAmount const& ta) {
      if (ta.tags().contains("BAS") and
          !(BAS::to_account_no(ta.tags().at("BAS")))) {
        // Whine about invalid tagging of 'BAS' tag!
        // It is vital we do NOT have any badly tagged BAS account transactions
        // as this will screw up the saldo calculation!
        logger::cout_proxy << "\nDESIGN_INSUFFICIENCY: tas::to_bas_omslutning failed to "
                     "create a valid BAS account no from tag 'BAS' with value "
                  << std::quoted(ta.tags().at("BAS"));
        return false;
      } else
        return (     (ta.tags().contains("BAS")) 
                 and (BAS::to_account_no(ta.tags().at("BAS")))
                 and (ta.tags().contains("IB") == false)
                 and (    ((ta.tags().contains("type") == false)) 
                       or (     (ta.tags().contains("type") == true) 
                            and (ta.tags().at("type") != "saldo"))));
    };

    for (auto const& ta : ordered_tas_view) {
      if (is_valid_bas_account_transaction(ta)) {
        bas_buckets[*BAS::to_account_no(ta.tags().at("BAS"))].push_back(ta);
      }
    }

    for (auto const& [bas_account_no, tas] : bas_buckets) {
      Date period_end_date{};
      logger::cout_proxy << "\n" << std::dec << bas_account_no;
      auto cents_saldo = std::accumulate(
          tas.begin(), tas.end(), CentsAmount{0},
          [&period_end_date](auto acc, auto const& ta) {
            period_end_date =
                std::max(period_end_date,
                         ta.date()); // Ensure we keep the latest date. NOTE: We
                                     // expect they are in growing date order.
                                     // But just in case...
            acc += ta.cents_amount();
            logger::cout_proxy << "\n\t" << period_end_date << " "
                      << to_string(to_units_and_cents(ta.cents_amount()))
                      << " ackumulerat:" << to_string(to_units_and_cents(acc));
            return acc;
          });

      TaggedAmount saldo_ta{period_end_date, cents_saldo};
      saldo_ta.tags()["BAS"] = std::to_string(bas_account_no);
      saldo_ta.tags()["type"] = "saldo";
      result.push_back(saldo_ta);
    }
    std::ranges::sort(result,[](auto const& lhs,auto const& rhs){
      return (lhs.tag_value("BAS").value() < rhs.tag_value("BAS").value());
    });
    return result;
  }
          
} // namespace tas

using OptionalDateOrderedTaggedAmounts = std::optional<DateOrderedTaggedAmountsContainer>;

// upstream (lift) projections
namespace CSV {
	namespace NORDEA {
		OptionalTaggedAmount to_tagged_amount(FieldRow const& field_row, Table::Heading const& heading = Table::Heading{}); 
	} // namespace NORDEA 

	namespace SKV {
		OptionalTaggedAmount to_tagged_amount(FieldRow const& field_row, Table::Heading const& heading = Table::Heading{});
	} // namespace SKV
} // namespace CSV

namespace CSV {
  namespace project {
    using ToTaggedAmountProjection = std::function<OptionalTaggedAmount(CSV::FieldRow const& field_row)>;
    ToTaggedAmountProjection make_tagged_amount_projection(
      HeadingId const& csv_heading_id
      ,CSV::TableHeading const& table_heading);
    OptionalTaggedAmounts to_tas(CSV::project::HeadingId const& csv_heading_id, CSV::OptionalTable const& maybe_csv_table);
  }
}

OptionalTaggedAmounts tas_from_statment_file(std::filesystem::path const& statement_file_path);
