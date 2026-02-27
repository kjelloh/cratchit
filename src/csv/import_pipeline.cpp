
#include "import_pipeline.hpp"

namespace csv {

  namespace monadic {
  } // monadic

  AnnotatedMaybe<TaggedAmounts> table_to_tagged_amounts_shortcut(
      CSV::Table const& table) {
    logger::scope_logger log_raii{logger::development_trace,
      "table_to_tagged_amounts_shortcut(table)"};

    return account::statement::monadic::to_statement_id_ed_step(table)
      .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
      .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

  } // table_to_tagged_amounts_shortcut

  AnnotatedMaybe<TaggedAmounts> csv_to_tagged_amounts_shortcut(
      std::string_view csv_text) {
    logger::scope_logger log_raii{logger::development_trace,
      "csv_to_tagged_amounts_shortcut(csv_text)"};

    return persistent::in::text::monadic::injected_string_to_istream_ptr(std::string(csv_text))
      .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
      .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
      .and_then(text::encoding::monadic::to_with_detected_encoding_step)
      .and_then(text::encoding::monadic::to_platform_encoded_string_step)
      .and_then(CSV::parse::monadic::csv_text_to_table_step)
      .and_then(account::statement::monadic::to_statement_id_ed_step)
      .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
      .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

  } // csv_to_tagged_amounts_shortcut

  AnnotatedMaybe<AccountStatement> path_to_account_statement_shortcut(
      std::filesystem::path const& file_path) {
    logger::scope_logger log_raii{logger::development_trace,
      "path_to_account_statement_shortcut(file_path)"};

    return persistent::in::text::monadic::path_to_istream_ptr_step(file_path)
      .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
      .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
      .and_then(text::encoding::monadic::to_with_detected_encoding_step)
      .and_then(text::encoding::monadic::to_platform_encoded_string_step)
      .and_then(CSV::parse::monadic::csv_text_to_table_step)
      .and_then(account::statement::monadic::to_statement_id_ed_step)
      .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step);

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
    auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(text);

    if (!maybe_table) {
      result.push_message("Pipeline failed at Step 6: CSV parsing failed - Could not parse text as CSV");
      return result;
    }

    result.push_message(std::format("Step 6 complete: CSV parsed successfully ({} rows)",
      maybe_table->rows.size()));

    // ============================================================
    // Step 6.5: CSV::Table -> MDTable<AccountID>
    // ============================================================
    auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(*maybe_table);

    if (!maybe_statement_id_ed) {
      // Unknown format - fully unknown AccountID (no prefix, no value)
      result.push_message("Step 6.5 failed: Unknown CSV format - could not identify account");
      return result;
    }

    result.push_message(std::format("(3) Step 6.5 complete: AccountID detected: '{}'",
      maybe_statement_id_ed->meta.account_id.to_string()));

    // ============================================================
    // Step 7: MDTable<AccountID> -> AccountStatement
    // ============================================================
    auto maybe_statement = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);

    if (!maybe_statement) {
      result.push_message("Pipeline failed at Step 7: Domain transformation failed - Could not extract account statement");
      return result;
    }

    result.m_value = std::move(*maybe_statement);
    result.push_message(std::format("Pipeline complete: AccountStatement with {} entries created from '{}'",
      result.value().entries().size(),
      file_path.filename().string()));

    return result;
  } // path_to_account_statement_shortcut

  AnnotatedMaybe<TaggedAmounts> path_to_tagged_amounts_shortcut(
      std::filesystem::path const& file_path) {
    logger::scope_logger log_raii{logger::development_trace,
      "csv::path_to_tagged_amounts_shortcut(file_path)"};

    return persistent::in::text::monadic::path_to_istream_ptr_step(file_path)
      .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
      .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
      .and_then(text::encoding::monadic::to_with_detected_encoding_step)
      .and_then(text::encoding::monadic::to_platform_encoded_string_step)
      .and_then(CSV::parse::monadic::csv_text_to_table_step)
      .and_then(account::statement::monadic::to_statement_id_ed_step)
      .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
      .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

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
    auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(text);

    if (!maybe_table) {
      result.push_message("Pipeline failed at Step 6: CSV parsing failed - Could not parse text as CSV");
      return result;
    }

    result.push_message(std::format("Step 6 complete: CSV parsed successfully ({} rows)",
      maybe_table->rows.size()));

    // ============================================================
    // Step 6.5: CSV::Table -> MDTable<AccountID>
    // ============================================================
    auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(*maybe_table);

    if (!maybe_statement_id_ed) {
      // Unknown format - fully unknown AccountID (no prefix, no value)
      result.push_message("Step 6.5 failed: Unknown CSV format - could not identify account");
      return result;
    }

    AccountID const& account_id = maybe_statement_id_ed->meta.account_id;
    CSV::Table const& identified_table = maybe_statement_id_ed->defacto;
    result.push_message(std::format("(4) Step 6.5 complete: AccountID detected: '{}'",
      account_id.to_string()));

    auto maybe_tagged = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed)
        .and_then(tas::maybe::account_statement_to_tagged_amounts_step);

    if (!maybe_tagged) {
      result.push_message("Pipeline failed at Steps 7-8: Domain transformation failed - Could not extract tagged amounts");
      return result;
    }

    result.m_value = std::move(*maybe_tagged);
    result.push_message(std::format("Pipeline complete: {} TaggedAmounts created from '{}'",
      result.value().size(),
      file_path.filename().string()));

    return result;
  } // path_to_tagged_amounts_shortcut

} // csv
