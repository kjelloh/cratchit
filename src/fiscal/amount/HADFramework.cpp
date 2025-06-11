#include "HADFramework.hpp"
#include <algorithm> // std::transform
#include <iterator> // std::back_inserter

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
      return *to_had(ev); // Assume success
    });
  } else {
    // Nop
  }
  return result;
}
