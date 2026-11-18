#pragma once

#include "core/balances.hpp"
#include "core/date.hpp"
#include "core/sie/document.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The year-end closing of a fiscal year in an SIE file, shared by cratchit_ar
// (--sie-to-ar) and cratchit_ink2 (--sie-to-ink2): from both sessions, the same code.
namespace cratchit {

// The closing entries BAS asks for at the balance day that the books of a
// fiscal year lack, one note each:
// - Moms (2610-2659) netting to a receivable: move it to 1650 Momsfordran.
// - Skatteskulder (2500-2599) netting to a receivable: move it to 1640 Skattefordringar.
// - 1630 Skattekonto with a credit balance: move it to 2850.
// - Redovisningsmedel (1990-1999) not matched by Avräkning för annans räkning (2830-2839).
// - 2999 OBS-konto not zero.
// - No depreciation (7800-7899) although an account for accumulated depreciation
//   of anläggningstillgångar (10x9, 11x9, 12x9) has a balance.
// - 2098 Vinst eller förlust från föregående år not zero: last year's result is
//   not yet moved on as the årsstämma decided.
std::vector<std::string> check_closing(Balances const& balances, Date balance_day);

// The year's result of fiscal year 0 as booked (debit positive, so a loss is
// positive) when the books are not closed: nothing is booked to 8990-8999 in
// year 0, the result accounts (3000-8989) add up to something, and the balance
// accounts (class 1 and 2) are off by exactly that, as the result is not on 2099.
// Nothing when the books are closed or something else is wrong.
std::optional<Amount> unbooked_result(sie::Document const& document);

// document with the closing entry it lacks for result (see unbooked_result): a
// voucher at the end of fiscal year 0 that books it to 2099 against 8999, as
// Fortnox does, with the closing balance of 2099 and the result of 8999 to match.
// Its series is "*" and its text says it is not booked.
sie::Document with_closing_entry(sie::Document document, Amount result);

// The text of the simulated closing entry
constexpr std::string_view simulated_closing_text = "Ej bokfört: Årets resultat (simulerat av cratchit)";

} // namespace cratchit
