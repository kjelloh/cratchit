
#include "parse_csv.hpp"
#include "logger/log.hpp"
#include <fstream>

namespace CSV {

  std::string encoding_caption(encoding::icu::EncodingDetectionResult const& detection_result) {
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

  ParseCSVResult try_parse_csv(std::filesystem::path const& m_file_path) {
    ParseCSVResult result{};
    
    try {
      std::ifstream ifs{m_file_path};
      if (!ifs.is_open()) {
        spdlog::error("Failed to open file: {}", m_file_path.string());
        return result;
      }
      
      CSV::OptionalFieldRows field_rows;
      
      // Use ICU detection to determine appropriate encoding stream
      result.icu_detection_result = encoding::icu::EncodingDetector::detect_file_encoding(m_file_path);
      
      switch (result.icu_detection_result.encoding) {
        case encoding::icu::DetectedEncoding::UTF8: {
          encoding::UTF8::istream utf8_in{ifs};
          field_rows = CSV::to_field_rows(utf8_in, ';');
          break;
        }
        
        case encoding::icu::DetectedEncoding::ISO_8859_1: {
          encoding::ISO_8859_1::istream iso8859_in{ifs}; 
          field_rows = CSV::to_field_rows(iso8859_in, ';');
          break;
        }
        
        case encoding::icu::DetectedEncoding::CP437: {
          encoding::CP437::istream cp437_in{ifs};
          field_rows = CSV::to_field_rows(cp437_in, ';');
          break;
        }
        
        default: {
          spdlog::error("Unsupported encoding {} for CSV parsing: {}", 
                        result.icu_detection_result.display_name, m_file_path.string());
        }
      }
      
      if (field_rows && !field_rows->empty()) {
        result.heading_id = CSV::project::to_csv_heading_id(field_rows->at(0));
        auto heading_projection = CSV::project::make_heading_projection(result.heading_id);
        result.maybe_table = CSV::to_table(field_rows,heading_projection);
      }      
    } catch (const std::exception& e) {
      spdlog::error("Failed to parse CSV file {}: {}", m_file_path.string(), e.what());
    }    
    return result;
  }

}