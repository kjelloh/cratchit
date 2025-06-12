#include "ProjectState.hpp"
#include "FiscalYearState.hpp" // TODO: We need a FiscalYerState with dynamic FiscalPeriod
#include "Q1State.hpp" // We need a dynamic QuarterState
#include "FiscalPeriod.hpp" // Quarter,
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

        Option operator()(Quarter quarter,std::chrono::year fiscal_year) const {
            std::string caption = std::format("Q1 of Fiscal Year {}", fiscal_year);
            auto q1_ux = StateImpl::UX{caption};
            return {caption, [fiscal_year, q1_ux, this]() {
                return std::make_shared<Q1State>(q1_ux);
            }};
        }
    }; // struct Q1StateFactory

    struct Q1Optionfactory : public QuarterOptionFactory {
        Q1Optionfactory(Environment const& environment)
            : QuarterOptionFactory(environment) {}

        Option operator()(std::chrono::year fiscal_year) const {
            return QuarterOptionFactory::operator()(Quarter{1}, fiscal_year);
        }
    }; // struct Q1Optionfactory

    StateFactory q1_factory = []() {
      auto q1_ux = StateImpl::UX{"Q1 UX goes here"};
      return std::make_shared<Q1State>(q1_ux);
    };

    this->add_option('0',FiscalYearOptionFactory{m_environment}(std::chrono::year{2025}));
    this->add_option('1',FiscalYearOptionFactory{m_environment}(std::chrono::year{2024}));
    this->add_option('2',FiscalYearOptionFactory{m_environment}(std::chrono::year{2023}));
    this->add_option('3',Q1Optionfactory{m_environment}(std::chrono::year{2025}));
    // TODO: We need a dynamic QuarterState to handle quarets 1..4 (only Q1 possible now)
  } // ProjectState::ProjectState

}