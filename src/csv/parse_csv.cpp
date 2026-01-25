
#include "parse_csv.hpp"
#include "text/encoding_pipeline.hpp" // path_to_platform_encoded_string_shortcut,...
#include "logger/log.hpp"
#include "std_overload.hpp" // std_overload::overload,...
#include <fstream>

namespace CSV {

  namespace parse {

    namespace deprecated {
      std::string encoding_caption(text::encoding::icu_facade::EncodingDetectionResult const& detection_result) {
        // Format display string with confidence and method
        std::string result;
        if (detection_result.meta.confidence >= 70) {
          result = std::format(
             "{} ({}%)"
            ,detection_result.meta.display_name
            ,detection_result.meta.confidence);
        } else {
          result = std::format(
             "{} ({}%, {})"
            ,detection_result.meta.display_name
            ,detection_result.meta.confidence
            ,detection_result.meta.detection_method);
        }
        
        return result;
      }

      ParseCSVResult try_parse_csv(std::filesystem::path const& file_path) {
        ParseCSVResult result{};
        
        try {
                
          // Use ICU detection to determine appropriate encoding stream

          if (auto maybe_detection_result = text::encoding::icu_facade::maybe::to_file_at_path_encoding(file_path)) {
            result.icu_detection_result = maybe_detection_result.value();

              logger::development_trace("try_parse_csv: icu_detection_result:{}",result.icu_detection_result.meta.display_name);

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
                    ,result.icu_detection_result.meta.display_name, file_path.string());
                  return {};
                });
              
              if (field_rows && !field_rows->empty()) {
                logger::development_trace("try_parse_csv: field_rows->size() = {}",field_rows->size());
                result.heading_id = CSV::project::deprecated::to_csv_heading_id(field_rows->at(0));
                auto heading_projection = CSV::project::deprecated::make_heading_projection(result.heading_id);
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

    } // deprecated

    namespace maybe {

      std::optional<CSV::Table> csv_to_table(std::string_view csv_text, char delim) {
        logger::scope_logger log_raii{logger::development_trace, "CSV::parse::maybe::csv_to_table(string_view)"};
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

          auto row = csv_row_to_fields(csv_text, pos, delim);

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
      } // csv_to_table

      std::optional<CSV::Table> csv_text_to_table_step(std::string_view csv_text) {
        return csv_to_table(csv_text,to_csv_delimiter(csv_text));
      } // csv_text_to_table_step

    } // maybe

    namespace monadic {

      AnnotatedMaybe<CSV::Table> path_to_table_shortcut(std::filesystem::path const& file_path) {

        using cratchit::functional::to_annotated_maybe_f;
        return text::encoding::path_to_platform_encoded_string_shortcut(file_path)
          .and_then(to_annotated_maybe_f(
            CSV::parse::maybe::csv_text_to_table_step
            ,"Failed to parse csv into a valid table"));
      }

      AnnotatedMaybe<CSV::Table> csv_text_to_table_step(std::string_view csv_text) {
        using cratchit::functional::to_annotated_maybe_f;

        auto f =  to_annotated_maybe_f(
           CSV::parse::maybe::csv_text_to_table_step
          ,"Failed to parse csv into a valid table");

        return f(csv_text);

      }

    }

  } // parse


} // CSV