#pragma once

#include "core/amount.hpp"
#include "core/sie/document.hpp"

#include <cstdint>
#include <map>
#include <string>

namespace cratchit {

// Whole kronor, as the forms show them
using Kronor = std::int64_t;

// An amount in öre to whole kronor, half away from zero
Kronor to_kronor(Amount amount);

// What a fiscal year ends with, by account: the closing balance (#UB) of
// balance accounts and the result (#RES) of result accounts
using Balances = std::map<std::string, Amount>;

Balances year_balances(sie::Document const& document, int year);

} // namespace cratchit
