#pragma once

#include "text/encoding_pipeline.hpp"
#include "csv/parse_csv.hpp"
#include "csv/csv_to_statement_id_ed.hpp"
#include "domain/csv_to_account_statement.hpp"
#include "domain/account_statement_to_tagged_amounts.hpp"
#include "functional/maybe.hpp"
#include "logger/log.hpp"
#include <filesystem>
#include <format>

namespace csv {

  namespace monadic {


  } // monadic

  AnnotatedMaybe<TaggedAmounts> csv_to_tagged_amounts_shortcut(
      std::string_view csv_text);

  AnnotatedMaybe<TaggedAmounts> table_to_tagged_amounts_shortcut(
      CSV::Table const& table);

  AnnotatedMaybe<AccountStatement> path_to_account_statement_shortcut(
      std::filesystem::path const& file_path);

  AnnotatedMaybe<TaggedAmounts> path_to_tagged_amounts_shortcut(
      std::filesystem::path const& file_path);

} // csv
