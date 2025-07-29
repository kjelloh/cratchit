#include "HADFramework.hpp"
#include "logger/log.hpp"
#include <algorithm> // std::transform
#include <iterator> // std::back_inserter
#include <stdexcept> // std::runtime_error
#include <format> // std::format
#include <tuple> // std::tie

bool HeadingAmountDateTransEntry::operator==(HeadingAmountDateTransEntry const& other) const {
  // spdlog::info("NOTE: HeadingAmountDateTransEntry::operator== compares only heading, amount and date (Options ignored)");
  return std::tie(heading,amount,date) == std::tie(other.heading,other.amount,other.date);
}

std::ostream& operator<<(std::ostream& os,HeadingAmountDateTransEntry const& had) {
	if (auto me = had.optional.current_candidate) {
		os << '*';
	}
	else {
		os << ' ';
	}

	// if (auto me = had.optional.series) {
	// 	os << *had.optional.series;
	// }
	// else {
	// 	os << '-';
	// }

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

std::string to_string(HeadingAmountDateTransEntry const& had) {
  std::ostringstream oss{};
  oss << had;
  return oss.str();
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
  if (true) {
    spdlog::info("BEGIN hads_from_environment");
    spdlog::default_logger()->flush();;
  }
  HeadingAmountDateTransEntries result{};
  if (environment.contains("HeadingAmountDateTransEntry")) {
    if (true) {
      spdlog::info(R"(if (environment.contains("HeadingAmountDateTransEntry")) ok)");
      spdlog::default_logger()->flush();;
    }
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

OptionalHeadingAmountDateTransEntry to_had(std::vector<std::string> const& tokens) {
  if (tokens.size()==3) {
    HeadingAmountDateTransEntry had {
      .heading = tokens[0]
      ,.amount = *to_amount(tokens[1]) // Assume success
      ,.date = *to_date(tokens[2]) // Assume success
    };
    return had;
  }
  return std::nullopt;
}

// ----------------------------------------------
// <-- HAD(s)

EnvironmentValue to_environment_value(HeadingAmountDateTransEntry const had) {
	// std::cout << "\nto_environment_value: had.amount" << had.amount << " had.date" << had.date;
	std::ostringstream os{};
	os << had.amount;
	EnvironmentValue ev{};
	ev["rubrik"] = had.heading;
	ev["belopp"] = os.str();
	ev["datum"] = to_string(had.date);
	return ev;
}

// std_overload::generator<EnvironmentIdValuePair> indexed_env_entries_from(HeadingAmountDateTransEntries const& entries) {
//     size_t index = 0;
//     for (const auto& entry : entries) {
//         co_yield {index++, to_environment_value(entry)};
//     }
// }
