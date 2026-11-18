#pragma once

#include "core/balances.hpp"
#include "core/ink2/form.hpp"
#include "core/ink2/previous.hpp"
#include "core/ink2/skv.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace cratchit::ink2 {

// INK2S (skattemässiga justeringar) and INK2 page 1 of a fiscal year
struct TaxReturn {
  std::map<std::string, Kronor> ink2s; // the amounts given, by SRU code
  std::set<std::string> ink2s_marks;   // the yes/no marks given (as X)
  std::map<std::string, Kronor> ink2;  // page 1
  std::vector<std::string> errors;
  std::vector<std::string> notes;
};

// The rules of INK2S and INK2 page 1, from INK2R (form) and last year (previous):
// - 4.1 Årets resultat, vinst (7650) is 3.26 (7450); 4.2 förlust (7750) is 3.27 (7550).
// - The INK2S fields of the mapping (e.g. 4.3a, 4.3c, 4.5c) as form made them from the accounts.
// - 4.14a Outnyttjat underskott från föregående år (7763) is last year's 4.16, when not 0.
// - 4.15 Överskott (7670) or 4.16 Underskott (7770): the fields SKV marks + less those it marks -
//   (skv), 4.15 when that is 0 or more, else 4.16.
// - INK2 1.1 Överskott av näringsverksamhet (7104) is 4.15; 1.2 Underskott (7114) is 4.16.
// - The marks 8040/8041 (uppdragstagare) and 8044/8045 (revision) as last year gave them, with a note.
// Notes for what the books show that is not computed: periodiseringsfonder (4.6a schablonintäkt).
TaxReturn build_tax_return(std::vector<SkvField> const& skv, Form const& form, Previous const& previous);

} // namespace cratchit::ink2
