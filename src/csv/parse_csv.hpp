#pragma once

#include "text/encoding.hpp"
#include "csv/projections.hpp"
#include "persistent/in/raw_text_read.hpp"
#include "text/to_inferred_encoding.hpp"
#include "logger/log.hpp"
#include <filesystem>

namespace CSV {

  namespace parse {

    namespace maybe {

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
      * Parse a single CSV row (line) into fields.
      */
      inline std::vector<std::string> csv_row_to_fields(std::string_view text, size_t& pos, char delimiter) {
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
              if (pos >= text.size()) {
                fields.push_back(""); // Trailing delimiter => one more empty field
              }
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

      std::optional<CSV::Table> csv_to_table(std::string_view csv_text, char delim = ';');
      std::optional<CSV::Table> csv_text_to_table_step(std::string_view csv_text);

    } // maybe

    namespace monadic {
      AnnotatedMaybe<CSV::Table> csv_text_to_table_step(std::string_view csv_text);
      
    }

  }


}