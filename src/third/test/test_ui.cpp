#include "core/books/books.hpp"
#include "core/details_project.hpp"
#include "core/dot_folder.hpp"
#include "core/fiscal_year.hpp"
#include "core/inventory.hpp"
#include "core/sie_import.hpp"
#include "core/path_value.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"
#include "core/sie_project.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "pyroceph/tui/path_completion.hpp"
#include "ui/add_project_menu.hpp"
#include "ui/booked_menu.hpp"
#include "ui/command_hint_menu.hpp"
#include "ui/draft_menu.hpp"
#include "ui/voucher_menu.hpp"
#include "ui/design_insufficiency.hpp"
#include "ui/help_topics.hpp"
#include "ui/project_menu.hpp"
#include "ui/root_menu.hpp"
#include "ui/start.hpp"
#include "ui/vat_return_menu.hpp"
#include "ui/test_menu.hpp"
#include "ui/workspace_menu.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

using pyroceph::tui::CodePoint;
using pyroceph::tui::Navigator;
using pyroceph::tui::OptionsList;
using pyroceph::tui::View;

namespace {

// A project folder with project.txt holding text, remembered in workspace.txt of
// the workspace in workspace_folder as `project.<uuid> = <folder_name>`
void add_project(fs::path const& workspace_folder, std::string const& folder_name, std::string const& uuid,
                 std::string const& text) {
  fs::create_directories(workspace_folder / folder_name / ".cratchit");
  std::ofstream(workspace_folder / folder_name / ".cratchit" / "project.txt") << text;
  std::ofstream(workspace_folder / ".cratchit" / "workspace.txt", std::ios::app)
      << "project." << uuid << " = " << folder_name << '\n';
}

// An empty folder of its own for each test, standing in for Documents, removed afterwards
class RootMenuTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    documents = fs::temp_directory_path() / (std::string("cratchit_ui_test_") + info->name());
    fs::remove_all(documents);
    fs::create_directories(documents);
  }
  void TearDown() override { fs::remove_all(documents); }

  fs::path documents;
};

// Makes today another day while it lives, then the tests' day again (see test_today.cpp)
struct Today {
  explicit Today(std::string day) { cratchit::set_today(std::move(day)); }
  ~Today() { cratchit::set_today("2026-11-09"); }
};

// The options with only their first column, the name: text before two spaces
OptionsList names_of(OptionsList options) {
  for (auto& [key, caption] : options) caption = caption.substr(0, caption.find("  "));
  return options;
}

} // namespace

TEST_F(RootMenuTest, IsCratchitWithWorkspaceAsZeroAndTestMenuAsNine) {
  auto const root = cratchit::ui::make_root_menu(documents / "cratchit");
  EXPECT_EQ(root->name(), "Cratchit");
  EXPECT_TRUE(root->view().empty());
  EXPECT_EQ(root->options(), (OptionsList{{U'0', "Samlingsmapp"}, {U'9', "Testmeny"}}));
}

TEST_F(RootMenuTest, ZeroCreatesTheWorkspaceTheFirstTime) {
  auto const folder = documents / "cratchit";
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
  EXPECT_EQ(navigator.current().view(), (View{"cratchit skapade en ny samlingsmapp.", folder.string()}));
  EXPECT_TRUE(fs::is_regular_file(folder / ".cratchit" / "workspace.txt"));
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Cratchit");
}

TEST_F(RootMenuTest, ZeroOpensAnExistingWorkspace) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2020-01-01"));
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
  EXPECT_EQ(navigator.current().view(), (View{folder.string()}));
}

TEST_F(RootMenuTest, ZeroLeadsToAnErrorWhenTheWorkspaceCannotBeOpened) {
  // Documents itself is missing, and only the workspace folder is created
  auto const folder = documents / "saknas" / "cratchit";
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Fel");
  ASSERT_EQ(navigator.current().view().size(), 1u);
  EXPECT_TRUE(navigator.current().view()[0].starts_with("cratchit kunde inte öppna samlingsmappen: "));
  EXPECT_FALSE(fs::exists(documents / "saknas"));
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Cratchit");
}

TEST(RootMenu, ZeroLeadsToAnErrorWhenTheWorkspaceFolderIsNotKnown) {
  Navigator navigator(cratchit::ui::make_root_menu(std::unexpected(std::string("Hittar inte mappen Dokument"))));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Fel");
  EXPECT_EQ(navigator.current().view(), (View{"cratchit kunde inte öppna samlingsmappen: Hittar inte mappen Dokument"}));
}

TEST_F(RootMenuTest, NineLeadsToTheTestMenu) {
  Navigator navigator(cratchit::ui::make_root_menu(documents / "cratchit"));
  EXPECT_TRUE(navigator.press(U'9'));
  EXPECT_EQ(navigator.current().name(), "Testmeny");
  EXPECT_FALSE(fs::exists(documents / "cratchit"));
}

TEST_F(RootMenuTest, OtherKeysAreNotMapped) {
  auto root = cratchit::ui::make_root_menu(documents / "cratchit");
  EXPECT_EQ(root->on_key(U'1'), nullptr);
}

TEST(TestMenu, KeepsItsStates) {
  Navigator navigator(cratchit::ui::make_test_menu());
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Projekt"}, {U'2', "Bokslut"}}));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Projekt");
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Öppna projekt");
  navigator.back();
  navigator.back();
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(TestMenu, ProjektHasOneOpenThatWorksAndOneThatFails) {
  Navigator navigator(cratchit::ui::make_test_menu());
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Öppna"}, {U'2', "Öppna (misslyckas)"}}));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Fel");
  EXPECT_EQ(navigator.current().view(), (pyroceph::tui::View{"Kunde inte öppna projektet."}));
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Projekt");
}

TEST(TestMenu, BokslutTakesAYearOfFourDigits) {
  Navigator navigator(cratchit::ui::make_test_menu());
  navigator.press(U'2');
  auto const accepted = navigator.current().on_input(U"2025");
  ASSERT_TRUE(accepted);
  EXPECT_EQ(accepted->reply, "Bokslut för 2025 noterat.");
  EXPECT_FALSE(navigator.current().on_input(U"25"));
}

TEST_F(RootMenuTest, WorkspaceListsItsProjectsAndOpensOne) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-08"));
  add_project(folder, "itfied", "1", "created = 2026-10-23\nuuid = 1\nname = The ITfied AB\n");
  add_project(folder, "annat", "2", "uuid = 2\nname = Annat AB\n");
  add_project(folder, "not remembered", "3", "uuid = 3\nname = Glömt AB\n");
  // The workspace lists only what it remembers: forget the last one again
  std::ofstream(folder / ".cratchit" / "workspace.txt") << "created = 2026-11-08\nproject.1 = itfied\nproject.2 = annat\n";
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(names_of(navigator.current().options()), (OptionsList{{U'1', "Annat AB"}, {U'2', "The ITfied AB"}, {U'+', "Lägg till bokföring"}}));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "The ITfied AB");
  EXPECT_EQ(navigator.current().view(), (View{(folder / "itfied").string(), "Bokföringen har inga räkenskapsår än."}));
  EXPECT_EQ(navigator.current().options(), (OptionsList{}));
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
}

TEST_F(RootMenuTest, ProjectThatCannotBeOpenedCanBeForgotten) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-08"));
  add_project(folder, "Utan namn", "1", "uuid = 1\n");
  add_project(folder, "Kvar", "2", "uuid = 2\nname = Kvar AB\n");
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(names_of(navigator.current().options()),
            (OptionsList{{U'1', "Kvar AB"}, {U'2', "Utan namn"}, {U'+', "Lägg till bokföring"}}));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Utan namn");
  ASSERT_EQ(navigator.current().view().size(), 1u);
  EXPECT_TRUE(navigator.current().view()[0].ends_with("'name' saknas")) << navigator.current().view()[0];
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "Glöm"}}));
  // - goes back without forgetting
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().options().size(), 3u);
  // 0 forgets and goes back to the workspace; the folder stays
  ASSERT_TRUE(navigator.press(U'2'));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
  EXPECT_EQ(names_of(navigator.current().options()), (OptionsList{{U'1', "Kvar AB"}, {U'+', "Lägg till bokföring"}}));
  EXPECT_TRUE(fs::exists(folder / "Utan namn" / ".cratchit" / "project.txt"));
}

TEST_F(RootMenuTest, ProjectThatIsGoneCanBeForgotten) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-08"));
  std::ofstream(folder / ".cratchit" / "workspace.txt", std::ios::app) << "project.1 = " << (documents / "Borta AB").string() << "\n";
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Borta AB");
  ASSERT_FALSE(navigator.current().view().empty());
  EXPECT_TRUE(navigator.current().view()[0].ends_with("men mappen finns inte längre.")) << navigator.current().view()[0];
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'+', "Lägg till bokföring"}}));
}

namespace {

cratchit::Workspace workspace_at(fs::path const& folder) {
  return {folder, folder / ".cratchit" / "workspace.txt", {}};
}

} // namespace

TEST(WorkspaceMenu, WithoutProjectsShowsItsFolderAndOffersOnlyToAdd) {
  auto workspace = cratchit::ui::make_workspace_menu(workspace_at("/x/cratchit"), {});
  EXPECT_EQ(workspace->view(), (View{fs::path("/x/cratchit").string()}));
  EXPECT_EQ(workspace->options(), (OptionsList{{U'+', "Lägg till bokföring"}}));
  EXPECT_EQ(workspace->on_key(U'1'), nullptr);
  EXPECT_EQ(workspace->on_key(U'0'), nullptr);
  auto const input = workspace->on_input(U"x");
  ASSERT_FALSE(input);
  EXPECT_EQ(input.error(), pyroceph::tui::no_input_error);
}

TEST(WorkspaceMenu, OffersNineProjectsAndSaysWhenThereAreMore) {
  std::vector<cratchit::ListedProject> projects;
  for (char c = 'a'; c <= 'j'; ++c) projects.push_back({std::string(1, c), fs::path("/x/cratchit") / std::string(1, c)});
  auto workspace = cratchit::ui::make_workspace_menu(workspace_at("/x/cratchit"), projects);
  auto const options = names_of(workspace->options());
  ASSERT_EQ(options.size(), 10u);
  EXPECT_EQ(options[0], (pyroceph::tui::Option{U'1', "a"}));
  EXPECT_EQ(options[8], (pyroceph::tui::Option{U'9', "i"}));
  EXPECT_EQ(options[9], (pyroceph::tui::Option{U'+', "Lägg till bokföring"}));
  auto const view = workspace->view();
  ASSERT_EQ(view.size(), 3u);
  EXPECT_EQ(view[0], fs::path("/x/cratchit").string());
  EXPECT_EQ(view[1], "Samlingsmappen har fler bokföringar än cratchit visar här.");
  EXPECT_TRUE(view[2].starts_with("    Namn")) << view[2];
}

TEST(ProjectMenu, IsNamedByTheProjectAndShowsItsFolder) {
  auto project = cratchit::ui::make_project_menu({"/x/p", "/x/p/.cratchit/project.txt", "1", "The ITfied AB"});
  EXPECT_EQ(project->name(), "The ITfied AB");
  EXPECT_EQ(project->view(), (View{fs::path("/x/p").string(), "Bokföringen har inga räkenskapsår än."}));
  EXPECT_EQ(project->options(), (OptionsList{}));
  EXPECT_EQ(project->on_key(U'1'), nullptr);
}

namespace {

// A project in folder whose books.txt holds text
cratchit::Project project_with_books(fs::path const& folder, std::string const& text) {
  fs::create_directories(folder / ".cratchit" / "books");
  std::ofstream(folder / ".cratchit" / "books" / "books.txt") << text;
  return {folder, folder / ".cratchit" / "project.txt", "1", "The ITfied AB"};
}

} // namespace

TEST_F(RootMenuTest, ProjectOffersItsFiscalYearsTheCurrentAsZero) {
  Navigator navigator(cratchit::ui::make_project_menu(project_with_books(
      documents / "p", "chart_type = EUBAS97\nyear.2024-05-01 = 2025-04-30\nyear.2025-05-01 = 2026-04-30\n")));
  EXPECT_EQ(navigator.current().view(), (View{(documents / "p").string()}));
  // Today (2026-11-09) is in 2026/27, which is not in the books: it is listed as the current year all the same
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "2026-05-01 – 2027-04-30  bokfört: inget"},
                                                        {U'1', "2025-05-01 – 2026-04-30  bokfört: inget"},
                                                        {U'2', "2024-05-01 – 2025-04-30  bokfört: inget"},
                                                        {U'3', "2023-05-01 – 2024-04-30  bokfört: inget"}}));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "2024/25");
  EXPECT_EQ(navigator.current().view(), (View{"Räkenskapsår 2024-05-01 – 2025-04-30", "Hela året: inget bokfört"}));
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'0', "Årsflöde"}, {U'1', "Exportera till SIE"}, {U'2', "Importera från SIE"},
                         {U'3', "Rapporter"}}));
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "The ITfied AB");
}

TEST_F(RootMenuTest, FiscalYearZeroIsTheYearFlowWithItsStepsNotThereYet) {
  Navigator navigator(cratchit::ui::make_project_menu(
      project_with_books(documents / "p", "year.2025-05-01 = 2026-04-30\n")));
  ASSERT_TRUE(navigator.press(U'1'));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Årsflöde");
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'0', "Utkast"},
                         {U'1', "Bokfört"},
                         {U'2', "Momsrapport"},
                         {U'3', "Bokslut"},
                         {U'4', "Årsredovisning"},
                         {U'5', "Årsstämma"},
                         {U'6', "Rapport till Bolagsverket"},
                         {U'7', "Deklaration till SKV"}}));
  // Årsredovisning and Deklaration till SKV tell how to make them from the command line
  ASSERT_TRUE(navigator.press(U'4'));
  EXPECT_EQ(navigator.current().name(), "Årsredovisning");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::annual_report);
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'7'));
  EXPECT_EQ(navigator.current().name(), "Deklaration till SKV");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::tax_return);
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Momsrapport");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::vat_return);
  ASSERT_TRUE(navigator.back());
  // Årsstämma and Rapport till Bolagsverket are not there yet
  for (auto const& [key, caption] : navigator.current().options()) {
    if (key == U'0' || key == U'1' || key == U'2' || key == U'3' || key == U'4' || key == U'7') continue;
    ASSERT_TRUE(navigator.press(key));
    EXPECT_EQ(navigator.current().name(), "Inte än (" + caption + ")");
    ASSERT_TRUE(navigator.back());
  }
  EXPECT_FALSE(navigator.press(U'8'));
  // Utkast has Beskriv händelser, Samma?, Fullborda utkast, Läs kontoutdrag and Läs pdf-underlag
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Utkast");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "Beskriv händelser"},
                                                        {U'1', "Samma?"},
                                                        {U'2', "Fullborda utkast"},
                                                        {U'3', "Läs kontoutdrag"},
                                                        {U'4', "Läs pdf-underlag"},
                                                        {U'5', "Gör bokfört till utkast"},
                                                        {U'<', "Föregående sida"},
                                                        {U'>', "Nästa sida"}}));
  ASSERT_TRUE(navigator.back());
  // Bokfört lists the year's vouchers
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Bokfört");
  ASSERT_TRUE(navigator.back());
  // Bokslut has Avstämning, not there yet
  ASSERT_TRUE(navigator.press(U'3'));
  EXPECT_EQ(navigator.current().name(), "Bokslut");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "Avstämning"}}));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Inte än (Avstämning)");
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "2025/26");
}

TEST_F(RootMenuTest, ProjectSaysWhenItsFiscalYearsCannotBeRead) {
  auto project = cratchit::ui::make_project_menu(project_with_books(documents / "p", "year.2025 = 2026-04-30\n"));
  ASSERT_EQ(project->view().size(), 2u);
  EXPECT_TRUE(project->view()[1].starts_with("cratchit kunde inte läsa räkenskapsåren: "));
  EXPECT_EQ(project->options(), (OptionsList{}));
}

TEST_F(RootMenuTest, ProjectOffersTenFiscalYearsAndSaysWhenThereAreMore) {
  Today const today("2019-06-01");
  std::string text;
  for (int year = 2009; year < 2020; ++year) text += std::format("year.{}-01-01 = {}-12-31\n", year, year);
  auto project = cratchit::ui::make_project_menu(project_with_books(documents / "p", text));
  ASSERT_EQ(project->options().size(), 10u);
  EXPECT_EQ(project->options().front(), (pyroceph::tui::Option{U'0', "2019-01-01 – 2019-12-31  bokfört: inget"}));
  EXPECT_EQ(project->options().back(), (pyroceph::tui::Option{U'9', "2010-01-01 – 2010-12-31  bokfört: inget"}));
  EXPECT_EQ(project->view().back(), "cratchit visar bara det aktuella räkenskapsåret och 9 äldre. Bokföringen har fler.");
}

TEST(WorkspaceMenu, PlusLeadsToAddWithOpenExistingAndNew) {
  Navigator navigator(cratchit::ui::make_workspace_menu(workspace_at("/x/cratchit"), {}));
  ASSERT_TRUE(navigator.press(U'+'));
  EXPECT_EQ(navigator.current().name(), "Lägg till");
  EXPECT_TRUE(navigator.current().view().empty());
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Befintlig"}, {U'2', "Ny"}}));
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
}

TEST(AddProjectMenu, OneLeadsToOpenExisting) {
  Navigator navigator(cratchit::ui::make_add_project_menu(workspace_at("/x/cratchit"), "/x"));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Befintlig");
  EXPECT_FALSE(navigator.current().view().empty());
  EXPECT_TRUE(navigator.current().options().empty());
  EXPECT_EQ(navigator.current().start_input(), pyroceph::tui::path_input_of("/x"));
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Lägg till");
}

TEST_F(RootMenuTest, OpenExistingCompletesFolderNames) {
  fs::create_directories(documents / "The ITfied AB");
  auto state = cratchit::ui::make_open_existing_project(workspace_at(documents / "cratchit"), documents);
  auto const typed = state->start_input() + U"The";
  auto const separator = static_cast<char32_t>(fs::path::preferred_separator);
  EXPECT_EQ(state->complete(typed).added, U" ITfied AB" + std::u32string(1, separator));
}

TEST_F(RootMenuTest, ExistingIsAddedAndTheWorkspaceListsIt) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-08"));
  fs::create_directories(documents / "elsewhere" / "itfied" / ".cratchit");
  std::ofstream(documents / "elsewhere" / "itfied" / ".cratchit" / "project.txt") << "uuid = 7\nname = The ITfied AB\n";
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'+', "Lägg till bokföring"}}));
  ASSERT_TRUE(navigator.press(U'+'));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Befintlig");
  auto const add = [&](fs::path const& project_folder) {
    auto result = navigator.current().on_input(pyroceph::tui::path_input_of(project_folder));
    EXPECT_TRUE(result) << result.error();
    EXPECT_FALSE(result->next);
    EXPECT_TRUE(navigator.follow_back_to());
    return result->reply;
  };
  // Added: back in the workspace, which lists it
  EXPECT_EQ(add(documents / "elsewhere" / "itfied"), "Samlingsmappen kommer ihåg The ITfied AB.");
  EXPECT_EQ(navigator.path().size(), 2u);
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
  EXPECT_EQ(names_of(navigator.current().options()), (OptionsList{{U'1', "The ITfied AB"}, {U'+', "Lägg till bokföring"}}));
  auto const entries = cratchit::read_path_values(folder / ".cratchit" / "workspace.txt");
  ASSERT_TRUE(entries);
  EXPECT_EQ(cratchit::find_value(*entries, "project.7"), (documents / "elsewhere" / "itfied").string());
  // Added again: already remembered
  ASSERT_TRUE(navigator.press(U'+'));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(add(documents / "elsewhere" / "itfied"), "Samlingsmappen kommer redan ihåg The ITfied AB.");
  // Moved: remembered at the new place
  fs::rename(documents / "elsewhere", documents / "moved");
  ASSERT_TRUE(navigator.press(U'+'));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(add(documents / "moved" / "itfied"), "Samlingsmappen kommer ihåg The ITfied AB på det nya stället.");
  auto const moved = cratchit::list_projects(*cratchit::open_workspace(folder, "2026-11-08"));
  ASSERT_TRUE(moved);
  ASSERT_EQ(moved->size(), 1u);
  EXPECT_EQ((*moved)[0].folder, documents / "moved" / "itfied");
}

TEST_F(RootMenuTest, OpenExistingRefusesAFolderWithoutAProject) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-08"));
  auto state = cratchit::ui::make_open_existing_project(*cratchit::open_workspace(folder, "2026-11-08"), documents);
  auto const result = state->on_input(pyroceph::tui::path_input_of(documents));
  ASSERT_FALSE(result);
  EXPECT_TRUE(result.error().starts_with("cratchit kunde inte öppna bokföringen: "));
  EXPECT_TRUE(result.error().ends_with("saknas"));
}

TEST(AddProjectMenu, NewOffersFromSieAndFromDetails) {
  Navigator navigator(cratchit::ui::make_add_project_menu(workspace_at("/x/cratchit"), "/x"));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Ny");
  EXPECT_TRUE(navigator.current().view().empty());
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Från SIE"}, {U'2', "Från uppgifter"}}));
  EXPECT_EQ(navigator.current().on_key(U'3'), nullptr);
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Lägg till");
}

TEST(AddProjectMenu, FromSieStartsInTheStartFolder) {
  Navigator navigator(cratchit::ui::make_add_project_menu(workspace_at("/x/cratchit"), "/x/cratchit"));
  ASSERT_TRUE(navigator.press(U'2'));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Från SIE");
  EXPECT_EQ(navigator.current().start_input(), pyroceph::tui::path_input_of("/x/cratchit"));
}

TEST(AddProjectMenu, OtherKeysAreNotMapped) {
  EXPECT_EQ(cratchit::ui::make_add_project_menu(workspace_at("/x/cratchit"), "/x")->on_key(U'3'), nullptr);
}

TEST_F(RootMenuTest, EveryStateHasItsHelpTopic) {
  namespace topic = cratchit::ui::help_topic;
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-08"));
  add_project(folder, "itfied", "1", "uuid = 1\nname = The ITfied AB\n");
  project_with_books(folder / "itfied", "year.2025-05-01 = 2026-04-30\n");
  add_project(folder, "trasig", "2", "uuid = 2\n");
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  EXPECT_EQ(navigator.current().help_topic(), topic::root);
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().help_topic(), topic::workspace);
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().help_topic(), topic::project);
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().help_topic(), topic::fiscal_year);
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().help_topic(), topic::year_flow);
  ASSERT_TRUE(navigator.press(U'3'));
  EXPECT_EQ(navigator.current().help_topic(), topic::closing);
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'2')); // trasig has no name
  EXPECT_EQ(navigator.current().help_topic(), topic::forget_project);
  EXPECT_EQ(pyroceph::tui::make_error_state("x")->help_topic(), topic::error);
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'+'));
  EXPECT_EQ(navigator.current().help_topic(), topic::add_project);
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().help_topic(), topic::open_existing);
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().help_topic(), topic::new_project);
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().help_topic(), topic::from_sie);
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().help_topic(), topic::from_details);
  for (auto const& [key, field_topic] : {std::pair{U'1', topic::details_name}, std::pair{U'2', topic::details_orgnr},
                                         std::pair{U'3', topic::details_year}, std::pair{U'4', topic::details_months}}) {
    ASSERT_TRUE(navigator.press(key));
    EXPECT_EQ(navigator.current().help_topic(), field_topic);
    ASSERT_TRUE(navigator.back());
  }
  EXPECT_EQ(cratchit::ui::make_design_insufficiency_state("Val", "något")->help_topic(), topic::design_insufficiency);
}

TEST(WorkspaceMenu, OpenExistingStartsInTheWorkspace) {
  Navigator navigator(cratchit::ui::make_workspace_menu(workspace_at("/x/cratchit"), {}));
  ASSERT_TRUE(navigator.press(U'+'));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().start_input(), pyroceph::tui::path_input_of("/x/cratchit"));
}

TEST_F(RootMenuTest, FromSieCreatesTheProjectAndTheWorkspaceRemembersIt) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-09"));
  auto const company = documents / "Bolaget"; // the user's own folder, holding the SIE file
  fs::create_directories(company);
  std::ofstream(company / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 1930 \"Bank\"\r\n#KONTO 3010 \"Sales\"\r\n"
         "#VER A 1 20250201 \"Sale\"\r\n{\r\n#TRANS 1930 {} 500.00\r\n#TRANS 3010 {} -500.00\r\n}\r\n";
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  ASSERT_TRUE(navigator.press(U'+'));
  ASSERT_TRUE(navigator.press(U'2'));
  ASSERT_TRUE(navigator.press(U'1'));
  // Tab completes the SIE file
  auto const typed = pyroceph::tui::path_input_of(company) + U"te";
  EXPECT_EQ(navigator.current().complete(typed).added, U"st.se");
  auto result = navigator.current().on_input(pyroceph::tui::path_input_of(company) + U"test.se");
  ASSERT_TRUE(result) << result.error();
  ASSERT_TRUE(result->next);
  navigator.enter(std::move(result->next));
  EXPECT_EQ(navigator.current().name(), "Bekräfta");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'J', "Ja"}, {U'N', "Nej"}}));
  EXPECT_FALSE(navigator.press(U'\n')); // Enter only shows the options again
  EXPECT_EQ(navigator.current().view()[1], "  Mapp:           " + company.string());
  EXPECT_FALSE(fs::exists(company / ".cratchit")); // nothing created before J
  // J creates the project in the SIE file's folder, no new folder, and goes back to the workspace, which lists it
  ASSERT_TRUE(navigator.press(U'J'));
  EXPECT_TRUE(fs::exists(company / ".cratchit" / "project.txt"));
  EXPECT_FALSE(fs::exists(company / "Test AB"));
  EXPECT_TRUE(fs::exists(company / "test.se")); // the SIE file stays where it is
  EXPECT_FALSE(fs::exists(company / "importerade_sie_filer"));
  EXPECT_EQ(navigator.path().size(), 2u);
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
  EXPECT_EQ(names_of(navigator.current().options()), (OptionsList{{U'1', "Test AB"}, {U'+', "Lägg till bokföring"}}));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Test AB");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "2026-01-01 – 2026-12-31  bokfört: inget"},
                                                        {U'1', "2025-01-01 – 2025-12-31  bokfört: 1 verifikation"},
                                                        {U'2', "2024-01-01 – 2024-12-31  bokfört: inget"}}));
}

namespace {

// Types text into the field the key leads to in the details form, and enters it
std::expected<void, std::string> fill(Navigator& navigator, CodePoint key, std::u32string_view text) {
  if (!navigator.press(key)) return std::unexpected("no field");
  auto result = navigator.current().on_input(text);
  if (!result) {
    navigator.back();
    return std::unexpected(result.error());
  }
  navigator.follow_back_to();
  return {};
}

} // namespace

TEST_F(RootMenuTest, FromDetailsIsAFormThatCreatesTheProjectOnEnter) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-09"));
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  ASSERT_TRUE(navigator.press(U'+'));
  ASSERT_TRUE(navigator.press(U'2'));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Från uppgifter");
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'1', "Namn: <ange organisationens namn>"},
                         {U'2', "Organisationsnummer: <ange organisationens organisationsnummer>"},
                         {U'3', "Räkenskapsår: <ange räkenskapsårets första månad>"},
                         {U'4', "Första årets längd: 12 månader"}}));
  // Enter before everything is given creates nothing
  EXPECT_FALSE(navigator.press(U'\n'));
  // A field is a text field, empty until given, then showing its value
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Namn");
  EXPECT_TRUE(navigator.current().is_text_field());
  EXPECT_TRUE(navigator.current().start_input().empty());
  ASSERT_TRUE(navigator.back());
  // Fields in any order; a refused value leaves the field as it was
  ASSERT_TRUE(fill(navigator, U'2', U"5566778899"));
  EXPECT_FALSE(fill(navigator, U'2', U"556677-8898"));
  ASSERT_TRUE(fill(navigator, U'1', U"Exempel AB"));
  ASSERT_TRUE(fill(navigator, U'3', U"2026-05"));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().start_input(), U"556677-8899");
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'1', "Namn: Exempel AB"}, {U'2', "Organisationsnummer: 556677-8899"},
                         {U'3', "Räkenskapsår: 2026-05-01 – 2027-04-30"}, {U'4', "Första årets längd: 12 månader"}}));
  ASSERT_TRUE(fill(navigator, U'4', U"8"));
  EXPECT_EQ(navigator.current().options()[2].second, "Räkenskapsår: 2026-05-01 – 2026-12-31");
  EXPECT_FALSE(fill(navigator, U'4', U"19"));
  EXPECT_FALSE(fs::exists(folder / "Exempel AB")); // nothing created before Enter
  // Enter creates and goes back to the workspace, which lists the project
  ASSERT_TRUE(navigator.press(U'\n'));
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
  EXPECT_EQ(names_of(navigator.current().options()), (OptionsList{{U'1', "Exempel AB"}, {U'+', "Lägg till bokföring"}}));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "2026-05-01 – 2026-12-31  bokfört: inget"},
                                                        {U'1', "2025-05-01 – 2026-04-30  bokfört: inget"}}));
}

TEST_F(RootMenuTest, WorkspaceShowsItsProjectsInColumnsReadFromEachProject) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-09"));
  add_project(folder, "Exempel AB", "1", "created = 2026-11-09\nuuid = 1\nname = Exempel AB\norgnr = 556677-8899\n");
  add_project(folder, "Åtta", "2", "uuid = 2\nname = Åtta AB\n"); // no orgnr, no created
  add_project(folder, "Fel", "3", "uuid = 9\nname = Fel AB\n");    // another uuid than remembered
  std::ofstream(folder / ".cratchit" / "workspace.txt", std::ios::app) << "project.4 = Borta\n";
  auto const workspace = cratchit::open_workspace(folder, "2026-11-09");
  ASSERT_TRUE(workspace);
  auto const projects = cratchit::list_projects(*workspace);
  ASSERT_TRUE(projects) << projects.error();
  auto const state = cratchit::ui::make_workspace_menu(*workspace, *projects);
  EXPECT_EQ(state->view(), (View{folder.string(), "    Namn        Org.nr       Skapad      Mapp"}));
  EXPECT_EQ(state->options(),
            (OptionsList{{U'1', "Borta       " "             " "            Borta  (finns inte längre)"},
                         {U'2', "Exempel AB  556677-8899  2026-11-09  Exempel AB"},
                         {U'3', "Fel         " "             " "            Fel  (går inte att öppna)"},
                         {U'4', "Åtta AB     " "             " "            Åtta"},
                         {U'+', "Lägg till bokföring"}}));
  // Read anew: a project renamed in its own file shows so at once
  std::ofstream(folder / "Exempel AB" / ".cratchit" / "project.txt") << "uuid = 1\nname = Annat namn AB\norgnr = 556677-8899\n";
  EXPECT_TRUE(state->options()[0].second.starts_with("Annat namn AB")) << state->options()[0].second;
}

TEST_F(RootMenuTest, FiscalYearExportsAllAndTheDifference) {
  Today const today("2025-11-09");
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-09"));
  auto const sie = [](std::string const& vouchers) {
    return "#FLAGGA 0\r\n#SIETYP 4\r\n#PROGRAM \"Annat\" 2.0\r\n#GEN 20251015\r\n#FNAMN \"Test AB\"\r\n"
           "#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n#KONTO 1930 \"Bank\"\r\n#KONTO 3010 \"Sales\"\r\n" +
           vouchers;
  };
  auto const sale = [](int number, std::string const& date) {
    return std::format("#VER A {} {} \"Sale\"\r\n{{\r\n#TRANS 1930 {{}} {}.00\r\n#TRANS 3010 {{}} -{}.00\r\n}}\r\n",
                       number, date, 100 * number, 100 * number);
  };
  fs::create_directories(documents / "Bolaget");
  std::ofstream(documents / "Bolaget" / "full.se", std::ios::binary) << sie(sale(1, "20250201") + sale(2, "20250301"));
  auto plan = cratchit::plan_project_from_sie(documents / "Bolaget" / "full.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  Navigator navigator(cratchit::ui::make_project_menu(*project));
  ASSERT_TRUE(navigator.press(U'0'));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Exportera till SIE");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Allt"}, {U'2', "Skillnad"}}));
  // Allt: written in the project folder, back in the year, which says so
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "2025");
  EXPECT_TRUE(fs::exists(project->folder / "sie_2025_allt.se"));
  ASSERT_EQ(navigator.current().view().size(), 5u);
  EXPECT_EQ(navigator.current().view()[1], "Hela året: 2 verifikationer");
  EXPECT_TRUE(navigator.current().view()[2].starts_with("cratchit skrev sie_2025_allt.se i ")) << navigator.current().view()[2];
  // Then the year's vouchers, newest first
  EXPECT_EQ(navigator.current().view()[3], "  A2            2025-03-01        200.00  Sale");
  EXPECT_EQ(navigator.current().view()[4], "  A1            2025-02-01        100.00  Sale");
  // Again: never overwritten
  ASSERT_TRUE(navigator.press(U'1'));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_TRUE(fs::exists(project->folder / "sie_2025_allt_2.se"));
  // The written file reads as SIE with the year's vouchers
  auto const written = cratchit::sie::lex_file(project->folder / "sie_2025_allt.se");
  ASSERT_TRUE(written);
  EXPECT_EQ(cratchit::sie::parse_document(written->lines).document.vouchers.size(), 2u);
  // Skillnad against a file that lacks the second sale
  std::ofstream(project->folder / "older.se", std::ios::binary) << sie(sale(1, "20250201"));
  ASSERT_TRUE(navigator.press(U'1'));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Skillnad");
  EXPECT_EQ(navigator.current().start_input(), pyroceph::tui::path_input_of(project->folder));
  auto refused = navigator.current().on_input(pyroceph::tui::path_input_of(project->folder) + U"gone.se");
  EXPECT_FALSE(refused);
  auto compared = navigator.current().on_input(pyroceph::tui::path_input_of(project->folder) + U"older.se");
  ASSERT_TRUE(compared) << compared.error();
  navigator.enter(std::move(compared->next));
  EXPECT_EQ(navigator.current().name(), "older.se");
  EXPECT_EQ(navigator.current().view()[0], "Jämfört med older.se (Annat 2.0, skapad 2025-10-15):");
  EXPECT_EQ(navigator.current().view()[1], "Filen skapades 2025-10-15. Är det den senaste exporten från programmet?");
  EXPECT_EQ(navigator.current().view()[2], "  I räkenskapsåret, inte i filen (1):");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Skriv skillnaden till en SIE-fil"}}));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "2025");
  EXPECT_TRUE(fs::exists(project->folder / "sie_2025_skillnad.se"));
  auto const difference = cratchit::sie::lex_file(project->folder / "sie_2025_skillnad.se");
  ASSERT_TRUE(difference);
  auto const document = cratchit::sie::parse_document(difference->lines).document;
  ASSERT_EQ(document.vouchers.size(), 1u);
  EXPECT_EQ(document.vouchers[0].number, "2");
  EXPECT_TRUE(document.opening.empty()); // no balances
  // Against a file with every voucher: nothing to write
  ASSERT_TRUE(navigator.press(U'1'));
  ASSERT_TRUE(navigator.press(U'2'));
  auto same = navigator.current().on_input(pyroceph::tui::path_input_of(documents / "Bolaget") + U"full.se"); // where it stayed
  ASSERT_TRUE(same) << same.error();
  navigator.enter(std::move(same->next));
  EXPECT_TRUE(navigator.current().options().empty());
  EXPECT_EQ(navigator.current().view().back(), "Alla verifikationer i räkenskapsåret finns redan i SIE-filen. Det finns inget att exportera.");
}

TEST_F(RootMenuTest, ImportListsTheChangedVouchersAPageAtATime) {
  Today const today("2025-11-09");
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-09"));
  auto const sie = [](bool changed) {
    std::string text = "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
                       "#KONTO 1930 \"Bank\"\r\n#KONTO 3010 \"Sales\"\r\n";
    for (int number = 1; number <= 12; ++number) {
      text += std::format("#VER A {} 202502{:02} \"Sale\"\r\n{{\r\n", number, number);
      text += changed ? "#BTRANS 1930 {} 100 \"\" \"\" \"\" \"K\"\r\n#BTRANS 3010 {} -100 \"\" \"\" \"\" \"K\"\r\n"
                        "#RTRANS 1930 {} 110 \"\" \"\" \"\" \"K\"\r\n#TRANS 1930 {} 110\r\n"
                        "#RTRANS 3010 {} -110 \"\" \"\" \"\" \"K\"\r\n#TRANS 3010 {} -110\r\n"
                      : "#TRANS 1930 {} 100\r\n#TRANS 3010 {} -100\r\n";
      text += "}\r\n";
    }
    return text;
  };
  fs::create_directories(documents / "Bolaget");
  std::ofstream(documents / "Bolaget" / "start.se", std::ios::binary) << sie(false);
  auto plan = cratchit::plan_project_from_sie(documents / "Bolaget" / "start.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  std::ofstream(documents / "changed.se", std::ios::binary) << sie(true);
  Navigator navigator(cratchit::ui::make_project_menu(*project));
  ASSERT_TRUE(navigator.press(U'0'));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Importera från SIE");
  // A file of another year is refused
  std::ofstream(documents / "other.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20260101 20261231\r\n";
  auto const other = navigator.current().on_input(pyroceph::tui::path_input_of(documents) + U"other.se");
  ASSERT_FALSE(other);
  EXPECT_EQ(other.error(), "cratchit kan inte importera filen: SIE-filen gäller räkenskapsåret 2026-01-01 – 2026-12-31, inte 2025");
  auto result = navigator.current().on_input(pyroceph::tui::path_input_of(documents) + U"changed.se");
  ASSERT_TRUE(result) << result.error();
  navigator.enter(std::move(result->next));
  auto const lines = [&navigator](std::string_view start) {
    std::size_t count = 0;
    for (auto const& line : navigator.current().view()) count += line.starts_with(start) ? 1 : 0;
    return count;
  };
  EXPECT_EQ(lines("  A  lika, t.o.m. A12"), 1u);
  EXPECT_EQ(lines("Importen går att göra: 0 nya verifikationer, 12 ändrade."), 1u);
  EXPECT_EQ(lines("    A"), 10u);
  EXPECT_EQ(lines("    … och 2 till. Tryck 2 för fler."), 1u);
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'J', "Ja"}, {U'N', "Nej"}, {U'2', "Mer…"}}));
  // Mer… shows the rest, in place
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Bekräfta");
  EXPECT_EQ(lines("    A"), 12u);
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'J', "Ja"}, {U'N', "Nej"}}));
  EXPECT_FALSE(navigator.press(U'2'));
  // Importing takes the changes and goes back to the fiscal year, which says so
  ASSERT_TRUE(navigator.press(U'J'));
  EXPECT_EQ(navigator.current().name(), "2025");
  ASSERT_EQ(navigator.current().view().size(), 14u); // then the ten newest of the year's 12 vouchers, and "… 2 äldre"
  EXPECT_EQ(navigator.current().view()[13], "  … 2 äldre");
  EXPECT_TRUE(navigator.current().view()[2].starts_with("cratchit importerade changed.se: ")) << navigator.current().view()[2];
  auto const books = cratchit::books::read_books(project->folder);
  ASSERT_TRUE(books) << books.error();
  EXPECT_EQ(books->years[0].vouchers[11].postings.size(), 4u);
}

TEST_F(RootMenuTest, AYearNotBookedYetCanBeImportedIntoAndExported) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-09"));
  auto const sie = [](int year, std::string const& vouchers) {
    return std::format("#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 {0}0101 {0}1231\r\n"
                       "#KONTO 1930 \"Bank\"\r\n#KONTO 3010 \"Sales\"\r\n{1}",
                       year, vouchers);
  };
  fs::create_directories(documents / "Bolaget");
  std::ofstream(documents / "Bolaget" / "start.se", std::ios::binary)
      << sie(2025, "#VER A 1 20250201 \"Sale\"\r\n{\r\n#TRANS 1930 {} 100\r\n#TRANS 3010 {} -100\r\n}\r\n");
  auto plan = cratchit::plan_project_from_sie(documents / "Bolaget" / "start.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  Navigator navigator(cratchit::ui::make_project_menu(*project));
  // 2026 is not booked: export gives an empty year
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "2026");
  EXPECT_EQ(navigator.current().view()[1], "Hela året: inget bokfört");
  ASSERT_TRUE(navigator.press(U'1'));
  ASSERT_TRUE(navigator.press(U'1'));
  auto const lexed = cratchit::sie::lex_file(project->folder / "sie_2026_allt.se");
  ASSERT_TRUE(lexed);
  EXPECT_TRUE(cratchit::sie::parse_document(lexed->lines).document.vouchers.empty());
  // Import into it, and it is in the books
  std::ofstream(documents / "next.se", std::ios::binary)
      << sie(2026, "#VER A 1 20260110 \"Sale\"\r\n{\r\n#TRANS 1930 {} 200\r\n#TRANS 3010 {} -200\r\n}\r\n");
  ASSERT_TRUE(navigator.press(U'2'));
  auto result = navigator.current().on_input(pyroceph::tui::path_input_of(documents) + U"next.se");
  ASSERT_TRUE(result) << result.error();
  navigator.enter(std::move(result->next));
  ASSERT_TRUE(navigator.press(U'J'));
  EXPECT_EQ(navigator.current().name(), "2026");
  EXPECT_EQ(navigator.current().view()[1], "Hela året: 1 verifikation");
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().options()[0].second, "2026-01-01 – 2026-12-31  bokfört: 1 verifikation");
  EXPECT_EQ(cratchit::list_fiscal_years(*project)->size(), 2u);
}

TEST_F(RootMenuTest, ANewYearShowsAtTheTurnOfTheYear) {
  Today today("2025-12-31");
  Navigator navigator(cratchit::ui::make_project_menu(project_with_books(documents / "p", "year.2025-01-01 = 2025-12-31\n")));
  EXPECT_EQ(navigator.current().options().size(), 2u); // 2025, and 2024 to import into
  ASSERT_TRUE(navigator.press(U'0'));
  cratchit::set_today("2026-01-01");
  ASSERT_TRUE(navigator.back()); // the years are read again on the way back
  EXPECT_EQ(navigator.current().options()[0].second, "2026-01-01 – 2026-12-31  bokfört: inget");
  EXPECT_EQ(navigator.current().options()[1].second, "2025-01-01 – 2025-12-31  bokfört: inget");
}

TEST_F(RootMenuTest, AChangeOfFiscalYearFromSieShowsAndTheYearTakesItsDates) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-09"));
  auto const sie = [](std::string const& start, std::string const& end, std::string const& day) {
    return std::format("#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 {} {}\r\n"
                       "#KONTO 1930 \"Bank\"\r\n#KONTO 3010 \"Sales\"\r\n"
                       "#VER A 1 {} \"Sale\"\r\n{{\r\n#TRANS 1930 {{}} 100\r\n#TRANS 3010 {{}} -100\r\n}}\r\n",
                       start, end, day);
  };
  fs::create_directories(documents / "Bolaget");
  std::ofstream(documents / "Bolaget" / "start.se", std::ios::binary) << sie("20250101", "20251231", "20250201");
  auto plan = cratchit::plan_project_from_sie(documents / "Bolaget" / "start.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  std::ofstream(documents / "change.se", std::ios::binary) << sie("20260101", "20260430", "20260201");
  Navigator navigator(cratchit::ui::make_project_menu(*project));
  ASSERT_TRUE(navigator.press(U'0')); // 2026, counted on from 2025
  ASSERT_TRUE(navigator.press(U'2'));
  auto result = navigator.current().on_input(pyroceph::tui::path_input_of(documents) + U"change.se");
  ASSERT_TRUE(result) << result.error();
  navigator.enter(std::move(result->next));
  auto const view = navigator.current().view();
  ASSERT_GE(view.size(), 3u);
  EXPECT_EQ(view[0], "SIE-fil: " + (documents / "change.se").string());
  EXPECT_EQ(view[1], "  Räkenskapsår:   2026-01-01 – 2026-04-30, ofullbordat (cratchit räknade med 2026-01-01 – 2026-12-31)");
  EXPECT_EQ(view[2], "  Ansvar:         cratchit kontrollerar inte Skatteverkets godkännande av en omläggning. Du ansvarar för att räkenskapsåret är korrekt.");
  ASSERT_TRUE(navigator.press(U'J'));
  EXPECT_EQ(navigator.current().name(), "2026");
  EXPECT_EQ(navigator.current().view()[0], "Räkenskapsår 2026-01-01 – 2026-04-30");
  EXPECT_EQ(navigator.current().view()[1], "Hela året: 1 verifikation");
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().options()[0].second, "2026-05-01 – 2027-04-30  bokfört: inget");
  EXPECT_EQ(navigator.current().options()[1].second, "2026-01-01 – 2026-04-30  bokfört: 1 verifikation");
}

// Starting in a project folder

TEST_F(RootMenuTest, StartInAFolderWithoutProjectStaysAtTheRoot) {
  auto const workspace = documents / "cratchit";
  Navigator navigator(cratchit::ui::make_root_menu(workspace));
  EXPECT_TRUE(cratchit::ui::open_working_folder_project(navigator, workspace, documents).empty());
  EXPECT_EQ(navigator.path().size(), 1u);
  EXPECT_FALSE(fs::exists(workspace)); // nothing created
}

TEST_F(RootMenuTest, StartInAProjectFolderOpensTheProjectAndRemembersIt) {
  auto const workspace = documents / "cratchit";
  auto const project = documents / "elsewhere" / "itfied";
  fs::create_directories(project / ".cratchit");
  std::ofstream(project / ".cratchit" / "project.txt") << "uuid = 7\nname = The ITfied AB\n";
  {
    Navigator navigator(cratchit::ui::make_root_menu(workspace));
    EXPECT_EQ(cratchit::ui::open_working_folder_project(navigator, workspace, project),
              (std::vector<std::string>{"cratchit öppnade bokföringen i arbetsmappen: The ITfied AB",
                                        "Samlingsmappen kommer ihåg The ITfied AB."}));
    ASSERT_EQ(navigator.path().size(), 3u);
    EXPECT_EQ(navigator.path()[1]->name(), "Samlingsmapp");
    EXPECT_EQ(navigator.current().name(), "The ITfied AB");
    // Back in the workspace, which lists it
    ASSERT_TRUE(navigator.back());
    EXPECT_EQ(names_of(navigator.current().options()),
              (OptionsList{{U'1', "The ITfied AB"}, {U'+', "Lägg till bokföring"}}));
  }
  auto const entries = cratchit::read_path_values(workspace / ".cratchit" / "workspace.txt");
  ASSERT_TRUE(entries);
  EXPECT_EQ(cratchit::find_value(*entries, "project.7"), project.string());
  {
    // Started there again: already remembered, nothing more to say
    Navigator navigator(cratchit::ui::make_root_menu(workspace));
    EXPECT_EQ(cratchit::ui::open_working_folder_project(navigator, workspace, project),
              (std::vector<std::string>{"cratchit öppnade bokföringen i arbetsmappen: The ITfied AB"}));
  }
  {
    // A copy with the same uuid elsewhere (e.g. test data): remembered there instead
    auto const copy = documents / "copy";
    fs::create_directories(copy / ".cratchit");
    fs::copy_file(project / ".cratchit" / "project.txt", copy / ".cratchit" / "project.txt");
    Navigator navigator(cratchit::ui::make_root_menu(workspace));
    EXPECT_EQ(cratchit::ui::open_working_folder_project(navigator, workspace, copy),
              (std::vector<std::string>{
                  "cratchit öppnade bokföringen i arbetsmappen: The ITfied AB",
                  std::format("Samlingsmappen kommer ihåg The ITfied AB här i stället för i {}.", project.string())}));
  }
}

TEST_F(RootMenuTest, StartInAFolderWithABrokenProjectStaysAtTheRootAndSaysWhy) {
  auto const workspace = documents / "cratchit";
  fs::create_directories(documents / "broken" / ".cratchit");
  std::ofstream(documents / "broken" / ".cratchit" / "project.txt") << "uuid = 7\n"; // no name
  Navigator navigator(cratchit::ui::make_root_menu(workspace));
  auto const lines = cratchit::ui::open_working_folder_project(navigator, workspace, documents / "broken");
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_TRUE(lines[0].starts_with("cratchit kunde inte öppna bokföringen i arbetsmappen: ")) << lines[0];
  EXPECT_EQ(navigator.path().size(), 1u);
  EXPECT_FALSE(fs::exists(workspace / ".cratchit" / "workspace.txt"));
}

// A new project in the working folder (1 in the root)

TEST_F(RootMenuTest, OneIsOfferedOnlyWhenTheWorkspaceFolderIsKnownAndHoldsNoProject) {
  auto const workspace = documents / "cratchit";
  EXPECT_EQ(cratchit::ui::make_root_menu(workspace)->options(),
            (OptionsList{{U'0', "Samlingsmapp"}, {U'9', "Testmeny"}}));
  fs::create_directories(documents / "company");
  auto const root = cratchit::ui::make_root_menu(workspace, documents / "company");
  EXPECT_EQ(root->options(), (OptionsList{{U'0', "Samlingsmapp"}, {U'1', "Ny bokföring här"}, {U'9', "Testmeny"}}));
  EXPECT_TRUE(root->view().empty());
  fs::create_directories(documents / "company" / ".cratchit");
  std::ofstream(documents / "company" / ".cratchit" / "project.txt") << "uuid = 1\nname = Företaget AB\n";
  EXPECT_EQ(root->options(), (OptionsList{{U'0', "Samlingsmapp"}, {U'9', "Testmeny"}}));
  EXPECT_EQ(root->view(), (View{"Arbetsmappen har bokföringen Företaget AB. Du hittar den i samlingsmappen."}));
}

TEST_F(RootMenuTest, NewProjectHereIsCreatedInTheWorkingFolderAndRemembered) {
  auto const workspace = documents / "cratchit";
  auto const company = documents / "The ITfied";
  fs::create_directories(company / "Avtal");
  Navigator navigator(cratchit::ui::make_root_menu(workspace, company));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Ny bokföring här");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Från SIE"}, {U'2', "Från uppgifter"}}));
  EXPECT_EQ(navigator.current().view(), (View{std::format("Bokföringen hamnar i {}.", company.string())}));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Från uppgifter");
  EXPECT_EQ(navigator.current().view().back(), std::format("Bokföringen hamnar i {}.", company.string()));
  EXPECT_FALSE(fill(navigator, U'1', U"   ")); // blank
  ASSERT_TRUE(fill(navigator, U'1', U"The ITfied AB"));
  ASSERT_TRUE(fill(navigator, U'2', U"556782-8172"));
  ASSERT_TRUE(fill(navigator, U'3', U"2026-05"));
  EXPECT_FALSE(fs::exists(company / ".cratchit")); // nothing created before Enter
  // Enter creates it here and ends in the workspace, which lists it
  ASSERT_TRUE(navigator.press(U'\n'));
  ASSERT_EQ(navigator.path().size(), 2u);
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
  EXPECT_EQ(navigator.current().view().front(), "cratchit skapade bokföringen The ITfied AB.");
  EXPECT_EQ(names_of(navigator.current().options()),
            (OptionsList{{U'1', "The ITfied AB"}, {U'+', "Lägg till bokföring"}}));
  EXPECT_TRUE(fs::is_regular_file(company / ".cratchit" / "project.txt"));
  EXPECT_TRUE(fs::is_directory(company / "Avtal"));
  EXPECT_FALSE(fs::exists(workspace / "The ITfied AB")); // no folder in the workspace
  // Back in the root, which no longer offers 1
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "Samlingsmapp"}, {U'9', "Testmeny"}}));
  EXPECT_EQ(navigator.current().view(),
            (View{"Arbetsmappen har bokföringen The ITfied AB. Du hittar den i samlingsmappen."}));
}

TEST_F(RootMenuTest, NewProjectHereIsRefusedWithTheReason) {
  auto const workspace = documents / "cratchit";
  fs::create_directories(workspace);
  Navigator navigator(cratchit::ui::make_root_menu(workspace, workspace));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Fel");
  EXPECT_EQ(navigator.current().view(),
            (View{"cratchit skapar ingen bokföring här. Arbetsmappen är samlingsmappen. Starta cratchit i bokföringens egen mapp."}));
  EXPECT_FALSE(fs::exists(workspace / ".cratchit" / "project.txt"));
}

TEST_F(RootMenuTest, NewProjectHereFromSieStartsInTheWorkingFolderAndCreatesTheProjectThere) {
  auto const workspace = documents / "cratchit";
  auto const seed = documents / "_seed";
  fs::create_directories(seed / "older");
  auto const sie = "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB: provet\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
                   "#KONTO 1930 \"Bank\"\r\n#KONTO 3010 \"Sales\"\r\n"
                   "#VER A 1 20250201 \"Sale\"\r\n{\r\n#TRANS 1930 {} 500.00\r\n#TRANS 3010 {} -500.00\r\n}\r\n";
  std::ofstream(seed / "older" / "test.se", std::ios::binary) << sie; // the user looks it up
  Navigator navigator(cratchit::ui::make_root_menu(workspace, seed));
  ASSERT_TRUE(navigator.press(U'1'));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Från SIE");
  // The path starts in the working folder, not in the workspace, with nothing filled in
  EXPECT_EQ(navigator.current().start_input(), pyroceph::tui::path_input_of(seed));
  auto result = navigator.current().on_input(pyroceph::tui::path_input_of(seed / "older") + U"test.se");
  ASSERT_TRUE(result) << result.error(); // the name need not be a folder name (':'), and the file may lie below
  ASSERT_TRUE(result->next);
  navigator.enter(std::move(result->next));
  EXPECT_EQ(navigator.current().name(), "Bekräfta");
  EXPECT_EQ(navigator.current().view(),
            (View{"SIE-fil: " + (seed / "older" / "test.se").string(),
                  "  Mapp:           " + seed.string(),
                  "  Företag:        Test AB: provet (556000-0001)",
                  "  Räkenskapsår:   2025-01-01 – 2025-12-31, ofullbordat",
                  "  Verifikationer: 1 t.o.m. 2025-02-01  A1",
                  "  Konton:         2",
                  "Skapa bokföringen från SIE-filen?"}));
  EXPECT_FALSE(fs::exists(seed / ".cratchit")); // nothing created before J
  // Keys match exactly: j and n do nothing
  EXPECT_FALSE(navigator.press(U'j'));
  EXPECT_FALSE(navigator.press(U'n'));
  EXPECT_EQ(navigator.current().name(), "Bekräfta");
  EXPECT_FALSE(fs::exists(seed / ".cratchit"));
  ASSERT_TRUE(navigator.press(U'J'));
  // Created in the working folder itself; the SIE file stays where it is, recorded in the inventory
  EXPECT_TRUE(fs::is_regular_file(seed / ".cratchit" / "project.txt"));
  EXPECT_TRUE(fs::is_regular_file(seed / "older" / "test.se"));
  EXPECT_FALSE(fs::exists(seed / "importerade_sie_filer"));
  auto const inventory = cratchit::read_inventory(seed, cratchit::parse_iso_date("2025-01-01"));
  ASSERT_TRUE(inventory) << inventory.error();
  ASSERT_EQ(inventory->size(), 1u);
  EXPECT_EQ((*inventory)[0].path, "older/test.se");
  // Ends in the workspace, which lists it
  ASSERT_EQ(navigator.path().size(), 2u);
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
  EXPECT_EQ(navigator.current().view().front(), "cratchit skapade bokföringen Test AB: provet.");
  EXPECT_EQ(names_of(navigator.current().options()),
            (OptionsList{{U'1', "Test AB: provet"}, {U'+', "Lägg till bokföring"}}));
}

TEST_F(RootMenuTest, NewProjectHereFromSieTakesAFileAlreadyInImportedSieFiles) {
  // E.g. starting over: .cratchit removed, the file left in importerade_sie_filer
  auto const workspace = documents / "cratchit";
  auto const seed = documents / "_seed";
  fs::create_directories(seed / "importerade_sie_filer");
  std::ofstream(seed / "importerade_sie_filer" / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 1930 \"Bank\"\r\n";
  Navigator navigator(cratchit::ui::make_root_menu(workspace, seed));
  ASSERT_TRUE(navigator.press(U'1'));
  ASSERT_TRUE(navigator.press(U'1'));
  auto result = navigator.current().on_input(pyroceph::tui::path_input_of(seed / "importerade_sie_filer") + U"test.se");
  ASSERT_TRUE(result) << result.error();
  navigator.enter(std::move(result->next));
  EXPECT_EQ(navigator.current().view()[1], "  Mapp:           " + seed.string()); // here, not the file's folder
  ASSERT_TRUE(navigator.press(U'J'));
  EXPECT_TRUE(fs::is_regular_file(seed / ".cratchit" / "project.txt"));
  EXPECT_FALSE(fs::exists(seed / "importerade_sie_filer" / ".cratchit"));
  EXPECT_TRUE(fs::is_regular_file(seed / "importerade_sie_filer" / "test.se"));
}

TEST_F(RootMenuTest, NoInConfirmGoesBackToChooseAnotherFileInTheSameFolder) {
  auto const workspace = documents / "cratchit";
  auto const seed = documents / "_seed";
  fs::create_directories(seed);
  std::ofstream(seed / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#PROGRAM \"Fortnox\" 3.62\r\n#GEN 20261019\r\n#FNAMN \"Test AB\"\r\n"
         "#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n#KONTO 1930 \"Bank\"\r\n";
  Navigator navigator(cratchit::ui::make_root_menu(workspace, seed));
  ASSERT_TRUE(navigator.press(U'1'));
  ASSERT_TRUE(navigator.press(U'1'));
  auto result = navigator.current().on_input(pyroceph::tui::path_input_of(seed) + U"test.se");
  ASSERT_TRUE(result) << result.error();
  navigator.enter(std::move(result->next));
  EXPECT_EQ(navigator.current().view()[4], "  Skapad:         2026-10-19 (Fortnox 3.62)");
  ASSERT_TRUE(navigator.press(U'N'));
  EXPECT_EQ(navigator.current().name(), "Från SIE");
  EXPECT_EQ(navigator.current().start_input(), pyroceph::tui::path_input_of(seed));
  EXPECT_FALSE(fs::exists(seed / ".cratchit"));
  EXPECT_TRUE(fs::exists(seed / "test.se"));
}

TEST_F(RootMenuTest, ConfirmWarnsForAnIncompleteYearOlderThanLastYearAndForAMismatch) {
  auto const workspace = documents / "cratchit";
  auto const seed = documents / "_seed";
  fs::create_directories(seed);
  auto const head = std::string("#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n"
                                "#KONTO 1930 \"Bank\"\r\n#KONTO 3010 \"Sales\"\r\n#KONTO 8999 \"Result\"\r\n#KONTO 2099 \"Result\"\r\n");
  auto const sale = std::string("#VER A 1 20240201 \"Sale\"\r\n{\r\n#TRANS 1930 {} 500.00\r\n#TRANS 3010 {} -500.00\r\n}\r\n");
  // 2024 ended more than a year before the tests' today (2026-11-09)
  std::ofstream(seed / "old.se", std::ios::binary) << head << "#RAR 0 20240101 20241231\r\n" << sale;
  std::ofstream(seed / "wrong.se", std::ios::binary)
      << head << "#RAR 0 20240101 20241231\r\n" << sale
      << "#VER M 1 20241231 \"Result\"\r\n{\r\n#TRANS 8999 {} 400.00\r\n#TRANS 2099 {} -400.00\r\n}\r\n";
  auto const confirm = [&](char32_t const* name) {
    Navigator navigator(cratchit::ui::make_root_menu(workspace, seed));
    EXPECT_TRUE(navigator.press(U'1'));
    EXPECT_TRUE(navigator.press(U'1'));
    auto result = navigator.current().on_input(pyroceph::tui::path_input_of(seed) + name);
    EXPECT_TRUE(result) << result.error();
    navigator.enter(std::move(result->next));
    return navigator.current().view();
  };
  auto const old = confirm(U"old.se");
  EXPECT_EQ(old[3], "  Räkenskapsår:   2024-01-01 – 2024-12-31, ofullbordat");
  EXPECT_EQ(old[4], "  Varning:        Ett räkenskapsår före förra året ska vara fullbordat. Kontrollera SIE-filen.");
  auto const wrong = confirm(U"wrong.se");
  EXPECT_EQ(wrong[3], "  Räkenskapsår:   2024-01-01 – 2024-12-31, misstämmer");
  EXPECT_EQ(wrong[4], "  Varning:        Ett resultat är bokfört på 8999, men balansräkningen balanserar inte (skillnad 100.00).");
}

TEST_F(RootMenuTest, ImportInTheWayOffersToReplaceTheYearWhichBurnsTheBridges) {
  auto const folder = documents / "cratchit";
  auto const sie = [](std::string const& vouchers) {
    return "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
           "#KONTO 1930 \"Bank\"\r\n#KONTO 3010 \"Sales\"\r\n" + vouchers;
  };
  auto const sale = [](int number, char const* date, char const* amount) {
    return std::format("#VER A {} {} \"Sale\"\r\n{{\r\n#TRANS 1930 {{}} {}\r\n#TRANS 3010 {{}} -{}\r\n}}\r\n", number, date,
                       amount, amount);
  };
  fs::create_directories(folder);
  std::ofstream(documents / "full.se", std::ios::binary) << sie(sale(1, "20250201", "100") + sale(2, "20250301", "200"));
  auto plan = cratchit::plan_project_from_sie(documents / "full.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  fs::create_directories(documents / "older");
  std::ofstream(documents / "older" / "less.se", std::ios::binary) << sie(sale(1, "20250201", "100"));
  auto workspace = cratchit::open_workspace(folder, "2026-11-09");
  ASSERT_TRUE(workspace);
  ASSERT_TRUE(cratchit::remember_project(*workspace, *project));
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  ASSERT_TRUE(navigator.press(U'1'));
  ASSERT_TRUE(navigator.press(U'1')); // 2025, the year before the one now
  ASSERT_TRUE(navigator.press(U'2'));
  auto result = navigator.current().on_input(pyroceph::tui::path_input_of(documents / "older") + U"less.se");
  ASSERT_TRUE(result) << result.error();
  navigator.enter(std::move(result->next));
  EXPECT_EQ(navigator.current().name(), "Bekräfta");
  auto const view = navigator.current().view();
  auto const has = [&view](std::string const& line) { return std::ranges::find(view, line) != view.end(); };
  EXPECT_TRUE(has("Jämfört med bokföringen (2025):"));
  EXPECT_TRUE(has("  A  bokföringen t.o.m. A2, filen t.o.m. A1"));
  EXPECT_TRUE(has("      A2  2025-03-01  Sale  200.00  finns inte i filen"));
  EXPECT_TRUE(has("Importen går inte att göra. Bokföringen har 1 verifikation som filen skulle ta bort eller byta ut."));
  EXPECT_EQ(view.back(), "Ersätt året med SIE-filens innehåll?");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'J', "Ja"}, {U'N', "Nej"}}));
  // J shows what replacing takes away; nothing is changed yet
  ASSERT_TRUE(navigator.press(U'J'));
  EXPECT_EQ(navigator.current().name(), "Ersätt");
  EXPECT_EQ(navigator.current().view(),
            (View{"Räkenskapsåret 2025 blir exakt som SIE-filen.", "Det här försvinner ur bokföringen:",
                  "  A2  2025-03-01  Sale  200.00  finns inte i filen", "Ersätt räkenskapsåret med SIE-filen?"}));
  EXPECT_EQ(cratchit::books::read_books(project->folder)->years[0].vouchers.size(), 2u);
  // - goes back a step, N back to choose another file
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Bekräfta");
  ASSERT_TRUE(navigator.press(U'N'));
  EXPECT_EQ(navigator.current().name(), "Importera från SIE");
  EXPECT_EQ(navigator.current().start_input(), pyroceph::tui::path_input_of(documents / "older"));
  // J, J: the year is the file's, and the fiscal year says what is gone
  result = navigator.current().on_input(pyroceph::tui::path_input_of(documents / "older") + U"less.se");
  ASSERT_TRUE(result) << result.error();
  navigator.enter(std::move(result->next));
  ASSERT_TRUE(navigator.press(U'J'));
  ASSERT_TRUE(navigator.press(U'J'));
  EXPECT_EQ(navigator.current().name(), "2025");
  auto const after = navigator.current().view();
  EXPECT_EQ(after[2], "cratchit ersatte räkenskapsåret med less.se: 1 verifikationer, alla från filen.");
  EXPECT_EQ(after[3], "Det här försvann. Bokför det igen från underlag eller kontoutdrag om det behövs:");
  EXPECT_EQ(after[4], "  A2  2025-03-01  Sale  200.00  finns inte i filen");
  auto const books = cratchit::books::read_books(project->folder);
  ASSERT_TRUE(books) << books.error();
  ASSERT_EQ(books->years[0].vouchers.size(), 1u);
  EXPECT_EQ(books->years[0].vouchers[0].number, "1");
  auto const inventory = cratchit::read_inventory(project->folder, cratchit::parse_iso_date("2025-01-01"));
  ASSERT_TRUE(inventory) << inventory.error();
  EXPECT_EQ(inventory->back().use, "ersätt");
  EXPECT_TRUE(fs::exists(documents / "older" / "less.se")); // the file stays where it is
}

TEST_F(RootMenuTest, ImportWithAGapIsNotPossibleAndNotReplaceable) {
  auto const folder = documents / "cratchit";
  auto const sie = [](std::string const& vouchers) {
    return "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
           "#KONTO 1930 \"Bank\"\r\n#KONTO 3010 \"Sales\"\r\n" + vouchers;
  };
  auto const sale = [](int number, char const* date) {
    return std::format("#VER A {} {} \"Sale\"\r\n{{\r\n#TRANS 1930 {{}} 100\r\n#TRANS 3010 {{}} -100\r\n}}\r\n", number, date);
  };
  fs::create_directories(folder);
  std::ofstream(documents / "start.se", std::ios::binary) << sie(sale(1, "20250201") + sale(2, "20250301"));
  auto plan = cratchit::plan_project_from_sie(documents / "start.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  // A 3 is missing, and A 2 is gone: in the way, but the gap keeps it from being replaced
  std::ofstream(documents / "gap.se", std::ios::binary) << sie(sale(1, "20250201") + sale(4, "20250401"));
  auto import = cratchit::plan_sie_import(*project, documents / "gap.se");
  ASSERT_TRUE(import) << import.error();
  EXPECT_FALSE(import->possible());
  EXPECT_FALSE(import->replaceable);
  EXPECT_FALSE(cratchit::replace_year_with_sie(*project, *import, *cratchit::parse_iso_date("2026-11-09")));
}

TEST_F(RootMenuTest, FromDetailsRefusesAFirstYearOlderThanNineBeforeTheCurrent) {
  // The tests' today is 2026-11-09: 2016 ended ten years before 2026
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-09"));
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  ASSERT_TRUE(navigator.press(U'+'));
  ASSERT_TRUE(navigator.press(U'2'));
  ASSERT_TRUE(navigator.press(U'2'));
  ASSERT_TRUE(navigator.press(U'3'));
  auto const refused = navigator.current().on_input(U"2016-01");
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "cratchit hanterar bara det aktuella räkenskapsåret och 9 äldre. 2016-01 är för tidigt.");
  EXPECT_TRUE(navigator.current().on_input(U"2017-01"));
}

TEST_F(RootMenuTest, FromSieWarnsWhenTheYearWillNotShow) {
  auto const workspace = documents / "cratchit";
  auto const seed = documents / "_seed";
  fs::create_directories(seed);
  std::ofstream(seed / "old.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20160101 20161231\r\n";
  Navigator navigator(cratchit::ui::make_root_menu(workspace, seed));
  ASSERT_TRUE(navigator.press(U'1'));
  ASSERT_TRUE(navigator.press(U'1'));
  auto result = navigator.current().on_input(pyroceph::tui::path_input_of(seed) + U"old.se");
  ASSERT_TRUE(result) << result.error();
  navigator.enter(std::move(result->next));
  auto const view = navigator.current().view();
  EXPECT_NE(std::ranges::find(view, "  Varning:        cratchit visar bara det aktuella räkenskapsåret och 9 äldre. 2016 kommer inte att synas."),
            view.end());
}

TEST_F(RootMenuTest, BookedShowsTheVouchersNewestFirstAPageAtATime) {
  // 45 vouchers in 2025, A 1 on January 1st up to A 45
  auto const folder = documents / "cratchit";
  fs::create_directories(folder);
  std::string vouchers;
  for (int i = 1; i <= 45; ++i) {
    auto const date = *cratchit::parse_iso_date("2025-01-01") ;
    auto const day = std::chrono::sys_days(date) + std::chrono::days(i - 1);
    auto const text = cratchit::format_iso_date(std::chrono::year_month_day(day));
    vouchers += std::format("#VER A {} {}{}{} \"Sale {}\"\r\n{{\r\n#TRANS 1930 {{}} 100\r\n#TRANS 3010 {{}} -100\r\n}}\r\n", i,
                            text.substr(0, 4), text.substr(5, 2), text.substr(8, 2), i);
  }
  std::ofstream(documents / "many.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 1930 \"Bank\"\r\n#KONTO 3010 \"Sales\"\r\n"
      << vouchers;
  auto plan = cratchit::plan_project_from_sie(documents / "many.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  Navigator navigator(cratchit::ui::make_booked_menu(*project, {"2025-01-01", "2025-12-31"}));
  auto const& booked = navigator.current();
  EXPECT_EQ(booked.name(), "Bokfört");
  auto view = booked.view();
  ASSERT_EQ(view.size(), 22u); // heading, 20 vouchers, "…"
  EXPECT_EQ(view[0], "Bokfört 2025: verifikationer 1–20 av 45");
  EXPECT_TRUE(view[1].starts_with("  A45 ")) << view[1];
  EXPECT_TRUE(view[20].starts_with("  A26 ")) << view[20];
  EXPECT_EQ(view[21], "  …");
  EXPECT_EQ(booked.options(), (OptionsList{{U'0', "Gör till utkast"}, {U'<', "Föregående sida"}, {U'>', "Nästa sida"}}));
  EXPECT_FALSE(navigator.press(U'<')); // at the first page: nothing
  // > turns a page in place
  ASSERT_TRUE(navigator.press(U'>'));
  EXPECT_EQ(navigator.path().size(), 1u);
  view = navigator.current().view();
  EXPECT_EQ(view[0], "Bokfört 2025: verifikationer 21–40 av 45");
  EXPECT_EQ(view[1], "  …");
  EXPECT_TRUE(view[2].starts_with("  A25 ")) << view[2];
  EXPECT_EQ(view.back(), "  …");
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'0', "Gör till utkast"}, {U'<', "Föregående sida"}, {U'>', "Nästa sida"}}));
  ASSERT_TRUE(navigator.press(U'>'));
  view = navigator.current().view();
  EXPECT_EQ(view[0], "Bokfört 2025: verifikationer 41–45 av 45");
  EXPECT_TRUE(view.back().starts_with("  A1 ")) << view.back();
  EXPECT_FALSE(navigator.press(U'>')); // at the last page: nothing
  ASSERT_TRUE(navigator.press(U'<'));
  EXPECT_EQ(navigator.current().view()[0], "Bokfört 2025: verifikationer 21–40 av 45");
}

TEST_F(RootMenuTest, BookedSaysWhenNothingIsBooked) {
  auto const folder = documents / "Exempel AB";
  ASSERT_TRUE(cratchit::create_project_here(fs::create_directories(folder) ? folder : folder, "Exempel AB", "556677-8899",
                                            {"2025-01-01", "2025-12-31"}, "2026-11-09"));
  auto const project = cratchit::open_project(folder);
  ASSERT_TRUE(project) << project.error();
  auto const booked = cratchit::ui::make_booked_menu(*project, {"2025-01-01", "2025-12-31"});
  EXPECT_EQ(booked->view(), (View{"Bokfört 2025: inget bokfört i räkenskapsåret."}));
}

TEST_F(RootMenuTest, BookedOpensAVoucherByItsIdAsOneWord) {
  auto const folder = documents / "cratchit";
  fs::create_directories(folder);
  auto const ver = [](char const* number, char const* rows) {
    return std::format("#VER D {} 20250201 \"Faktura {}\" 20250205\r\n{{\r\n{}}}\r\n", number, number, rows);
  };
  std::string const plain = "#TRANS 2440 {} -125.00\r\n#TRANS 6210 {} 100.00\r\n#TRANS 2641 {} 25.00\r\n";
  std::string const changed = "#TRANS 2440 {} -125.00\r\n#BTRANS 6210 {} 100.00 \"\" \"\" \"\" \"Kjell\"\r\n"
                              "#RTRANS 6230 {} 100.00 \"\" \"Rättat konto\" \"\" \"Kjell\"\r\n#TRANS 6230 {} 100.00 \"\" \"Rättat konto\"\r\n"
                              "#TRANS 2641 {} 25.00\r\n";
  std::ofstream(documents / "d.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 2440 \"Leverantörsskulder\"\r\n#KONTO 6210 \"Telekommunikation\"\r\n#KONTO 6230 \"Datakommunikation\"\r\n"
         "#KONTO 2641 \"Debiterad ingående moms\"\r\n"
      << ver("1", plain.c_str()) << ver("11", changed.c_str());
  auto plan = cratchit::plan_project_from_sie(documents / "d.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  Navigator navigator(cratchit::ui::make_booked_menu(*project, {"2025-01-01", "2025-12-31"}));
  // Typed otherwise, or not in the year: refused with the reason
  auto spaced = navigator.current().on_input(U"D 1");
  ASSERT_FALSE(spaced);
  EXPECT_EQ(spaced.error(), "Skriv verifikationens id som ett ord, till exempel A12.");
  auto missing = navigator.current().on_input(U"D2");
  ASSERT_FALSE(missing);
  EXPECT_EQ(missing.error(), "Det finns ingen verifikation D2 i 2025.");
  EXPECT_FALSE(navigator.current().on_input(U"d1")); // the series as written
  // D1 is never D11
  auto one = navigator.current().on_input(U"D1");
  ASSERT_TRUE(one) << one.error();
  navigator.enter(std::move(one->next));
  EXPECT_EQ(navigator.current().name(), "D1");
  EXPECT_EQ(navigator.current().view(),
            (View{"Verifikation D1", "  Serie:          D  Leverantörsfakturor", "  Registrerad:    2025-02-05",
                  "Händelse:", "  Datum:          2025-02-01", "  Rubrik:         Faktura 1", "  Belopp:         125.00",
                  "Konteringar:",
                  "  2440  Leverantörsskulder            -125.00", "  6210  Telekommunikation              100.00",
                  "  2641  Debiterad ingående moms         25.00"}));
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'0', "Händelse"}, {U'1', "Konteringar"}, {U'2', "Ändringsverifikation"}, {U'3', "Utskrift"}}));
  // The last in D is D11, the highest number (not the last in the file, nor by text)
  ASSERT_TRUE(navigator.press(U'0'));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_NE(navigator.current().view()[1].find("I serie D är det D11."), std::string::npos) << navigator.current().view()[1];
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.back());
  auto eleven = navigator.current().on_input(U"D11");
  ASSERT_TRUE(eleven) << eleven.error();
  navigator.enter(std::move(eleven->next));
  EXPECT_EQ(navigator.current().name(), "D11");
  auto const view = navigator.current().view();
  auto const has = [&view](std::string const& line) { return std::ranges::find(view, line) != view.end(); };
  EXPECT_TRUE(has("  6210  Telekommunikation              100.00  (struken)"));
  EXPECT_TRUE(has("  6230  Datakommunikation              100.00  Rättat konto  (tillagd)"));
  EXPECT_FALSE(has("  Signatur:       Kjell")); // the signature of a row is not the voucher's
}

TEST(VoucherMenu, HandelseAndKonteringarShowTheVoucherInParts) {
  using cratchit::books::Posting;
  cratchit::books::Voucher voucher;
  voucher.series = "D";
  voucher.number = "19";
  voucher.event = {"Faktura Websupport", 123375, *cratchit::parse_iso_date("2026-02-27")};
  voucher.registered = cratchit::parse_iso_date("2026-04-17");
  voucher.signature = "KO";
  auto const posting = [](char const* account, cratchit::Amount amount, Posting::Kind kind = Posting::Kind::normal) {
    Posting p;
    p.kind = kind;
    p.account = account;
    p.amount = amount;
    return p;
  };
  voucher.postings = {posting("2440", -123375), posting("6210", 98700, Posting::Kind::removed), posting("2641", 24675),
                      posting("6230", 98700, Posting::Kind::added)};
  voucher.postings.back().text = "Rättat konto";
  voucher.postings.back().date = cratchit::parse_iso_date("2026-05-02");
  std::vector<cratchit::books::Account> accounts;
  for (auto const& [number, name] : {std::pair{"2440", "Leverantörsskulder"}, std::pair{"6210", "Telekommunikation"},
                                     std::pair{"2641", "Debiterad ingående moms"}, std::pair{"6230", "Datakommunikation"}}) {
    cratchit::books::Account account;
    account.number = number;
    account.name = name;
    accounts.push_back(account);
  }
  Navigator navigator(cratchit::ui::make_voucher_menu(voucher, accounts, "Leverantörsfakturor", "D23"));
  auto const view = navigator.current().view();
  ASSERT_GE(view.size(), 4u);
  EXPECT_EQ(view[1], "  Serie:          D  Leverantörsfakturor");
  EXPECT_EQ(view[2], "  Registrerad:    2026-04-17");
  EXPECT_EQ(view[3], "  Signatur:       KO");
  EXPECT_EQ(view[4], "Händelse:");
  // 0 Händelse: date, heading and amount, which cannot be changed yet
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Händelse");
  EXPECT_EQ(navigator.current().view(), (View{"Händelsen i D19"}));
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'0', "Datum: 2026-02-27"}, {U'1', "Rubrik: Faktura Websupport"}, {U'2', "Belopp: 1233.75"}}));
  // The event of a booked voucher is locked; D19 is not the last in D, D23 is
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Låst (Datum)");
  EXPECT_EQ(navigator.current().view(),
            (View{"Det går inte att ändra datumet i en bokförd verifikation.",
                  "Bara den sista verifikationen i en serie går att göra om till ett utkast och ändra fritt. I serie D är "
                  "det D23. Är händelsen i D19 fel rättar du den med en ny verifikation."}));
  EXPECT_TRUE(navigator.current().options().empty());
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Låst (Belopp)");
  EXPECT_EQ(navigator.current().view()[0], "Det går inte att ändra beloppet i en bokförd verifikation.");
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.back());
  // 1 Konteringar: the postings numbered, the struck one too
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Konteringar");
  EXPECT_EQ(navigator.current().view(), (View{"Konteringar i D19"}));
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'0', "2440  Leverantörsskulder           -1233.75"},
                         {U'1', "6210  Telekommunikation              987.00  (struken)"},
                         {U'2', "2641  Debiterad ingående moms        246.75"},
                         {U'3', "6230  Datakommunikation              987.00  Rättat konto  2026-05-02  (tillagd)"}}));
  EXPECT_FALSE(navigator.press(U'4')); // no such row
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "1");
  EXPECT_EQ(navigator.current().view(), (View{"Kontering 1 i D19", "  Raden är struken."}));
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'0', "Konto: 6210  Telekommunikation"}, {U'1', "Beskrivning:"}, {U'2', "Belopp: 987.00"}}));
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'3'));
  EXPECT_EQ(navigator.current().view(), (View{"Kontering 3 i D19", "  Datum:          2026-05-02", "  Raden är tillagd."}));
  EXPECT_EQ(navigator.current().options()[1], (pyroceph::tui::Option{U'1', "Beskrivning: Rättat konto"}));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Inte än (Konto)");
  EXPECT_EQ(navigator.current().view(), (View{"Utvecklaren av cratchit har inte infört att ändra kontot i en kontering ännu."}));
}

TEST(VoucherMenu, KonteringarAreTurnedTenAtATime) {
  cratchit::books::Voucher voucher;
  voucher.series = "A";
  voucher.number = "1";
  voucher.event = {"Lön", 0, *cratchit::parse_iso_date("2026-02-27")};
  for (int i = 0; i < 12; ++i) {
    cratchit::books::Posting p;
    p.account = std::to_string(7000 + i);
    p.amount = i % 2 == 0 ? 100 : -100;
    voucher.postings.push_back(p);
  }
  Navigator navigator(cratchit::ui::make_voucher_menu(voucher, {}, "", "A1"));
  EXPECT_EQ(navigator.current().view()[1], "  Serie:          A"); // a series without name
  // A1 is the last in A: it may become a draft, with 0 in Bokfört
  ASSERT_TRUE(navigator.press(U'0'));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Låst (Rubrik)");
  EXPECT_EQ(navigator.current().view(),
            (View{"Det går inte att ändra rubriken i en bokförd verifikation.",
                  "A1 är den sista verifikationen i serie A. Den sista i en serie går att göra om till ett utkast, där du "
                  "kan ändra händelsen fritt och sedan bokföra den igen: tryck 0 (Gör till utkast) i Bokfört."}));
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().view(), (View{"Konteringar i A1: 1–10 av 12"}));
  auto options = navigator.current().options();
  ASSERT_EQ(options.size(), 12u);
  EXPECT_EQ(options[9].first, U'9');
  EXPECT_EQ(options[10], (pyroceph::tui::Option{U'<', "Föregående sida"}));
  EXPECT_EQ(options[11], (pyroceph::tui::Option{U'>', "Nästa sida"}));
  EXPECT_FALSE(navigator.press(U'<')); // already at the start: nothing happens
  ASSERT_TRUE(navigator.press(U'>'));
  EXPECT_EQ(navigator.current().view(), (View{"Konteringar i A1: 11–12 av 12"}));
  options = navigator.current().options();
  ASSERT_EQ(options.size(), 4u);
  EXPECT_EQ(options[0].second.substr(0, 4), "7010");
  EXPECT_FALSE(navigator.press(U'>')); // at the last page: nothing happens
  EXPECT_FALSE(navigator.press(U'2')); // no such row on this page
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "11");
}

TEST_F(RootMenuTest, DraftsSayHowManyTheYearHas) {
  auto const folder = documents / "Exempel AB";
  fs::create_directories(folder);
  ASSERT_TRUE(cratchit::create_project_here(folder, "Exempel AB", "556677-8899", {"2025-01-01", "2025-12-31"},
                                            "2026-11-09"));
  auto const project = cratchit::open_project(folder);
  ASSERT_TRUE(project) << project.error();
  Navigator navigator(cratchit::ui::make_draft_menu(*project, {"2025-01-01", "2025-12-31"}));
  EXPECT_EQ(navigator.current().name(), "Utkast");
  EXPECT_EQ(navigator.current().view(), (View{"Utkast 2025: inga utkast."}));
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::drafts);
  ASSERT_TRUE(navigator.press(U'3'));
  EXPECT_EQ(navigator.current().name(), "Läs kontoutdrag");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::read_statement);
  EXPECT_EQ(navigator.current().start_input(), pyroceph::tui::path_input_of(folder));
}

TEST_F(RootMenuTest, ReadStatementCompletesAnyFileAndRefusesWhatItCannotRead) {
  auto const folder = documents / "Exempel AB";
  fs::create_directories(folder / "bank");
  ASSERT_TRUE(cratchit::create_project_here(folder, "Exempel AB", "556677-8899", {"2025-01-01", "2025-12-31"},
                                            "2026-11-09"));
  auto const project = cratchit::open_project(folder);
  ASSERT_TRUE(project) << project.error();
  std::ofstream(folder / "bank" / "nordea_2025.csv") << "Bokföringsdag;Belopp\n";
  Navigator navigator(cratchit::ui::make_read_statement_menu(*project, {"2025-01-01", "2025-12-31"}, nullptr, {}));
  auto const bank = pyroceph::tui::path_input_of(folder / "bank");
  // Tab completes the folder, then any file in it (no filter on the kind of file)
  auto const separator = bank.back();
  EXPECT_EQ(navigator.current().complete(pyroceph::tui::path_input_of(folder) + U"ba").added,
            U"nk" + std::u32string(1, separator));
  EXPECT_EQ(navigator.current().complete(bank + U"n").added, U"ordea_2025.csv");
  // Nothing typed, a folder or a missing file is refused
  EXPECT_FALSE(navigator.current().on_input(U""));
  auto refused = navigator.current().on_input(bank);
  ASSERT_FALSE(refused);
  EXPECT_TRUE(refused.error().ends_with("är en mapp. Välj en fil i den med Tab.")) << refused.error();
  refused = navigator.current().on_input(bank + U"saknas.csv");
  ASSERT_FALSE(refused);
  EXPECT_TRUE(refused.error().starts_with("Det finns ingen fil ")) << refused.error();
  // A file that is no statement is refused with the reason, and the path starts in its folder after
  refused = navigator.current().on_input(bank + U"nordea_2025.csv");
  ASSERT_FALSE(refused);
  EXPECT_TRUE(refused.error().starts_with("cratchit kan inte läsa filen som kontoutdrag.")) << refused.error();
  EXPECT_EQ(navigator.current().start_input(), bank);
}

TEST_F(RootMenuTest, ReadStatementMakesDraftsOnTheAccountTyped) {
  std::ofstream(documents / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 1930 \"Bank\"\r\n#KONTO 3010 \"Sales\"\r\n#KONTO 6570 \"Bank costs\"\r\n"
         "#VER A 1 20250301 \"Sale\"\r\n{\r\n#TRANS 1930 {} 500\r\n#TRANS 3010 {} -500\r\n}\r\n";
  auto plan = cratchit::plan_project_from_sie(documents / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  fs::create_directories(documents / "bank");
  std::ofstream(documents / "bank" / "nordea.csv", std::ios::binary) << "\xEF\xBB\xBF"
                                                               "Datum;Belopp;Text;Saldo\n"
                                                               "2025/03/05;-10,00;AVGIFTER;490,00\n"
                                                               "2025/03/01;500,00;Kund;500,00\n";
  Navigator navigator(cratchit::ui::make_draft_menu(*project, {"2025-01-01", "2025-12-31"}));
  ASSERT_TRUE(navigator.press(U'3'));
  auto accepted = navigator.current().on_input(pyroceph::tui::path_input_of(documents / "bank") + U"nordea.csv");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  EXPECT_EQ(navigator.current().name(), "Konto");
  EXPECT_EQ(navigator.current().start_input(), U"1930"); // suggested: A1 has 500 on 1930 that day
  auto const view = navigator.current().view();
  EXPECT_NE(std::ranges::find(view, "  Kodning   UTF-8 med BOM"), view.end());
  EXPECT_NE(std::ranges::find(view, "Förslag: 1930 Bank (1 av 2 rader stämmer med konteringar på 1930 i bokföringen)."),
            view.end());
  EXPECT_FALSE(navigator.current().on_input(U"1931")); // not in the chart of accounts
  accepted = navigator.current().on_input(U"1930");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  EXPECT_EQ(navigator.current().name(), "Bekräfta");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'J', "Ja"}, {U'N', "Nej"}}));
  auto const confirm = navigator.current().view();
  EXPECT_NE(std::ranges::find(confirm, "  Nya utkast      1"), confirm.end());
  EXPECT_NE(std::ranges::find(confirm, "  Redan bokförda  1"), confirm.end());
  // J makes the draft and goes back to Utkast, which tells what was done
  ASSERT_TRUE(navigator.press(U'J'));
  EXPECT_EQ(navigator.current().name(), "Utkast");
  EXPECT_EQ(navigator.current().view(),
            (View{"Utkast 2025: utkast 1–1 av 1",
                  "  #1            2025-03-05        -10.00  1930  AVGIFTER (kontohändelse)",
                  "cratchit skapade 1 utkast från nordea.csv på konto 1930 Bank."}));
  EXPECT_FALSE(navigator.press(U'>')); // one page: nothing
  // The next statement starts in the folder of this one
  ASSERT_TRUE(navigator.press(U'3'));
  EXPECT_EQ(navigator.current().start_input(), pyroceph::tui::path_input_of(documents / "bank"));
}

TEST_F(RootMenuTest, DescribeEventsMakesADraftOfEachEvent) {
  auto const folder = documents / "Exempel AB";
  fs::create_directories(folder);
  ASSERT_TRUE(cratchit::create_project_here(folder, "Exempel AB", "556677-8899", {"2025-01-01", "2025-12-31"},
                                            "2026-11-09"));
  auto const project = cratchit::open_project(folder);
  ASSERT_TRUE(project) << project.error();
  Navigator navigator(cratchit::ui::make_draft_menu(*project, {"2025-01-01", "2025-12-31"}));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Beskriv händelser");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::describe_event);
  EXPECT_EQ(navigator.current().start_input(), U"");
  // Errors keep the line
  auto refused = navigator.current().on_input(U"Telia 1118");
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Inget datum. Skriv datumet, till exempel 2026-10-22.");
  refused = navigator.current().on_input(U"Telia 1118 2026-01-02");
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "2026-01-02 ligger utanför räkenskapsåret 2025-01-01 – 2025-12-31.");
  // An unclear amount: choose the reading
  auto accepted = navigator.current().on_input(U"Telia faktura 1 118,00 2025-10-22");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  EXPECT_EQ(navigator.current().name(), "Belopp?");
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'1', "      118.00  Telia faktura 1"}, {U'2', "     1118.00  Telia faktura"}}));
  // - goes back to the line, kept
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Beskriv händelser");
  EXPECT_EQ(navigator.current().start_input(), U"Telia faktura 1 118,00 2025-10-22");
  accepted = navigator.current().on_input(U"Telia faktura 1 118,00 2025-10-22");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  // A reading makes the draft at once, without confirmation, and comes back here with an empty line
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Beskriv händelser");
  EXPECT_EQ(navigator.current().start_input(), U"");
  auto view = navigator.current().view();
  ASSERT_EQ(view.size(), 6u);
  EXPECT_EQ(view[4], "Skapade nu:");
  EXPECT_EQ(view[5], "  2025-10-22       1118.00  Telia faktura");
  // A clear line makes the draft at once too, and the user stays for the next
  accepted = navigator.current().on_input(U"Loopia 1308,75 2025-09-28");
  ASSERT_TRUE(accepted) << accepted.error();
  EXPECT_FALSE(accepted->next);
  EXPECT_FALSE(navigator.follow_back_to());
  EXPECT_EQ(navigator.current().name(), "Beskriv händelser");
  EXPECT_EQ(navigator.current().start_input(), U"");
  view = navigator.current().view();
  ASSERT_EQ(view.size(), 7u);
  EXPECT_EQ(view[6], "  2025-09-28       1308.75  Loopia");
  // - goes back to Utkast, which lists them and tells how many
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Utkast");
  EXPECT_EQ(navigator.current().view(), (View{"Utkast 2025: utkast 1–2 av 2",
                                              "  #1            2025-10-22       1118.00  Telia faktura (händelse)",
                                              "  #2            2025-09-28       1308.75  Loopia (händelse)",
                                              "cratchit skapade 2 utkast."}));
}

TEST_F(RootMenuTest, ReadPdfChoosesAFolderShowingItsPdfFiles) {
  auto const folder = documents / "Exempel AB";
  fs::create_directories(folder / "underlag" / "äldre");
  ASSERT_TRUE(cratchit::create_project_here(folder, "Exempel AB", "556677-8899", {"2025-01-01", "2025-12-31"},
                                            "2026-11-09"));
  auto const project = cratchit::open_project(folder);
  ASSERT_TRUE(project) << project.error();
  std::ofstream(folder / "underlag" / "faktura.pdf") << "%PDF-1.4\n";
  std::ofstream(folder / "underlag" / "Kvitto.PDF") << "%PDF-1.4\n";
  std::ofstream(folder / "underlag" / "anteckning.txt") << "x\n";
  Navigator navigator(cratchit::ui::make_draft_menu(*project, {"2025-01-01", "2025-12-31"}));
  ASSERT_TRUE(navigator.press(U'4'));
  EXPECT_EQ(navigator.current().name(), "Läs pdf-underlag");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::read_pdf);
  EXPECT_EQ(navigator.current().start_input(), pyroceph::tui::path_input_of(folder));
  // Tab completes the folder and shows its pdf files, not to choose
  auto const underlag = pyroceph::tui::path_input_of(folder / "underlag");
  auto completion = navigator.current().complete(pyroceph::tui::path_input_of(folder) + U"un");
  EXPECT_EQ(completion.added, underlag.substr(pyroceph::tui::path_input_of(folder).size() + 2));
  EXPECT_EQ(completion.notes, (std::vector<std::string>{"2 pdf-filer i mappen:", "  Kvitto.PDF", "  faktura.pdf"}));
  completion = navigator.current().complete(underlag + U"f");
  EXPECT_TRUE(completion.added.empty()); // files are not completed
  // A file or a missing folder is refused
  auto refused = navigator.current().on_input(underlag + U"faktura.pdf");
  ASSERT_FALSE(refused);
  EXPECT_TRUE(refused.error().ends_with("är en fil. Välj mappen den ligger i.")) << refused.error();
  refused = navigator.current().on_input(underlag + U"saknas");
  ASSERT_FALSE(refused);
  EXPECT_TRUE(refused.error().starts_with("Det finns ingen mapp ")) << refused.error();
  // A folder lists its pdf files with what walk_pdf finds in them
  auto accepted = navigator.current().on_input(underlag);
  ASSERT_TRUE(accepted) << accepted.error();
  ASSERT_TRUE(accepted->next);
  navigator.enter(std::move(accepted->next));
  EXPECT_EQ(navigator.current().name(), "Pdf-underlag");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::pdf_list);
  EXPECT_EQ(navigator.current().view(), (View{"Pdf-underlag i " + (folder / "underlag").string() + ": 2 filer",
                                              "    1  Kvitto.PDF   inga sidor hittade",
                                              "    2  faktura.pdf  inga sidor hittade"}));
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'0', "Tolka som händelser"}, {U'<', "Föregående sida"}, {U'>', "Nästa sida"}}));
  EXPECT_FALSE(navigator.press(U'>')); // one page: nothing
  // Läs kontoutdrag starts in the folder chosen last
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'3'));
  EXPECT_EQ(navigator.current().start_input(), underlag);
}

TEST_F(RootMenuTest, CommandHintsUseTheProjectsExportAndLastYearsReturn) {
  std::ofstream(documents / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#RAR -1 20240101 20241231\r\n#KONTO 1930 \"Bank\"\r\n";
  auto plan = cratchit::plan_project_from_sie(documents / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  cratchit::FiscalYear const year{"2025-01-01", "2025-12-31"};
  // No export yet, and no return of last year
  auto facts = cratchit::ui::command_hint_facts(*project, year);
  EXPECT_EQ(facts.orgnr, "556000-0001");
  EXPECT_FALSE(facts.sie);
  EXPECT_EQ(facts.output_folder, "556000-0001_2025-01-01_2025-12-31");
  EXPECT_FALSE(facts.last_return_folder);
  auto lines = cratchit::ui::tax_return_hint(facts);
  EXPECT_NE(std::ranges::find(lines, "     cratchit --sie-to-ink2 <SIE-fil> --previous-deficit 0"), lines.end());
  // Two exports: the latest counts. Last year's return folder with its BLANKETTER.SRU is found.
  std::ofstream(project->folder / "sie_2025_allt.se") << "x";
  std::ofstream(project->folder / "sie_2025_allt_2.se") << "x";
  fs::create_directories(project->folder / "556000-0001_2024-01-01_2024-12-31");
  std::ofstream(project->folder / "556000-0001_2024-01-01_2024-12-31" / "BLANKETTER.SRU") << "x";
  facts = cratchit::ui::command_hint_facts(*project, year);
  ASSERT_TRUE(facts.sie);
  EXPECT_EQ(facts.sie->filename(), "sie_2025_allt_2.se");
  EXPECT_EQ(facts.last_return_folder, "556000-0001_2024-01-01_2024-12-31");
  lines = cratchit::ui::annual_report_hint(facts);
  EXPECT_NE(std::ranges::find(lines, "     cratchit --sie-to-ar sie_2025_allt_2.se"), lines.end());
  lines = cratchit::ui::tax_return_hint(facts);
  EXPECT_NE(std::ranges::find(lines, "     cratchit --sie-to-ink2 sie_2025_allt_2.se 556000-0001_2024-01-01_2024-12-31"),
            lines.end());
}

namespace {

// A one-page PDF with the lines given, as walk_pdf's own tests make them
std::string pdf_with_lines(std::vector<std::string> const& lines) {
  std::string widths;
  for (int i = 0; i < 224; ++i) widths += "500 ";
  std::string content = "BT /F1 10 Tf 12 TL 72 700 Td";
  for (auto const& line : lines) content += " (" + line + ") Tj T*";
  content += " ET";
  return std::string{"%PDF-1.4\n"
                     "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"
                     "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n"
                     "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 4 0 R"
                     " /Resources << /Font << /F1 5 0 R >> >> >>\nendobj\n"
                     "4 0 obj\n<< /Length "} +
         std::to_string(content.size()) + " >>\nstream\n" + content +
         "\nendstream\nendobj\n"
         "5 0 obj\n<< /Type /Font /Subtype /Type1 /Encoding /WinAnsiEncoding /FirstChar 32 /Widths [" +
         widths + "] >>\nendobj\ntrailer\n<< /Size 6 /Root 1 0 R >>\n%%EOF\n";
}

} // namespace

TEST_F(RootMenuTest, InterpretGoesThroughThePdfFilesOneAtATime) {
  auto const folder = documents / "Exempel AB";
  fs::create_directories(folder / "underlag");
  ASSERT_TRUE(cratchit::create_project_here(folder, "Exempel AB", "556677-8899", {"2025-01-01", "2025-12-31"},
                                            "2026-11-09"));
  auto const project = cratchit::open_project(folder);
  ASSERT_TRUE(project) << project.error();
  std::ofstream(folder / "underlag" / "a_faktura.pdf", std::ios::binary) << pdf_with_lines({"Faktura", "Summa 615,00"});
  std::vector<std::string> many;
  for (int i = 1; i <= 25; ++i) many.push_back("Rad " + std::to_string(i));
  std::ofstream(folder / "underlag" / "b_lang.pdf", std::ios::binary) << pdf_with_lines(many);
  std::ofstream(folder / "underlag" / "c_trasig.pdf", std::ios::binary) << "x";
  Navigator navigator(cratchit::ui::make_draft_menu(*project, {"2025-01-01", "2025-12-31"}));
  ASSERT_TRUE(navigator.press(U'4'));
  auto accepted = navigator.current().on_input(pyroceph::tui::path_input_of(folder / "underlag"));
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  // The files were read with the list: going through them reads nothing again
  fs::remove(folder / "underlag" / "a_faktura.pdf");
  ASSERT_TRUE(navigator.press(U'0'));
  // The first, with its text framed, a blank line before and after
  auto const top = std::string(27, '=') + " sida 1 av 1 " + std::string(28, '=');
  auto const bottom = std::string(68, '=');
  EXPECT_EQ(navigator.current().name(), "Underlag 1");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::document);
  EXPECT_EQ(navigator.current().view(), (View{"Underlag 1 av 3: a_faktura.pdf (text, 1 sida)", "", top, "Faktura",
                                              "Summa 615,00", bottom, ""}));
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'<', "Föregående"}, {U'0', "Glöm"}, {U'1', "Tolka"}, {U'>', "Nästa"}}));
  EXPECT_FALSE(navigator.press(U'<')); // the first: nothing
  // The next, cut after 20 lines, how many more under the frame
  ASSERT_TRUE(navigator.press(U'>'));
  EXPECT_EQ(navigator.current().name(), "Underlag 2");
  auto const view = navigator.current().view();
  ASSERT_EQ(view.size(), 26u);
  EXPECT_EQ(view[2], top);
  EXPECT_EQ(view[3], "Rad 1");
  EXPECT_EQ(view[22], "Rad 20");
  EXPECT_EQ(view[23], bottom);
  EXPECT_EQ(view[24], "… 5 rader till");
  EXPECT_EQ(view[25], "");
  // A file without text shows its status
  ASSERT_TRUE(navigator.press(U'>'));
  EXPECT_EQ(navigator.current().view(),
            (View{"Underlag 3 av 3: c_trasig.pdf (inte en pdf)", "", "Ingen text att visa.", ""}));
  EXPECT_FALSE(navigator.press(U'>')); // the last: nothing
  // Tolka shows the whole text, without options; back to the same file
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Tolka");
  EXPECT_EQ(navigator.current().view(), (View{"Tolka c_trasig.pdf (inte en pdf)", "", "Ingen text att tolka.", "",
                                              "Att göra en händelse av underlaget finns inte än."}));
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Underlag 3");
  ASSERT_TRUE(navigator.press(U'<'));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::interpret);
  EXPECT_TRUE(navigator.current().options().empty());
  auto const whole = navigator.current().view();
  ASSERT_EQ(whole.size(), 31u); // the heading, a blank, the frame around 25 lines, a blank and the last line
  EXPECT_EQ(whole[0], "Tolka b_lang.pdf (text, 1 sida, 25 rader)");
  EXPECT_EQ(whole[27], "Rad 25");
  EXPECT_EQ(whole[28], bottom);
  EXPECT_EQ(whole[30], "Att göra en händelse av underlaget finns inte än.");
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'>'));
  // Glöm is not there yet
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Inte än (Glöm)");
}

TEST_F(RootMenuTest, ADraftIsOpenedByItsIdAndShownAsAVoucher) {
  std::ofstream(documents / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 1930 \"Bank\"\r\n";
  auto plan = cratchit::plan_project_from_sie(documents / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  fs::create_directories(documents / "bank");
  std::ofstream(documents / "bank" / "nordea.csv", std::ios::binary) << "\xEF\xBB\xBF"
                                                                       "Datum;Belopp;Text;Saldo\n"
                                                                       "2025/03/05;-10,00;AVGIFTER;490,00\n"
                                                                       "2025/03/01;500,00;Kund;500,00\n";
  Navigator navigator(cratchit::ui::make_draft_menu(*project, {"2025-01-01", "2025-12-31"}));
  // Drafts from the statement (#3 Kund, #2 AVGIFTER) and an event (#1, later)
  ASSERT_TRUE(navigator.press(U'3'));
  auto accepted = navigator.current().on_input(pyroceph::tui::path_input_of(documents / "bank") + U"nordea.csv");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  accepted = navigator.current().on_input(U"1930");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  ASSERT_TRUE(navigator.press(U'J'));
  ASSERT_TRUE(navigator.press(U'0'));
  accepted = navigator.current().on_input(U"Telia faktura 1118,00 2025-10-22");
  ASSERT_TRUE(accepted) << accepted.error();
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Utkast");
  // Ids typed wrong, or not there
  auto refused = navigator.current().on_input(U"1");
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Skriv utkastets id som #13.");
  refused = navigator.current().on_input(U"#4");
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Det finns inget utkast #4 i 2025.");
  EXPECT_FALSE(navigator.current().on_input(U"#0"));
  // A draft from the statement
  accepted = navigator.current().on_input(U"#2");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  EXPECT_EQ(navigator.current().name(), "#2");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::draft);
  EXPECT_EQ(navigator.current().view(),
            (View{"Utkast #2 i 2025 (kontohändelse)", "  Noterad:        " + cratchit::today_iso(), "Händelse:",
                  "  Datum:          2025-03-05", "  Rubrik:         ", "  Belopp:         ", "Konteringar:",
                  "  1930  Bank        -10.00  AVGIFTER", "Innan utkastet kan bokföras:",
                  "  Rubrik saknas: tryck 0 och sedan 1.", "  Belopp saknas: tryck 0 och sedan 2.",
                  "  Konteringarna balanserar inte, kvar debet 10.00: tryck 1."}));
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "Händelse"}, {U'1', "Konteringar"}, {U'3', "Använd mall"}, {U'9', "Glöm"}}));
  // Its postings are changed in Kontera, which tells where a posting came from
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Kontera");
  auto const kontera = navigator.current().view();
  EXPECT_EQ(kontera.back(), "1930 från bank/nordea.csv rad 2"); // the file's line 2
  auto typed = navigator.current().on_input(U"1930 10");
  ASSERT_TRUE(typed) << typed.error();
  ASSERT_TRUE(navigator.back());
  // Back in the draft: shown as it is now
  EXPECT_EQ(navigator.current().name(), "#2");
  auto const now = navigator.current().view();
  EXPECT_NE(std::ranges::find(now, "  1930  Bank         10.00"), now.end());
  // Its event: each part changed for itself; back in the draft, shown as it is now
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Händelse");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::complete_event);
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'0', "Datum: 2025-03-05"}, {U'1', "Rubrik:"}, {U'2', "Belopp:"}, {U'3', "Serie:"}}));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Rubrik");
  typed = navigator.current().on_input(U"");
  ASSERT_FALSE(typed);
  EXPECT_EQ(typed.error(), "Skriv rubriken.");
  typed = navigator.current().on_input(U"Bankavgift rättad");
  ASSERT_TRUE(typed) << typed.error();
  navigator.follow_back_to();
  EXPECT_EQ(navigator.current().name(), "Händelse");
  EXPECT_EQ(navigator.current().options()[1], (std::pair<char32_t, std::string>{U'1', "Rubrik: Bankavgift rättad"}));
  // A date outside the year, or not a date: refused
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().start_input(), U"2025-03-05");
  typed = navigator.current().on_input(U"2026-01-02");
  ASSERT_FALSE(typed);
  EXPECT_EQ(typed.error(), "cratchit kunde inte ändra utkastet: 2026-01-02 ligger inte i räkenskapsåret 2025-01-01 – 2025-12-31.");
  typed = navigator.current().on_input(U"igår");
  ASSERT_FALSE(typed);
  EXPECT_EQ(typed.error(), "igår är inget datum. Skriv till exempel 2026-10-22.");
  ASSERT_TRUE(navigator.back());
  // The amount: typed in from the parts, a zero refused
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().start_input(), U"10.00");
  typed = navigator.current().on_input(U"0");
  ASSERT_FALSE(typed);
  EXPECT_EQ(typed.error(), "Beloppet är noll.");
  typed = navigator.current().on_input(U"-10,00");
  ASSERT_TRUE(typed) << typed.error();
  navigator.follow_back_to();
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "#2");
  EXPECT_EQ(navigator.current().view()[4], "  Rubrik:         Bankavgift rättad");
  EXPECT_EQ(navigator.current().view()[5], "  Belopp:         -10.00");
  ASSERT_TRUE(navigator.back());
  // The event without postings
  accepted = navigator.current().on_input(U"#1");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  auto const view = navigator.current().view();
  EXPECT_EQ(view[0], "Utkast #1 i 2025 (händelse)");
  EXPECT_EQ(view[4], "  Rubrik:         Telia faktura");
  EXPECT_EQ(view[5], "  Belopp:         1118.00");
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Kontera");
  EXPECT_EQ(navigator.current().view().back(), "Inga konteringar än.");
  ASSERT_TRUE(navigator.back());
  // 9 Glöm, apart from the other options; N goes back to the draft
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "Händelse"}, {U'1', "Konteringar"}, {U'3', "Använd mall"}, {U'9', "Glöm"}}));
  ASSERT_TRUE(navigator.press(U'9'));
  EXPECT_EQ(navigator.current().name(), "Glöm");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::forget_draft);
  EXPECT_EQ(navigator.current().view(),
            (View{"Glöm utkast #1?", "  2025-10-22       1118.00  Telia faktura (händelse)",
                  "Utkastet tas bort ur bokföringen och går inte att få tillbaka."}));
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'9', "Glöm"}, {U'N', "Nej"}}));
  ASSERT_TRUE(navigator.press(U'N'));
  EXPECT_EQ(navigator.current().name(), "#1");
  // 9 again forgets it, and Utkast tells so
  ASSERT_TRUE(navigator.press(U'9'));
  ASSERT_TRUE(navigator.press(U'9'));
  EXPECT_EQ(navigator.current().name(), "Utkast");
  auto const after = navigator.current().view();
  EXPECT_EQ(after.front(), "Utkast 2025: utkast 1–2 av 2");
  EXPECT_EQ(after.back(), "cratchit glömde utkast #1: Telia faktura (händelse).");
  auto const books = cratchit::books::read_books(project->folder);
  ASSERT_TRUE(books) << books.error();
  EXPECT_EQ(books->years.front().drafts.size(), 2u);
}

TEST_F(RootMenuTest, SameListsPairsToMergeOrForgetAsBooked) {
  std::ofstream(documents / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 1930 \"Bank\"\r\n#KONTO 1510 \"Kundfordringar\"\r\n"
         "#VER \"A\" \"1\" 20250301 \"Kundinbetalning\"\r\n{\r\n#TRANS 1930 {} 500.00\r\n#TRANS 1510 {} -500.00\r\n}\r\n";
  auto plan = cratchit::plan_project_from_sie(documents / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  fs::create_directories(documents / "bank");
  std::ofstream(documents / "bank" / "nordea.csv", std::ios::binary) << "Datum;Belopp;Text;Saldo\n"
                                                                       "2025/03/05;-10,00;AVGIFTER;490,00\n"
                                                                       "2025/03/01;500,00;Kund;500,00\n";
  Navigator navigator(cratchit::ui::make_draft_menu(*project, {"2025-01-01", "2025-12-31"}));
  // #3 Kund betalar (in A1 already), #2 Bankavgift (the event) and #1 AVGIFTER (its payment);
  // the entry Kund is in A1, so it makes no draft
  ASSERT_TRUE(navigator.press(U'3'));
  auto accepted = navigator.current().on_input(pyroceph::tui::path_input_of(documents / "bank") + U"nordea.csv");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  accepted = navigator.current().on_input(U"1930");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  ASSERT_TRUE(navigator.press(U'J'));
  ASSERT_TRUE(navigator.press(U'0'));
  ASSERT_TRUE(navigator.current().on_input(U"Bankavgift 10,00 2025-03-04"));
  ASSERT_TRUE(navigator.current().on_input(U"Kund betalar 500,00 2025-03-02"));
  ASSERT_TRUE(navigator.back());
  // The pairs, the likely first: the one that fits
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Samma?");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::same);
  EXPECT_EQ(navigator.current().view(),
            (View{"Möjliga par i 2025 (samma belopp, högst 4 dagar emellan):",
                  "  1  utkast   #2            2025-03-04         10.00  Bankavgift (händelse)",
                  "              #1            2025-03-05        -10.00  1930  AVGIFTER (kontohändelse)",
                  "              1 dag emellan",
                  "  2  bokfört  #3            2025-03-02        500.00  Kund betalar (händelse)",
                  "              A1            2025-03-01        500.00  Kundinbetalning",
                  "              1 dag emellan"}));
  auto refused = navigator.current().on_input(U"3");
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Det finns inget par 3.");
  refused = navigator.current().on_input(U"#2");
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Skriv parets nummer, till exempel 1.");
  // Two drafts: both in full, 0 makes them one at once
  accepted = navigator.current().on_input(U"1");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  EXPECT_EQ(navigator.current().name(), "1");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::same_pair);
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "Samma!"}}));
  auto view = navigator.current().view();
  EXPECT_EQ(view.front(), "Samma händelse? 1 dag emellan.");
  EXPECT_EQ(view.back(), "Det ena är en händelse och det andra en kontohändelse.");
  EXPECT_NE(std::ranges::find(view, "    1930  Bank        -10.00  AVGIFTER"), view.end());
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Samma?");
  EXPECT_EQ(navigator.current().view(),
            (View{"Möjliga par i 2025 (samma belopp, högst 4 dagar emellan):",
                  "  1  bokfört  #2            2025-03-02        500.00  Kund betalar (händelse)",
                  "              A1            2025-03-01        500.00  Kundinbetalning",
                  "              1 dag emellan",
                  "cratchit slog ihop #2 och #1 till #1: Bankavgift : AVGIFTER (samma)."}));
  // A draft already booked: 9 and 9 again forget it
  accepted = navigator.current().on_input(U"1");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'9', "Glöm"}}));
  EXPECT_EQ(navigator.current().view().back(), "Utkastet har inga konteringar.");
  ASSERT_TRUE(navigator.press(U'9'));
  EXPECT_EQ(navigator.current().name(), "Glöm");
  ASSERT_TRUE(navigator.press(U'N')); // no: back to the pair
  EXPECT_EQ(navigator.current().name(), "1");
  ASSERT_TRUE(navigator.press(U'9'));
  ASSERT_TRUE(navigator.press(U'9'));
  EXPECT_EQ(navigator.current().name(), "Samma?");
  EXPECT_EQ(navigator.current().view(),
            (View{"Inga möjliga par i 2025 (samma belopp, högst 4 dagar emellan).",
                  "cratchit slog ihop #2 och #1 till #1: Bankavgift : AVGIFTER (samma).",
                  "cratchit glömde utkast #2, redan bokfört som A1: Kund betalar (händelse)."}));
  // Utkast tells what was done
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().view(),
            (View{"Utkast 2025: utkast 1–1 av 1",
                  "  #1            2025-03-04         10.00  Bankavgift : AVGIFTER (samma)",
                  "cratchit slog ihop #2 och #1 till #1: Bankavgift : AVGIFTER (samma).",
                  "cratchit glömde utkast #2, redan bokfört som A1: Kund betalar (händelse)."}));
  // The one draft is made of the same amount: 9 is Inte samma, and splits it at once
  auto opened = navigator.current().on_input(U"#1");
  ASSERT_TRUE(opened) << opened.error();
  navigator.enter(std::move(opened->next));
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'0', "Händelse"}, {U'1', "Konteringar"}, {U'3', "Använd mall"}, {U'9', "Inte samma"}}));
  ASSERT_TRUE(navigator.press(U'9'));
  EXPECT_EQ(navigator.current().name(), "Utkast");
  EXPECT_EQ(navigator.current().view(),
            (View{"Utkast 2025: utkast 1–2 av 2",
                  "  #1            2025-03-05        -10.00  1930  AVGIFTER (kontohändelse)",
                  "  #2            2025-03-04         10.00  Bankavgift (händelse)",
                  "cratchit delade #1 i 2 utkast."}));
  // Each is lone again: 9 is Glöm
  opened = navigator.current().on_input(U"#2");
  ASSERT_TRUE(opened) << opened.error();
  navigator.enter(std::move(opened->next));
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "Händelse"}, {U'1', "Konteringar"}, {U'3', "Använd mall"}, {U'9', "Glöm"}}));
}

TEST_F(RootMenuTest, CompleteGoesThroughTheDraftsFromTheOldest) {
  std::ofstream(documents / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 1930 \"Bank\"\r\n#KONTO 6212 \"Mobiltelefon\"\r\n";
  auto plan = cratchit::plan_project_from_sie(documents / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  Navigator navigator(cratchit::ui::make_draft_menu(*project, {"2025-01-01", "2025-12-31"}));
  // None yet
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Fullborda");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::complete);
  EXPECT_EQ(navigator.current().view(), (View{"Inga utkast att fullborda i 2025."}));
  EXPECT_TRUE(navigator.current().options().empty());
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'0'));
  ASSERT_TRUE(navigator.current().on_input(U"Telia 1118,00 2025-10-22"));
  ASSERT_TRUE(navigator.current().on_input(U"Loopia 311,25 2025-03-05"));
  ASSERT_TRUE(navigator.back());
  // The oldest first, as its view shows it: the highest id, counting down
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Fullborda #2");
  auto const first = navigator.current().view();
  ASSERT_FALSE(first.empty());
  EXPECT_EQ(first.front(), "Utkast #2 av 2 i 2025 (händelse)");
  EXPECT_NE(std::ranges::find(first, "  Rubrik:         Loopia"), first.end());
  // The options of the draft's view, and < and >
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "Händelse"},
                                                        {U'1', "Konteringar"},
                                                        {U'3', "Använd mall"},
                                                        {U'9', "Glöm"},
                                                        {U'<', "Föregående"},
                                                        {U'>', "Nästa"}}));
  EXPECT_FALSE(navigator.press(U'<')); // the first: nothing
  ASSERT_TRUE(navigator.press(U'>'));
  EXPECT_EQ(navigator.current().name(), "Fullborda #1");
  EXPECT_EQ(navigator.current().view().front(), "Utkast #1 av 2 i 2025 (händelse)");
  EXPECT_FALSE(navigator.press(U'>')); // the last: nothing
  // As the prompt loop does: enter the next state, else go back where the state says
  auto const follow = [&](auto& result) {
    if (result->next) {
      navigator.enter(std::move(result->next));
    } else {
      navigator.follow_back_to();
    }
  };
  // Konteringar: Kontera, postings typed one at a time
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Kontera");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::post);
  EXPECT_TRUE(navigator.current().options().empty()); // the postings only: no Händelse here
  EXPECT_EQ(navigator.current().view().back(), "Inga konteringar än.");
  // Words of a name: one account fits, the line is shown again with its number, to check
  auto chosen = navigator.current().on_input(U"BANK -1 118,00");
  ASSERT_TRUE(chosen) << chosen.error();
  EXPECT_EQ(chosen->reply, "1930  Bank passar. Kontrollera beloppet och tryck Enter.");
  EXPECT_EQ(navigator.current().start_input(), U"1930 -1 118,00");
  // Several fit, or none
  auto refused = navigator.current().on_input(U"n");
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Flera konton passar:\n  1930  Bank\n  6212  Mobiltelefon\nSkriv fler ord eller numret.");
  EXPECT_EQ(navigator.current().start_input(), U""); // the line typed stays instead
  refused = navigator.current().on_input(U"hyra 500");
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Inget konto har hyra i namnet.");
  EXPECT_EQ(navigator.current().complete(U"mobil").notes,
            (std::vector<std::string>{"6212  Mobiltelefon passar; Enter väljer det."}));
  refused = navigator.current().on_input(U"1940 1118");
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Kontot 1940 finns inte i kontoplanen.");
  refused = navigator.current().on_input(U"1930");
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Skriv beloppet: debet positivt, kredit negativt, till exempel -3,70.");
  // Tab completes a number
  auto const completion = navigator.current().complete(U"19");
  EXPECT_EQ(completion.added, U"30 ");
  // The whole amount first, then what remains with the account alone
  auto accepted = navigator.current().on_input(U"1930 -1 118,00");
  ASSERT_TRUE(accepted) << accepted.error();
  follow(accepted);
  EXPECT_EQ(navigator.current().name(), "Kontera");
  auto view = navigator.current().view();
  EXPECT_NE(std::ranges::find(view, "Kvar: debet 1118.00."), view.end());
  EXPECT_NE(std::ranges::find(view, "Debet 0.00, beloppet 1118.00."), view.end());
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'9', "Ta bort sista"}}));
  // A name without amount: the line again with what remains, then Enter adds it
  accepted = navigator.current().on_input(U"telefon");
  ASSERT_TRUE(accepted) << accepted.error();
  follow(accepted);
  EXPECT_EQ(navigator.current().start_input(), U"6212 1118.00");
  accepted = navigator.current().on_input(navigator.current().start_input());
  ASSERT_TRUE(accepted) << accepted.error();
  follow(accepted);
  view = navigator.current().view();
  EXPECT_EQ(view.front(), "Utkast #1 i 2025 (klart)");
  EXPECT_NE(std::ranges::find(view, "Konteringarna balanserar."), view.end());
  EXPECT_EQ(view.back(), "Utkastet är klart att bokföra: tryck - och sedan 2.");
  // 9 removes the last posting typed
  ASSERT_TRUE(navigator.press(U'9'));
  EXPECT_EQ(navigator.current().view().front(), "Utkast #1 i 2025 (samma)");
  accepted = navigator.current().on_input(U"6212 1118");
  ASSERT_TRUE(accepted) << accepted.error();
  follow(accepted);
  // Back in Fullborda: read again, at the same draft
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Fullborda #1");
  // Händelse (0 here, not in Kontera): the heading changed for itself, typed in
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Händelse");
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().start_input(), U"Telia");
  accepted = navigator.current().on_input(U"Telia oktober");
  ASSERT_TRUE(accepted) << accepted.error();
  follow(accepted);
  EXPECT_EQ(navigator.current().name(), "Händelse");
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Fullborda #1");
  view = navigator.current().view();
  EXPECT_NE(std::ranges::find(view, "  Rubrik:         Telia oktober"), view.end());
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "Händelse"},
                                                        {U'1', "Konteringar"},
                                                        {U'2', "Bokför"},
                                                        {U'3', "Använd mall"},
                                                        {U'9', "Glöm"},
                                                        {U'<', "Föregående"},
                                                        {U'>', "Nästa"}}));
  // A ready draft is still changed in Kontera
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Kontera");
  EXPECT_EQ(navigator.current().view().back(), "Utkastet är klart att bokföra: tryck - och sedan 2.");
  ASSERT_TRUE(navigator.back());
  // Bokför: the voucher in full, the series to type
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Bokför");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::book);
  view = navigator.current().view();
  EXPECT_EQ(view.front(), "Bokför utkast #1 som verifikation i 2025:");
  EXPECT_NE(std::ranges::find(view, "  Text:           Telia oktober"), view.end());
  EXPECT_NE(std::ranges::find(view, "  D  Leverantörsfakturor (nästa D1)"), view.end());
  auto refused_series = navigator.current().on_input(U"Q");
  ASSERT_FALSE(refused_series);
  EXPECT_EQ(refused_series.error(), "Q är ingen serie i bokföringen.");
  accepted = navigator.current().on_input(U"d");
  ASSERT_TRUE(accepted) << accepted.error();
  follow(accepted);
  // Back in Fullborda: the draft is booked, the one before is at the place
  EXPECT_EQ(navigator.current().name(), "Fullborda #1");
  EXPECT_EQ(accepted->reply, "cratchit bokförde #1 som D1: Telia oktober.");
  view = navigator.current().view();
  EXPECT_EQ(std::ranges::find(view, accepted->reply), view.end()); // not in the view
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Utkast");
  view = navigator.current().view();
  EXPECT_EQ(view.back(), "cratchit bokförde #1 som D1: Telia oktober.");
  EXPECT_EQ(view[0], "Utkast 2025: utkast 1–1 av 1");
  // Gör bokfört till utkast: the last voucher of each series, chosen by its series
  ASSERT_TRUE(navigator.press(U'5'));
  EXPECT_EQ(navigator.current().name(), "Gör till utkast");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::unbook);
  view = navigator.current().view();
  EXPECT_EQ(view[0], "Den sista verifikationen i varje serie i 2025:");
  EXPECT_TRUE(view[1].starts_with("  D  D1            2025-10-22       1118.00  Telia oktober  registrerad ")) << view[1];
  auto refused_unbook = navigator.current().on_input(U"A");
  ASSERT_FALSE(refused_unbook);
  EXPECT_EQ(refused_unbook.error(), "Serie A har ingen verifikation i 2025.");
  accepted = navigator.current().on_input(U"d");
  ASSERT_TRUE(accepted) << accepted.error();
  follow(accepted);
  // Back in Utkast: D1 is a draft again, ready, and not in Bokfört
  EXPECT_EQ(navigator.current().name(), "Utkast");
  view = navigator.current().view();
  EXPECT_EQ(view[0], "Utkast 2025: utkast 1–2 av 2");
  EXPECT_TRUE(view[1].ends_with("Telia oktober (klart)")) << view[1];
  EXPECT_EQ(view.back(), "cratchit gjorde D1 till utkast #1: Telia oktober.");
}

TEST_F(RootMenuTest, AReadyDraftIsBookedFromItsView) {
  std::ofstream(documents / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 1930 \"Bank\"\r\n#KONTO 6570 \"Bankkostnader\"\r\n";
  auto plan = cratchit::plan_project_from_sie(documents / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  Navigator navigator(cratchit::ui::make_draft_menu(*project, {"2025-01-01", "2025-12-31"}));
  ASSERT_TRUE(navigator.press(U'0'));
  ASSERT_TRUE(navigator.current().on_input(U"Bankavgift 10,00 2025-03-05"));
  ASSERT_TRUE(navigator.back());
  // Opened by its id: not ready, no Bokför
  auto accepted = navigator.current().on_input(U"#1");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "Händelse"}, {U'1', "Konteringar"}, {U'3', "Använd mall"}, {U'9', "Glöm"}}));
  EXPECT_FALSE(navigator.press(U'2'));
  // Made ready in Kontera; back in its view, 2 Bokför
  ASSERT_TRUE(navigator.press(U'1'));
  ASSERT_TRUE(navigator.current().on_input(U"1930 -10"));
  ASSERT_TRUE(navigator.current().on_input(U"6570"));
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().view().front(), "Utkast #1 i 2025 (klart)");
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'0', "Händelse"}, {U'1', "Konteringar"}, {U'2', "Bokför"}, {U'3', "Använd mall"}, {U'9', "Glöm"}}));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Bokför");
  accepted = navigator.current().on_input(U"a");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.follow_back_to();
  // Booked: the draft is gone, back in Utkast, which tells it
  EXPECT_EQ(navigator.current().name(), "Utkast");
  EXPECT_EQ(navigator.current().view(),
            (View{"Utkast 2025: inga utkast.", "cratchit bokförde #1 som A1: Bankavgift."}));
}

TEST_F(RootMenuTest, AnAndringsverifikationIsADraftThatReversesTheVoucher) {
  std::ofstream(documents / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 2440 \"Leverantörsskulder\"\r\n#KONTO 6210 \"Telekommunikation\"\r\n"
         "#VER \"D\" \"19\" 20250310 \"Telia faktura\"\r\n{\r\n#TRANS 2440 {} -100.00\r\n#TRANS 6210 {} 100.00\r\n}\r\n"
         "#VER \"D\" \"20\" 20250311 \"Loopia\"\r\n{\r\n#TRANS 2440 {} -50.00\r\n#TRANS 6210 {} 50.00\r\n}\r\n";
  auto plan = cratchit::plan_project_from_sie(documents / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  Navigator navigator(cratchit::ui::make_booked_menu(*project, {"2025-01-01", "2025-12-31"}));
  // Any voucher, not only the last of its series
  auto accepted = navigator.current().on_input(U"D19");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'0', "Händelse"}, {U'1', "Konteringar"}, {U'2', "Ändringsverifikation"}, {U'3', "Utskrift"}}));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "D19"); // stays, and tells what it made
  auto const view = navigator.current().view();
  ASSERT_GE(view.size(), 2u);
  EXPECT_EQ(view[view.size() - 2], "cratchit skapade utkast #1: Ändring av D19, daterat 2025-03-10 som D19.");
  EXPECT_EQ(view.back(), "Se till att datumet inte ligger i ett år eller en momsperiod som redan är rapporterad; då krävs "
                         "en ny rapport. Datumet ändrar du med 0 i utkastets vy innan du bokför.");
  // The draft reverses D19 and is ready to book
  Navigator drafts(cratchit::ui::make_draft_menu(*project, {"2025-01-01", "2025-12-31"}));
  EXPECT_EQ(drafts.current().view(),
            (View{"Utkast 2025: utkast 1–1 av 1", "  #1            2025-03-10       -100.00  Ändring av D19 (klart)"}));
  // No statement entries in it: 9 Glöm forgets it whole
  auto opened = drafts.current().on_input(U"#1");
  ASSERT_TRUE(opened) << opened.error();
  drafts.enter(std::move(opened->next));
  EXPECT_EQ(drafts.current().options(),
            (OptionsList{{U'0', "Händelse"}, {U'1', "Konteringar"}, {U'2', "Bokför"}, {U'3', "Använd mall"}, {U'9', "Glöm"}}));
  ASSERT_TRUE(drafts.press(U'9'));
  ASSERT_TRUE(drafts.press(U'9'));
  EXPECT_EQ(drafts.current().name(), "Utkast");
  EXPECT_EQ(drafts.current().view().front(), "Utkast 2025: inga utkast.");
}

TEST_F(RootMenuTest, AStatementIsReadIntoAYearNotInTheBooksYet) {
  std::ofstream(documents / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 1930 \"Bank\"\r\n";
  auto plan = cratchit::plan_project_from_sie(documents / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  fs::create_directories(documents / "bank");
  std::ofstream(documents / "bank" / "nordea.csv", std::ios::binary) << "Datum;Belopp;Text;Saldo\n"
                                                                       "2026/01/06;-5,00;AVGIFTER;985,00\n"
                                                                       "2026/01/05;-10,00;AVGIFTER;990,00\n";
  // 2026 is not in the books (only 2025): the books run on into it
  Navigator navigator(cratchit::ui::make_draft_menu(*project, {"2026-01-01", "2026-12-31"}));
  ASSERT_TRUE(navigator.press(U'3'));
  auto accepted = navigator.current().on_input(pyroceph::tui::path_input_of(documents / "bank") + U"nordea.csv");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  accepted = navigator.current().on_input(U"1930");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  ASSERT_TRUE(navigator.press(U'J'));
  EXPECT_EQ(navigator.current().name(), "Utkast");
  EXPECT_EQ(navigator.current().view().front(), "Utkast 2026: utkast 1–2 av 2");
  auto const books = cratchit::books::read_books(documents);
  ASSERT_TRUE(books);
  ASSERT_EQ(books->years.size(), 2u);
  EXPECT_EQ(cratchit::format_iso_date(books->years[1].start), "2026-01-01");
}

TEST_F(RootMenuTest, ATemplateCompletesAnAccountEventInItsSeries) {
  std::ofstream(documents / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 1930 \"Bank\"\r\n#KONTO 6570 \"Bankkostnader\"\r\n"
         "#VER \"E\" \"1\" 20250201 \"Bankavgift\"\r\n{\r\n#TRANS 1930 {} -2.00\r\n#TRANS 6570 {} 2.00\r\n}\r\n";
  auto plan = cratchit::plan_project_from_sie(documents / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  fs::create_directories(documents / "bank");
  std::ofstream(documents / "bank" / "nordea.csv", std::ios::binary) << "Datum;Belopp;Text;Saldo\n"
                                                                       "2025/03/06;100,00;Kund;590,00\n"
                                                                       "2025/03/05;-10,00;AVGIFTER;490,00\n";
  Navigator navigator(cratchit::ui::make_draft_menu(*project, {"2025-01-01", "2025-12-31"}));
  ASSERT_TRUE(navigator.press(U'3'));
  auto accepted = navigator.current().on_input(pyroceph::tui::path_input_of(documents / "bank") + U"nordea.csv");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  accepted = navigator.current().on_input(U"1930");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  ASSERT_TRUE(navigator.press(U'J'));
  // Fullborda #2, the fee (the oldest): 3 Använd mall lists E1 (1930 credit, as the fee)
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Fullborda #2");
  ASSERT_TRUE(navigator.press(U'3'));
  EXPECT_EQ(navigator.current().name(), "Mall");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::use_template);
  EXPECT_EQ(navigator.current().view(),
            (View{"Utkastet:", "  #2            2025-03-05        -10.00  1930  AVGIFTER (kontohändelse)",
                  "Tidigare verifikationer med samma konton och samma tecken som utkastets konteringar:",
                  "  1  E1            2025-02-01          2.00  Bankavgift",
                  "       1930  Bank                  -2.00",
                  "       6570  Bankkostnader          2.00",
                  "Skriv mallens nummer och tryck Enter."}));
  auto refused = navigator.current().on_input(U"2");
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Det finns ingen mall 2.");
  accepted = navigator.current().on_input(U"1");
  ASSERT_TRUE(accepted) << accepted.error();
  EXPECT_EQ(accepted->reply, "cratchit använde E1 som mall: 1 rad tillagd, serie E.");
  // On to what is missing: the heading (the amount came with the template), from Fullborda
  navigator.follow_back_to();
  EXPECT_EQ(navigator.current().name(), "Rubrik");
  ASSERT_EQ(navigator.path().size(), 3u); // Utkast > Fullborda #2 > Rubrik
  EXPECT_EQ(navigator.path()[1]->name(), "Fullborda #2");
  // No text from the template: the heading is the account event's, typed in
  EXPECT_EQ(navigator.current().start_input(), U"AVGIFTER");
  // - leaves the chain at the draft, which tells what is missing
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Fullborda #2");
  auto view = navigator.current().view();
  EXPECT_NE(std::ranges::find(view, "  Serie:          E"), view.end());
  EXPECT_NE(std::ranges::find(view, "  6570  Bankkostnader         10.00"), view.end());
  EXPECT_EQ(view.back(), "  Rubrik saknas: tryck 0 och sedan 1.");
  // The template again: nothing to add, on to the heading, then Bokför with the template's series
  ASSERT_TRUE(navigator.press(U'3'));
  accepted = navigator.current().on_input(U"1");
  ASSERT_TRUE(accepted) << accepted.error();
  EXPECT_EQ(accepted->reply, "cratchit använde E1 som mall: 0 rader tillagda, serie E.");
  navigator.follow_back_to();
  EXPECT_EQ(navigator.current().name(), "Rubrik");
  accepted = navigator.current().on_input(navigator.current().start_input());
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.follow_back_to();
  EXPECT_EQ(navigator.current().name(), "Bokför");
  EXPECT_EQ(navigator.path()[1]->name(), "Fullborda #2");
  EXPECT_EQ(navigator.current().start_input(), U"E");
  accepted = navigator.current().on_input(navigator.current().start_input());
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.follow_back_to();
  EXPECT_EQ(accepted->reply, "cratchit bokförde #2 som E2: AVGIFTER.");
  // Back in Fullborda, at the draft left: ready to go on
  EXPECT_EQ(navigator.current().name(), "Fullborda #1");
}

TEST_F(RootMenuTest, ATemplateLeadsOnToKonteraWhenThePostingsDoNotBalance) {
  std::ofstream(documents / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 1930 \"Bank\"\r\n#KONTO 6570 \"Bankkostnader\"\r\n"
         "#VER \"E\" \"1\" 20250201 \"Bankavgift\"\r\n{\r\n#TRANS 1930 {} -2.00\r\n#TRANS 6570 {} 2.00\r\n}\r\n";
  auto plan = cratchit::plan_project_from_sie(documents / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  Navigator navigator(cratchit::ui::make_draft_menu(*project, {"2025-01-01", "2025-12-31"}));
  // An event of 20.00 with half of it paid from 1930: the template adds 6570 10.00 to balance
  ASSERT_TRUE(navigator.press(U'0'));
  ASSERT_TRUE(navigator.current().on_input(U"Bankavgift mars 20,00 2025-03-05"));
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'2'));
  ASSERT_TRUE(navigator.press(U'1'));
  auto accepted = navigator.current().on_input(U"1930 -10");
  ASSERT_TRUE(accepted) << accepted.error();
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'3'));
  accepted = navigator.current().on_input(U"1");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.follow_back_to();
  // Heading and amount are there, debit 10.00 is not the amount 20.00: Kontera
  EXPECT_EQ(navigator.current().name(), "Kontera");
  EXPECT_EQ(navigator.path()[1]->name(), "Fullborda #1");
  accepted = navigator.current().on_input(U"1930 -10");
  ASSERT_TRUE(accepted) << accepted.error();
  EXPECT_FALSE(navigator.follow_back_to()); // not ready yet: Kontera stays
  EXPECT_EQ(navigator.current().name(), "Kontera");
  accepted = navigator.current().on_input(U"6570 10");
  ASSERT_TRUE(accepted) << accepted.error();
  // Ready: on to Bokför
  navigator.follow_back_to();
  EXPECT_EQ(navigator.current().name(), "Bokför");
  EXPECT_EQ(navigator.current().start_input(), U"E");
}

TEST_F(RootMenuTest, MomsrapportClearsAQuarterAndWritesItsFiles) {
  // Q1 2025: sales 1000 with 250 VAT, a purchase with 100 VAT; nothing in the other quarters
  std::ofstream(documents / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 1930 \"Bank\"\r\n#KONTO 3001 \"Försäljning\"\r\n#KONTO 2611 \"Utgående moms 25%\"\r\n"
         "#KONTO 2641 \"Debiterad ingående moms\"\r\n#KONTO 2650 \"Redovisningskonto för moms\"\r\n"
         "#KONTO 6540 \"IT-tjänster\"\r\n#KONTO 3740 \"Öres- och kronutjämning\"\r\n"
         "#VER \"A\" \"1\" 20250210 \"Försäljning\"\r\n{\r\n#TRANS 1930 {} 1250.00\r\n#TRANS 3001 {} -1000.00\r\n"
         "#TRANS 2611 {} -250.00\r\n}\r\n"
         "#VER \"A\" \"2\" 20250220 \"Inköp\"\r\n{\r\n#TRANS 1930 {} -500.00\r\n#TRANS 6540 {} 400.00\r\n"
         "#TRANS 2641 {} 100.00\r\n}\r\n";
  auto plan = cratchit::plan_project_from_sie(documents / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  Navigator navigator(cratchit::ui::make_vat_return_menu(*project, {"2025-01-01", "2025-12-31"}));
  EXPECT_EQ(navigator.current().name(), "Momsrapport");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::vat_return);
  // The quarters, the latest first; all have ended
  auto const options = navigator.current().options();
  ASSERT_EQ(options.size(), 4u);
  EXPECT_TRUE(options[0].second.starts_with("2025 Q4")) << options[0].second;
  EXPECT_EQ(options[3].first, U'3');
  EXPECT_TRUE(options[3].second.starts_with("2025 Q1       -150.00        0.00        0.00")) << options[3].second;
  // Q1: the report, the checks, the clearing proposed and the eSKD file
  ASSERT_TRUE(navigator.press(U'3'));
  EXPECT_EQ(navigator.current().name(), "2025 Q1");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::vat_quarter);
  auto view = navigator.current().view();
  EXPECT_EQ(view.front(), "Momsrapport 2025-01-01 – 2025-03-31");
  EXPECT_NE(std::ranges::find(view, "Momstömning (förslag): M 2025-03-31 \"Momsrapport 2025-01-01 - 2025-03-31\""),
            view.end());
  EXPECT_NE(std::ranges::find(view, "eSKD-fil moms_202503.eskd:"), view.end());
  EXPECT_EQ(navigator.current().options(),
            (OptionsList{{U'0', "Granska rutorna"}, {U'1', "Momstömning"}, {U'2', "Skriv rapport och eSKD-fil"}}));
  // Momstömning: a draft, and Bokför for it; - leaves it a draft
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Bokför");
  EXPECT_EQ(navigator.current().start_input(), U"M");
  ASSERT_TRUE(navigator.back());
  view = navigator.current().view();
  EXPECT_NE(std::ranges::find(view, "Den finns som utkast #1."), view.end());
  // Again: the same draft, booked this time
  ASSERT_TRUE(navigator.press(U'1'));
  auto accepted = navigator.current().on_input(navigator.current().start_input());
  ASSERT_TRUE(accepted) << accepted.error();
  EXPECT_EQ(accepted->reply, "cratchit bokförde #1 som M1: Momsrapport 2025-01-01 - 2025-03-31.");
  navigator.follow_back_to();
  EXPECT_EQ(navigator.current().name(), "2025 Q1");
  view = navigator.current().view();
  EXPECT_NE(std::ranges::find(view, "2025 Q1 är redan avräknat av M1."), view.end());
  EXPECT_EQ(view.back(), "cratchit bokförde #1 som M1: Momsrapport 2025-01-01 - 2025-03-31.");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "Granska rutorna"}, {U'2', "Skriv rapport och eSKD-fil"}}));
  auto const books = cratchit::books::read_books(project->folder);
  ASSERT_TRUE(books) << books.error();
  EXPECT_TRUE(books->years.front().drafts.empty());
  // The report is the same after the clearing: the period's own clearing is left out
  EXPECT_EQ(view.front(), "Momsrapport 2025-01-01 – 2025-03-31");
  EXPECT_NE(std::ranges::find(view, std::format("  {:<24}{:>10}", "MomsBetala", 150)), view.end());
  // Skriv: the folder of the quarter; an eSKD file is never overwritten
  ASSERT_TRUE(navigator.press(U'2'));
  auto const folder = project->folder / "moms_2025-01-01_2025-03-31";
  EXPECT_TRUE(fs::exists(folder / "momsrapport.txt"));
  EXPECT_TRUE(fs::exists(folder / "verifikation.txt"));
  EXPECT_TRUE(fs::exists(folder / "moms_202503.eskd"));
  view = navigator.current().view();
  // Each file with its full path, to find it again
  auto const text = [](fs::path const& path) {
    auto const u8 = path.u8string();
    return std::string(u8.begin(), u8.end());
  };
  ASSERT_GE(view.size(), 5u);
  EXPECT_EQ(View(view.end() - 5, view.end()),
            (View{"cratchit skrev:", "  " + text(folder / "momsrapport.txt"), "  " + text(folder / "verifikation.txt"),
                  "  " + text(folder / "moms_202503.eskd"),
                  "Ladda upp moms_202503.eskd i Skatteverkets e-tjänst Lämna momsdeklaration."}));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_TRUE(fs::exists(folder / "moms_202503_2.eskd"));
  // Granska rutorna: a box by its number, < and > the box before and after
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Granska rutorna");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::vat_review);
  EXPECT_FALSE(navigator.current().on_input(U"99"));
  accepted = navigator.current().on_input(U"10");
  ASSERT_TRUE(accepted) << accepted.error();
  navigator.enter(std::move(accepted->next));
  EXPECT_EQ(navigator.current().name(), "Ruta 10");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::vat_box);
  EXPECT_EQ(navigator.current().view().front(), "Ruta 10  Utgående moms 25%");
  ASSERT_TRUE(navigator.press(U'>'));
  EXPECT_EQ(navigator.current().name(), "Ruta 11");
}
