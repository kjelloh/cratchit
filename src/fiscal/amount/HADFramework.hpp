#pragma once

#include "environment.hpp"
#include "fiscal/BASFramework.hpp"
#include "fiscal/SKVFramework.hpp"
#include <string>
#include <optional>

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
};
std::ostream& operator<<(std::ostream& os,HeadingAmountDateTransEntry const& had);
std::string to_string(HeadingAmountDateTransEntry const& had);

using OptionalHeadingAmountDateTransEntry = std::optional<HeadingAmountDateTransEntry>;

// Environment Entry -> HAD
OptionalHeadingAmountDateTransEntry to_had(EnvironmentValue const& ev);

using HeadingAmountDateTransEntries = std::vector<HeadingAmountDateTransEntry>;

// Environment -> HADs
HeadingAmountDateTransEntries hads_from_environment(Environment const &environment);

// Tokens -> HAD
OptionalHeadingAmountDateTransEntry to_had(std::vector<std::string> const& tokens);
