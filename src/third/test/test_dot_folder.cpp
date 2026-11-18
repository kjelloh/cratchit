#include "core/dot_folder.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace {

// An empty folder of its own for each test, removed afterwards
class DotFolderTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_test_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
  }
  void TearDown() override { fs::remove_all(root); }

  fs::path root;
};

std::string read_file(fs::path const& path) {
  std::ifstream in(path);
  std::ostringstream text;
  text << in.rdbuf();
  return text.str();
}

void write_file(fs::path const& path, std::string const& text) {
  fs::create_directories(path.parent_path());
  std::ofstream(path) << text;
}

} // namespace

// Framework: read only, next to the program

TEST_F(DotFolderTest, FrameworkIsReadWithItsVersion) {
  write_file(root / ".cratchit" / "framework.txt", "version = 0.1.0\n");
  auto const framework = cratchit::open_framework(root);
  ASSERT_TRUE(framework.has_value()) << framework.error();
  EXPECT_EQ(framework->file, root / ".cratchit" / "framework.txt");
  EXPECT_EQ(framework->version, "0.1.0");
}

TEST_F(DotFolderTest, MissingFrameworkIsAnErrorAndIsNotCreated) {
  auto const framework = cratchit::open_framework(root);
  ASSERT_FALSE(framework.has_value());
  EXPECT_NE(framework.error().find("Installationen av cratchit är inte hel"), std::string::npos);
  EXPECT_FALSE(fs::exists(root / ".cratchit"));
}

TEST_F(DotFolderTest, FrameworkWithoutVersionIsAnError) {
  write_file(root / ".cratchit" / "framework.txt", "created = 2020-01-01\n");
  auto const framework = cratchit::open_framework(root);
  ASSERT_FALSE(framework.has_value());
  EXPECT_NE(framework.error().find("'version' saknas"), std::string::npos);
}

TEST_F(DotFolderTest, MalformedFrameworkIsAnError) {
  write_file(root / ".cratchit" / "framework.txt", "nonsense\n");
  auto const framework = cratchit::open_framework(root);
  ASSERT_FALSE(framework.has_value());
  EXPECT_NE(framework.error().find("framework.txt"), std::string::npos);
}

TEST_F(DotFolderTest, FrameworkFolderIsAnError) {
  fs::create_directories(root / ".cratchit" / "framework.txt");
  EXPECT_FALSE(cratchit::open_framework(root).has_value());
}

TEST_F(DotFolderTest, FrameworkIsNeverChanged) {
  write_file(root / ".cratchit" / "framework.txt", "version = 0.1.0\n");
  ASSERT_TRUE(cratchit::open_framework(root).has_value());
  EXPECT_EQ(read_file(root / ".cratchit" / "framework.txt"), "version = 0.1.0\n");
}

// Runtime: the user's own, in the home folder

TEST_F(DotFolderTest, FreshStartCreatesRuntime) {
  auto const runtime = cratchit::open_runtime(root, "2026-10-20");
  ASSERT_TRUE(runtime.has_value()) << runtime.error();
  EXPECT_EQ(runtime->file, root / ".cratchit" / "runtime.txt");
  EXPECT_EQ(read_file(runtime->file), "created = 2026-10-20\n");
  ASSERT_EQ(runtime->initiated.size(), 1u);
  EXPECT_EQ(runtime->initiated[0].folder, root);
  EXPECT_EQ(runtime->initiated[0].file, fs::path(".cratchit") / "runtime.txt");
  EXPECT_FALSE(fs::exists(root / ".cratchit" / "framework.txt"));
}

TEST_F(DotFolderTest, SecondStartChangesNothing) {
  ASSERT_TRUE(cratchit::open_runtime(root, "2026-10-20").has_value());
  auto const runtime = cratchit::open_runtime(root, "2030-01-01");
  ASSERT_TRUE(runtime.has_value()) << runtime.error();
  EXPECT_TRUE(runtime->initiated.empty());
  EXPECT_EQ(read_file(runtime->file), "created = 2026-10-20\n");
}

TEST_F(DotFolderTest, ExistingFolderGetsMissingRuntime) {
  write_file(root / ".cratchit" / "framework.txt", "created = 2020-01-01\nruntime = .\n"); // left from earlier versions
  auto const runtime = cratchit::open_runtime(root, "2026-10-20");
  ASSERT_TRUE(runtime.has_value()) << runtime.error();
  ASSERT_EQ(runtime->initiated.size(), 1u);
  EXPECT_EQ(read_file(runtime->file), "created = 2026-10-20\n");
  EXPECT_EQ(read_file(root / ".cratchit" / "framework.txt"), "created = 2020-01-01\nruntime = .\n");
}

TEST_F(DotFolderTest, DotCratchitFileIsAnError) {
  std::ofstream(root / ".cratchit") << "not a folder\n";
  EXPECT_FALSE(cratchit::open_runtime(root, "2026-10-20").has_value());
}

TEST_F(DotFolderTest, RuntimeFileFolderIsAnError) {
  fs::create_directories(root / ".cratchit" / "runtime.txt");
  EXPECT_FALSE(cratchit::open_runtime(root, "2026-10-20").has_value());
}

// Workspace: the folder that holds the user's projects

TEST_F(DotFolderTest, FreshWorkspaceIsCreated) {
  auto const folder = root / "cratchit";
  auto const workspace = cratchit::open_workspace(folder, "2026-11-08");
  ASSERT_TRUE(workspace.has_value()) << workspace.error();
  EXPECT_EQ(workspace->folder, folder);
  EXPECT_EQ(workspace->file, folder / ".cratchit" / "workspace.txt");
  EXPECT_EQ(read_file(workspace->file), "created = 2026-11-08\n");
  ASSERT_EQ(workspace->initiated.size(), 1u);
  EXPECT_EQ(workspace->initiated[0].folder, folder);
  EXPECT_EQ(workspace->initiated[0].file, fs::path(".cratchit") / "workspace.txt");
}

TEST_F(DotFolderTest, ExistingWorkspaceIsNotChanged) {
  auto const folder = root / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-08").has_value());
  auto const workspace = cratchit::open_workspace(folder, "2030-01-01");
  ASSERT_TRUE(workspace.has_value()) << workspace.error();
  EXPECT_TRUE(workspace->initiated.empty());
  EXPECT_EQ(read_file(workspace->file), "created = 2026-11-08\n");
}

TEST_F(DotFolderTest, ExistingFolderBecomesAWorkspace) {
  write_file(root / "cratchit" / "notes.txt", "mine\n");
  auto const workspace = cratchit::open_workspace(root / "cratchit", "2026-11-08");
  ASSERT_TRUE(workspace.has_value()) << workspace.error();
  EXPECT_EQ(workspace->initiated.size(), 1u);
  EXPECT_EQ(read_file(root / "cratchit" / "notes.txt"), "mine\n");
}

TEST_F(DotFolderTest, WorkspaceParentsAreNotCreated) {
  auto const workspace = cratchit::open_workspace(root / "Documents" / "cratchit", "2026-11-08");
  ASSERT_FALSE(workspace.has_value());
  EXPECT_NE(workspace.error().find("Kan inte skapa"), std::string::npos);
  EXPECT_FALSE(fs::exists(root / "Documents"));
}

TEST_F(DotFolderTest, WorkspaceThatIsAFileIsAnError) {
  std::ofstream(root / "cratchit") << "not a folder\n";
  auto const workspace = cratchit::open_workspace(root / "cratchit", "2026-11-08");
  ASSERT_FALSE(workspace.has_value());
  EXPECT_NE(workspace.error().find("inte en mapp"), std::string::npos);
}

TEST_F(DotFolderTest, WorkspaceFileFolderIsAnError) {
  fs::create_directories(root / "cratchit" / ".cratchit" / "workspace.txt");
  EXPECT_FALSE(cratchit::open_workspace(root / "cratchit", "2026-11-08").has_value());
}

TEST(DefaultWorkspaceFolder, IsCratchitInDocuments) {
  auto const documents = cratchit::documents_folder();
  ASSERT_TRUE(documents.has_value()) << documents.error();
  auto const folder = cratchit::default_workspace_folder();
  ASSERT_TRUE(folder.has_value()) << folder.error();
  EXPECT_EQ(*folder, *documents / "cratchit");
}

// The program's own folder

TEST(ProgramFolder, HoldsTheRunningProgram) {
  auto const folder = cratchit::program_folder();
  ASSERT_TRUE(folder.has_value()) << folder.error();
  EXPECT_TRUE(folder->is_absolute());
  EXPECT_TRUE(fs::is_regular_file(*folder / "cratchit_tests") || fs::is_regular_file(*folder / "cratchit_tests.exe"))
      << folder->string();
}

TEST(ProgramPath, IsTheRunningProgram) {
  auto const file = cratchit::program_path();
  ASSERT_TRUE(file.has_value()) << file.error();
  EXPECT_TRUE(file->is_absolute());
  EXPECT_TRUE(fs::is_regular_file(*file)) << file->string();
  EXPECT_EQ(file->stem(), "cratchit_tests");
  EXPECT_EQ(file->parent_path(), cratchit::program_folder().value());
}

// The user's home folder

TEST_F(DotFolderTest, RootIsUserHome) {
  auto const chosen = cratchit::choose_user_home(root);
  ASSERT_TRUE(chosen.has_value()) << chosen.error();
  EXPECT_EQ(*chosen, root);
}

TEST_F(DotFolderTest, MissingUserHomeFolderIsAnErrorAndIsNotCreated) {
  auto const missing = root / "missing";
  auto const chosen = cratchit::choose_user_home(missing);
  ASSERT_FALSE(chosen.has_value());
  EXPECT_NE(chosen.error().find("en mapp som inte finns"), std::string::npos);
  EXPECT_FALSE(fs::exists(missing));
}

TEST(UserHome, RelativeUserHomeIsAnError) {
  auto const chosen = cratchit::choose_user_home(fs::path("relative") / "folder");
  ASSERT_FALSE(chosen.has_value());
  EXPECT_NE(chosen.error().find("en hel sökväg"), std::string::npos);
}

TEST(UserHome, NotGivenOrEmptyIsAnError) {
  auto const chosen = cratchit::choose_user_home(std::nullopt);
  ASSERT_FALSE(chosen.has_value());
  EXPECT_NE(chosen.error().find("Hittar inte hemkatalogen"), std::string::npos);
  EXPECT_FALSE(cratchit::choose_user_home(fs::path{}).has_value());
}

TEST(TodayIso, HasDateShape) {
  auto const today = cratchit::today_iso();
  ASSERT_EQ(today.size(), 10u);
  EXPECT_EQ(today[4], '-');
  EXPECT_EQ(today[7], '-');
}
