#pragma once

#include "TaggedAmountFramework.hpp" // TaggedAmount, Date, CentsAmount
#include <algorithm>
#include <functional>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>
#include <map>

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
        namespace detail {
            template <typename Range, typename T, typename BinaryOp>
            auto scan_impl(Range&& input, T init, BinaryOp op) {
                using ValueType = std::remove_cvref_t<T>;
                std::vector<ValueType> results;
                results.reserve(std::ranges::size(input) + 1);

                ValueType acc = init;
                results.push_back(acc);

                for (auto&& elem : input) {
                    acc = op(acc, elem);
                    results.push_back(acc);
                }
                return results;
            }
        }

        inline constexpr auto scan = [](auto init, auto&& func) {
            return [=, f = std::forward<decltype(func)>(func)](auto&& range) {
                return detail::scan_impl(std::forward<decltype(range)>(range), init, f);
            };
        };

        // 4. key
        // Critical for grouping operations — think of it like SQL’s GROUP BY.
        // Example: Group amounts by tag or by month.
        template <
            typename Range,
            typename KeyFunc,
            typename MapType = std::map<
                std::invoke_result_t<KeyFunc, std::ranges::range_reference_t<Range>>,
                std::vector<std::ranges::range_value_t<Range>>
            >
        >
        MapType key(Range&& range, KeyFunc keyFunc) {
            MapType groups;

            for (auto&& elem : range) {
                auto k = keyFunc(elem);
                groups[k].push_back(std::forward<decltype(elem)>(elem));
            }

            return groups;
        }


        // 5. stencil
        // Usually used in numerical computations (e.g., neighborhoods), but in bookkeeping could model temporal patterns.
        // Possible use: Detect "outlier" days or smoothing transactions (e.g., moving average).
        inline constexpr auto stencil = [](std::size_t window_size, auto&& func) {
            return [=, f = std::forward<decltype(func)>(func)](auto&& range) {
                using std::ranges::begin, std::ranges::end;

                auto first = begin(range);
                auto last = end(range);
                std::vector<std::ranges::range_value_t<decltype(range)>> result;

                const std::size_t n = std::ranges::distance(first, last);
                if (window_size == 0 || n < window_size)
                    return result;  // empty output

                for (std::size_t i = 0; i <= n - window_size; ++i) {
                    auto win_first = std::next(first, i);
                    auto win_last = std::next(first, i + window_size);
                    result.push_back(f(std::ranges::subrange(win_first, win_last)));
                }

                return result;
            };
        };


        // 6. filter
        // Select based on predicates (e.g., tag, date, amount thresholds).
        // Example: Only expenses over $100.
        inline constexpr auto filter = [](auto&& pred) {
            return std::views::filter(std::forward<decltype(pred)>(pred));
        };

        // 7. partition
        // Split a list into two based on a predicate.
        // Example: Separate income and expenses.
        // auto [expenses, income] = partition([](auto t) { return t.amount < 0; }, taggedAmounts);
        inline constexpr auto partition = [](auto&& pred) {
            return [=, p = std::forward<decltype(pred)>(pred)](auto&& range) {
                using ValueType = std::ranges::range_value_t<std::remove_reference_t<decltype(range)>>;
                std::vector<ValueType> true_partition;
                std::vector<ValueType> false_partition;

                for (auto&& elem : range) {
                    if (p(elem)) {
                        true_partition.push_back(std::forward<decltype(elem)>(elem));
                    } else {
                        false_partition.push_back(std::forward<decltype(elem)>(elem));
                    }
                }

                return std::pair{std::move(true_partition), std::move(false_partition)};
            };
        };

        // 8. groupBy (or bucketBy)
        // Stronger form of key, where you actually group values into maps/lists.
        // Example: Group all transactions by month or by category.
        // map<string, vector<TaggedAmount>> grouped = groupBy([](auto t) { return t.tag; }, transactions);
        inline constexpr auto groupBy = [](auto&& keyFunc) {
            return [=, f = std::forward<decltype(keyFunc)>(keyFunc)](auto&& range) {
                return key(std::forward<decltype(range)>(range), f);
            };
        };

        // 9. sortBy
        // Sorting is often needed by date, amount, etc.
        // Can be combined with groupBy for reporting.
        inline constexpr auto sortBy = [](auto&& keyFunc) {
            return [=, f = std::forward<decltype(keyFunc)>(keyFunc)](auto&& range) {
                using ValueType = std::ranges::range_value_t<std::remove_reference_t<decltype(range)>>;

                std::vector<ValueType> result(std::ranges::begin(range), std::ranges::end(range));
                std::ranges::sort(result, std::less<>{}, f);
                return result;
            };
        };

        // 10. zip / zipWith
        // Combine two lists element-wise.
        // Useful if merging two accounts, aligning monthly budget vs actual spend.
        inline constexpr auto zip = [](auto&& range1, auto&& range2) {
            using std::ranges::begin, std::ranges::end;
            auto it1 = begin(range1), end1 = end(range1);
            auto it2 = begin(range2), end2 = end(range2);

            using T1 = std::remove_cvref_t<decltype(*it1)>;
            using T2 = std::remove_cvref_t<decltype(*it2)>;

            std::vector<std::pair<T1, T2>> result;

            for (; it1 != end1 && it2 != end2; ++it1, ++it2) {
                result.emplace_back(*it1, *it2);
            }

            return result;
        };

        namespace detail {
            // Helper to dereference all iterators in a tuple into a tuple of references
            template <typename Tuple, std::size_t... I>
            auto deref_tuple_impl(Tuple& t, std::index_sequence<I...>) {
                return std::make_tuple((*std::get<I>(t))...);
            }

            template <typename Tuple>
            auto deref_tuple(Tuple& t) {
                return deref_tuple_impl(t, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
            }

            // Helper to increment all iterators in a tuple
            template <typename Tuple, std::size_t... I>
            void increment_tuple_impl(Tuple& t, std::index_sequence<I...>) {
                (..., ++std::get<I>(t));
            }

            template <typename Tuple>
            void increment_tuple(Tuple& t) {
                increment_tuple_impl(t, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
            }

        } // namespace detail

        inline constexpr auto nzip = [](auto&&... ranges) {
            using std::begin;
            using std::end;

            // Compute minimum size among input ranges
            std::size_t min_size = std::min({std::ranges::size(ranges)...});

            using ValueType = std::tuple<std::ranges::range_value_t<std::remove_reference_t<decltype(ranges)>>...>;
            std::vector<ValueType> result;
            result.reserve(min_size);

            auto its = std::make_tuple(begin(ranges)...);

            for (std::size_t i = 0; i < min_size; ++i) {
                result.push_back(detail::deref_tuple(its));
                detail::increment_tuple(its);
            }

            return result;
        };

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
