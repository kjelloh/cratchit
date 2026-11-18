#include "core/ar/cli.hpp"
#include "core/build_stamp.hpp"
#include "core/ink2/cli.hpp"
#include "core/skv/vat_return/cli.hpp"
#include "core/date.hpp"
#include "core/dot_folder.hpp"
#include "core/log.hpp"
#include "core/manual.hpp"
#include "core/terminal_encoding.hpp"
#include "core/version.hpp"
#include "pyroceph/tui/key_reader.hpp"
#include "pyroceph/tui/prompt_loop.hpp"
#include "pyroceph/tui/raw_mode.hpp"
#include "pyroceph/tui/state.hpp"
#include "pyroceph/tui/terminal_width.hpp"
#include "ui/root_menu.hpp"
#include "ui/start.hpp"
#include "walk_pdf/cli.hpp"

#include <cstdio>
#include <filesystem>
#include <format>
#include <string>
#include <print>
#include <span>
#include <string_view>
#include <vector>

namespace {

// The charset encoding to read terminal input with. choose_terminal_encoding
// only gives charsets cratchit can read; ASCII is read as its superset UTF-8.
charset::Encoding encoding_of(cratchit::Charset charset) {
  return charset == cratchit::Charset::Cp437 ? charset::Encoding::cp437 : charset::Encoding::utf8;
}

} // namespace

int main(int argc, char* argv[]) {
  // With arguments cratchit runs a command and ends, without its user interface:
  // nothing else is printed, logged or required (no ~/.cratchit, no framework)
  std::vector<std::string_view> const arguments(argv + 1, argv + argc);
  if (!arguments.empty()) {
    if (arguments.front() == "--walk-pdf") {
      return walk_pdf::run_cli(std::span(arguments).subspan(1), stdout, stderr, cratchit::version());
    }
    if (arguments.front() == "--sie-to-ar") {
      return cratchit::ar::run_cli(std::span(arguments).subspan(1), stdout, stderr, cratchit::version());
    }
    if (arguments.front() == "--sie-to-ink2") {
      return cratchit::ink2::run_cli(std::span(arguments).subspan(1), stdout, stderr, cratchit::version());
    }
    if (arguments.front() == "--sie-to-vat") {
      return cratchit::skv::vat_return::run_cli(std::span(arguments).subspan(1), stdout, stderr, cratchit::version());
    }
    std::println(stderr,
                 "Fel: okänt argument '{}'. cratchit tar emot --walk-pdf, --sie-to-ar, --sie-to-ink2 och --sie-to-vat (se till exempel cratchit "
                 "--sie-to-ar --help).",
                 arguments.front());
    return 2;
  }
  // First of all, which build runs: so that the user sees at once whether it is the one just built or installed
  auto const program = cratchit::program_path();
  auto const startup = cratchit::startup_lines(cratchit::version(), cratchit::build_type(), cratchit::build_time(),
                                               program ? std::optional(*program) : std::nullopt);
  for (auto const& line : startup) std::println("{}", line);
  auto const cwd = std::filesystem::current_path();
  LOG_BUSINESS("cratchit startade i {}", cwd.string());
  // CRATCHIT_TODAY=YYYY-MM-DD makes cratchit take that day as today, e.g. to try a new fiscal year
  if (auto const today = cratchit::environment_variable("CRATCHIT_TODAY"); today && !today->empty()) {
    if (cratchit::parse_iso_date(*today)) {
      cratchit::set_today(*today);
      std::println(stderr, "cratchit räknar med att det är {} i dag (CRATCHIT_TODAY).", *today);
      LOG_BUSINESS("Dagens datum satt till {} med CRATCHIT_TODAY", *today);
    } else {
      std::println(stderr, "Varning: CRATCHIT_TODAY={} är inget datum ÅÅÅÅ-MM-DD. cratchit använder dagens datum.", *today);
    }
  }
  auto const terminal_encoding = cratchit::choose_terminal_encoding(cratchit::report_terminal_encoding());
  for (auto const& warning : terminal_encoding.warnings) {
    std::println(stderr, "{}", warning);
    LOG_DESIGN_INSUFFICIENCY("{}", warning);
  }
  // The framework comes with the installation, next to the program
  auto const program_folder = cratchit::program_folder();
  if (!program_folder) {
    std::println(stderr, "Fel: {}", program_folder.error());
    return 1;
  }
  auto const framework = cratchit::open_framework(*program_folder);
  if (!framework) {
    std::println(stderr, "Fel: {}", framework.error());
    return 1;
  }
  // The runtime is the user's own, in the home folder
  auto const home = cratchit::user_home();
  if (!home) {
    std::println(stderr, "Fel: {}", home.error());
    return 1;
  }
  auto const runtime = cratchit::open_runtime(*home, cratchit::today_iso());
  if (!runtime) {
    std::println(stderr, "Fel: {}", runtime.error());
    return 1;
  }
  // One log for every run, next to runtime.txt
  auto const log_file = runtime->file.parent_path() / "log.txt";
  if (auto const opened = cratchit::log::open(log_file); !opened) {
    std::println(stderr, "Varning: {}. cratchit loggar ingenting.", opened.error());
  }
  for (auto const& line : startup) LOG_BUSINESS("{}", line); // which build ran, shown before the log was open
  LOG_BUSINESS("Framework {} från {}", framework->version, framework->file.string());
  for (auto const& entry : runtime->initiated) {
    std::println("cratchit skapade {} i {}", entry.file.generic_string(), entry.folder.string());
    LOG_BUSINESS("Skapade {} i {}", entry.file.generic_string(), entry.folder.string());
  }
  // The user manual comes with the framework. Without it, ? shows only the common keys.
  auto const manual = cratchit::read_manual(framework->file.parent_path() / cratchit::manual_file_name);
  if (!manual) LOG_DESIGN_INSUFFICIENCY("Ingen användarmanual: {}", manual.error());
  auto const help = [&manual](std::string_view topic) -> pyroceph::tui::View {
    if (!manual) return {std::format("cratchit hittar inte användarmanualen: {}", manual.error())};
    return manual->help(topic);
  };
  {
    pyroceph::tui::RawMode raw_mode;
    auto keys = pyroceph::tui::terminal_key_reader(encoding_of(terminal_encoding.in));
    // The workspace is opened when the user chooses it (0 in the root), or at once
    // with the project when cratchit is started in a project folder
    auto const workspace_folder = cratchit::default_workspace_folder();
    pyroceph::tui::Navigator navigator(cratchit::ui::make_root_menu(workspace_folder, cwd));
    // A project in the working folder is opened at once
    for (auto const& line : cratchit::ui::open_working_folder_project(navigator, workspace_folder, cwd)) {
      std::println("{}", line);
    }
    std::println("Tryck ? för gemensamma val.");
    LOG_BUSINESS("Promptloopen startade");
    pyroceph::tui::run_prompt_loop(keys, navigator, stdout, pyroceph::tui::terminal_width, help);
    if (keys.failure_count() > 0) LOG_DESIGN_INSUFFICIENCY("{} tangenter kunde inte avkodas", keys.failure_count());
  }
  LOG_BUSINESS("cratchit avslutades");
}
