#pragma once

#include "TaggedAmountFramework.hpp" // TaggedAmount, Date, CentsAmount
#include <algorithm>
#include <functional>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>

// map, fold, scan, key, stencil, filter, reduce, etc.

namespace cratchit {
    namespace functional {
        // 1. map
        // Transform a value (e.g., convert currencies, apply tax).
        // Example: Change all amounts to Euros.
        inline constexpr auto map = [](auto&& func) {
            return std::views::transform(std::forward<decltype(func)>(func));
        };

        // 2. fold (reduce)
        // Aggregate data (e.g., sum totals, compute balances).
        // Example: Sum all amounts with tag "food".
        inline constexpr auto fold = [](auto init, auto&& func) {
            return [=, f = std::forward<decltype(func)>(func)](auto&& range) {
                return std::accumulate(std::ranges::begin(range), std::ranges::end(range), init, f);
            };
        };

        // 3. scan
        // Like fold, but keeps history — perfect for running totals or cumulative balances.
        // Example: Monthly balance progression.

        // 4. key
        // Critical for grouping operations — think of it like SQL’s GROUP BY.
        // Example: Group amounts by tag or by month.

        // 5. stencil
        // Usually used in numerical computations (e.g., neighborhoods), but in bookkeeping could model temporal patterns.
        // Possible use: Detect "outlier" days or smoothing transactions (e.g., moving average).

        // 6. filter
        // Select based on predicates (e.g., tag, date, amount thresholds).
        // Example: Only expenses over $100.

        // 7. partition
        // Split a list into two based on a predicate.
        // Example: Separate income and expenses.
        // auto [expenses, income] = partition([](auto t) { return t.amount < 0; }, taggedAmounts);

        // 8. groupBy (or bucketBy)
        // Stronger form of key, where you actually group values into maps/lists.
        // Example: Group all transactions by month or by category.
        // map<string, vector<TaggedAmount>> grouped = groupBy([](auto t) { return t.tag; }, transactions);

        // 9. sortBy
        // Sorting is often needed by date, amount, etc.
        // Can be combined with groupBy for reporting.

        // 10. zip / zipWith
        // Combine two lists element-wise.
        // Useful if merging two accounts, aligning monthly budget vs actual spend.


    } // namespace functional
} // namespace cratchit
 // }

namespace tafw { // TaggedAmount Framework namespace

  // Predicate combinators for TaggedAmount filtering

  // Predicate on TaggedAmount checking if a tag key equals a value
  inline auto keyEquals(std::string const &key, std::string const &value) {
    return [=](TaggedAmount const &ta) -> bool {
      auto v = ta.tag_value(key);
      return v && (*v == value);
    };
  }

  // Predicate on TaggedAmount checking if a tag key contains a substring
  inline auto keyContains(std::string const &key, std::string const &substring) {
    return [=](TaggedAmount const &ta) -> bool {
      auto v = ta.tag_value(key);
      return v && (v->find(substring) != std::string::npos);
    };
  }

  // Logical combinators for predicates

  inline auto and_(auto const &a, auto const &b) {
    return [=](auto const &x) { return a(x) && b(x); };
  }

  inline auto or_(auto const &a, auto const &b) {
    return [=](auto const &x) { return a(x) || b(x); };
  }

  inline auto not_(auto const &pred) {
    return [=](auto const &x) { return !pred(x); };
  }

  // Date equality predicate
  inline auto dateEquals(Date const &target_date) {
    return [=](TaggedAmount const &ta) {
      return ta.date() == target_date;
    };
  }

  // Extractor helpers for std::ranges algorithms

  // Returns a projection that extracts a tag value (optional string)
  inline auto tagValueExtractor(std::string const &key) {
    return [=](TaggedAmount const &ta) -> std::optional<std::string> {
      return ta.tag_value(key);
    };
  }

  // Returns a projection extracting cents_amount
  inline auto centsAmountExtractor() {
    return [](TaggedAmount const &ta) -> CentsAmount {
      return ta.cents_amount();
    };
  }

  // Returns a projection extracting date
  inline auto dateExtractor() {
    return [](TaggedAmount const &ta) -> Date {
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
