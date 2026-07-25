#pragma once

#include <ranges>

namespace until_std {

  namespace views {
    auto enumerate = [](auto && range) {
      return std::views::zip(
        std::views::iota(0)
        ,std::forward<decltype(range)>(range));
    };
  } // views

} // until_std