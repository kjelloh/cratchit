#pragma once

#include "text/encoding.hpp"
#include "csv/projections.hpp"
#include "io/file_reader.hpp"
#include <filesystem>

namespace CSV {

  struct ParseCSVResult {
    text::encoding::icu::EncodingDetectionResult icu_detection_result;
    CSV::project::HeadingId heading_id;
    CSV::OptionalTable maybe_table;
  };

  // 'Older' csv file path -> CSV::Table result
  std::string encoding_caption(text::encoding::icu::EncodingDetectionResult const& detection_result);
  ParseCSVResult try_parse_csv(std::filesystem::path const& file_path);

  // 'Newer' csv file path -> Table
  AnnotatedMaybe<CSV::Table> file_to_table(std::filesystem::path const& file_path);

}