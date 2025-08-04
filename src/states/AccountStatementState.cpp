#include "AccountStatementState.hpp"
#include "DeleteItemState.hpp"
#include "msgs/msg.hpp"
#include "logger/log.hpp"
#include <format>
#include <variant>

namespace first {

  AccountStatementState::AccountStatementState(ExpectedAccountStatement expected_account_statement) 
    :  StateImpl()
      ,m_expected_account_statement{expected_account_statement}
      ,m_period_paired_expected_account_statement{
         FiscalYear::to_current_fiscal_year(std::chrono::month{5}).period()
        ,expected_account_statement} {}

  AccountStatementState::AccountStatementState(
    PeriodPairedExpectedAccountStatement period_paired_expected_account_statement)
    :  StateImpl{}
      ,m_expected_account_statement{period_paired_expected_account_statement.content()}
      ,m_period_paired_expected_account_statement{period_paired_expected_account_statement} {}

  std::string AccountStatementState::caption() const {
    return "Account Statement";
  }

  using DeltaType = ::Delta<TaggedAmount>;
  using StateType = ::State<TaggedAmount>;
  std::vector<std::string> to_elements(std::vector<std::string> headers,DeltaType const& delta) {
    std::vector<std::string> result(headers.size(),"??");
    std::map<std::string,std::function<std::string(DeltaType)>> projector = {
       {"Type",[](DeltaType const& delta){return "Trans";}}
      ,{"Date",[](DeltaType const& delta){return to_string(delta.m_t.date());}}
      ,{"Description",[](DeltaType const& delta){return delta.m_t.tag_value("Text").value_or("??");}}
      ,{"Amount",[](DeltaType const& delta){return to_string(to_units_and_cents(delta.m_t.cents_amount()));}}
      ,{"Tags",[](DeltaType const& delta){return to_string(delta.m_t.tags());}}
    };
    for (int i=0;i<headers.size();++i) {  
      if (projector.contains(headers[i])) {
        result[i] = projector[headers[i]](delta);
      }
    }
    return result;
  }
  std::vector<std::string> to_elements(std::vector<std::string> headers,StateType const& state) {
    std::vector<std::string> result(headers.size(),"??");
    std::map<std::string,std::function<std::string(StateType)>> projector = {
       {"Type",[](StateType const& state){return "Saldo";}}
      ,{"Date",[](StateType const& state){return to_string(state.m_t.date());}}
      ,{"Description",[](StateType const& state){return "";}}
      ,{"Amount",[](StateType const& state){return to_string(to_units_and_cents(state.m_t.cents_amount()));}}
      ,{"Tags",[](StateType const& state){return to_string(state.m_t.tags());}}
    };
    for (int i=0;i<headers.size();++i) {  
      if (projector.contains(headers[i])) {
        result[i] = projector[headers[i]](state);
      }
    }
    return result;
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
      std::vector<std::string> headers = {"Type","Date", "Description", "Amount", "Tags"};
      std::vector<size_t> column_widths = {8,8, 30, 15, 25};  // Adjusted widths
      
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
      size_t max_entries = std::min(size_t(5), entries.size());
      for (size_t i = 0; i < max_entries; ++i) {
        const auto& entry = entries[i];
        std::string row_line{};
        
        std::visit([&](const auto& variant_entry) {
          using T = std::decay_t<decltype(variant_entry)>;

          auto elements = to_elements(headers,variant_entry);
          
          // Truncate strings to fit column widths
          for (int i=0;i<headers.size();++i) {
            if (elements[i].length() > column_widths[i]) {
              elements[i] = elements[i].substr(0, column_widths[i] - 3) + "...";
            }
          }
          for (int i=0;i<headers.size();++i) {
            if (i > 0) row_line += " | ";
            row_line += std::format("{:<{}}",elements[i],column_widths[i]);
          }
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