#include "TaggedAmount.hpp" 

// BEGIN class TaggedAmount

// TaggedAmount::TaggedAmount(Date const& date, CentsAmount const& cents_amount, Tags&& tags)
//     : m_date{date},m_cents_amount{cents_amount}, m_tags{tags} {}

// lvalue/rvalue constructor
// An lvalue will be copied + moved.
// An rvalue will be 'perfactly' moved
TaggedAmount::TaggedAmount(Date date, CentsAmount cents_amount,Tags tags)
    : m_date{std::move(date)},m_cents_amount{std::move(cents_amount)}, m_tags{std::move(tags)} {}

// Replaced with text::format::to_hex_string
// std::string TaggedAmount::to_string(TaggedAmount::ValueId value_id) {
//   // TODO: Consider a safe way to ensure ALL value ids gets converted to the same string
//   //       to ernsure consistent environment value encoding / 20251021
//   //       Also see to_cas_environment(indexed_environment)

//   // std::ostringstream os{};
//   // os << std::setw(sizeof(std::size_t) * 2) << std::setfill('0') << std::hex
//   //    << value_id << std::dec;
//   // return os.str();
//   return std::format("{:x}",value_id);
// }

bool TaggedAmount::operator==(TaggedAmount const& other) const {
  auto result =
      this->date() == other.date()
      and this->cents_amount() == other.cents_amount()
  //  and std::all_of(m_tags.begin(), m_tags.end(),
  //                 [&other](Tags::value_type const& entry) {
  //                   return ((entry.first.starts_with("_")) or
  //                           (other.tags().contains(entry.first) and
  //                            other.tags().at(entry.first) == entry.second));
  //                 });

      // Changed 20251026 - Now equal is all-tags-equal (no meta '_xxx' tag excepion)
      and this->m_tags == other.m_tags;

  // std::cout << "\nTaggedAmountClass::operator== ";
  // if (result) std::cout << "TRUE"; else std::cout << "FALSE";
  return result;
}

bool TaggedAmount::operator<(TaggedAmount const& other) const {
  // Tagged Amounts are implicitally ordered by date
  // DateOrderedTaggedAmountsContainer orders samet date values by upper_bound (insertion order for same date)
  return this->date() < other.date();
}

// END class TaggedAmount

std::ostream& operator<<(std::ostream &os, TaggedAmount const& ta) {
  // os <<  text::format::to_hex_string(to_value_id(ta));
  os << " " << ::to_string(ta.date());
  os << " " << ::to_string(to_units_and_cents(ta.cents_amount()));
  for (auto const& tag : ta.tags()) {
    os << "\n\t|--> \"" << tag.first << "=" << tag.second << "\"";
  }
  return os;
}

std::string to_string(TaggedAmount const& ta) {
  std::ostringstream oss;
  oss << ta;
  return oss.str();
}
