#include "mod10view.hpp"

namespace first {

  // ----------------------------------
  Mod10View::Mod10View(Mod10View::Range const &range)
      :  m_range{range}
        ,m_subrange_size{to_subrange_size(range)} {}

  // ----------------------------------
  std::vector<Mod10View::Range> Mod10View::subranges() {
    std::vector<std::pair<size_t, size_t>> result{};
    if (m_subrange_size > 0) {
      for (size_t i = m_range.first; i < m_range.second; i += m_subrange_size) {
        result.push_back(std::make_pair(i, std::min(i + m_subrange_size, m_range.second)));
      }
    }
    return result;
  }

  size_t Mod10View::to_subrange_size(Mod10View::Range const &range) {
    size_t result{0};
    if (range.second > range.first) {
      const size_t range_size = range.second - range.first;

      result = static_cast<size_t>(
          std::pow(10, std::max(0.0, std::ceil(std::log10(range_size)) - 1)));
    }
    return result;
  }

} // namespace first