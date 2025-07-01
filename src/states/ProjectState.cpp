#include "ProjectState.hpp"
#include "FiscalPeriodState.hpp"
#include "FiscalPeriod.hpp" // QuarterIndex,
#include <spdlog/spdlog.h>
#include <format>
#include <vector>
#include <set>
#include <ranges> // std::views::transform,
#include <algorithm> // std::ranges::find,

namespace first {

  // ----------------------------------
  ProjectState::ProjectState(StateImpl::UX ux,std::filesystem::path root_path) 
    :  StateImpl{ux}
      ,m_root_path{root_path}
      ,m_persistent_environment_file{m_root_path / "cratchit.env",environment_from_file,environment_to_file}
      ,m_environment{} {

    try {
      // m_environment = environment_from_file(m_root_path / "cratchit.env");
      m_persistent_environment_file.init();
      if (auto const &cached_env = m_persistent_environment_file.cached()) {
        m_environment = *cached_env;
        m_ux.push_back(std::format("Environment {}: {} entries.",m_persistent_environment_file.path().string(), m_environment.size()));
      }
      else {
        m_environment = Environment{}; // Initialize with an empty environment if no cached environment
        m_ux.push_back(std::format("Null Environment {}",m_persistent_environment_file.path().string()));
      }
    }
    catch (const std::exception& e) {
      m_ux.push_back("Error initializing environment: " + std::string(e.what()));
    }

    auto current_fiscal_year = FiscalYear::to_current_fiscal_year(std::chrono::month{5}); // month hard coded for now
    auto current_fiscal_quarter = FiscalQuarter::to_current_fiscal_quarter();
    this->add_option('0',FiscalPeriodState::option_from(current_fiscal_year,m_environment));    
    this->add_option('1',FiscalPeriodState::option_from(current_fiscal_year.to_relative_fiscal_year(-1),m_environment));
    this->add_option('2',FiscalPeriodState::option_from(current_fiscal_year.to_relative_fiscal_year(-2),m_environment));
    this->add_option('3',FiscalPeriodState::option_from(current_fiscal_quarter,m_environment));
    this->add_option('4',FiscalPeriodState::option_from(current_fiscal_quarter.to_relative_fiscal_quarter(-1),m_environment));
  } // ProjectState::ProjectState

  ProjectState::~ProjectState() {
    // TODO: Save environment to file here
    // NOTE: cratchit update(model) will defer destructor as Cmd invoked by runtime (i.e., side effects ok)
    spdlog::info("ProjectState::~ProjectState");
    m_persistent_environment_file.update(m_environment); // Save the environment to file
  }

  std::pair<std::optional<State>, Cmd> ProjectState::update(Msg const& msg) {
      spdlog::info("ProjectState::update - didn't handle message");
      return {std::nullopt, Cmd{}}; // Didn't handle - let base dispatch use fallback
  }

  // A non owning diff of period sliced ranges = lhs - rhs
  template <typename T, typename DateProj, typename ValueProj>
  class diff_view {
  public:
    diff_view(
        T const &lhs,
        T const &rhs,
        FiscalPeriod period,
        DateProj date_projection,
        ValueProj value_projection)
        : m_lhs(lhs), m_rhs(rhs),
          m_period(period),
          m_date_projection(date_projection),
          m_value_projection(value_projection) {

      auto in_period = [&](auto const &it) {
        auto date = m_date_projection(*it);
        bool result = m_period.contains(date);
        spdlog::info("in_period? {} -> {}", to_string(date), result);
        return result;
      };

      spdlog::info("diff_view - lhs size: {}, rhs size: {}, period: {}", m_lhs.size(), m_rhs.size(), m_period.to_string());

      auto contains_ev = [&](auto const r,auto const& ev) {
        return std::ranges::any_of(r, [&](auto const& pair) {
          spdlog::info("contains_ev - checking: {} == {}", to_string(m_value_projection(pair)), to_string(ev));
          return m_value_projection(pair) == ev;
        });
      };

      for (auto it = std::ranges::begin(m_lhs); it != std::ranges::end(m_lhs); ++it) {
        if (in_period(it) && !contains_ev(m_rhs, m_value_projection(*it))) {
          spdlog::info("diff_view - lhs removed: {}", to_string(m_value_projection(*it)));
          m_removed.insert(it);
        }
      }
      for (auto it = std::ranges::begin(m_rhs); it != std::ranges::end(m_rhs); ++it) {
        if (in_period(it) && !contains_ev(m_lhs, m_value_projection(*it))) {
          spdlog::info("diff_view - rhs inserted: {}", to_string(m_value_projection(*it)));
          m_inserted.insert(it);
        }
      }
    }

    explicit operator bool() const {
      return !m_inserted.empty() || !m_removed.empty();
    }

    auto inserted() const {
      return m_inserted | std::views::transform([](auto it) { return *it; });
    }

    auto removed() const {
      return m_removed | std::views::transform([](auto it) { return *it; });
    }

    private:
      T const &m_lhs;
      T const &m_rhs;
      FiscalPeriod m_period;
      DateProj m_date_projection;
      ValueProj m_value_projection;
      std::set<typename T::const_iterator> m_inserted;
      std::set<typename T::const_iterator> m_removed;
    };

    std::pair<std::optional<State>, Cmd> ProjectState::apply(cargo::EnvironmentCargo const &cargo) const {
      std::optional<State> mutated_state{};
      Cmd cmd{Nop};
      spdlog::info("ProjectState::apply - Received EnvironmentCargo");
      // Update the environment with the cargo of environment slice
      auto [env_slice,slice_period] = cargo.m_payload;
      spdlog::info("ProjectState::apply - Period: {}, Slice size: {}", slice_period.to_string(), env_slice.size());
      std::string section{"HeadingAmountDateTransEntry"};
      if (m_environment.contains(section) and env_slice.contains(section)) {
        spdlog::info("ProjectState::apply - Processing section: {}", section);

        auto to_date = [](EnvironmentIdValuePair const& pair) -> Date {
          // Ok, so EnvironmentIdValuePair is not type safe regarding we asume it represents a HAD.
          // For now, this follows only from processing section 'HeadingAmountDateTransEntry'.
          if (auto had = to_had(pair.second)) {
            spdlog::info("by_date date: {}", to_string(had->date));
            return had->date;
          }
          spdlog::warn("ProjectState::apply::by_date - Invalid HAD in EnvironmentIdValuePair: {}", to_string(pair.second));
          return Date{}; // Return a default date if HAD is invalid
        };

        auto to_ev = [](EnvironmentIdValuePair const& pair) {return pair.second;};

        diff_view<EnvironmentCasEntryVector,decltype(to_date), decltype(to_ev)> 
          difference{m_environment.at(section), env_slice.at(section),slice_period, to_date,to_ev};
        if (difference) {
          spdlog::info("ProjectState::apply - Difference found in section: {}", section);
          auto new_state = std::make_shared<ProjectState>(*this); // Create a new state
          auto &mutated_environment = new_state->m_environment;   // Make a copy to mutate
          auto &mutated_section = mutated_environment[section];
          // Remove entriers in mutated section
          for (auto const &[index, ev] : difference.removed()) {
            if (auto iter = std::ranges::find(mutated_section, ev, [](auto const &pair) { return pair.second; }); iter != mutated_section.end()) {
              spdlog::info("ProjectState::apply - Removing entry {}:{}", index, to_string(ev));
              mutated_section.erase(iter); // Remove the entry
            } else {
              spdlog::warn("ProjectState::apply - Entry not found for removal: {}", to_string(ev));
            }
          }
          // Insert entries in mutated section
          for (auto const &[index, ev] : difference.inserted()) {
            if (auto iter = std::ranges::find(mutated_section, ev, [](auto const &pair) { return pair.second; });
                iter == mutated_section.end()) {
              spdlog::info("ProjectState::apply - Inserting entry {}:{}", index, to_string(ev));
              mutated_section.push_back({mutated_section.size(), ev}); // Insert the entry
            } else {
              spdlog::warn("ProjectState::apply - Entry already exists for insertion: {}", to_string(ev));
            }
          }
          mutated_state = new_state; // Set the mutated state
        }
      }
      return {mutated_state, cmd};
    }

  Cargo ProjectState::get_cargo() const {
      spdlog::info("ProjectState::get_cargo");
      return Cargo{}; // Null cargo
  }
} // namespace first