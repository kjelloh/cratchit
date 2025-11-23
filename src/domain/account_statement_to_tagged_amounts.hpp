#pragma once

#include "fiscal/amount/AccountStatement.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"
#include "fiscal/amount/AmountFramework.hpp"
#include "domain/csv_to_account_statement.hpp"
#include "logger/log.hpp"
#include <optional>
#include <string>
#include <ranges>
#include <algorithm>

namespace domain {

/**
 * Helper function to create an AccountID
 *
 * @param prefix The domain prefix (e.g., "NORDEA:", "SKV:", "PG:", "BG:")
 * @param value The account identifier value
 * @return AccountID with the specified prefix and value
 */
inline AccountID make_account_id(std::string const& prefix, std::string const& value) {
  return AccountID{.m_prefix = prefix, .m_value = value};
}

/**
 * Transform AccountStatement to TaggedAmounts
 *
 * This is Step 8 of the CSV import pipeline, bridging raw account data
 * to the domain model used for bookkeeping.
 *
 * Tagging Scheme:
 * - "Text" -> entry.transaction_caption (the description)
 * - "Account" -> statement.meta().m_maybe_account_irl_id->to_string() (AccountID)
 *
 * The Account tag format follows the AccountID scheme:
 * - If AccountID has prefix "NORDEA:" and value "51 86 87-9", tag value is "NORDEA:51 86 87-9"
 * - If no AccountID is present, an empty string is used
 *
 * @param statement The AccountStatement containing entries to transform
 * @return Optional vector of TaggedAmounts, or nullopt on fundamental failure
 *
 * Returns std::nullopt if:
 * - The transformation fundamentally fails (reserved for future validation)
 *
 * Returns empty vector if:
 * - The statement has no entries (this is valid, not an error)
 */
inline std::optional<TaggedAmounts> account_statement_to_tagged_amounts(
    AccountStatement const& statement) {
  logger::scope_logger log_raii{logger::development_trace,
    "domain::account_statement_to_tagged_amounts(statement)"};

  // Extract account ID string from meta
  // If no AccountID is present, use empty string
  std::string account_id_string;
  if (statement.meta().m_maybe_account_irl_id.has_value()) {
    account_id_string = statement.meta().m_maybe_account_irl_id->to_string();
  }

  logger::development_trace("Processing statement with {} entries, account_id: '{}'",
    statement.entries().size(), account_id_string);

  // Transform each AccountStatementEntry to TaggedAmount
  TaggedAmounts result;
  result.reserve(statement.entries().size());

  for (auto const& entry : statement.entries()) {
    // Convert Amount to CentsAmount using existing conversion function
    CentsAmount cents_amount = to_cents_amount(entry.transaction_amount);

    // Build tags map with required "Text" and "Account" entries
    TaggedAmount::Tags tags = {
      {"Text", entry.transaction_caption},
      {"Account", account_id_string}
    };

    // Merge any existing transaction_tags from the entry
    // (preserving the "Text" and "Account" values we just set)
    for (auto const& [key, value] : entry.transaction_tags) {
      if (key != "Text" && key != "Account") {
        tags[key] = value;
      }
    }

    // Create TaggedAmount with date, cents_amount, and tags
    result.emplace_back(entry.transaction_date, cents_amount, std::move(tags));

    logger::development_trace("Created TaggedAmount: cents={}, text='{}'",
      to_amount_in_cents_integer(cents_amount), entry.transaction_caption);
  }

  logger::development_trace("Returning {} TaggedAmounts", result.size());

  return result;
}

/**
 * Compose CSV::Table + AccountID -> Maybe<TaggedAmounts>
 *
 * This is a convenience function that composes steps 7 and 8:
 * 1. csv_table_to_account_statement (Step 7)
 * 2. account_statement_to_tagged_amounts (Step 8)
 *
 * @param table The CSV::Table containing transaction data
 * @param account_id The AccountID identifying the account
 * @return Optional vector of TaggedAmounts, or nullopt on failure
 */
inline std::optional<TaggedAmounts> csv_table_to_tagged_amounts(
    CSV::Table const& table,
    AccountID const& account_id) {
  logger::scope_logger log_raii{logger::development_trace,
    "domain::csv_table_to_tagged_amounts(table, account_id)"};

  // Step 7: CSV::Table + AccountID -> AccountStatement
  auto maybe_statement = csv_table_to_account_statement(table, account_id);

  if (!maybe_statement) {
    logger::development_trace("Step 7 failed: csv_table_to_account_statement returned nullopt");
    return std::nullopt;
  }

  // Step 8: AccountStatement -> TaggedAmounts
  return account_statement_to_tagged_amounts(*maybe_statement);
}

} // namespace domain
