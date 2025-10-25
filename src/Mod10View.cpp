#include "Mod10View.hpp"
#include <cmath>

namespace first {

  // ----------------------------------
  Mod10View::Mod10View(Mod10View::Range const& range)
      :  m_range{range}
        ,m_subrange_size{to_subrange_size(range)} {
    validate_range();
  }

  // ----------------------------------
  std::vector<Mod10View::Range> Mod10View::subranges() const {
    std::vector<std::pair<size_t, size_t>> result{};
    if (m_subrange_size > 0) {
      for (size_t i = m_range.first; i < m_range.second; i += m_subrange_size) {
        result.push_back(std::make_pair(i, std::min(i + m_subrange_size, m_range.second)));
      }
    }
    return result;
  }

  std::optional<Mod10View::Range> Mod10View::digit_range(char digit) const {
    if (!is_valid_digit(digit)) {
      return std::nullopt;
    }
    
    auto ranges = subranges();
    size_t digit_index = static_cast<size_t>(digit - '0');
    
    if (digit_index < ranges.size()) {
      return ranges[digit_index];
    }
    
    return std::nullopt;
  }

  bool Mod10View::is_valid_digit(char digit) const {
    if (digit < '0' || digit > '9') {
      return false;
    }
    
    size_t digit_index = static_cast<size_t>(digit - '0');
    return digit_index < subranges().size();
  }

  Mod10View Mod10View::drill_down(char digit) const {
    auto range_opt = digit_range(digit);
    if (!range_opt) {
      throw std::invalid_argument(std::format("Invalid digit '{}' for view {}", digit, to_string()));
    }
    
    return Mod10View(*range_opt);
  }

  std::optional<size_t> Mod10View::direct_index(char digit) const {
    auto range_opt = digit_range(digit);
    if (!range_opt) {
      return std::nullopt;
    }
    
    auto range = *range_opt;
    if (range.second - range.first == 1) {
      return range.first;
    }
    
    return std::nullopt; // Not a single item
  }

  std::string Mod10View::to_string() const {
    return std::format("Mod10View[{}, {}) size={}", m_range.first, m_range.second, size());
  }

  // Static factory methods
  Mod10View Mod10View::from_range(size_t first, size_t last) {
    return Mod10View(Range(first, last));
  }

  Mod10View Mod10View::empty_view() {
    return Mod10View(Range(0, 0));
  }

  Mod10View Mod10View::single_item(size_t index) {
    return Mod10View(Range(index, index + 1));
  }

  size_t Mod10View::to_subrange_size(Mod10View::Range const& range) {
    size_t result{0};
    if (range.second > range.first) {
      const size_t range_size = range.second - range.first;

      result = static_cast<size_t>(
          std::pow(10, std::max(0.0, std::ceil(std::log10(range_size)) - 1)));
    }
    return result;
  }

  void Mod10View::validate_range() const {
    if (m_range.first > m_range.second) {
      throw std::invalid_argument(std::format("Invalid range: first ({}) > second ({})", 
                                             m_range.first, m_range.second));
    }
  }

} // namespace first