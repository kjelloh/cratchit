#pragma once

#include "core/amount.hpp"
#include "core/balances.hpp"
#include "core/ink2/form.hpp"
#include "core/ink2/output.hpp"
#include "core/ink2/previous.hpp"

#include <optional>
#include <string>
#include <vector>

namespace cratchit::ink2 {

// Something the return needs, or may need, that the books do not give
struct CheckItem {
  int number = 0;                 // its [n]: TBD [n] at its fields in the return; 0 for items to confirm
  std::string title;              // what it is, by the form's words, e.g. "4.6a Schablonintäkt på periodiseringsfonder"
  std::vector<std::string> fields; // the SRU codes it is about
  std::string found;              // what the books show
  std::string provide;            // what to do
};

// The INK2 check of a return
struct Check {
  std::vector<CheckItem> missing;    // required now: numbered, TBD [n] in the return; no SRU files while any
  std::vector<CheckItem> to_confirm; // may apply, and the books cannot tell

  // TBD [n] at the fields of each missing item
  Markers markers() const;
};

// What the books and last year's return tell:
// - missing, in this order, when it applies:
//   - the books are not closed (unbooked): 3.26/3.27 and the fields made from them;
//   - periodiseringsfonder at the start of the year (opening 2110-2139): 4.6a (7654);
//   - an amount on 3.17 (7521): 4.3b (7652), whether the nedskrivning may be deducted;
//   - pension costs (7410-7419): 4.21 (8022) and 1.4/1.5 särskild löneskatt (7132/7133);
//   - no marks from last year: 8040/8041 and 8044/8045;
//   - no postnummer and postort for INFO.SRU (postal_found false);
// - to confirm, always: 4.4b and 4.6e, 4.13, 4.14b-c, 4.20; and 4.14a when typed (previous_typed).
Check check_return(Form const& form, Balances const& opening, Balances const& balances, Previous const& previous,
                   bool previous_typed, std::optional<Amount> unbooked, bool postal_found);

// The check as text for the report: the missing items, then those to confirm
std::string format_check(Check const& check);

} // namespace cratchit::ink2
