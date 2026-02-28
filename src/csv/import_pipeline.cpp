
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

  } // path_to_tagged_amounts_shortcut

} // csv
