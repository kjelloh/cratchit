#include "pyroceph/tui/path_completion.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using pyroceph::tui::complete_path;
using pyroceph::tui::path_input_of;

namespace {

std::u32string separator() { return std::u32string(1, static_cast<char32_t>(fs::path::preferred_separator)); }

// An empty folder of its own for each test, removed afterwards (tests from take 3)
class PathCompletionTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_path_completion_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
  }
  void TearDown() override { fs::remove_all(root); }

  // The root as typed, ending in a separator, then text
  std::u32string typed(std::u32string const& text) const { return path_input_of(root) + text; }

  fs::path root;
};

} // namespace

TEST_F(PathCompletionTest, UniqueFolderIsCompletedWithSeparator) {
  fs::create_directories(root / "books");
  std::ofstream(root / "bonus.txt") << "a file, not a folder\n";
  auto const completion = complete_path(typed(U"bo"));
  EXPECT_EQ(completion.added, U"oks" + separator());
  EXPECT_TRUE(completion.candidates.empty());
}

TEST_F(PathCompletionTest, SeveralFoldersAreCompletedAsFarAsTheyAgree) {
  fs::create_directories(root / "books2025");
  fs::create_directories(root / "books2026");
  auto const completion = complete_path(typed(U"b"));
  EXPECT_EQ(completion.added, U"ooks202");
  EXPECT_TRUE(completion.candidates.empty());
}

TEST_F(PathCompletionTest, SeveralFoldersAreListedWhenNothingCanBeAdded) {
  fs::create_directories(root / "books2026");
  fs::create_directories(root / "books2025");
  auto const completion = complete_path(typed(U"books202"));
  EXPECT_TRUE(completion.added.empty());
  EXPECT_EQ(completion.candidates, (std::vector<std::string>{"books2025", "books2026"}));
}

TEST_F(PathCompletionTest, NoMatchCompletesNothing) {
  fs::create_directories(root / "books");
  auto const completion = complete_path(typed(U"x"));
  EXPECT_TRUE(completion.added.empty());
  EXPECT_TRUE(completion.candidates.empty());
}

TEST_F(PathCompletionTest, TextWithoutSeparatorCompletesNothing) {
  auto const completion = complete_path(U"books");
  EXPECT_TRUE(completion.added.empty());
  EXPECT_TRUE(completion.candidates.empty());
}

TEST_F(PathCompletionTest, MissingFolderCompletesNothing) {
  auto const completion = complete_path(typed(U"gone") + separator() + U"b");
  EXPECT_TRUE(completion.added.empty());
  EXPECT_TRUE(completion.candidates.empty());
}

TEST_F(PathCompletionTest, HiddenFoldersNeedADot) {
  fs::create_directories(root / ".cratchit");
  fs::create_directories(root / "books");
  EXPECT_EQ(complete_path(typed(U"")).added, U"books" + separator());
  EXPECT_EQ(complete_path(typed(U".")).added, U"cratchit" + separator());
}

TEST_F(PathCompletionTest, FilesAreCompletedOnlyWhenAsked) {
  std::ofstream(root / "export.se") << "#FLAGGA 0\n";
  EXPECT_TRUE(complete_path(typed(U"ex")).added.empty());
  EXPECT_EQ(complete_path(typed(U"ex"), true).added, U"port.se");
}

TEST_F(PathCompletionTest, FilesAndFoldersAreListedTogether) {
  std::ofstream(root / "export.se") << "#FLAGGA 0\n";
  fs::create_directories(root / "exports");
  auto const completion = complete_path(typed(U"export"), true);
  EXPECT_TRUE(completion.added.empty());
  EXPECT_EQ(completion.candidates, (std::vector<std::string>{"export.se", "exports"}));
}

TEST_F(PathCompletionTest, NamesWithSpacesAndSwedishLettersAreCompleted) {
  fs::create_directories(root / fs::path(u8"Bokföring The ITfied AB"));
  EXPECT_EQ(complete_path(typed(U"Bok")).added, U"föring The ITfied AB" + separator());
}

TEST(PathInput, FolderGetsOneSeparatorAfterIt) {
  auto const folder = fs::temp_directory_path() / "x";
  EXPECT_EQ(path_input_of(folder).back(), static_cast<char32_t>(fs::path::preferred_separator));
  // Typed back, the path gets no second separator
  EXPECT_EQ(path_input_of(pyroceph::tui::path_of(path_input_of(folder))), path_input_of(folder));
}

TEST_F(PathCompletionTest, OnlyAcceptedFilesCount) {
  std::ofstream(root / "export.se") << "#FLAGGA 0\n";
  std::ofstream(root / "export.txt") << "text\n";
  auto const accept = [](fs::path const& file) { return file.extension() == ".se"; };
  EXPECT_EQ(complete_path(typed(U"ex"), true, accept).added, U"port.se");
}

// Describing files

namespace {

std::vector<std::string> size_of(fs::path const& file) {
  return {std::to_string(fs::file_size(file)) + " bytes"};
}

} // namespace

TEST_F(PathCompletionTest, CandidatesHaveTheirDescriptionIndentedUnderFilesOnly) {
  fs::create_directories(root / "arkiv");
  std::ofstream(root / "a.se") << "12";
  std::ofstream(root / "b.se") << "123";
  auto const completion = complete_path(typed(U""), true, {}, size_of);
  EXPECT_TRUE(completion.added.empty());
  EXPECT_EQ(completion.candidates,
            (std::vector<std::string>{"a.se", "    2 bytes", "arkiv", "b.se", "    3 bytes"}));
  EXPECT_TRUE(completion.notes.empty());
}

TEST_F(PathCompletionTest, TheOneMatchingFileIsDescribedInTheNotes) {
  std::ofstream(root / "fil.se") << "1234";
  auto const completed = complete_path(typed(U"f"), true, {}, size_of);
  EXPECT_EQ(completed.added, U"il.se");
  EXPECT_EQ(completed.notes, (std::vector<std::string>{"4 bytes"}));
  // Also when the whole name is typed already
  auto const typed_whole = complete_path(typed(U"fil.se"), true, {}, size_of);
  EXPECT_TRUE(typed_whole.added.empty());
  EXPECT_EQ(typed_whole.notes, (std::vector<std::string>{"4 bytes"}));
}

TEST_F(PathCompletionTest, FoldersAreNotDescribed) {
  fs::create_directories(root / "arkiv");
  auto const completed = complete_path(typed(U"a"), true, {}, size_of);
  EXPECT_EQ(completed.added, U"rkiv" + separator());
  EXPECT_TRUE(completed.notes.empty());
}

TEST_F(PathCompletionTest, ListLaysOutTheMatchingFilesAndFoldersItself) {
  fs::create_directories(root / "arkiv");
  std::ofstream(root / "b.se") << "b";
  std::ofstream(root / "a.se") << "a";
  std::vector<fs::path> got_files;
  std::vector<fs::path> got_folders;
  auto const completion = complete_path(
      typed(U""), true, {}, size_of, [&](std::vector<fs::path> const& files, std::vector<fs::path> const& folders) {
        got_files = files;
        got_folders = folders;
        return std::vector<std::string>{"Rubrik", "  rad"};
      });
  EXPECT_EQ(completion.candidates, (std::vector<std::string>{"Rubrik", "  rad"}));
  EXPECT_EQ(got_files, (std::vector<fs::path>{root / "a.se", root / "b.se"}));
  EXPECT_EQ(got_folders, (std::vector<fs::path>{root / "arkiv"}));
}
