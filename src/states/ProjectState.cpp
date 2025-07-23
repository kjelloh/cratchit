#include "ProjectState.hpp"
#include "FiscalPeriodState.hpp"
#include "FiscalPeriod.hpp" // QuarterIndex,
#include "diff_view.hpp"
#include <spdlog/spdlog.h>
#include <format>
#include <vector>
#include <set>
#include <ranges> // std::views::transform,
#include <algorithm> // std::ranges::find,

namespace first {

  ProjectState::ProjectState(
     std::optional<std::string> caption
    ,PersistentFile<Environment> persistent_environment_file
    ,Environment environment)
    :  StateImpl{caption}
      ,m_root_path{persistent_environment_file.root_path()}
      ,m_persistent_environment_file{persistent_environment_file}
      ,m_environment{environment} {
  }

  // ----------------------------------
  ProjectState::ProjectState(std::filesystem::path root_path) 
    :  StateImpl{std::nullopt}
      ,m_root_path{root_path}
      ,m_persistent_environment_file{m_root_path / "cratchit.env",environment_from_file,environment_to_file,PruningPolicy::count_only(10)}
      ,m_environment{} {

    try {
      m_persistent_environment_file.init();
      if (auto const &cached_env = m_persistent_environment_file.cached()) {
        m_environment = *cached_env;
      }
      else {
        m_environment = Environment{}; // Initialize with an empty environment if no cached environment
        m_has_null_environment = true; // Track for UX creation
      }
    }
    catch (const std::exception& e) {
      m_init_error = e.what(); // Track for UX creation
    }
  } // ProjectState::ProjectState

  ProjectState::~ProjectState() {
    // TODO: Save environment to file here
    // NOTE: cratchit update(model) will defer destructor as Cmd invoked by runtime (i.e., side effects ok)
    spdlog::info("ProjectState::~ProjectState");
    m_persistent_environment_file.update(m_environment); // Save the environment to file
  }

  std::string ProjectState::caption() const {
    if (m_caption.has_value()) {
      return m_caption.value();
    }
    return m_root_path.string();
  }

  StateUpdateResult ProjectState::update(Msg const& msg) const {
    using EnvironmentPeriodSliceMsg = CargoMsgT<EnvironmentPeriodSlice>;
    if (auto pimpl = std::dynamic_pointer_cast<EnvironmentPeriodSliceMsg>(msg); pimpl != nullptr) {
      std::optional<State> mutated_state{};
      Cmd cmd{Nop};
      spdlog::info("ProjectState::update - handling EnvironmentPeriodSliceMsg");
      // Update the environment with the cargo of environment slice
      auto [env_slice,slice_period] = pimpl->payload;
      spdlog::info("ProjectState::update - Period: {}, Slice size: {}", slice_period.to_string(), env_slice.size());
      std::string section{"HeadingAmountDateTransEntry"};
      if ((not m_environment.contains(section)) and env_slice.contains(section)) {
        spdlog::info("ProjectState::update - mutated_environment[section] = env_slice.at(section for section:{})", section);

        auto mutated_environment = m_environment;
        mutated_environment[section] = env_slice.at(section);
        mutated_state = make_state<ProjectState>(this->caption(), this->m_persistent_environment_file, mutated_environment);
      }
      else if (m_environment.contains(section) and env_slice.contains(section)) {
        spdlog::info("ProjectState::update - Processing section: {}", section);

        auto to_date = [](EnvironmentIdValuePair const& pair) -> Date {
          // Ok, so EnvironmentIdValuePair is not type safe regarding we asume it represents a HAD.
          // For now, this follows only from processing section 'HeadingAmountDateTransEntry'.
          if (auto had = to_had(pair.second)) {
            // spdlog::info("by_date date: {}", to_string(had->date));
            return had->date;
          }
          spdlog::warn("ProjectState::update::by_date - Invalid HAD in EnvironmentIdValuePair: {}", to_string(pair.second));
          return Date{}; // Return a default date if HAD is invalid
        };

        auto to_ev = [](EnvironmentIdValuePair const& pair) {return pair.second;};

        diff_view<
           EnvironmentCasEntryVector
          ,decltype(to_date)
          ,decltype(to_ev)> difference{
             m_environment.at(section)
            ,env_slice.at(section)
            ,slice_period
            ,to_date,to_ev};

        if (difference) {
          spdlog::info("ProjectState::update - Difference found in section: {}", section);
          auto mutated_environment = this->m_environment;   // Make a copy to mutate
          auto &mutated_section = mutated_environment[section];
          // Remove entriers in mutated section
          for (auto const &[index, ev] : difference.removed()) {
            if (auto iter = std::ranges::find(mutated_section, ev, [](auto const &pair) { return pair.second; }); iter != mutated_section.end()) {
              spdlog::info("ProjectState::update - Removing entry {}:{}", index, to_string(ev));
              mutated_section.erase(iter); // Remove the entry
            } else {
              spdlog::warn("ProjectState::update - Entry not found for removal: {}", to_string(ev));
            }
          }
          // Insert entries in mutated section
          for (auto const &[index, ev] : difference.inserted()) {
            if ( auto iter = std::ranges::find(mutated_section, ev, [](auto const &pair) { return pair.second; })
                ;iter == mutated_section.end()) {
              spdlog::info("ProjectState::update - Inserting entry {}:{}", index, to_string(ev));
              mutated_section.push_back({mutated_section.size(), ev}); // Insert the entry
            } else {
              spdlog::warn("ProjectState::update - Entry already exists for insertion: {}", to_string(ev));
            }
          }
          mutated_state = make_state<ProjectState>(this->caption(), this->m_persistent_environment_file, mutated_environment);
        }
        else {
          spdlog::info("ProjectState::update - No diff found");
        }
      }
      else {
        spdlog::info("ProjectState::update - Ignored, because m_environment.contains(section):{} and env_slice.contains(section):{}",m_environment.contains(section),env_slice.contains(section));
      }
      return {mutated_state, cmd};
    }
    spdlog::info("ProjectState::update - didn't handle message");
    return {std::nullopt, Cmd{}}; // Didn't handle - let base dispatch use fallback
  }

  static std::string to_underscored_spaces(const std::string &name) {
    std::string result;
    result.reserve(name.size());
    std::ranges::transform(name, std::back_inserter(result),
                           [](char c) { return c == ' ' ? '_' : c; });
    return result;
  }

  StateImpl::UpdateOptions ProjectState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    
    auto current_fiscal_year = FiscalYear::to_current_fiscal_year(std::chrono::month{5}); // month hard coded for now
    auto current_fiscal_quarter = FiscalQuarter::to_current_fiscal_quarter();
    
    result.add('0', {std::format("Fiscal Year: {}", current_fiscal_year.to_string()), 
      [current_fiscal_year, env = m_environment]() -> StateUpdateResult {
        return {std::nullopt, [current_fiscal_year, env]() -> std::optional<Msg> {
          State new_state = make_state<FiscalPeriodState>(current_fiscal_year.period(), env);
          return std::make_shared<PushStateMsg>(new_state);
        }};
      }});
      
    result.add('1', {std::format("Fiscal Year: {}", current_fiscal_year.to_relative_fiscal_year(-1).to_string()), 
      [current_fiscal_year, env = m_environment]() -> StateUpdateResult {
        auto fiscal_year = current_fiscal_year.to_relative_fiscal_year(-1);
        return {std::nullopt, [fiscal_year, env]() -> std::optional<Msg> {
          State new_state = make_state<FiscalPeriodState>(fiscal_year.period(), env);
          return std::make_shared<PushStateMsg>(new_state);
        }};
      }});
      
    result.add('2', {std::format("Fiscal Year: {}", current_fiscal_year.to_relative_fiscal_year(-2).to_string()), 
      [current_fiscal_year, env = m_environment]() -> StateUpdateResult {
        auto fiscal_year = current_fiscal_year.to_relative_fiscal_year(-2);
        return {std::nullopt, [fiscal_year, env]() -> std::optional<Msg> {
          State new_state = make_state<FiscalPeriodState>(fiscal_year.period(), env);
          return std::make_shared<PushStateMsg>(new_state);
        }};
      }});
      
    result.add('3', {std::format("Fiscal Quarter: {}", current_fiscal_quarter.to_string()), 
      [current_fiscal_quarter, env = m_environment]() -> StateUpdateResult {
        return {std::nullopt, [current_fiscal_quarter, env]() -> std::optional<Msg> {
          State new_state = make_state<FiscalPeriodState>(current_fiscal_quarter.period(), env);
          return std::make_shared<PushStateMsg>(new_state);
        }};
      }});
      
    result.add('4', {std::format("Fiscal Quarter: {}", current_fiscal_quarter.to_relative_fiscal_quarter(-1).to_string()), 
      [current_fiscal_quarter, env = m_environment]() -> StateUpdateResult {
        auto fiscal_quarter = current_fiscal_quarter.to_relative_fiscal_quarter(-1);
        return {std::nullopt, [fiscal_quarter, env]() -> std::optional<Msg> {
          State new_state = make_state<FiscalPeriodState>(fiscal_quarter.period(), env);
          return std::make_shared<PushStateMsg>(new_state);
        }};
      }});
    
    // Add '-' key option last - back with environment as payload
    result.add('-', {"Back", [this]() -> StateUpdateResult {
      return {std::nullopt, [env = this->m_environment]() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>();
      }};
    }});
    
    return result;
  }

  StateImpl::UX ProjectState::create_ux() const {
    UX result{};
    
    // Base UX from constructor
    result.push_back(caption());
    
    // Add dynamic content based on state
    if (!m_init_error.empty()) {
      result.push_back("Error initializing environment: " + m_init_error);
    } else if (m_has_null_environment) {
      result.push_back(std::format("Null Environment {}", m_persistent_environment_file.path().string()));
    } else {
      result.push_back(std::format("Environment {}: {} entries.", m_persistent_environment_file.path().string(), m_environment.size()));
    }
    
    return result;
  }

} // namespace first