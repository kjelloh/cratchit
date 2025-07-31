#pragma once

#include "text/encoding.hpp"
#include "csv/projections.hpp"
#include <filesystem>

namespace CSV {

  struct ParseCSVResult {
    encoding::icu::EncodingDetectionResult icu_detection_result;
    CSV::project::HeadingId heading_id;
    CSV::OptionalTable maybe_table;
  };

  std::string encoding_caption(encoding::icu::EncodingDetectionResult const& detection_result);
  ParseCSVResult try_parse_csv(std::filesystem::path const& m_file_path);

}