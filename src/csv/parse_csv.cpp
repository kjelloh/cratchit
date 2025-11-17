
#include "parse_csv.hpp"
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
      result.icu_detection_result = text::encoding::icu::EncodingDetector::detect_file_encoding(file_path);
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
            
      // switch (result.icu_detection_result.encoding) {
      //   case text::encoding::DetectedEncoding::UTF8: {
      //     text::encoding::UTF8::istream utf8_in{ifs};
      //     field_rows = CSV::to_field_rows(utf8_in, ';');
      //     break;
      //   }
        
      //   case text::encoding::DetectedEncoding::ISO_8859_1: {
      //     text::encoding::ISO_8859_1::istream iso8859_in{ifs}; 
      //     field_rows = CSV::to_field_rows(iso8859_in, ';');
      //     break;
      //   }
        
      //   case text::encoding::DetectedEncoding::CP437: {
      //     text::encoding::CP437::istream cp437_in{ifs};
      //     field_rows = CSV::to_field_rows(cp437_in, ';');
      //     break;
      //   }
        
      //   default: {
      //     spdlog::error("Unsupported encoding {} for CSV parsing: {}", 
      //                   result.icu_detection_result.display_name, file_path.string());
      //   }
      // }
      
      if (field_rows && !field_rows->empty()) {
        logger::development_trace("try_parse_csv: field_rows->size() = {}",field_rows->size());
        result.heading_id = CSV::project::to_csv_heading_id(field_rows->at(0));
        auto heading_projection = CSV::project::make_heading_projection(result.heading_id);
        result.maybe_table = CSV::to_table(field_rows,heading_projection);
      }      
      else {
        logger::development_trace("try_parse_csv: NO field_rows -> nullopt table");
      }
    } catch (const std::exception& e) {
      spdlog::error("Failed to parse CSV file {}: {}", file_path.string(), e.what());
    }    
    return result;
  }

}