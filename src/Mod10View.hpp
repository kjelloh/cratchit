#pragma once

#include <utility> // std::pair,
#include <vector>  // std::vector,
#include <optional>
#include <string>
#include <format>
#include <stdexcept>

namespace first {

  class Mod10View {
  public:
    using Range = std::pair<size_t, size_t>;

    Mod10View(Range const &range);

    template <class T>
    Mod10View(T const &container);

    // Encapsulated access to internal state
    const Range& range() const { return m_range; }
    size_t subrange_size() const { return m_subrange_size; }
    size_t first() const { return m_range.first; }
    size_t second() const { return m_range.second; }
    size_t size() const { return m_range.second - m_range.first; }
    bool empty() const { return size() == 0; }

    // Iterator interface for range-based loops
    class iterator {
    public:
      iterator(size_t value) : m_value(value) {}
      size_t operator*() const { return m_value; }
      iterator& operator++() { ++m_value; return *this; }
      bool operator!=(const iterator& other) const { return m_value != other.m_value; }
    private:
      size_t m_value;
    };
    
    iterator begin() const { return iterator(m_range.first); }
    iterator end() const { return iterator(m_range.second); }

    // Semantic navigation methods
    std::vector<Range> subranges() const;
    std::optional<Range> digit_range(char digit) const;
    bool is_single_item() const { return size() == 1; }
    bool needs_pagination() const { return size() > 10; }
    bool is_valid_digit(char digit) const;

    // Navigation helpers
    Mod10View drill_down(char digit) const;
    std::optional<size_t> direct_index(char digit) const;

    // Debugging & introspection
    std::string to_string() const;

    // Static factory methods
    template <class T>
    static Mod10View from_container(T const &container); 
       
    static Mod10View from_range(size_t first, size_t last);
    static Mod10View empty_view();
    static Mod10View single_item(size_t index);

  private:
    Range m_range;
    size_t m_subrange_size;

    size_t to_subrange_size(Mod10View::Range const &range);
    void validate_range() const;
  }; // class Mod10View

  // ----------------------------------
  template <class T>
  Mod10View::Mod10View(T const &container)
      : Mod10View(Range(0, container.size())) {}

  template <class T>
  Mod10View Mod10View::from_container(T const &container) {
      return Mod10View(container);
  }

} // namespace first