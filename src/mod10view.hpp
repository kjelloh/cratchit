#pragma once

#include <utility> // std::pair,
#include <vector> // std::vector,

namespace first {

  struct Mod10View {
    using Range = std::pair<size_t, size_t>;
    Range m_range;
    size_t m_subrange_size;

    Mod10View(Range const &range);

    template <class T> Mod10View(T const &container);

    std::vector<Range> subranges();
  }; // struct Mod10View

  // ----------------------------------
  template <class T>
  Mod10View::Mod10View(T const &container)
      : Mod10View(Range(0, container.size())) {}

} // namespace first