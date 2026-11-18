#pragma once

#include "core/ar/notes.hpp"
#include "core/ar/overview.hpp"
#include "core/ar/statement.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::ar {

// The version of the rules the requirements were checked against
constexpr std::string_view requirements_source =
    "BFN:s vägledning Årsredovisning i mindre företag (K2), BFNAR 2016:10, uppdaterad 2025-06-16";

// Something a K2 årsredovisning of an aktiebolag must contain that the SIE file cannot provide
struct Requirement {
  std::string_view id;      // stable key, e.g. "sate"
  std::string_view title;   // what it is, in the words of the årsredovisning
  std::string_view rule;    // where it is required
  std::string_view provide; // what the company has to provide
  std::string_view heading; // the heading of the input template that will provide it
};

// All requirements, in the order they appear in the årsredovisning
std::vector<Requirement> const& requirements();

// A requirement as it stands for one årsredovisning
struct CheckItem {
  Requirement const* requirement = nullptr;
  int number = 0;     // its [n] in the årsredovisning and the summary
  std::string detail; // what the books say about it, e.g. why it applies; may be empty
};

struct Check {
  std::vector<CheckItem> missing;    // required now: TBD [n] in the årsredovisning
  std::vector<CheckItem> to_confirm; // required only if something applies that the SIE file does not tell

  // "TBD [n]" for a missing requirement, else nothing
  std::optional<std::string> marker(std::string_view id) const;
};

// The requirements for the årsredovisning of statement, overview and notes, all
// still without the company's input. Missing ones are numbered first, in the order
// of the årsredovisning, then those to confirm:
// - missing first when the books are not closed (unbooked_result, see closing.hpp)
// - always missing: säte, allmänt om verksamheten, väsentliga händelser, the
//   flerårsöversikt's two earlier years, the board's proposal, datering, underskrifter
// - missing when the books show it applies, else to confirm: the comment on a
//   Nettoomsättning that varies more than 30 %, depreciation periods (an
//   Av- och nedskrivningar amount or a note), Medelantal anställda (Personalkostnader)
// - missing for each shown post under Immateriella or Materiella
//   anläggningstillgångar without a note
// - to confirm: the board's statement on a dividend, ställda säkerheter,
//   eventualförpliktelser, lån till styrelseledamöter, events after the balance day
Check check_requirements(Statement const& statement, Overview const& overview, Notes const& notes,
                         std::optional<Amount> unbooked_result = std::nullopt);

} // namespace cratchit::ar
