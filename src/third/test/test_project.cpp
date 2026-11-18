#include "core/project.hpp"

#include "core/path_value.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

// A workspace of its own for each test, in an empty temporary folder, removed afterwards
class ProjectTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_project_test_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
    workspace = *cratchit::open_workspace(root / "cratchit", "2026-11-08");
  }
  void TearDown() override { fs::remove_all(root); }

  // A project folder holding project.txt with text
  static fs::path add_project(fs::path const& folder, std::string const& text) {
    fs::create_directories(folder / ".cratchit");
    std::ofstream(folder / ".cratchit" / "project.txt") << text;
    return folder;
  }

  // Adds a line to workspace.txt
  void remember(std::string const& line) { std::ofstream(workspace.file, std::ios::app) << line << '\n'; }

  fs::path root;
  cratchit::Workspace workspace;
};

} // namespace

TEST_F(ProjectTest, FreshWorkspaceRemembersNoProjects) {
  auto const projects = cratchit::list_projects(workspace);
  ASSERT_TRUE(projects.has_value()) << projects.error();
  EXPECT_TRUE(projects->empty());
}

TEST_F(ProjectTest, RememberedProjectsAreListedOnlyByWhereTheyAre) {
  add_project(workspace.folder / "b", "uuid = 2\nname = Beta AB\n");
  add_project(root / "elsewhere", "created = 2026-10-23\nuuid = 1\nname = Gamma AB\norgnr = 556782-8172\n");
  remember("project.2 = b");
  remember("project.1 = " + (root / "elsewhere").string());
  auto const projects = cratchit::list_projects(workspace);
  ASSERT_TRUE(projects.has_value()) << projects.error();
  ASSERT_EQ(projects->size(), 2u);
  EXPECT_EQ((*projects)[0].uuid, "2");
  EXPECT_EQ((*projects)[0].folder, workspace.folder / "b"); // relative is read from the workspace folder
  EXPECT_EQ((*projects)[1].uuid, "1");
  EXPECT_EQ((*projects)[1].folder, root / "elsewhere");
}

TEST_F(ProjectTest, ProjectIsDescribedFromItsOwnFile) {
  add_project(root / "elsewhere", "created = 2026-10-23\nuuid = 1\nname = Gamma AB\norgnr = 556782-8172\n");
  auto summary = cratchit::describe_project({"1", root / "elsewhere"});
  EXPECT_EQ(summary.found, cratchit::ProjectSummary::Found::yes);
  EXPECT_EQ(summary.name, "Gamma AB");
  EXPECT_EQ(summary.orgnr, "556782-8172");
  EXPECT_EQ(summary.created, "2026-10-23");
  // Read anew each time: a change in the project shows at once
  add_project(root / "elsewhere", "uuid = 1\nname = Gamma Nya AB\n");
  summary = cratchit::describe_project({"1", root / "elsewhere"});
  EXPECT_EQ(summary.name, "Gamma Nya AB");
  EXPECT_TRUE(summary.orgnr.empty());
  EXPECT_TRUE(summary.created.empty());
}

TEST_F(ProjectTest, FoldersAreNotSearched) {
  add_project(workspace.folder / "not remembered", "uuid = 1\nname = Alfa AB\n");
  auto const projects = cratchit::list_projects(workspace);
  ASSERT_TRUE(projects.has_value()) << projects.error();
  EXPECT_TRUE(projects->empty());
}

TEST_F(ProjectTest, ProjectThatIsGoneOrCannotBeOpenedIsDescribedByFolderName) {
  add_project(workspace.folder / "Fel uuid", "uuid = 2\nname = Alfa AB\n");
  auto const unreadable = cratchit::describe_project({"1", workspace.folder / "Fel uuid"});
  EXPECT_EQ(unreadable.found, cratchit::ProjectSummary::Found::unreadable);
  EXPECT_EQ(unreadable.name, "Fel uuid");
  EXPECT_TRUE(unreadable.orgnr.empty());
  auto const missing = cratchit::describe_project({"9", workspace.folder / "Saknas"});
  EXPECT_EQ(missing.found, cratchit::ProjectSummary::Found::missing);
  EXPECT_EQ(missing.name, "Saknas");
}

TEST(ShortFolder, InsideTheWorkspaceIsRelative) {
  EXPECT_EQ(cratchit::short_folder_text("/home/anna/Documents/cratchit/Exempel AB", "/home/anna/Documents/cratchit",
                                        fs::path("/home/anna")),
            "Exempel AB");
}

TEST(ShortFolder, InsideHomeStartsWithTilde) {
  EXPECT_EQ(cratchit::short_folder_text("/home/anna/Bokföring/Exempel AB", "/home/anna/Documents/cratchit",
                                        fs::path("/home/anna")),
            (fs::path("~") / "Bokföring" / "Exempel AB").string());
}

TEST(ShortFolder, ElsewhereIsInFull) {
  EXPECT_EQ(cratchit::short_folder_text("/srv/books/Exempel AB", "/home/anna/Documents/cratchit", fs::path("/home/anna")),
            fs::path("/srv/books/Exempel AB").string());
  EXPECT_EQ(cratchit::short_folder_text("/home/anna/Exempel AB", "/home/anna/Documents/cratchit", std::nullopt),
            fs::path("/home/anna/Exempel AB").string());
}

TEST_F(ProjectTest, EntryWithoutUuidOrFolderIsAnError) {
  remember("project. = x");
  auto const projects = cratchit::list_projects(workspace);
  ASSERT_FALSE(projects.has_value());
  EXPECT_NE(projects.error().find("saknar uuid"), std::string::npos);
}

TEST_F(ProjectTest, MissingWorkspaceFileIsAnError) {
  fs::remove(workspace.file);
  EXPECT_FALSE(cratchit::list_projects(workspace).has_value());
}

TEST_F(ProjectTest, ProjectIsOpenedWithItsNameAndNotChanged) {
  std::string const text = "created = 2026-10-23\nuuid = bffb66d8-a6bc-4cdc-bace-68bb0de0b1c4\nname = The ITfied AB\n";
  auto const folder = add_project(root / "The ITfied AB", text);
  auto const project = cratchit::open_project(folder, "bffb66d8-a6bc-4cdc-bace-68bb0de0b1c4");
  ASSERT_TRUE(project.has_value()) << project.error();
  EXPECT_EQ(project->folder, folder);
  EXPECT_EQ(project->file, folder / ".cratchit" / "project.txt");
  EXPECT_EQ(project->uuid, "bffb66d8-a6bc-4cdc-bace-68bb0de0b1c4");
  EXPECT_EQ(project->name, "The ITfied AB");
  std::ifstream in(project->file);
  EXPECT_EQ(std::string(std::istreambuf_iterator<char>(in), {}), text);
}

TEST_F(ProjectTest, OtherUuidIsAnError) {
  auto const project = cratchit::open_project(add_project(root / "x", "uuid = 2\nname = Alfa AB\n"), "1");
  ASSERT_FALSE(project.has_value());
  EXPECT_NE(project.error().find("uuid 2 är inte 1"), std::string::npos);
}

TEST_F(ProjectTest, ProjectWithoutUuidIsAnError) {
  auto const project = cratchit::open_project(add_project(root / "x", "name = Alfa AB\n"), "1");
  ASSERT_FALSE(project.has_value());
  EXPECT_NE(project.error().find("'uuid' saknas"), std::string::npos);
}

TEST_F(ProjectTest, ProjectWithoutNameIsAnError) {
  auto const project = cratchit::open_project(add_project(root / "x", "uuid = 1\n"), "1");
  ASSERT_FALSE(project.has_value());
  EXPECT_NE(project.error().find("'name' saknas"), std::string::npos);
}

TEST_F(ProjectTest, MalformedProjectIsAnError) {
  EXPECT_FALSE(cratchit::open_project(add_project(root / "x", "nonsense\n"), "1").has_value());
}

TEST_F(ProjectTest, FolderWithoutProjectFileIsAnError) {
  fs::create_directories(root / "x");
  auto const project = cratchit::open_project(root / "x", "1");
  ASSERT_FALSE(project.has_value());
  EXPECT_NE(project.error().find("saknas"), std::string::npos);
  EXPECT_FALSE(fs::exists(root / "x" / ".cratchit"));
}

TEST_F(ProjectTest, AFolderHasOneProjectSoRememberingForgetsAnotherUuidThere) {
  // The folder now holds another project
  add_project(workspace.folder / "b", "uuid = 2\nname = Beta AB\n");
  add_project(root / "test", "uuid = 3\nname = Test AB\n");
  remember("project.1 = " + (root / "test").string());
  remember("project.2 = b");
  remember("project.4 = " + (root / "test" / "").string()); // the same folder, written otherwise
  remember("project.5 = " + (root / "other").string());
  ASSERT_TRUE(cratchit::remember_project(workspace, {root / "test", root / "test" / ".cratchit" / "project.txt", "3", "Test AB"}));
  auto const entries = cratchit::read_path_values(workspace.file);
  ASSERT_TRUE(entries);
  EXPECT_FALSE(cratchit::find_value(*entries, "project.1"));
  EXPECT_FALSE(cratchit::find_value(*entries, "project.4"));
  EXPECT_EQ(cratchit::find_value(*entries, "project.2"), "b");
  EXPECT_EQ(cratchit::find_value(*entries, "project.3"), (root / "test").string());
  EXPECT_EQ(cratchit::find_value(*entries, "project.5"), (root / "other").string());
  // Remembering b again, relative in workspace.txt, forgets nothing else
  ASSERT_TRUE(cratchit::remember_project(workspace, {workspace.folder / "b", workspace.folder / "b" / ".cratchit" / "project.txt", "2", "Beta AB"}));
  EXPECT_EQ(cratchit::list_projects(workspace)->size(), 3u);
}

TEST_F(ProjectTest, ForgetRemovesOnlyTheProjectsEntryAndTouchesNoFiles) {
  add_project(workspace.folder / "b", "uuid = 2\nname = Beta AB\n");
  remember("project.1 = Borta");
  remember("project.2 = b");
  ASSERT_TRUE(cratchit::forget_project(workspace, "1"));
  auto const projects = cratchit::list_projects(workspace);
  ASSERT_TRUE(projects) << projects.error();
  ASSERT_EQ(projects->size(), 1u);
  EXPECT_EQ((*projects)[0].uuid, "2");
  auto const entries = cratchit::read_path_values(workspace.file);
  ASSERT_TRUE(entries);
  EXPECT_EQ(cratchit::find_value(*entries, "created"), "2026-11-08");
  // Forgetting b leaves its folder as it is
  ASSERT_TRUE(cratchit::forget_project(workspace, "2"));
  EXPECT_TRUE(fs::exists(workspace.folder / "b" / ".cratchit" / "project.txt"));
  // Forgetting what is not remembered does nothing
  EXPECT_TRUE(cratchit::forget_project(workspace, "7"));
}
