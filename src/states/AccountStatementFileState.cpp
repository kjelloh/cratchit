#include "AccountStatementFileState.hpp"
#include "TaggedAmountsState.hpp"
#include "AccountStatementState.hpp"
#include "csv/projections.hpp" // CSV::project::to_account_statement,...
#include "csv/csv_to_account_id.hpp" // CSV::project::to_account_id_ed
#include "domain/csv_to_account_statement.hpp" // domain::md_table_to_account_statement
#include "functional/maybe.hpp" // to_annotated_nullopt, AnnotatedMaybe
#include <format>
#include <fstream>
#include "logger/log.hpp"

namespace first {
  
  AccountStatementFileState::AccountStatementFileState(PeriodPairedFilePath period_paired_file_path)
    :  StateImpl{}
      ,m_maybe_table_result{CSV::parse::monadic::file_to_table(period_paired_file_path.content())}
      ,m_period_paired_file_path{period_paired_file_path} {}

  std::string AccountStatementFileState::caption() const {
    if (not m_caption.has_value()) {
      m_caption = std::format(
         "File: {} [{}]"
        ,m_period_paired_file_path.content().filename().string()
        ,m_maybe_table_result.to_caption());
    }
    return m_caption.value();
  }

  StateImpl::UpdateOptions AccountStatementFileState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    auto fiscal_period = this->m_period_paired_file_path.period();
    if (this->m_maybe_table_result) result.add('t',{"To Tagged Amounts",[
       fiscal_period
      ,csv_heading_id = CSV::project::HeadingId{}
      ,maybe_table = this->m_maybe_table_result.m_value]() -> StateUpdateResult {

      return {std::nullopt, [fiscal_period,csv_heading_id,maybe_table]() -> std::optional<Msg> {
        auto maybe_tas = maybe_table
          .and_then([&csv_heading_id](auto const& table) {
            return CSV::project::to_tas(csv_heading_id,table);
          });
        State new_state{};
        // Hack - 'glue' optional date ordered tagged amounts with
        //        TaggedAmountsState aggregating raw tagged amounts
        if (maybe_tas) {
          new_state = make_state<TaggedAmountsState>(
             TaggedAmountsState::TaggedAmountsSlice{fiscal_period, maybe_tas.value()});
        }
        else {
          new_state = make_state<TaggedAmountsState>(
             TaggedAmountsState::TaggedAmountsSlice{fiscal_period, TaggedAmounts{}});
        }
        return std::make_shared<PushStateMsg>(new_state);
      }};
    }});

    if (this->m_maybe_table_result) result.add('s', {"Account Statement", [
       fiscal_period
      ,annotated_table = this->m_maybe_table_result  // AnnotatedMaybe<CSV::Table>
    ]() -> StateUpdateResult {
      return {std::nullopt, [fiscal_period, annotated_table]() -> std::optional<Msg> {
        using namespace cratchit::functional;

        // Compose: AnnotatedMaybe<Table> → AnnotatedMaybe<MDTable<AccountID>> → AnnotatedMaybe<AccountStatement>
        auto annotated_statement = annotated_table
          .and_then(to_annotated_nullopt(
            CSV::project::to_account_id_ed,
            "Unknown CSV format - could not identify account"))
          .and_then(to_annotated_nullopt(
            domain::md_table_to_account_statement,
            "Could not extract account statement entries"));

        AccountStatementState::PeriodPairedAnnotatedAccountStatement period_paired{
           fiscal_period
          ,annotated_statement
        };
        State new_state = make_state<AccountStatementState>(period_paired);
        return std::make_shared<PushStateMsg>(new_state);
      }};
    }});
    
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

    result.push_back(this->m_period_paired_file_path.period().to_string());
    result.push_back(this->caption());    
    result.push_back("");
    
    if (m_maybe_table_result) {
      auto const& csv_table = m_maybe_table_result.value();
      
      // Calculate column widths for fixed-width formatting
      std::vector<size_t> column_widths{};
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
        std::string row_line{};
        for (size_t j = 0; j < std::min(row.size(), column_widths.size()); ++j) {
          if (j > 0) row_line += " | ";
          std::string field = row[j];
          if (true) {
            field = "";
            // Detect / filter control characters
            for (int ix=0;ix<row[j].size();++ix) {
              auto ch = row[j][ix];
              if (static_cast<unsigned char>(ch) < ' ') {
                field += std::format("<{}>",static_cast<unsigned char>(ch));
              }
              else {
                field.push_back(ch);
              }
            }
          }
          else {
          }
          // Truncate field if too long
          if (field.length() > column_widths[j]) {
            field = field.substr(0, column_widths[j] - 3) + "...";
          }
          row_line += std::format("{:<{}}",field, column_widths[j]);
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

}