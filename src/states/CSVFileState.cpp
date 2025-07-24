#include "CSVFileState.hpp"
#include <format>
#include <fstream>
#include "spdlog/spdlog.h"

namespace first {
  
  CSVFileState::CSVFileState(std::filesystem::path file_path)
    : StateImpl{}, m_file_path{std::move(file_path)} {}

  CSVFileState::CSVFileState(std::optional<std::string> caption, std::filesystem::path file_path)
    : StateImpl{caption}, m_file_path{std::move(file_path)} {}

  std::string CSVFileState::caption() const {
    if (m_caption.has_value()) {
      return m_caption.value();
    }
    return std::format("CSV File: {}", m_file_path.filename().string());
  }

  StateImpl::UpdateOptions CSVFileState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    
    // Add '-' key option for back navigation
    result.add('-', {"Back", []() -> StateUpdateResult {
      using StringMsg = CargoMsgT<std::string>;
      return {std::nullopt, []() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>(
          std::make_shared<StringMsg>("CSVFileState")
        );
      }};
    }});
    
    return result;
  }

  StateImpl::UX CSVFileState::create_ux() const {
    UX result{};
    
    auto encoding = detect_encoding();
    result.push_back(std::format("File: {} [{}]", m_file_path.filename().string(), encoding));
    result.push_back("");
    
    auto table = parse_csv_content();
    if (table && table.has_value()) {
      auto& csv_table = table.value();
      
      // Display heading
      if (csv_table.heading.size() > 0) {
        std::string heading_line;
        for (size_t i = 0; i < csv_table.heading.size(); ++i) {
          if (i > 0) heading_line += " | ";
          heading_line += csv_table.heading[i];
        }
        result.push_back(std::format("Columns: {}", heading_line));
        result.push_back("");
      }
      
      // Display first 10 rows as preview
      size_t max_rows = std::min(size_t(10), csv_table.rows.size());
      for (size_t i = 0; i < max_rows; ++i) {
        auto& row = csv_table.rows[i];
        std::string row_line;
        for (size_t j = 0; j < row.size(); ++j) {
          if (j > 0) row_line += " | ";
          // Truncate long fields
          std::string field = row[j];
          if (field.length() > 20) {
            field = field.substr(0, 17) + "...";
          }
          row_line += field;
        }
        result.push_back(std::format("Row {}: {}", i + 1, row_line));
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

  std::string CSVFileState::detect_encoding() const {
    if (m_cached_encoding.has_value()) {
      return *m_cached_encoding;
    }
    
    // Use extension-based detection following zeroth/main.cpp pattern
    std::string encoding;
    if (m_file_path.extension() == ".csv") {
      encoding = "UTF-8";
    } else if (m_file_path.extension() == ".skv") {
      encoding = "ISO-8859-1";
    } else {
      encoding = "Unknown";
    }
    
    m_cached_encoding = encoding;
    return encoding;
  }

  CSV::OptionalTable CSVFileState::parse_csv_content() const {
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
      
      // Parse based on file extension following zeroth/main.cpp pattern
      if (m_file_path.extension() == ".csv") {
        encoding::UTF8::istream utf8_in{ifs};
        field_rows = CSV::to_field_rows(utf8_in, ';');
      } else if (m_file_path.extension() == ".skv") {
        encoding::ISO_8859_1::istream iso8859_in{ifs};
        field_rows = CSV::to_field_rows(iso8859_in, ';');
      } else {
        // Default to UTF-8 for unknown extensions
        encoding::UTF8::istream utf8_in{ifs};
        field_rows = CSV::to_field_rows(utf8_in, ';');
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