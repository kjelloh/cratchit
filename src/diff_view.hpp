#pragma once

// A non owning diff of period sliced ranges = lhs - rhs
template <typename T, typename DateProj, typename ValueProj>
class diff_view {
public:
  diff_view(
      T const &lhs,
      T const &rhs,
      FiscalPeriod period,
      DateProj date_projection,
      ValueProj value_projection)
      : m_lhs(lhs), m_rhs(rhs),
        m_period(period),
        m_date_projection(date_projection),
        m_value_projection(value_projection) {

    auto in_period = [&](auto const &it) {
      auto date = m_date_projection(*it);
      bool result = m_period.contains(date);
      // spdlog::info("in_period? {} -> {}", to_string(date), result);
      return result;
    };

    spdlog::info("diff_view - lhs size: {}, rhs size: {}, period: {}", m_lhs.size(), m_rhs.size(), m_period.to_string());

    auto contains_ev = [&](auto const r,auto const& ev) {
      return std::ranges::any_of(r, [&](auto const& pair) {
        // spdlog::info("contains_ev - checking: {} == {}", to_string(m_value_projection(pair)), to_string(ev));
        return m_value_projection(pair) == ev;
      });
    };

    for (auto it = std::ranges::begin(m_lhs); it != std::ranges::end(m_lhs); ++it) {
      if (in_period(it) && !contains_ev(m_rhs, m_value_projection(*it))) {
        spdlog::info("diff_view - lhs removed: {}", to_string(m_value_projection(*it)));
        m_removed.insert(it);
      }
    }
    for (auto it = std::ranges::begin(m_rhs); it != std::ranges::end(m_rhs); ++it) {
      if (in_period(it) && !contains_ev(m_lhs, m_value_projection(*it))) {
        spdlog::info("diff_view - rhs inserted: {}", to_string(m_value_projection(*it)));
        m_inserted.insert(it);
      }
    }
  }

  explicit operator bool() const {
    spdlog::info("diff_view::bool(): m_inserted.size() = {} m_removed.size() = {}",m_inserted.size(),m_removed.size());
    return !m_inserted.empty() || !m_removed.empty();
  }

  auto inserted() const {
    return m_inserted | std::views::transform([](auto it) { return *it; });
  }

  auto removed() const {
    return m_removed | std::views::transform([](auto it) { return *it; });
  }

  private:
    T const &m_lhs;
    T const &m_rhs;
    FiscalPeriod m_period;
    DateProj m_date_projection;
    ValueProj m_value_projection;
    std::set<typename T::const_iterator> m_inserted;
    std::set<typename T::const_iterator> m_removed;
  }; // diff_view
