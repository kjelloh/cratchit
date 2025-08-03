#include "FiscalPeriodState.hpp"
#include "HADsState.hpp"
#include "TaggedAmountsState.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include "VATReturnsState.hpp"
#include "AccountStatementFilesState.hpp"
#include "environment.hpp"
#include "logger/log.hpp"
#include <ranges>

namespace first {

  FiscalPeriodState::FiscalPeriodState(
     std::optional<std::string> caption
    ,FiscalPeriod fiscal_period
    ,HeadingAmountDateTransEntries period_hads
    ,TaggedAmounts period_tagged_amounts)
      :  StateImpl{caption}
        ,m_fiscal_period{fiscal_period}

        // 'Older' using separate period and container
        // ,m_period_hads{period_hads}
        // ,m_period_tagged_amounts{period_tagged_amounts}

        // 'newer' using PeriodSlice mechanism
        ,m_hads_slice{make_period_sliced(fiscal_period,period_hads,[](HAD const& had){return had.date;})}
        ,m_tas_slice{make_period_sliced(fiscal_period,period_tagged_amounts,[](TaggedAmount const& ta){return ta.date();})} {}

  FiscalPeriodState::FiscalPeriodState(
     FiscalPeriod fiscal_period
    ,Environment const &parent_environment_ref)
      : FiscalPeriodState(
         std::nullopt
        ,fiscal_period

        // 'Older' using separate period and container
        ,hads_from_environment(parent_environment_ref)
        ,to_tagged_amounts(parent_environment_ref)) {}
          
  std::string FiscalPeriodState::caption() const {
    if (m_caption.has_value()) {
      return m_caption.value();
    }
    return "Fiscal Period: " + m_fiscal_period.to_string();
  }

  StateUpdateResult FiscalPeriodState::update(Msg const& msg) const {
    using HADsMsg = CargoMsgT<HeadingAmountDateTransEntries>;
    using TaggedAmountsMsg = CargoMsgT<TaggedAmounts>;
    
    if (auto pimpl = std::dynamic_pointer_cast<HADsMsg>(msg); pimpl != nullptr) {
      std::optional<State> mutated_state{};
      Cmd cmd{Nop};
      spdlog::info("FiscalPeriodState::update - handling HADsMsg");
      if (pimpl->payload != this->m_hads_slice.content()) {
        // Changes has been made
        spdlog::info("FiscalPeriodState::update - HADs has changed. payload size: {}", pimpl->payload.size());
        mutated_state = make_state<FiscalPeriodState>(this->caption(), this->m_fiscal_period, pimpl->payload, this->m_tas_slice.content());
      }
      return {mutated_state, cmd};
    }
    else if (auto pimpl = std::dynamic_pointer_cast<TaggedAmountsMsg>(msg); pimpl != nullptr) {
      std::optional<State> mutated_state{};
      Cmd cmd{Nop};
      spdlog::info("FiscalPeriodState::update - handling TaggedAmountsMsg");
      if (pimpl->payload != this->m_tas_slice.content()) {
        // Changes has been made
        spdlog::info("FiscalPeriodState::update - TaggedAmounts has changed. payload size: {}", pimpl->payload.size());
        mutated_state = make_state<FiscalPeriodState>(
           this->caption()
          ,this->m_fiscal_period
          ,this->m_hads_slice.content()
          ,pimpl->payload);
      }
      return {mutated_state, cmd};
    }
    spdlog::info("FiscalPeriodState::update - didn't handle message");
    return {std::nullopt, Cmd{}}; // Didn't handle - let base dispatch use fallback
  }

  // Cargo visit/apply double dispatch removed (cargo now message passed)

  StateImpl::UpdateOptions FiscalPeriodState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    
    // Convert HADsState::cmd_option_from to update option
    result.add('h', {std::format("HADs - count:{}", this->m_hads_slice.content().size()), 
      [hads_slice = this->m_hads_slice]() -> StateUpdateResult {
        return {std::nullopt, [hads_slice]() -> std::optional<Msg> {
          State new_state = make_state<HADsState>(hads_slice);
          return std::make_shared<PushStateMsg>(new_state);
        }};
      }});
    
    // Convert VATReturnsState::cmd_option_from to update option  
    result.add('v', {"VAT Returns", []() -> StateUpdateResult {
      return {std::nullopt, []() -> std::optional<Msg> {
        State new_state = make_state<VATReturnsState>();
        return std::make_shared<PushStateMsg>(new_state);
      }};
    }});
    
    // Add AccountStatementFilesState option
    result.add('a', {"Account Statements", []() -> StateUpdateResult {
      return {std::nullopt, []() -> std::optional<Msg> {
        State new_state = make_state<AccountStatementFilesState>();
        return std::make_shared<PushStateMsg>(new_state);
      }};
    }});
    
    // Add TaggedAmountsState option
    result.add('t', {std::format("Tagged Amounts - count:{}", m_tas_slice.content().size()), 
      [period_tagged_amounts = m_tas_slice.content(), fiscal_period = m_fiscal_period]() -> StateUpdateResult {
        return {std::nullopt, [period_tagged_amounts, fiscal_period]() -> std::optional<Msg> {
          State new_state = make_state<TaggedAmountsState>(period_tagged_amounts, fiscal_period);
          return std::make_shared<PushStateMsg>(new_state);
        }};
      }});
    
    // Add '-' key option last - back with EnvironmentPeriodSlice as payload (matching get_cargo())
    result.add('-', {"Back", [this]() -> StateUpdateResult {
      using EnvironmentPeriodSliceMsg = CargoMsgT<EnvironmentPeriodSlice>;
      return {std::nullopt, [period_hads = this->m_hads_slice.content(), 
                             period_tagged_amounts = this->m_tas_slice.content(),
                             fiscal_period = this->m_fiscal_period]() -> std::optional<Msg> {
        // Create EnvironmentPeriodSlice matching get_cargo() logic
        EnvironmentPeriodSlice result{{}, fiscal_period};
        result.environment()["HeadingAmountDateTransEntry"]; // Parent state 'diff' needs key to work also for empty slice
        result.environment()["TaggedAmount"]; // Parent state 'diff' needs key to work also for empty slice
        spdlog::info("FiscalPeriodState::create_update_options, '-' -> lambda() -> PopStateMsg(EnvironmentPeriodSliceMsg HADs={} TaggedAmounts={})",period_hads.size(),period_tagged_amounts.size());
        
        // Add HADs (using sequential indexing)
        for (auto const& [index, env_val] : indexed_env_entries_from(period_hads)) {
          result.environment()["HeadingAmountDateTransEntry"].push_back({index, env_val});
        }
        
        // Add TaggedAmounts (using hash-based indexing)
        for (auto const& [value_id, env_val] : indexed_env_entries_from(period_tagged_amounts)) {
          result.environment()["TaggedAmount"].push_back({value_id, env_val});
        }
        
        return std::make_shared<PopStateMsg>(
          std::make_shared<EnvironmentPeriodSliceMsg>(result)
        );
      }};
    }});
    
    return result;
  }

} // namespace first
