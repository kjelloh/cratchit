#pragma once

/**
 * CSV Import Pipeline - High-Level API
 *
 * This header provides the unified high-level API for the complete CSV import pipeline:
 *   file_path -> Maybe<TaggedAmounts>
 *
 * The pipeline composes all steps (1-8) of the CSV import refactoring:
 *   1. File I/O: Read file to byte buffer
 *   2. Encoding detection: Detect source encoding using ICU
 *   3. Transcoding: Bytes -> Unicode code points (lazy view)
 *   4. Encoding: Unicode -> Platform encoding (lazy view)
 *   5. Materialization: Lazy view -> std::string (read_file_with_encoding_detection)
 *   6. CSV parsing: Text -> CSV::Table
 *   6.5. AccountID detection: Identify bank/SKV format and extract account ID
 *   7. Domain extraction: CSV::Table + AccountID -> AccountStatement
 *   8. Final transformation: AccountStatement -> TaggedAmounts
 *
 * Error Handling:
 *   - File not found: Returns empty AnnotatedMaybe with error message
 *   - Encoding detection failure: Defaults to UTF-8 (permissive strategy)
 *   - CSV parsing failure: Returns empty AnnotatedMaybe with error message
 *   - AccountID detection failure: Uses empty AccountID (graceful fallback)
 *   - Invalid business data: Returns empty AnnotatedMaybe with error message
 *
 * All errors and success messages are preserved in AnnotatedMaybe::m_messages.
 */

#include "text/encoding_pipeline.hpp"        // read_file_with_encoding_detection (Steps 1-5)
#include "csv/neutral_parser.hpp"            // CSV::neutral::text_to_table (Step 6)
#include "csv/csv_to_account_id.hpp"         // CSV::project::to_account_id_ed (Step 6.5)
#include "domain/csv_to_account_statement.hpp"  // domain::csv_table_to_account_statement (Step 7)
#include "domain/account_statement_to_tagged_amounts.hpp" // domain::csv_table_to_tagged_amounts (Steps 7+8)
#include "functional/maybe.hpp"              // AnnotatedMaybe
#include "logger/log.hpp"                    // logger::...
#include <filesystem>
#include <format>

namespace cratchit::csv {

/**
 * Import CSV file to TaggedAmounts - Complete Pipeline
 *
 * This function composes the entire CSV import pipeline:
 *   1-5. File -> Text (with encoding detection via read_file_with_encoding_detection)
 *   6.   Text -> CSV::Table (via CSV::neutral::text_to_table)
 *   6.5  CSV::Table -> AccountID (via CSV::project::to_account_id)
 *   7+8. CSV::Table + AccountID -> TaggedAmounts (via domain::csv_table_to_tagged_amounts)
 *
 * The function preserves all messages from each step, providing a complete
 * audit trail of the import process.
 *
 * @param file_path Path to the CSV file to import
 * @return AnnotatedMaybe<TaggedAmounts> with result or error messages
 *
 * Example usage:
 * @code
 *   auto result = cratchit::csv::import_file_to_tagged_amounts("/path/to/file.csv");
 *   if (result) {
 *     for (auto const& tagged_amount : result.value()) {
 *       // Process tagged amounts
 *     }
 *   } else {
 *     for (auto const& msg : result.m_messages) {
 *       // Log error messages
 *     }
 *   }
 * @endcode
 */
inline AnnotatedMaybe<TaggedAmounts> import_file_to_tagged_amounts(
    std::filesystem::path const& file_path) {
  logger::scope_logger log_raii{logger::development_trace,
    "cratchit::csv::import_file_to_tagged_amounts(file_path)"};

  AnnotatedMaybe<TaggedAmounts> result{};

  // ============================================================
  // Steps 1-5: File -> Text (with encoding detection)
  // ============================================================
  auto text_result = text::encoding::read_file_with_encoding_detection(file_path);

  if (!text_result) {
    // Propagate file/encoding errors
    result.m_messages = std::move(text_result.m_messages);
    result.push_message("Pipeline failed at Step 1-5: File reading/encoding");
    return result;
  }

  // Copy messages from file reading/encoding
  result.m_messages = text_result.m_messages;
  auto const& text = text_result.value();

  // Handle empty file case
  if (text.empty()) {
    result.push_message("Pipeline complete: Empty file produced empty TaggedAmounts");
    result.m_value = TaggedAmounts{};
    return result;
  }

  // ============================================================
  // Step 6: Text -> CSV::Table
  // ============================================================
  auto maybe_table = CSV::neutral::text_to_table(text);

  if (!maybe_table) {
    result.push_message("Pipeline failed at Step 6: CSV parsing failed - Could not parse text as CSV");
    return result;
  }

  result.push_message(std::format("Step 6 complete: CSV parsed successfully ({} rows)",
    maybe_table->rows.size()));

  // ============================================================
  // Step 6.5: CSV::Table -> MDTable<AccountID>
  // ============================================================
  auto maybe_md_table = CSV::project::to_account_id_ed(*maybe_table);

  if (!maybe_md_table) {
    // Unknown format - fully unknown AccountID (no prefix, no value)
    result.push_message("Step 6.5 failed: Unknown CSV format - could not identify account");
    return result;
  }

  AccountID const& account_id = maybe_md_table->meta;
  CSV::Table const& identified_table = maybe_md_table->defacto;
  result.push_message(std::format("Step 6.5 complete: AccountID detected: '{}'",
    account_id.to_string()));

  // ============================================================
  // Steps 7+8: CSV::Table + AccountID -> TaggedAmounts
  // ============================================================
  auto maybe_tagged = domain::csv_table_to_tagged_amounts(identified_table, account_id);

  if (!maybe_tagged) {
    result.push_message("Pipeline failed at Steps 7-8: Domain transformation failed - Could not extract tagged amounts");
    return result;
  }

  result.m_value = std::move(*maybe_tagged);
  result.push_message(std::format("Pipeline complete: {} TaggedAmounts created from '{}'",
    result.value().size(),
    file_path.filename().string()));

  return result;
}

/**
 * Import CSV file to AccountStatement - Complete Pipeline (Steps 1-7)
 *
 * This function composes the CSV import pipeline up to AccountStatement:
 *   1-5. File -> Text (with encoding detection via read_file_with_encoding_detection)
 *   6.   Text -> CSV::Table (via CSV::neutral::text_to_table)
 *   6.5  CSV::Table -> MDTable<AccountID> (via CSV::project::to_account_id_ed)
 *   7.   MDTable<AccountID> -> AccountStatement (via domain::md_table_to_account_statement)
 *
 * The function preserves all messages from each step, providing a complete
 * audit trail of the import process.
 *
 * @param file_path Path to the CSV file to import
 * @return AnnotatedMaybe<AccountStatement> with result or error messages
 *
 * Example usage:
 * @code
 *   auto result = cratchit::csv::import_file_to_account_statement("/path/to/file.csv");
 *   if (result) {
 *     auto const& statement = result.value();
 *     for (auto const& entry : statement.entries()) {
 *       // Process account statement entries
 *     }
 *   } else {
 *     for (auto const& msg : result.m_messages) {
 *       // Log error messages
 *     }
 *   }
 * @endcode
 */
inline AnnotatedMaybe<AccountStatement> import_file_to_account_statement(
    std::filesystem::path const& file_path) {
  logger::scope_logger log_raii{logger::development_trace,
    "cratchit::csv::import_file_to_account_statement(file_path)"};

  AnnotatedMaybe<AccountStatement> result{};

  // ============================================================
  // Steps 1-5: File -> Text (with encoding detection)
  // ============================================================
  auto text_result = text::encoding::read_file_with_encoding_detection(file_path);

  if (!text_result) {
    // Propagate file/encoding errors
    result.m_messages = std::move(text_result.m_messages);
    result.push_message("Pipeline failed at Step 1-5: File reading/encoding");
    return result;
  }

  // Copy messages from file reading/encoding
  result.m_messages = text_result.m_messages;
  auto const& text = text_result.value();

  // Handle empty file case
  if (text.empty()) {
    result.push_message("Pipeline complete: Empty file produced empty AccountStatement");
    result.m_value = AccountStatement{AccountStatementEntries{}, AccountStatement::Meta{}};
    return result;
  }

  // ============================================================
  // Step 6: Text -> CSV::Table
  // ============================================================
  auto maybe_table = CSV::neutral::text_to_table(text);

  if (!maybe_table) {
    result.push_message("Pipeline failed at Step 6: CSV parsing failed - Could not parse text as CSV");
    return result;
  }

  result.push_message(std::format("Step 6 complete: CSV parsed successfully ({} rows)",
    maybe_table->rows.size()));

  // ============================================================
  // Step 6.5: CSV::Table -> MDTable<AccountID>
  // ============================================================
  auto maybe_md_table = CSV::project::to_account_id_ed(*maybe_table);

  if (!maybe_md_table) {
    // Unknown format - fully unknown AccountID (no prefix, no value)
    result.push_message("Step 6.5 failed: Unknown CSV format - could not identify account");
    return result;
  }

  result.push_message(std::format("Step 6.5 complete: AccountID detected: '{}'",
    maybe_md_table->meta.to_string()));

  // ============================================================
  // Step 7: MDTable<AccountID> -> AccountStatement
  // ============================================================
  auto maybe_statement = domain::md_table_to_account_statement(*maybe_md_table);

  if (!maybe_statement) {
    result.push_message("Pipeline failed at Step 7: Domain transformation failed - Could not extract account statement");
    return result;
  }

  result.m_value = std::move(*maybe_statement);
  result.push_message(std::format("Pipeline complete: AccountStatement with {} entries created from '{}'",
    result.value().entries().size(),
    file_path.filename().string()));

  return result;
}

/**
 * Import CSV text to TaggedAmounts - Text-based Pipeline Entry Point
 *
 * This is a convenience function for when you already have CSV text in memory.
 * It composes Steps 6-8 of the pipeline, skipping file I/O and encoding.
 *
 * @param csv_text UTF-8 encoded CSV text
 * @return AnnotatedMaybe<TaggedAmounts> with result or error messages
 */
inline AnnotatedMaybe<TaggedAmounts> import_text_to_tagged_amounts(
    std::string_view csv_text) {
  logger::scope_logger log_raii{logger::development_trace,
    "cratchit::csv::import_text_to_tagged_amounts(csv_text)"};

  AnnotatedMaybe<TaggedAmounts> result{};

  // Handle empty text case
  if (csv_text.empty()) {
    result.push_message("Pipeline complete: Empty text produced empty TaggedAmounts");
    result.m_value = TaggedAmounts{};
    return result;
  }

  // Step 6: Text -> CSV::Table
  auto maybe_table = CSV::neutral::text_to_table(csv_text);

  if (!maybe_table) {
    result.push_message("Pipeline failed at Step 6: CSV parsing failed - Could not parse text as CSV");
    return result;
  }

  result.push_message(std::format("Step 6 complete: CSV parsed successfully ({} rows)",
    maybe_table->rows.size()));

  // Step 6.5: CSV::Table -> MDTable<AccountID>
  auto maybe_md_table = CSV::project::to_account_id_ed(*maybe_table);

  if (!maybe_md_table) {
    // Unknown format - fully unknown AccountID (no prefix, no value)
    result.push_message("Step 6.5 failed: Unknown CSV format - could not identify account");
    return result;
  }

  AccountID const& account_id = maybe_md_table->meta;
  CSV::Table const& identified_table = maybe_md_table->defacto;
  result.push_message(std::format("Step 6.5 complete: AccountID detected: '{}'",
    account_id.to_string()));

  // Steps 7+8: CSV::Table + AccountID -> TaggedAmounts
  auto maybe_tagged = domain::csv_table_to_tagged_amounts(identified_table, account_id);

  if (!maybe_tagged) {
    result.push_message("Pipeline failed at Steps 7-8: Domain transformation failed - Could not extract tagged amounts");
    return result;
  }

  result.m_value = std::move(*maybe_tagged);
  result.push_message(std::format("Pipeline complete: {} TaggedAmounts created",
    result.value().size()));

  return result;
}

/**
 * Import CSV table to TaggedAmounts - Table-based Pipeline Entry Point
 *
 * This is a convenience function for when you already have a parsed CSV::Table.
 * It composes Steps 6.5-8 of the pipeline, skipping parsing.
 *
 * @param table Parsed CSV::Table
 * @return AnnotatedMaybe<TaggedAmounts> with result or error messages
 */
inline AnnotatedMaybe<TaggedAmounts> import_table_to_tagged_amounts(
    CSV::Table const& table) {
  logger::scope_logger log_raii{logger::development_trace,
    "cratchit::csv::import_table_to_tagged_amounts(table)"};

  AnnotatedMaybe<TaggedAmounts> result{};

  result.push_message(std::format("Starting from Step 6.5 with CSV::Table ({} rows)",
    table.rows.size()));

  // Step 6.5: CSV::Table -> MDTable<AccountID>
  auto maybe_md_table = CSV::project::to_account_id_ed(table);

  if (!maybe_md_table) {
    // Unknown format - fully unknown AccountID (no prefix, no value)
    result.push_message("Step 6.5 failed: Unknown CSV format - could not identify account");
    return result;
  }

  AccountID const& account_id = maybe_md_table->meta;
  CSV::Table const& identified_table = maybe_md_table->defacto;
  result.push_message(std::format("Step 6.5 complete: AccountID detected: '{}'",
    account_id.to_string()));

  // Steps 7+8: CSV::Table + AccountID -> TaggedAmounts
  auto maybe_tagged = domain::csv_table_to_tagged_amounts(identified_table, account_id);

  if (!maybe_tagged) {
    result.push_message("Pipeline failed at Steps 7-8: Domain transformation failed - Could not extract tagged amounts");
    return result;
  }

  result.m_value = std::move(*maybe_tagged);
  result.push_message(std::format("Pipeline complete: {} TaggedAmounts created",
    result.value().size()));

  return result;
}

} // namespace cratchit::csv
