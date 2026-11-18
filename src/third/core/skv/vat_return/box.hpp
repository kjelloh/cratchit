#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// SKV's VAT return (momsdeklaration): the boxes (rutor) of the form and the
// amounts the books report in them for a period
namespace cratchit::skv::vat_return {

// A box of the form by its two-digit number, e.g. "05" or "48"
using Box = std::string;

// The box a BAS account reports in, else nothing
std::optional<Box> box_of(std::string_view account);

// The boxes that hold VAT: output VAT (10-12, 30-32, 60-62) and input VAT (48)
inline constexpr std::array<std::string_view, 10> vat_boxes{"10", "11", "12", "30", "31", "32", "60", "61", "62", "48"};

// The boxes the books have as credit: sales (05-08, 35-42 but 37) and output VAT (10-12, 30-32,
// 60-62). The form takes them as positive figures, so their amount is the books' sum with the sign
// turned. The other boxes, the purchase bases (20-24, 37, 50) and input VAT (48), are debit in the
// books and keep the sign.
inline constexpr std::array<std::string_view, 20> credit_boxes{"05", "06", "07", "08", "10", "11", "12",
                                                               "30", "31", "32", "35", "36", "38", "39",
                                                               "40", "41", "42", "60", "61", "62"};

// Box 49, Moms att betala eller få tillbaka: not reported in by accounts; the
// form adds it up from the other VAT boxes as 10 + 11 + 12 + 30 + 31 + 32 + 60 + 61 + 62 - 48
inline constexpr std::string_view vat_to_pay_box = "49";
inline constexpr std::array<std::string_view, 9> vat_to_pay_added{"10", "11", "12", "30", "31", "32", "60", "61", "62"};
inline constexpr std::array<std::string_view, 1> vat_to_pay_subtracted{"48"};
inline constexpr std::string_view vat_to_pay_expression = "10 + 11 + 12 + 30 + 31 + 32 + 60 + 61 + 62 - 48";

// The accounts that report in a box (see box_of), by number
std::vector<std::string> accounts_of(Box const& box);

// True for an account that reports in one of vat_boxes
bool is_vat_account(std::string_view account);

// The BAS accounts the VAT of a period is cleared to, from the VAT accounts; SKV
// then charges or credits the tax account against them. Fixed for now; later
// perhaps from a chart of accounts or the box table.
inline constexpr std::string_view vat_liability_account = "2650";  // Redovisningskonto för moms
inline constexpr std::string_view vat_receivable_account = "1650"; // Momsfordran

// True for vat_liability_account and vat_receivable_account
bool is_vat_clearing_account(std::string_view account);

} // namespace cratchit::skv::vat_return
