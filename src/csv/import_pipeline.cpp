
#include "import_pipeline.hpp"

namespace csv {

  namespace monadic {
  } // monadic

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
