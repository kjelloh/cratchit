// Saving a document to a text file (core/save_document)
#include "core/dot_folder.hpp"
#include "core/save_document.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace {

class SaveDocumentTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    folder = fs::temp_directory_path() / (std::string("cratchit_save_test_") + info->name());
    fs::remove_all(folder);
    fs::create_directories(folder);
  }
  void TearDown() override { fs::remove_all(folder); }
  fs::path folder;
};

std::string content_of(fs::path const& file) {
  std::ifstream in(file, std::ios::binary);
  std::ostringstream text;
  text << in.rdbuf();
  return text.str();
}

} // namespace

TEST(FileName, ReplacesWhatAFileNameMayNotHave) {
  EXPECT_EQ(cratchit::file_name_of("Test AB Balansrapport 2025/26"), "Test AB Balansrapport 2025-26");
  EXPECT_EQ(cratchit::file_name_of("a\\b:c*d?e\"f<g>h|i"), "a-b-c-d-e-f-g-h-i");
  EXPECT_EQ(cratchit::file_name_of("Åtta öre AB"), "Åtta öre AB") << "letters are kept";
  EXPECT_EQ(cratchit::file_name_of("Slut. "), "Slut") << "Windows drops spaces and dots at the end";
  EXPECT_EQ(cratchit::file_name_of(""), "dokument");
}

TEST_F(SaveDocumentTest, WritesTheLinesInUtf8) {
  auto const saved = cratchit::save_document(folder, "Test AB Verifikation A1 2025", {"Verifikation A1", "Räkenskapsår"});
  ASSERT_TRUE(saved) << saved.error();
  EXPECT_EQ(*saved, folder / fs::path(u8"Test AB Verifikation A1 2025.txt"));
  EXPECT_EQ(content_of(*saved), "Verifikation A1\nRäkenskapsår\n");
}

TEST_F(SaveDocumentTest, NeverWritesOverAFile) {
  std::ofstream(folder / "Rapport.txt") << "mina egna rader\n";
  auto const second = cratchit::save_document(folder, "Rapport", {"ny"});
  ASSERT_TRUE(second) << second.error();
  EXPECT_EQ(second->filename(), "Rapport (2).txt");
  auto const third = cratchit::save_document(folder, "Rapport", {"nyare"});
  ASSERT_TRUE(third) << third.error();
  EXPECT_EQ(third->filename(), "Rapport (3).txt");
  EXPECT_EQ(content_of(folder / "Rapport.txt"), "mina egna rader\n");
}

TEST_F(SaveDocumentTest, AFolderThatIsNotThereIsAnError) {
  auto const saved = cratchit::save_document(folder / "saknas", "Rapport", {"x"});
  ASSERT_FALSE(saved);
  EXPECT_NE(saved.error().find("Kan inte skapa"), std::string::npos) << saved.error();
}

TEST(DownloadsFolder, IsAFullPath) {
  auto const downloads = cratchit::downloads_folder();
  ASSERT_TRUE(downloads) << downloads.error();
  EXPECT_TRUE(downloads->is_absolute()) << downloads->string();
}
