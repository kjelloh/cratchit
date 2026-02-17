#pragma once

#include "AmountFramework.hpp"
#include "FiscalPeriod.hpp"
#include <ostream>
#include <map>

class TaggedAmount {
public:
  friend std::ostream &operator<<(std::ostream &os, TaggedAmount const& ta);
  using OptionalTagValue = std::optional<std::string>;
  using Tags = std::map<std::string, std::string>;
  using ValueId = std::size_t;
  using OptionalValueId = std::optional<ValueId>;
  using ValueIds = std::vector<ValueId>;
  using OptionalValueIds = std::optional<ValueIds>;

  // TaggedAmount(Date const& date, CentsAmount const& cents_amount,Tags &&tags = Tags{});
  TaggedAmount(Date date, CentsAmount cents_amount,Tags tags = Tags{});

  // Getters
  Date const& date() const { return m_date; }
  CentsAmount const& cents_amount() const { return m_cents_amount; }
  Tags const& tags() const { return m_tags; }
  Tags &tags() { return m_tags; }

  // Map key to optional value
  OptionalTagValue tag_value(std::string const& key) const {
    OptionalTagValue result{};
    if (m_tags.contains(key)) {
      result = m_tags.at(key);
    }
    return result;
  }

  bool operator==(TaggedAmount const& other) const;
  bool operator<(TaggedAmount const& other) const;

  // Replaced with text::format::to_hex_string() / 20251022
  // tagged_amount::to_string ensures it does not override
  // std::to_string(integral type) or any local one
  // static std::string to_string(TaggedAmount::ValueId value_id);

private:
  Date m_date;
  CentsAmount m_cents_amount;
  Tags m_tags;
}; // class TaggedAmount

using TaggedAmounts = std::vector<TaggedAmount>;
using OptionalTaggedAmount = std::optional<TaggedAmount>;
using OptionalTaggedAmounts = std::optional<TaggedAmounts>;

// String conversion
std::ostream& operator<<(std::ostream &os, TaggedAmount const& ta);
std::string to_string(TaggedAmount const& ta);
