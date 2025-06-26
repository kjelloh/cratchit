#pragma once

#include "std_overload.hpp" // poor mans std until compiler supports new features (generator,...)
#include "environment.hpp"
#include "fiscal/BASFramework.hpp"
#include "fiscal/SKVFramework.hpp"
#include <string>
#include <optional>
#include <ranges> // std::views::zip,

struct HeadingAmountDateTransEntry {
	struct Optional {
		std::optional<char> series{};
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


std::ostream& operator<<(std::ostream& os,HeadingAmountDateTransEntry const& had);
std::string to_string(HeadingAmountDateTransEntry const& had);

using OptionalHeadingAmountDateTransEntry = std::optional<HeadingAmountDateTransEntry>;

// ----------------------------------------------
// --> HAD(s)

// Environment Entry -> HAD
OptionalHeadingAmountDateTransEntry to_had(EnvironmentValue const& ev);

using HeadingAmountDateTransEntries = std::vector<HeadingAmountDateTransEntry>;

// Environment -> HADs
HeadingAmountDateTransEntries hads_from_environment(Environment const &environment);

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