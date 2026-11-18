#pragma once

#include <string_view>

// The BAS accounts the books keep for SKV (Skatteverket). Fixed for now; later
// perhaps from a chart of accounts.
namespace cratchit::skv {

// Skattekonto: what SKV charges and credits the company, and what is paid to and from it
inline constexpr std::string_view tax_account = "1630";

} // namespace cratchit::skv
