#pragma once

#include <ranges>

namespace cratchit {
  namespace functional {
    namespace ranges {
      template <std::ranges::range R>
      auto adjacent_pairs(R&& r) {
          return std::views::zip(
              std::forward<R>(r) | std::views::take(r.size() - 1),
              std::forward<R>(r) | std::views::drop(1)
          );
      }
    } // ranges
  } // functional
} // cratchit
