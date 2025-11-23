#pragma once

#include "csv/csv.hpp" // CSV::Table,...
#include "fiscal/amount/AmountFramework.hpp"
#include "FiscalPeriod.hpp"
#include "fiscal/amount/AccountStatement.hpp"
#include "logger/log.hpp" // logger::...
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <ranges>

namespace domain {

/**
 * Column Detection Result
 *
 * Identifies which columns in a CSV::Table correspond to required account statement fields.
 * Uses -1 to indicate "not found".
 */
struct ColumnMapping {
  int date_column = -1;
  int transaction_amount_column = -1;  // Column with transaction delta/change
  int saldo_amount_column = -1;        // Column with running balance (optional)
  int description_column = -1;         // Primary description column
  std::vector<int> additional_description_columns;  // Additional columns to concatenate

  bool is_valid() const {
    return date_column >= 0 && transaction_amount_column >= 0 && description_column >= 0;
  }
};

/**
 * Detect column mapping from CSV table header
 *
 * Strategy: Look for keywords in header column names to identify columns
 * - Date: "date", "datum", "bokföringsdag", "dag"
 * - Amount: "amount", "belopp", "suma"
 * - Description: "description", "namn", "rubrik", "meddelande", "text"
 */
inline ColumnMapping detect_columns_from_header(CSV::Table::Heading const& heading) {
  ColumnMapping mapping;

  // Helper to check if a string contains a keyword (case-insensitive)
  auto contains_keyword = [](std::string_view text, std::vector<std::string_view> const& keywords) {
    std::string lower_text;
    lower_text.reserve(text.size());
    std::ranges::transform(text, std::back_inserter(lower_text),
      [](char c) { return std::tolower(c); });

    return std::ranges::any_of(keywords, [&lower_text](std::string_view keyword) {
      return lower_text.find(keyword) != std::string::npos;
    });
  };

  // Date keywords
  std::vector<std::string_view> date_keywords = {
    "date", "datum", "bokföringsdag", "dag", "bokforingsdag"
  };

  // Amount keywords
  std::vector<std::string_view> amount_keywords = {
    "amount", "belopp", "suma", "summa"
  };

  // Description keywords (prioritized)
  std::vector<std::string_view> primary_description_keywords = {
    "namn", "name", "description", "rubrik", "titel", "title"
  };

  std::vector<std::string_view> secondary_description_keywords = {
    "meddelande", "message", "text", "detaljer", "details"
  };

  // Scan header columns
  for (size_t i = 0; i < heading.size(); ++i) {
    std::string col_name = heading[i];

    // Check for date column
    if (mapping.date_column == -1 && contains_keyword(col_name, date_keywords)) {
      mapping.date_column = static_cast<int>(i);
    }

    // Check for transaction amount column
    if (mapping.transaction_amount_column == -1 && contains_keyword(col_name, amount_keywords)) {
      mapping.transaction_amount_column = static_cast<int>(i);
    }

    // Check for primary description column
    if (mapping.description_column == -1 && contains_keyword(col_name, primary_description_keywords)) {
      mapping.description_column = static_cast<int>(i);
    }

    // Check for additional description columns
    if (contains_keyword(col_name, secondary_description_keywords)) {
      mapping.additional_description_columns.push_back(static_cast<int>(i));
    }
  }

  return mapping;
}

/**
 * Detect column mapping from data rows (when no header is available)
 *
 * Strategy: Use heuristics to identify column types by analyzing data patterns
 * - Date column: All non-empty values parse as valid dates
 * - Amount column: All non-empty values parse as valid amounts
 * - Description column: Text fields that don't parse as dates or amounts
 */
inline ColumnMapping detect_columns_from_data(CSV::Table::Rows const& rows) {
  ColumnMapping mapping;

  if (rows.empty()) {
    return mapping;
  }

  // Determine number of columns
  size_t num_columns = 0;
  for (auto const& row : rows) {
    num_columns = std::max(num_columns, row.size());
  }

  if (num_columns == 0) {
    return mapping;
  }

  // Sample rows for analysis (skip empty first columns - balance rows)
  auto sample_rows = 
      rows
    | std::views::filter([](auto const& row) {
        return row.size() > 0 && row[0].size() > 0;
      })
    | std::views::take(10);

  std::vector<CSV::Table::Row> sampled;
  std::ranges::copy(sample_rows, std::back_inserter(sampled));

  if (sampled.empty()) {
    return mapping;
  }

  // Check each column
  for (size_t col = 0; col < num_columns; ++col) {

    int valid_dates = 0;
    int valid_amounts = 0;
    int non_empty = 0;

    for (auto const& row : sampled) {
      if (col >= row.size() || row[col].size() == 0) {
        continue;
      }

      non_empty++;

      if (to_date(row[col]).has_value()) {
        valid_dates++;
      }
      else if (to_amount(row[col]).has_value()) {
        valid_amounts++;
      }
      else {
        // Nor date or amount
      }
    } // for row

    if (valid_dates == sampled.size()) {
      // Column is Date if ALL values are valid dates
      mapping.date_column = static_cast<int>(col);
    }
    else if (valid_amounts == sampled.size()) {
      // First amount column found becomes transaction amount
      // Second amount column found becomes saldo amount
      if (mapping.transaction_amount_column == -1) {
        mapping.transaction_amount_column = static_cast<int>(col);
      }
      else if (mapping.saldo_amount_column == -1) {
        mapping.saldo_amount_column = static_cast<int>(col);
      }
    }
    else {
      // Not a date nor an amount column
    }
  } // for column

  // Description is typically the first text column that's not date or amount
  for (size_t col = 0; col < num_columns; ++col) {
    if (static_cast<int>(col) != mapping.date_column &&
        static_cast<int>(col) != mapping.transaction_amount_column &&
        static_cast<int>(col) != mapping.saldo_amount_column &&
        mapping.description_column == -1) {

      // Check if column has substantial text content
      bool has_text = false;
      for (auto const& row : sampled) {
        if (col < row.size() && row[col].size() > 2) {
          has_text = true;
          break;
        }
      }

      if (has_text) {
        mapping.description_column = static_cast<int>(col);
      }
    }
  }

  return mapping;
}

/**
 * Determine if a CSV row is "ignorable" (e.g., balance rows in SKV format)
 *
 * Ignorable rows:
 * - Rows where first column is empty (balance rows)
 * - Rows that contain "saldo" in the description
 * - Rows that contain "ingående" or "utgående"
 */
inline bool is_ignorable_row(CSV::Table::Row const& row, ColumnMapping const& mapping) {
  // Empty row is ignorable
  if (row.size() == 0) {
    return true;
  }

  // Row with empty first column (common pattern for balance rows)
  if (row[0].size() == 0) {
    return true;
  }

  // Check description column for balance keywords
  if (mapping.description_column >= 0 &&
      static_cast<size_t>(mapping.description_column) < row.size()) {

    std::string desc = row[mapping.description_column];
    std::ranges::transform(desc, desc.begin(),
      [](char c) { return std::tolower(c); });

    if (desc.find("saldo") != std::string::npos ||
        desc.find("ingående") != std::string::npos ||
        desc.find("ingaende") != std::string::npos ||
        desc.find("utgående") != std::string::npos ||
        desc.find("utgaende") != std::string::npos) {
      return true;
    }
  }

  return false;
}

/**
 * Extract account statement entry from a CSV row
 *
 * Returns nullopt if:
 * - Row is ignorable
 * - Required fields are missing
 * - Date cannot be parsed
 * - Amount cannot be parsed
 */
inline std::optional<AccountStatementEntry> extract_entry_from_row(
    CSV::Table::Row const& row,
    ColumnMapping const& mapping) {

  // Check if row is ignorable
  if (is_ignorable_row(row, mapping)) {
    return std::nullopt;
  }

  // Validate column mapping
  if (!mapping.is_valid()) {
    return std::nullopt;
  }

  // Extract and validate date
  if (static_cast<size_t>(mapping.date_column) >= row.size() ||
      row[mapping.date_column].size() == 0) {
    return std::nullopt;
  }

  auto maybe_date = to_date(row[mapping.date_column]);
  if (!maybe_date) {
    return std::nullopt;
  }

  // Extract and validate transaction amount
  if (static_cast<size_t>(mapping.transaction_amount_column) >= row.size() ||
      row[mapping.transaction_amount_column].size() == 0) {
    return std::nullopt;
  }

  auto maybe_amount = to_amount(row[mapping.transaction_amount_column]);
  if (!maybe_amount) {
    return std::nullopt;
  }

  // Extract description (concatenate multiple columns if needed)
  std::string description;

  if (static_cast<size_t>(mapping.description_column) < row.size()) {
    description = row[mapping.description_column];
  }

  // Add additional description columns
  for (int col : mapping.additional_description_columns) {
    if (col != mapping.description_column &&
        static_cast<size_t>(col) < row.size() &&
        row[col].size() > 0) {
      if (description.size() > 0) {
        description += " | ";
      }
      description += row[col];
    }
  }

  // Description cannot be empty
  if (description.size() == 0) {
    return std::nullopt;
  }

  return AccountStatementEntry(*maybe_date, *maybe_amount, description);
}

/**
 * Filter CSV table to remove first/last rows with different column counts
 *
 * Strategy:
 * - Count non-empty fields in each row to determine "effective" column count
 * - Find the most common effective column count
 * - Remove first row if its count differs from the majority
 * - Remove last row if its count differs from the majority
 *
 * This handles cases like SKV files where the first row is a company header
 * and doesn't match the structure of transaction rows.
 */
inline CSV::Table filter_outlier_boundary_rows(CSV::Table const& table) {
  if (table.rows.size() <= 2) {
    return table;  // Too few rows to filter
  }

  // Count non-empty fields for each row
  auto count_non_empty = [](CSV::Table::Row const& row) -> size_t {
    return std::ranges::count_if(row, [](std::string const& field) {
      return field.size() > 0;
    });
  };

  // Build histogram of column counts
  std::map<size_t, size_t> count_histogram;
  for (auto const& row : table.rows) {
    size_t non_empty = count_non_empty(row);
    count_histogram[non_empty]++;
  }

  // Find most common column count
  size_t most_common_count = 0;
  size_t max_frequency = 0;
  for (auto const& [count, freq] : count_histogram) {
    if (freq > max_frequency) {
      max_frequency = freq;
      most_common_count = count;
    }
  }

  // Determine which rows to keep
  bool skip_first = count_non_empty(table.rows.front()) != most_common_count;
  bool skip_last = count_non_empty(table.rows.back()) != most_common_count;

  // Build filtered table
  CSV::Table filtered = table;
  filtered.rows.clear();

  for (size_t i = 0; i < table.rows.size(); ++i) {
    if (i == 0 && skip_first) continue;
    if (i == table.rows.size() - 1 && skip_last) continue;
    filtered.rows.push_back(table.rows[i]);
  }

  return filtered;
}

/**
 * Transform CSV::Table to Account Statement Entries
 *
 * This is the main extraction function that:
 * 1. Filters outlier first/last rows with different column structure
 * 2. Detects column mapping from header or data
 * 3. Extracts entries from each row
 * 4. Returns Maybe<AccountStatementEntries>
 *
 * Validation Strategy: All-or-nothing for required fields
 * - All entries must have valid date, amount, and description
 * - Ignorable rows (balance rows) are skipped
 * - Returns nullopt if column detection fails
 * - Returns empty vector if no valid entries found (but detection succeeded)
 */
inline OptionalAccountStatementEntries csv_table_to_account_statement_entries(CSV::Table const& table) {
  logger::scope_logger log_raii{logger::development_trace, "domain::csv_table_to_account_statement_entries(table)"};
  
  // Filter out first/last rows if they have different column structure
  CSV::Table filtered_table = filter_outlier_boundary_rows(table);

  // Try to detect columns from header first
  ColumnMapping mapping = detect_columns_from_header(filtered_table.heading);

  // If header detection failed, try data-based detection
  if (!mapping.is_valid()) {
    mapping = detect_columns_from_data(filtered_table.rows);
  }

  // If we still can't detect required columns, fail
  if (!mapping.is_valid()) {
    return std::nullopt;
  }

  // Extract entries from rows
  AccountStatementEntries entries;

  for (auto const& row : filtered_table.rows) {
    auto maybe_entry = extract_entry_from_row(row, mapping);
    if (maybe_entry) {
      entries.push_back(*maybe_entry);
    }
  }

  logger::development_trace("Returns entries with size:{}",entries.size());

  // Return the entries (may be empty if all rows were ignorable)
  return entries;
}

/**
 * Transform CSV::Table + AccountID to AccountStatement
 *
 * This is a higher-level extraction function that combines:
 * 1. CSV::Table -> AccountStatementEntries (via csv_table_to_account_statement_entries)
 * 2. AccountID from external source (e.g., CSV::project::to_account_id)
 *
 * The resulting AccountStatement contains both the entries and the account identifier.
 *
 * @param table The CSV::Table containing transaction data
 * @param account_id The AccountID identifying the account (e.g., NORDEA:51 86 87-9)
 * @return Optional AccountStatement with entries and account metadata, or nullopt on failure
 *
 * Returns nullopt if:
 * - Column detection fails (required columns not found)
 * - The table is fundamentally invalid
 *
 * Returns AccountStatement (possibly with empty entries) if:
 * - Column detection succeeds but all rows are ignorable
 */
inline std::optional<AccountStatement> csv_table_to_account_statement(
    CSV::Table const& table,
    AccountID const& account_id) {
  logger::scope_logger log_raii{logger::development_trace, "domain::csv_table_to_account_statement(table, account_id)"};

  // Extract entries from the CSV table
  auto maybe_entries = csv_table_to_account_statement_entries(table);

  if (!maybe_entries) {
    logger::development_trace("Failed to extract entries from CSV table");
    return std::nullopt;
  }

  // Create AccountStatement with entries and account ID metadata
  AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};

  logger::development_trace("Creating AccountStatement with {} entries and account_id: {}",
    maybe_entries->size(), account_id.to_string());

  return AccountStatement(*maybe_entries, meta);
}

} // namespace domain
