
#include "parse_csv.hpp"
#include "text/encoding_pipeline.hpp" // text::encoding::read_file_with_encoding_detection 
#include "csv/neutral_parser.hpp" // CSV::neutral::parse_csv
#include "logger/log.hpp"
#include "std_overload.hpp" // std_overload::overload,...
#include <fstream>

namespace CSV {

  std::string encoding_caption(text::encoding::icu::EncodingDetectionResult const& detection_result) {
    // Format display string with confidence and method
    std::string result;
    if (detection_result.confidence >= 70) {
      result = std::format("{} ({}%)", detection_result.display_name, detection_result.confidence);
    } else {
      result = std::format("{} ({}%, {})", detection_result.display_name, detection_result.confidence, detection_result.detection_method);
    }
    
    // Add language info if detected
    if (detection_result.language.empty()) {
      result += std::format(" [{}]", detection_result.language);
    }    
    return result;
  }

  ParseCSVResult try_parse_csv(std::filesystem::path const& file_path) {
    ParseCSVResult result{};
    
    try {
            
      // Use ICU detection to determine appropriate encoding stream

      // result.icu_detection_result = text::encoding::icu::to_file_at_path_encoding(file_path);
      if (auto maybe_detection_result = text::encoding::icu::to_file_at_path_encoding(file_path)) {
        result.icu_detection_result = maybe_detection_result.value();

          logger::development_trace("try_parse_csv: icu_detection_result:{}",result.icu_detection_result.display_name);

          logger::development_trace("try_parse_csv: BEGIN");
          std::ifstream ifs{file_path};
          if (!ifs.is_open()) {
            spdlog::error("Failed to open file: {}", file_path.string());
            return result;
          }

          CSV::OptionalFieldRows field_rows = text::encoding::to_decoding_in(result.icu_detection_result,ifs)
            .and_then(decoding_in_to_field_rows)
            .or_else([&result,&file_path] -> CSV::OptionalFieldRows {
              spdlog::error(
                "Unsupported encoding {} for CSV parsing: {}"
                ,result.icu_detection_result.display_name, file_path.string());
              return {};
            });
          
          if (field_rows && !field_rows->empty()) {
            logger::development_trace("try_parse_csv: field_rows->size() = {}",field_rows->size());
            result.heading_id = CSV::project::to_csv_heading_id(field_rows->at(0));
            auto heading_projection = CSV::project::make_heading_projection(result.heading_id);
            result.maybe_table = CSV::to_table(field_rows,heading_projection);
          }      
          else {
            logger::development_trace("try_parse_csv: NO field_rows -> nullopt table");
          }
      }
    } 
    catch (const std::exception& e) {
      spdlog::error("Failed to parse CSV file {}: {}", file_path.string(), e.what());
    }    
    return result;
  }

  // 'Newer' csv file path -> Table
  AnnotatedMaybe<CSV::Table> file_to_table(std::filesystem::path const& file_path) {

    using cratchit::functional::to_annotated_nullopt;
    return text::encoding::read_file_with_encoding_detection(file_path)
      .and_then(to_annotated_nullopt(
         CSV::neutral::text_to_table
        ,"Failed to parse csv into a valid table"));
  }

} // CSV