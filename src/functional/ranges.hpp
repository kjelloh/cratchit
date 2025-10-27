#pragma once

#include <ranges>

namespace cratchit {
  namespace functional {
    namespace ranges {
      template <std::ranges::range R>
      auto adjacent_value_pairs(R&& r) {
          return std::views::zip(
              std::forward<R>(r) | std::views::take(r.size() - 1),
              std::forward<R>(r) | std::views::drop(1)
          );
      }

      template<std::input_or_output_iterator I, std::sentinel_for<I> S>
      auto adjacent_iterator_pairs(I first, S last) {
          // iota(first,last) produces a view of iterators [first,last)
          auto iters = std::views::iota(first, last);
          return std::views::zip(iters, iters | std::views::drop(1));
      }

    } // ranges
  } // functional
} // cratchit
