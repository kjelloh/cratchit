#include "AccountStatementState.hpp"
#include "DeleteItemState.hpp"
#include "msgs/msg.hpp"
#include "logger/log.hpp"
#include <format>
#include <variant>

namespace first {

  AccountStatementState::AccountStatementState(ExpectedAccountStatement expected_account_statement) 
    :  StateImpl()
      ,m_expected_account_statement{expected_account_statement} {}

  std::string AccountStatementState::caption() const {
    return "Account Statement";
  }

  StateImpl::UX AccountStatementState::create_ux() const {
    UX result{};
    result.push_back(this->caption());
    result.push_back("");
    
    if (m_expected_account_statement) {
      const auto& statement = m_expected_account_statement.value();
      const auto& entries = statement.entries();
      
      if (entries.empty()) {
        result.push_back("No entries in account statement");
        return result;
      }
      
      // Define column headers and widths
      std::vector<std::string> headers = {"Type", "Description", "Amount", "Tags"};
      std::vector<size_t> column_widths = {8, 30, 15, 25};  // Adjusted widths
      
      // Display formatted heading
      std::string heading_line;
      for (size_t i = 0; i < headers.size(); ++i) {
        if (i > 0) heading_line += " | ";
        heading_line += std::format("{:<{}}", headers[i], column_widths[i]);
      }
      result.push_back(heading_line);
      
      // Add separator line
      std::string separator_line;
      for (size_t i = 0; i < column_widths.size(); ++i) {
        if (i > 0) separator_line += "-+-";
        separator_line += std::string(column_widths[i], '-');
      }
      result.push_back(separator_line);
      
      // Display entries
      size_t max_entries = std::min(size_t(20), entries.size());
      for (size_t i = 0; i < max_entries; ++i) {
        const auto& entry = entries[i];
        std::string row_line;
        
        std::visit([&](const auto& variant_entry) {
          using T = std::decay_t<decltype(variant_entry)>;
          using DeltaType = ::Delta<TaggedAmount>;
          using StateType = ::State<TaggedAmount>;
          
          std::string type_str;
          std::string desc_str;
          std::string amount_str;
          std::string tags_str;
          
          if constexpr (std::is_same_v<T, DeltaType>) {
            type_str = "Delta";
            const TaggedAmount& ta = variant_entry.m_t;
            desc_str = to_string(ta);
            
            // Extract amount and tags from TaggedAmount string representation
            // This is a simplified extraction - you may want to improve this
            size_t pos = desc_str.find_first_of("0123456789-");
            if (pos != std::string::npos) {
              size_t end_pos = desc_str.find(' ', pos);
              if (end_pos != std::string::npos) {
                amount_str = desc_str.substr(pos, end_pos - pos);
                tags_str = desc_str.substr(end_pos + 1);
              } else {
                amount_str = desc_str.substr(pos);
              }
              desc_str = desc_str.substr(0, pos);
            }
          } 
          else if constexpr (std::is_same_v<T, StateType>) {
            type_str = "State";
            const TaggedAmount& ta = variant_entry.m_t;
            desc_str = to_string(ta);
            
            // Similar extraction for State
            size_t pos = desc_str.find_first_of("0123456789-");
            if (pos != std::string::npos) {
              size_t end_pos = desc_str.find(' ', pos);
              if (end_pos != std::string::npos) {
                amount_str = desc_str.substr(pos, end_pos - pos);
                tags_str = desc_str.substr(end_pos + 1);
              } else {
                amount_str = desc_str.substr(pos);
              }
              desc_str = desc_str.substr(0, pos);
            }
          }
          
          // Truncate strings to fit column widths
          if (type_str.length() > column_widths[0]) {
            type_str = type_str.substr(0, column_widths[0] - 3) + "...";
          }
          if (desc_str.length() > column_widths[1]) {
            desc_str = desc_str.substr(0, column_widths[1] - 3) + "...";
          }
          if (amount_str.length() > column_widths[2]) {
            amount_str = amount_str.substr(0, column_widths[2] - 3) + "...";
          }
          if (tags_str.length() > column_widths[3]) {
            tags_str = tags_str.substr(0, column_widths[3] - 3) + "...";
          }
          
          // Format the row
          row_line = std::format("{:<{}} | {:<{}} | {:<{}} | {:<{}}", 
                                type_str, column_widths[0],
                                desc_str, column_widths[1], 
                                amount_str, column_widths[2],
                                tags_str, column_widths[3]);
        }, entry);
        
        result.push_back(row_line);
      }
      
      if (entries.size() > max_entries) {
        result.push_back(std::format("... ({} more entries)", entries.size() - max_entries));
      }
      
      result.push_back("");
      result.push_back(std::format("Total entries: {}", entries.size()));
      
      if (statement.account_descriptor()) {
        result.push_back(std::format("Account: {}", *statement.account_descriptor()));
      }
    }
    else {
      result.push_back(std::format("Sorry - {}",m_expected_account_statement.error()));
    }
    return result;
  }

  StateImpl::UpdateOptions AccountStatementState::create_update_options() const {
    StateImpl::UpdateOptions result{};

    result.add('-', {"Back", []() -> StateUpdateResult {
      using StringMsg = CargoMsgT<std::string>;
      return {std::nullopt, []() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>(
          std::make_shared<StringMsg>("AccountStatementState")
        );
      }};
    }});

    return result;
  }

} // namespace first