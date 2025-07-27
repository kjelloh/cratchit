#pragma once

#include "std_overload.hpp" // poor mans std until compiler supports new features (generator,...)
#include "environment.hpp"
#include "fiscal/BASFramework.hpp"
#include "fiscal/SKVFramework.hpp"
#include <string>
#include <optional>
#include <ranges> // std::views::zip,
#include <algorithm> // std::sort

struct HeadingAmountDateTransEntry {
	struct Optional {
		// std::optional<char> series{};
		std::optional<BAS::AccountNo> gross_account_no{};
		BAS::OptionalMetaEntry current_candidate{};
		std::optional<ToNetVatAccountTransactions> counter_ats_producer{};
		std::optional<SKV::XML::VATReturns::FormBoxMap> vat_returns_form_box_map_candidate{};
	};
	std::string heading{};
	Amount amount{};
	Date date{};
	Optional optional{};
  bool operator==(HeadingAmountDateTransEntry const& other) const;
}; // HeadingAmountDateTransEntry

// Shorthand Alias
using HAD = HeadingAmountDateTransEntry;

std::ostream& operator<<(std::ostream& os,HeadingAmountDateTransEntry const& had);
std::string to_string(HeadingAmountDateTransEntry const& had);

// Date ordering predicate for sorting HADs by date
inline bool date_ordering(HeadingAmountDateTransEntry const& a, HeadingAmountDateTransEntry const& b) {
  return a.date < b.date;
}

using OptionalHeadingAmountDateTransEntry = std::optional<HeadingAmountDateTransEntry>;

// ----------------------------------------------
// --> HAD(s)

// Environment Entry -> HAD
OptionalHeadingAmountDateTransEntry to_had(EnvironmentValue const& ev);

using HeadingAmountDateTransEntries = std::vector<HeadingAmountDateTransEntry>;

// Shorthand Alias
using HADs = HeadingAmountDateTransEntries;

// Environment -> HADs
HeadingAmountDateTransEntries hads_from_environment(Environment const &environment);

inline auto to_period_hads(FiscalPeriod period, const Environment &env) -> HeadingAmountDateTransEntries {
  auto ev_to_maybe_had = [](EnvironmentValue const &ev) -> OptionalHeadingAmountDateTransEntry {
    return to_had(ev);
  };
  auto in_period = [](HeadingAmountDateTransEntry const &had,FiscalPeriod const &period) -> bool {
    return period.contains(had.date);
  };
  auto id_ev_pair_to_ev = [](EnvironmentIdValuePair const &id_ev_pair) {
    return id_ev_pair.second;
  };
  static constexpr auto section = "HeadingAmountDateTransEntry";
  if (!env.contains(section)) {
    return {}; // No entries of this type
  }
  auto const &id_ev_pairs = env.at(section);
  auto result = id_ev_pairs 
    | std::views::transform(id_ev_pair_to_ev) 
    | std::views::transform(ev_to_maybe_had) 
    | std::views::filter([](auto const &maybe_had) { return maybe_had.has_value(); }) 
    | std::views::transform([](auto const &maybe_had) { return *maybe_had; }) 
    | std::views::filter([&](auto const &had) { return in_period(had, period); }) 
    | std::ranges::to<std::vector>();
  
  // Sort by date for consistent ordering
  std::sort(result.begin(), result.end(), date_ordering);
  return result;
}

// Tokens -> HAD
OptionalHeadingAmountDateTransEntry to_had(std::vector<std::string> const& tokens);

// ----------------------------------------------
// <-- HAD(s)

EnvironmentValue to_environment_value(HeadingAmountDateTransEntry const had);

// std_overload::generator<EnvironmentIdValuePair> indexed_env_entries_from(HeadingAmountDateTransEntries const& entries);
inline auto indexed_env_entries_from(HeadingAmountDateTransEntries const& hads) {
  return std::views::zip(
     std::views::iota(0)
    ,hads | std::views::transform(
      [](HeadingAmountDateTransEntry const& had){return to_environment_value(had);}
    ));
}