#include "HADFramework.hpp"
#include <algorithm> // std::transform
#include <iterator> // std::back_inserter
#include <stdexcept> // std::runtime_error
#include <format> // std::format

std::ostream& operator<<(std::ostream& os,HeadingAmountDateTransEntry const& had) {
	if (auto me = had.optional.current_candidate) {
		os << '*';
	}
	else {
		os << ' ';
	}

	if (auto me = had.optional.series) {
		os << *had.optional.series;
	}
	else {
		os << '-';
	}

	if (auto me = had.optional.gross_account_no) {
		os << ' ' << *had.optional.gross_account_no << ' ';
	}
	else {
		os << " nnnn ";
	}

	os << had.heading;
	os << " " << had.amount;
	os << " " << to_string(had.date);
	return os;
}

OptionalHeadingAmountDateTransEntry to_had(EnvironmentValue const& ev) {
	OptionalHeadingAmountDateTransEntry result{};
	HeadingAmountDateTransEntry had{};
	while (true) {
		if (auto iter = ev.find("rubrik");iter != ev.end()) had.heading = iter->second;
		else break;
		if (auto iter = ev.find("belopp");iter != ev.end()) had.amount = *to_amount(iter->second); // assume success
		else break;
		if (auto iter = ev.find("datum");iter != ev.end()) had.date = *to_date(iter->second); // assume success
		else break;
		result = had;
		break;
	}
	return result;
}

HeadingAmountDateTransEntries hads_from_environment(Environment const &environment) {
  HeadingAmountDateTransEntries result{};
  // auto [begin,end] = environment.equal_range("HeadingAmountDateTransEntry");
  // std::transform(begin,end,std::back_inserter(result),[](auto const& entry){
  // 	return *to_had(entry.second); // Assume success
  // });
  if (environment.contains("HeadingAmountDateTransEntry")) {
    auto const id_ev_pairs = environment.at("HeadingAmountDateTransEntry");
    std::transform(id_ev_pairs.begin(), id_ev_pairs.end(), std::back_inserter(result), [](auto const &id_ev_pair) {
      auto const &[id, ev] = id_ev_pair;
      if (auto ohad = to_had(ev); !ohad) {
        throw std::runtime_error(std::format("Invalid HeadingAmountDateTransEntry in environment: {}", id));
      }
      return *to_had(ev); // Assume success
    });
  } else {
    // Nop
  }
  return result;
}
