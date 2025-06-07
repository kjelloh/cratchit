#include "TaggedAmountFramework.hpp"
#include <iostream> // ,std::cout

TaggedAmount::OptionalValueIds to_value_ids(Key::Path const &sids) {
  // std::cout << "\nto_value_ids()" << std::flush;
  TaggedAmount::OptionalValueIds result{};
  TaggedAmount::ValueIds value_ids{};
  for (auto const &sid : sids) {
    if (auto value_id = to_value_id(sid)) {
      // std::cout << "\n\tA valid instance id sid=" << std::quoted(sid);
      value_ids.push_back(*value_id);
    } else {
      std::cout
          << "\nDESIGN_INSUFFICIENCY: to_value_ids: Not a valid instance id string sid="
          << std::quoted(sid) << std::flush;
    }
  }
  if (value_ids.size() == sids.size()) {
    result = value_ids;
  } else {
    std::cout << "\nDESIGN_INSUFFICIENCY: to_value_ids(Key::Path const& "
              << sids.to_string() << ") Failed. Created" << value_ids.size()
              << " out of " << sids.size() << " possible.";
  }
  return result;
}
