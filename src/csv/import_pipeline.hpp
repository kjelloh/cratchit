#pragma once

#include "text/encoding_pipeline.hpp"        // path_to_platform_encoded_string_shortcut (Steps 1-5)
#include "csv/parse_csv.hpp"            // CSV::parse::maybe::text_to_table (Step 6)
#include "csv/csv_to_account_id.hpp"         // account::statement::maybe::to_account_id_ed_step (Step 6.5)
#include "domain/csv_to_account_statement.hpp"  // account::statement::csv_table_to_account_statement_step (Step 7)
#include "domain/account_statement_to_tagged_amounts.hpp"
#include "functional/maybe.hpp"              // AnnotatedMaybe
#include "logger/log.hpp"                    // logger::...
#include <filesystem>
#include <format>

namespace csv {

  /**
  * CSV monadic Pipeline
  *
  * This header provides the unified high-level API for the complete CSV import pipeline:
  *   file_path -> Maybe<TaggedAmounts>
  *
  * The pipeline composes all steps (1-8) of the CSV file path -> CSV monadic pipeline
  *   1. File I/O: Read file to byte buffer
  *   2. Encoding detection: Detect source encoding using ICU
  *   3. Transcoding: Bytes -> Unicode code points (lazy view)
  *   4. Encoding: Unicode -> Platform encoding (lazy view)
  *   5. Materialization: Lazy view -> std::string (path_to_platform_encoded_string_shortcut)
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

  namespace monadic {

    // TODO: Consider to rename namesopace monadic to 'annotated'?
    //       For now it contains 'single step' functions returning AnnotatedMaybe<T> things
    //       Note that 'shortcuts' combining several 'annotated' steps are outside
    //       this namespace.

  } // monadic

  // Monadic AnnotatedMaybe: Table -> TaggedAmounts
  inline AnnotatedMaybe<TaggedAmounts> table_to_tagged_amounts_shortcut(
      CSV::Table const& table) {
    logger::scope_logger log_raii{logger::development_trace,
      "table_to_tagged_amounts_shortcut(table)"};

    AnnotatedMaybe<TaggedAmounts> result{};

    result.push_message(std::format("Starting from Step 6.5 with CSV::Table ({} rows)",
      table.rows.size()));

    // Step 6.5: CSV::Table -> MDTable<AccountID>
    auto maybe_md_table = account::statement::maybe::to_account_id_ed_step(table);

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
    auto maybe_tagged = tas::csv_table_to_tagged_amounts_shortcut(identified_table, account_id);

    if (!maybe_tagged) {
      result.push_message("Pipeline failed at Steps 7-8: Domain transformation failed - Could not extract tagged amounts");
      return result;
    }

    result.m_value = std::move(*maybe_tagged);
    result.push_message(std::format("Pipeline complete: {} TaggedAmounts created",
      result.value().size()));

    return result;
  }

  // Monadic AnnotatedMaybe #? ... #? shortcut
  inline AnnotatedMaybe<TaggedAmounts> csv_to_tagged_amounts_shortcut(
      std::string_view csv_text) {
    logger::scope_logger log_raii{logger::development_trace,
      "csv_to_tagged_amounts_shortcut(csv_text)"};

    AnnotatedMaybe<TaggedAmounts> result{};

    // Handle empty text case
    if (csv_text.empty()) {
      result.push_message("Pipeline complete: Empty text produced empty TaggedAmounts");
      result.m_value = TaggedAmounts{};
      return result;
    }

    // Monadic Maybe: csv text -> Table
    auto maybe_table = CSV::parse::maybe::text_to_table(csv_text);

    if (!maybe_table) {
      result.push_message("Pipeline failed at Step 6: CSV parsing failed - Could not parse text as CSV");
      return result;
    }

    result.push_message(std::format("Step 6 complete: CSV parsed successfully ({} rows)",
      maybe_table->rows.size()));

    // Monadic Maybe: Table -> (account ID,table) pair
    auto maybe_md_table = account::statement::maybe::to_account_id_ed_step(*maybe_table);

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
    auto maybe_tagged = tas::csv_table_to_tagged_amounts_shortcut(identified_table, account_id);

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
  * Import CSV file to AccountStatement - Complete Pipeline (Steps 1-7)
  *
  * This function composes the CSV import pipeline up to AccountStatement:
  *   1-5. File -> Text (with encoding detection via path_to_platform_encoded_string_shortcut)
  *   6.   Text -> CSV::Table (via CSV::parse::maybe::text_to_table)
  *   6.5  CSV::Table -> MDTable<AccountID> (via account::statement::maybe::to_account_id_ed_step)
  *   7.   MDTable<AccountID> -> AccountStatement (via account::statement::md_table_to_account_statement)
  *
  */
  inline AnnotatedMaybe<AccountStatement> path_to_account_statement_shortcut(
      std::filesystem::path const& file_path) {
    logger::scope_logger log_raii{logger::development_trace,
      "path_to_account_statement_shortcut(file_path)"};

    AnnotatedMaybe<AccountStatement> result{};

    // ============================================================
    // Steps 1-5: File -> Text (with encoding detection)
    // ============================================================
    auto text_result = text::encoding::path_to_platform_encoded_string_shortcut(file_path);

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
    auto maybe_table = CSV::parse::maybe::text_to_table(text);

    if (!maybe_table) {
      result.push_message("Pipeline failed at Step 6: CSV parsing failed - Could not parse text as CSV");
      return result;
    }

    result.push_message(std::format("Step 6 complete: CSV parsed successfully ({} rows)",
      maybe_table->rows.size()));

    // ============================================================
    // Step 6.5: CSV::Table -> MDTable<AccountID>
    // ============================================================
    auto maybe_md_table = account::statement::maybe::to_account_id_ed_step(*maybe_table);

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
    auto maybe_statement = account::statement::md_table_to_account_statement(*maybe_md_table);

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

  // Monadic AnnotatedMaybe #1 .. #? shortcut
  inline AnnotatedMaybe<TaggedAmounts> path_to_tagged_amounts_shortcut(
      std::filesystem::path const& file_path) {
    logger::scope_logger log_raii{logger::development_trace,
      "csv::path_to_tagged_amounts_shortcut(file_path)"};

    AnnotatedMaybe<TaggedAmounts> result{};

    // ============================================================
    // Steps 1-5: File -> Text (with encoding detection)
    // ============================================================
    auto text_result = text::encoding::path_to_platform_encoded_string_shortcut(file_path);

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
    auto maybe_table = CSV::parse::maybe::text_to_table(text);

    if (!maybe_table) {
      result.push_message("Pipeline failed at Step 6: CSV parsing failed - Could not parse text as CSV");
      return result;
    }

    result.push_message(std::format("Step 6 complete: CSV parsed successfully ({} rows)",
      maybe_table->rows.size()));

    // ============================================================
    // Step 6.5: CSV::Table -> MDTable<AccountID>
    // ============================================================
    auto maybe_md_table = account::statement::maybe::to_account_id_ed_step(*maybe_table);

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
    auto maybe_tagged = tas::csv_table_to_tagged_amounts_shortcut(identified_table, account_id);

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

} // csv
