#pragma once

#include "core/amount.hpp"
#include "core/ar/statement.hpp"

#include <string>
#include <vector>

namespace cratchit::ar {

// Resultatdisposition of fiscal year 0: the funds at the årsstämma's disposal,
// and the board's proposal for them. The proposal is not in the SIE file.
struct Disposition {
  struct Row {
    std::string text;
    Amount amount = 0; // öre as shown
  };
  std::vector<Row> funds; // the shown posts under Fritt eget kapital, e.g. Balanserat resultat and Årets resultat
  Amount total = 0;       // their sum, as Summa fritt eget kapital
  // The proposal's rows, whose amounts are not known yet (koppling till stämmobeslut saknas)
  std::vector<std::string> proposal{"Utdelas till aktieägare", "Balanseras i ny räkning"};
};

// The funds are the shown posts under the heading Fritt eget kapital of the balansräkning
Disposition build_disposition(Statement const& statement);

} // namespace cratchit::ar
