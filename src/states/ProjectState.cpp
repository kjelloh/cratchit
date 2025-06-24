#include "ProjectState.hpp"
#include "FiscalYearState.hpp" // TODO: We need a FiscalYerState with dynamic FiscalPeriod
#include "QuarterState.hpp" // We need a dynamic QuarterState
#include "FiscalPeriod.hpp" // QuarterIndex,
#include <spdlog/spdlog.h>
#include <format>

namespace first {

  // ----------------------------------
  ProjectState::ProjectState(StateImpl::UX ux,std::filesystem::path root_path) 
    :  StateImpl{ux}
      ,m_root_path{root_path}
      ,m_environment{} {

    try {
      m_environment = environment_from_file(m_root_path / "cratchit.env");
      m_ux.push_back(std::format("Environment size is: {}", m_environment.size()));
    }
    catch (const std::exception& e) {
      m_ux.push_back("Error initializing environment: " + std::string(e.what()));
    }

    struct FiscalYearOptionFactory {
        using StateFactory = std::function<std::shared_ptr<StateImpl>()>;
        Environment const& m_environment;
        FiscalYearOptionFactory(Environment const& environment)
            : m_environment(environment) {}

        Option operator()(FiscalYear fiscal_year) const {
            std::string caption = fiscal_year.to_string();
            auto fiscal_ux = StateImpl::UX{caption};
            // BEWARE: Don't capture the this pointer as FiscalYearOptionFactory is used only as a temporary
            //         to create this lambda. So we must ensure the lampda captures stuff that is actually there
            //         when the lambda is called (The user selects the option).
            return {caption, [fiscal_ux, &env = this->m_environment,fiscal_year]() {
                return std::make_shared<FiscalYearState>(fiscal_ux, fiscal_year.period(), env);
            }};
        }

        Option operator()(std::chrono::year fiscal_start_year) const {
            auto fiscal_year = FiscalPeriod::to_fiscal_year(fiscal_start_year, std::chrono::month{5}); // May to April

            std::string caption = fiscal_year.to_string();
            auto fiscal_ux = StateImpl::UX{caption};
            // BEWARE: Don't capture the this pointer as FiscalYearOptionFactory is used only as a temporary
            //         to create this lambda. So we must ensure the lampda captures stuff that is actually there
            //         when the lambda is called (The user selects the option).
            return {caption, [fiscal_ux, &env = this->m_environment,fiscal_year]() {
                return std::make_shared<FiscalYearState>(fiscal_ux, fiscal_year, env);
            }};
        }
    }; // struct FiscalYearOptionFactory

    struct QuarterOptionFactory {
        using StateFactory = std::function<std::shared_ptr<StateImpl>()>;
        Environment const& m_environment;
        QuarterOptionFactory(Environment const& environment)
            : m_environment(environment) {}

        Option operator()(FiscalQuarter fiscal_quarter) const {
            std::string caption = std::format("Quarter {}", fiscal_quarter.to_string());
            auto ux = StateImpl::UX{caption};
            return {caption, [ux,fiscal_quarter]() {
                return std::make_shared<QuarterState>(ux,fiscal_quarter.period());
            }};
        }

        Option operator()(FiscalPeriod fiscal_quarter) const {
            std::string caption = std::format("Quarter {}", fiscal_quarter.to_string());
            auto ux = StateImpl::UX{"Quarter UX goes here"};
            return {caption, [ux,fiscal_quarter]() {
                return std::make_shared<QuarterState>(ux,fiscal_quarter);
            }};
        }
    }; // struct QuarterOptionFactory

    FiscalYearOptionFactory fiscal_year_option_factory{m_environment};
    QuarterOptionFactory fiscal_quarter_option_factory{m_environment};
    auto current_fiscal_year = FiscalYear::to_current_fiscal_year(std::chrono::month{5}); // month hard coded for now
    auto current_fiscal_quarter = FiscalQuarter::to_current_fiscal_quarter();
    this->add_option('0',fiscal_year_option_factory(current_fiscal_year));
    this->add_option('1',fiscal_year_option_factory(current_fiscal_year.to_relative_fiscal_year(-1)));
    this->add_option('2',fiscal_year_option_factory(current_fiscal_year.to_relative_fiscal_year(-2)));
    this->add_option('3',fiscal_quarter_option_factory(current_fiscal_quarter));
    this->add_option('4',fiscal_quarter_option_factory(current_fiscal_quarter.to_relative_fiscal_quarter(-1)));
  } // ProjectState::ProjectState

  ProjectState::~ProjectState() {
    // TODO: Save environment to file here
    // NOTE: cratchit update(model) will defer destructor as Cmd invoked by runtime (i.e., side effects ok)
    spdlog::info("ProjectState::~ProjectState");
  }

  std::pair<std::optional<State>, Cmd> ProjectState::update(Msg const& msg) {
      spdlog::info("ProjectState::update");
      return {std::nullopt, Nop};
  }

  std::pair<std::optional<State>, Cmd> ProjectState::apply(cargo::HADsCargo const& cargo) const {
      spdlog::info("ProjectState::apply - Received HADsCargo but should receive EnvirnmentCargo?");
      // TODO: Consider to receive Environment slice as cargo?      
      //       Then merge into a mutated Environment here?
      return {std::nullopt, Nop};
  }

  Cargo ProjectState::get_cargo() const {
      spdlog::info("ProjectState::get_cargo");
      return Cargo{}; // Null cargo
  }

}