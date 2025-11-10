#pragma once

#include "TaggedAmountFramework.hpp" // TaggedAmount, Date, CentsAmount
#include "functional/combinators.hpp" // cratchit::functional::xxx
#include <algorithm>
#include <functional>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>
#include <map>

// Now in functional/combinators unit / 20251027
// namespace cratchit {
//     namespace functional {

namespace tafw { // TaggedAmount Framework namespace

  // Predicate combinators for TaggedAmount filtering

  // Predicate on TaggedAmount checking if a tag key equals a value
  inline auto keyEquals(std::string const& key, std::string const& value) {
    return [=](TaggedAmount const& ta) -> bool {
      auto v = ta.tag_value(key);
      return v && (*v == value);
    };
  }

  // Predicate on TaggedAmount checking if a tag key contains a substring
  inline auto keyContains(std::string const& key, std::string const& substring) {
    return [=](TaggedAmount const& ta) -> bool {
      auto v = ta.tag_value(key);
      return v && (v->find(substring) != std::string::npos);
    };
  }

  // Logical combinators for predicates

  inline auto and_(auto const& a, auto const& b) {
    return [=](auto const& x) { return a(x) && b(x); };
  }

  inline auto or_(auto const& a, auto const& b) {
    return [=](auto const& x) { return a(x) || b(x); };
  }

  inline auto not_(auto const& pred) {
    return [=](auto const& x) { return !pred(x); };
  }

  // Date equality predicate
  inline auto dateEquals(Date const& target_date) {
    return [=](TaggedAmount const& ta) {
      return ta.date() == target_date;
    };
  }

  // Extractor helpers for std::ranges algorithms

  // Returns a projection that extracts a tag value (optional string)
  inline auto tagValueExtractor(std::string const& key) {
    return [=](TaggedAmount const& ta) -> std::optional<std::string> {
      return ta.tag_value(key);
    };
  }

  // Returns a projection extracting cents_amount
  inline auto centsAmountExtractor() {
    return [](TaggedAmount const& ta) -> CentsAmount {
      return ta.cents_amount();
    };
  }

  // Returns a projection extracting date
  inline auto dateExtractor() {
    return [](TaggedAmount const& ta) -> Date {
      return ta.date();
    };
  }

  // Sorting helpers

  // Sort a container of TaggedAmount by date ascending
  template <typename Container>
  void sortByDate(Container &c) {
    std::ranges::sort(c, std::less{}, dateExtractor());
  }

  // Sort a container of TaggedAmount by cents_amount ascending
  template <typename Container>
  void sortByAmount(Container &c) {
    std::ranges::sort(c, std::less{}, centsAmountExtractor());
  }

} // namespace tafw
