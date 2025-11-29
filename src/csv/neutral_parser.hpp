#pragma once

#include "csv/csv.hpp"
#include "logger/log.hpp" // logger::...
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <algorithm>
#include <ranges>

namespace CSV {

  namespace parse {

    /**
    * CSV Parser /refactored variant fall of 2025)
    *
    */

    namespace monadic {

      /**
      * Detect the delimiter used in CSV text.
      *
      * Strategy: Count occurrences of semicolon and comma in the first line.
      * The character with more occurrences is likely the delimiter.
      */
      inline char to_csv_delimiter(std::string_view text) {
        // Find first line
        auto first_newline = text.find('\n');
        auto first_line = first_newline != std::string_view::npos
                          ? text.substr(0, first_newline)
                          : text;

        // Count delimiters (ignore quoted content for simple detection)
        size_t semicolon_count = std::ranges::count(first_line, ';');
        size_t comma_count = std::ranges::count(first_line, ',');

        // Return the more common delimiter, default to semicolon
        return (comma_count > semicolon_count) ? ',' : ';';
      }

      /**
      * Parse a single CSV field, handling quotes and escapes.
      *
      * CSV Quoting Rules:
      *   - Fields may be quoted with double quotes: "field content"
      *   - Quotes within quoted fields are escaped by doubling: "text with ""quote"""
      *   - Quoted fields can contain delimiters and newlines
      *   - Unquoted fields end at delimiter or newline
      */
      inline std::string to_consumed_unquoted_field(std::string_view text, size_t& pos, char delimiter) {
        std::string field;

        if (pos >= text.size()) {
          return field;
        }

        // Check if field is quoted
        bool is_quoted = (text[pos] == '"');

        if (is_quoted) {
          pos++; // Skip opening quote

          // Parse quoted field
          while (pos < text.size()) {
            char ch = text[pos];

            if (ch == '"') {
              // Check if it's an escaped quote (doubled)
              if (pos + 1 < text.size() && text[pos + 1] == '"') {
                // Escaped quote - add one quote to field
                field += '"';
                pos += 2;
              } else {
                // End of quoted field
                pos++; // Skip closing quote

                // Skip to delimiter or newline
                while (pos < text.size() && text[pos] != delimiter && text[pos] != '\n' && text[pos] != '\r') {
                  pos++;
                }
                break;
              }
            } else {
              field += ch;
              pos++;
            }
          }
        } else {
          // Parse unquoted field
          while (pos < text.size()) {
            char ch = text[pos];

            if (ch == delimiter || ch == '\n' || ch == '\r') {
              break;
            }

            field += ch;
            pos++;
          }
        }

        return field;
      }

      /**
      * Parse a single CSV row (line) into a vector of fields.
      *
      * @param text Full CSV text
      * @param pos Current position in text (updated to start of next row)
      * @param delimiter Field delimiter (',' or ';')
      * @return Vector of field strings for this row
      */
      inline std::vector<std::string> parse_row(std::string_view text, size_t& pos, char delimiter) {
        std::vector<std::string> fields;

        if (pos >= text.size()) {
          return fields;
        }

        // Parse fields until end of line
        while (pos < text.size()) {
          // Parse one field
          fields.push_back(to_consumed_unquoted_field(text, pos, delimiter));

          // Check what comes after the field
          if (pos < text.size()) {
            char ch = text[pos];

            if (ch == delimiter) {
              pos++; // Skip delimiter, continue to next field
            } else if (ch == '\r') {
              pos++; // Skip \r
              if (pos < text.size() && text[pos] == '\n') {
                pos++; // Skip \n in \r\n
              }
              break; // End of row
            } else if (ch == '\n') {
              pos++; // Skip \n
              break; // End of row
            } else {
              // Unexpected character - this shouldn't happen with correct parsing
              pos++;
            }
          }
        }

        return fields;
      }

      /**
      * Parse CSV text into a Table structure.
      *
      */
      inline std::optional<CSV::Table> csv_to_table(std::string_view csv_text, char delim = ';') {
        logger::scope_logger log_raii{logger::development_trace, "CSV::parse::monadic::csv_to_table(string_view)"};
        // Handle empty input
        if (csv_text.empty()) {
          return std::nullopt;
        }

        // Parse all rows
        std::vector<std::vector<std::string>> all_rows;
        size_t pos = 0;

        while (pos < csv_text.size()) {
          // Skip empty lines
          while (pos < csv_text.size() && (csv_text[pos] == '\n' || csv_text[pos] == '\r')) {
            pos++;
          }

          if (pos >= csv_text.size()) {
            break;
          }

          auto row = parse_row(csv_text, pos, delim);

          // Only add non-empty rows
          if (!row.empty()) {
            all_rows.push_back(std::move(row));
          }
        }

        // Need at least one row (header)
        if (all_rows.empty()) {
          return std::nullopt;
        }

        // Create Table structure
        CSV::Table table;

        // First row is the heading
        // Convert vector<string> to Key::Path (which is what FieldRow/Heading is)
        table.heading = Key::Path{all_rows[0]};

        // Convert remaining rows
        for (size_t i = 0; i < all_rows.size(); ++i) {
          table.rows.push_back(Key::Path{all_rows[i]});
        }

        logger::development_trace("Returns table with size:{}",table.rows.size());
        return table;
      }

      // /**
      //  * Parse CSV text into a Table structure (string overload for convenience).
      //  *
      //  * @param csv_text UTF-8 encoded CSV text
      //  * @param delimiter Optional delimiter (auto-detected if not specified)
      //  * @return Optional CSV::Table, empty on parse failure
      //  */
      // inline std::optional<CSV::Table> csv_to_table(std::string const& csv_text, std::optional<char> delimiter = std::nullopt) {
      //   return csv_to_table(std::string_view{csv_text}, delimiter);
      // }

      // 'Liftable' parse function (clean text -> optional<Table>)
      inline std::optional<CSV::Table> text_to_table(std::string_view csv_text) {
        return csv_to_table(csv_text,to_csv_delimiter(csv_text));
      } 

    }
    
  } // parse

} // CSV
