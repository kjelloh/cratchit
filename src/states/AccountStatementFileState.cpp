#include "AccountStatementFileState.hpp"
#include <format>
#include <fstream>
#include "spdlog/spdlog.h"

namespace first {
  
  AccountStatementFileState::AccountStatementFileState(std::filesystem::path file_path)
    : StateImpl{}, m_file_path{std::move(file_path)} {}

  AccountStatementFileState::AccountStatementFileState(std::optional<std::string> caption, std::filesystem::path file_path)
    : StateImpl{caption}, m_file_path{std::move(file_path)} {}

  std::string AccountStatementFileState::caption() const {
    if (m_caption.has_value()) {
      return m_caption.value();
    }
    return std::format("CSV File: {}", m_file_path.filename().string());
  }

  StateImpl::UpdateOptions AccountStatementFileState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    
    // Add '-' key option for back navigation
    result.add('-', {"Back", []() -> StateUpdateResult {
      using StringMsg = CargoMsgT<std::string>;
      return {std::nullopt, []() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>(
          std::make_shared<StringMsg>("AccountStatementFileState")
        );
      }};
    }});
    
    return result;
  }

  StateImpl::UX AccountStatementFileState::create_ux() const {
    UX result{};
    
    auto encoding = detect_encoding();
    result.push_back(std::format("File: {} [{}]", m_file_path.filename().string(), encoding));
    result.push_back("");
    
    auto table = parse_csv_content();
    if (table && table.has_value()) {
      auto& csv_table = table.value();
      
      // Calculate column widths for fixed-width formatting
      std::vector<size_t> column_widths;
      if (csv_table.heading.size() > 0) {
        column_widths.resize(csv_table.heading.size());
        
        // Initialize with heading widths
        for (size_t i = 0; i < csv_table.heading.size(); ++i) {
          column_widths[i] = csv_table.heading[i].length();
        }
        
        // Check all rows to find maximum width per column
        size_t max_display_rows = std::min(size_t(10), csv_table.rows.size());
        for (size_t row_idx = 0; row_idx < max_display_rows; ++row_idx) {
          auto& row = csv_table.rows[row_idx];
          for (size_t col_idx = 0; col_idx < std::min(row.size(), column_widths.size()); ++col_idx) {
            size_t field_length = std::min(row[col_idx].length(), size_t(25)); // Cap at 25 chars
            column_widths[col_idx] = std::max(column_widths[col_idx], field_length);
          }
        }
        
        // Cap column widths at reasonable maximum
        for (auto& width : column_widths) {
          width = std::min(width, size_t(25));
        }
        
        // Display formatted heading
        std::string heading_line;
        for (size_t i = 0; i < csv_table.heading.size(); ++i) {
          if (i > 0) heading_line += " | ";
          std::string truncated_heading = csv_table.heading[i];
          if (truncated_heading.length() > column_widths[i]) {
            truncated_heading = truncated_heading.substr(0, column_widths[i] - 3) + "...";
          }
          heading_line += std::format("{:<{}}", truncated_heading, column_widths[i]);
        }
        result.push_back(heading_line);
        
        // Add separator line
        std::string separator_line;
        for (size_t i = 0; i < column_widths.size(); ++i) {
          if (i > 0) separator_line += "-+-";
          separator_line += std::string(column_widths[i], '-');
        }
        result.push_back(separator_line);
      }
      
      // Display formatted rows
      size_t max_rows = std::min(size_t(10), csv_table.rows.size());
      for (size_t i = 0; i < max_rows; ++i) {
        auto& row = csv_table.rows[i];
        std::string row_line;
        for (size_t j = 0; j < std::min(row.size(), column_widths.size()); ++j) {
          if (j > 0) row_line += " | ";
          std::string field = row[j];
          // Truncate field if too long
          if (field.length() > column_widths[j]) {
            field = field.substr(0, column_widths[j] - 3) + "...";
          }
          row_line += std::format("{:<{}}", field, column_widths[j]);
        }
        result.push_back(row_line);
      }
      
      if (csv_table.rows.size() > max_rows) {
        result.push_back(std::format("... ({} more rows)", csv_table.rows.size() - max_rows));
      }
      
      result.push_back("");
      result.push_back(std::format("Total rows: {}", csv_table.rows.size()));
    } else {
      result.push_back("Failed to parse CSV content");
    }
    
    return result;
  }

  std::string AccountStatementFileState::detect_encoding() const {
    if (m_cached_encoding.has_value()) {
      return *m_cached_encoding;
    }
    
    // Use ICU-based encoding detection
    auto detection_result = encoding::icu::EncodingDetector::detect_file_encoding(m_file_path);
    
    // Format display string with confidence and method
    std::string encoding_display;
    if (detection_result.confidence >= 70) {
      encoding_display = std::format("{} ({}%)", detection_result.display_name, detection_result.confidence);
    } else {
      encoding_display = std::format("{} ({}%, {})", detection_result.display_name, detection_result.confidence, detection_result.detection_method);
    }
    
    // Add language info if detected
    if (!detection_result.language.empty()) {
      encoding_display += std::format(" [{}]", detection_result.language);
    }
    
    m_cached_encoding = encoding_display;
    return encoding_display;
  }

  CSV::OptionalTable AccountStatementFileState::parse_csv_content() const {
    if (m_cached_table.has_value()) {
      return *m_cached_table;
    }
    
    CSV::OptionalTable result;
    
    try {
      std::ifstream ifs{m_file_path};
      if (!ifs.is_open()) {
        spdlog::error("Failed to open file: {}", m_file_path.string());
        m_cached_table = result;
        return result;
      }
      
      CSV::OptionalFieldRows field_rows;
      
      // Use ICU detection to determine appropriate encoding stream
      auto detection_result = encoding::icu::EncodingDetector::detect_file_encoding(m_file_path);
      
      switch (detection_result.encoding) {
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
                        detection_result.display_name, m_file_path.string());
          m_cached_table = result; // Cache the failure
          return result; // Return nullopt - explicit failure
        }
      }
      
      if (field_rows && !field_rows->empty()) {
        // Create a generic heading function that works for any CSV structure
        auto to_heading = [](CSV::FieldRow const& field_row) -> CSV::OptionalTableHeading {
          if (field_row.size() > 0) {
            return field_row; // Use first row as heading
          }
          return std::nullopt;
        };
        
        result = CSV::to_table(field_rows, to_heading);
      }
      
    } catch (const std::exception& e) {
      spdlog::error("Failed to parse CSV file {}: {}", m_file_path.string(), e.what());
    }
    
    m_cached_table = result;
    return result;
  }

}