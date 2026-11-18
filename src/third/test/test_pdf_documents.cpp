#include "core/pdf_documents.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using cratchit::PdfStatus;

namespace {

std::string stream_object(int number, std::string_view data) {
  return std::to_string(number) + " 0 obj\n<< /Length " + std::to_string(data.size()) + " >>\nstream\n" +
         std::string{data} + "\nendstream\nendobj\n";
}

// A one-page PDF drawing content with F1 (WinAnsi), as walk_pdf's own tests make them
std::string pdf(std::string_view content) {
  std::string widths;
  for (int i = 0; i < 224; ++i) widths += "500 ";
  return std::string{"%PDF-1.4\n"
                     "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"
                     "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n"
                     "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 4 0 R"
                     " /Resources << /Font << /F1 5 0 R >> >> >>\nendobj\n"} +
         stream_object(4, content) +
         "5 0 obj\n<< /Type /Font /Subtype /Type1 /Encoding /WinAnsiEncoding /FirstChar 32 /Widths [" + widths +
         "] >>\nendobj\ntrailer\n<< /Size 6 /Root 1 0 R >>\n%%EOF\n";
}

class PdfDocumentsTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Its own folder for each test, as ctest runs them at the same time
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    folder = fs::temp_directory_path() / (std::string("cratchit_pdf_documents_test_") + info->name());
    fs::remove_all(folder);
    fs::create_directories(folder / "under");
  }
  void TearDown() override { fs::remove_all(folder); }
  fs::path write(std::string const& name, std::string const& content) {
    std::ofstream(folder / name, std::ios::binary) << content;
    return folder / name;
  }
  fs::path folder;
};

} // namespace

TEST_F(PdfDocumentsTest, ListsThePdfFilesByName) {
  write("b.pdf", "x");
  write("A.PDF", "x");
  write("c.txt", "x");
  write(".dold.pdf", "x");
  write("under/d.pdf", "x");
  EXPECT_EQ(cratchit::pdf_files_in(folder), (std::vector<fs::path>{folder / "A.PDF", folder / "b.pdf"}));
  EXPECT_TRUE(cratchit::pdf_files_in(folder / "saknas").empty());
}

TEST_F(PdfDocumentsTest, TellsWhetherAPdfHasText) {
  auto status = cratchit::pdf_status(write("faktura.pdf", pdf("BT /F1 10 Tf 72 700 Td (Faktura 123) Tj ET")));
  EXPECT_EQ(status.kind, PdfStatus::Kind::text);
  EXPECT_EQ(cratchit::status_text(status), "text, 1 sida");
  status = cratchit::pdf_status(write("bild.pdf", pdf("0 0 100 100 re f")));
  EXPECT_EQ(status.kind, PdfStatus::Kind::no_text);
  EXPECT_EQ(cratchit::status_text(status), "ingen text, 1 sida (inskannad?)");
  status = cratchit::pdf_status(write("trasig.pdf", "inte en pdf\n"));
  EXPECT_EQ(cratchit::status_text(status), "inte en pdf");
  status = cratchit::pdf_status(folder / "saknas.pdf");
  EXPECT_EQ(cratchit::status_text(status), "går inte att läsa");
}

TEST(PdfStatusText, EveryKind) {
  EXPECT_EQ(cratchit::status_text({PdfStatus::Kind::text, 2, 4}), "text, 2 sidor, 4 tecken oläsliga");
  EXPECT_EQ(cratchit::status_text({PdfStatus::Kind::no_pages, 0, 0}), "inga sidor hittade");
  EXPECT_EQ(cratchit::status_text({PdfStatus::Kind::password, 0, 0}), "krypterad med lösenord");
  EXPECT_EQ(cratchit::status_text({PdfStatus::Kind::unsupported, 0, 0}), "krypterad (AES), går inte att läsa");
  EXPECT_EQ(cratchit::status_text({PdfStatus::Kind::not_permitted, 0, 0}), "textkopiering spärrad i filen");
}

TEST_F(PdfDocumentsTest, ReadsStatusAndTextOnce) {
  auto const file = write("faktura.pdf", pdf("BT /F1 10 Tf 72 700 Td (Faktura 123) Tj ET"));
  auto const document = cratchit::read_pdf(file);
  EXPECT_EQ(document.file, file);
  EXPECT_EQ(document.status.kind, PdfStatus::Kind::text);
  EXPECT_EQ(document.pages, (std::vector<std::vector<std::string>>{{"Faktura 123"}}));
  auto const broken = cratchit::read_pdf(write("trasig.pdf", "inte en pdf\n"));
  EXPECT_EQ(broken.status.kind, PdfStatus::Kind::not_a_pdf);
  EXPECT_TRUE(broken.pages.empty());
}
