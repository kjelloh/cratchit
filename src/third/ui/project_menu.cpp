#include "ui/project_menu.hpp"

#include "core/fiscal_year.hpp"
#include "core/log.hpp"
#include "ui/fiscal_year_menu.hpp"
#include "ui/help_topics.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <string>
#include <utility>
#include <vector>

namespace cratchit::ui {

namespace {

using pyroceph::tui::CodePoint;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

// "inget", "1 verifikation" or "37 verifikationer"
std::string booked_text(std::size_t vouchers) {
  if (vouchers == 0) return "inget";
  return std::format("{} {}", vouchers, vouchers == 1 ? "verifikation" : "verifikationer");
}

class ProjectState : public State {
public:
  explicit ProjectState(Project project) : project_(std::move(project)) { read_years(); }
  std::string name() const override { return project_.name; }
  View view() const override {
    View lines{project_.folder.string()};
    if (!error_.empty()) {
      lines.push_back(std::format("cratchit kunde inte läsa räkenskapsåren: {}", error_));
    } else if (years_.empty()) {
      lines.push_back("Bokföringen har inga räkenskapsår än.");
    } else if (years_.size() > max_listed_fiscal_years) {
      lines.push_back(std::format("cratchit visar bara det aktuella räkenskapsåret och {} äldre. Bokföringen har fler.",
                                  max_older_fiscal_years));
    }
    return lines;
  }
  OptionsList options() const override {
    OptionsList result;
    for (std::size_t i = 0; i < listed(); ++i) {
      auto const& [year, in_books, vouchers] = years_[i];
      result.push_back({key_of(i), std::format("{} – {}  bokfört: {}", year.start, year.end, booked_text(vouchers))});
    }
    return result;
  }
  OnKeyResult on_key(CodePoint key) override {
    for (std::size_t i = 0; i < listed(); ++i) {
      if (key == key_of(i)) return make_fiscal_year_menu(project_, years_[i].year);
    }
    return nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::project); }

  // The fiscal years may have changed on the way back here (an import, or a new day): read them again
  void on_return() override { read_years(); }

private:
  void read_years() {
    years_.clear();
    error_.clear();
    auto years = list_years_until(project_, today_iso());
    if (years) {
      years_ = std::move(*years);
    } else {
      LOG_BUSINESS("Kunde inte läsa räkenskapsåren i {}: {}", project_.folder.string(), years.error());
      error_ = years.error();
    }
  }

  std::size_t listed() const { return std::min(years_.size(), max_listed_fiscal_years); }
  static CodePoint key_of(std::size_t index) { return static_cast<CodePoint>(U'0' + index); }

  Project project_;
  std::vector<ListedYear> years_; // the latest first, the current year as key 0
  std::string error_;             // why the fiscal years could not be read, or empty
};

} // namespace

std::unique_ptr<State> make_project_menu(Project project) {
  return std::make_unique<ProjectState>(std::move(project));
}

} // namespace cratchit::ui
