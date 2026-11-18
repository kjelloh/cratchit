#include "core/pdf_documents.hpp"

#include "walk_pdf.hpp"

#include <algorithm>
#include <cctype>
#include <format>
#include <system_error>
#include <utility>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

// "1 sida", "2 sidor"
std::string pages_text(std::size_t pages) { return pages == 1 ? "1 sida" : std::format("{} sidor", pages); }

bool has_text(walk_pdf::PlainText const& text) {
  return std::ranges::any_of(text.pages, [](auto const& lines) {
    return std::ranges::any_of(lines, [](std::string const& line) { return !line.empty(); });
  });
}

} // namespace

bool is_pdf(fs::path const& file) {
  auto extension = file.extension().string();
  std::ranges::transform(extension, extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return extension == ".pdf";
}

std::vector<fs::path> pdf_files_in(fs::path const& folder) {
  std::vector<fs::path> files;
  std::error_code ec;
  for (fs::directory_iterator it(folder, ec), end; !ec && it != end; it.increment(ec)) {
    if (it->is_regular_file(ec) && is_pdf(it->path()) && !it->path().filename().string().starts_with('.')) {
      files.push_back(it->path());
    }
  }
  std::ranges::sort(files);
  return files;
}

PdfDocument read_pdf(fs::path const& file) {
  auto extracted = walk_pdf::extract_text(file);
  PdfDocument document{file, {}, {}};
  auto& status = document.status;
  if (!extracted) {
    using Kind = walk_pdf::TextError::Kind;
    switch (extracted.error().kind) {
    case Kind::cannot_read: status.kind = PdfStatus::Kind::cannot_read; break;
    case Kind::not_a_pdf: status.kind = PdfStatus::Kind::not_a_pdf; break;
    case Kind::needs_password: status.kind = PdfStatus::Kind::password; break;
    case Kind::unsupported_encryption: status.kind = PdfStatus::Kind::unsupported; break;
    case Kind::not_permitted: status.kind = PdfStatus::Kind::not_permitted; break;
    }
    return document;
  }
  status.pages = extracted->report.pages;
  if (status.pages == 0) {
    status.kind = PdfStatus::Kind::no_pages;
  } else if (has_text(extracted->text)) {
    status.kind = PdfStatus::Kind::text;
    status.unmapped = extracted->report.unmapped_glyphs;
    document.pages = std::move(extracted->text.pages);
  } else {
    status.kind = PdfStatus::Kind::no_text;
  }
  return document;
}

PdfStatus pdf_status(fs::path const& file) { return read_pdf(file).status; }

std::string status_text(PdfStatus const& status) {
  switch (status.kind) {
  case PdfStatus::Kind::text: {
    auto text = std::format("text, {}", pages_text(status.pages));
    if (status.unmapped > 0) text += std::format(", {} tecken oläsliga", status.unmapped);
    return text;
  }
  case PdfStatus::Kind::no_text: return std::format("ingen text, {} (inskannad?)", pages_text(status.pages));
  case PdfStatus::Kind::no_pages: return "inga sidor hittade";
  case PdfStatus::Kind::password: return "krypterad med lösenord";
  case PdfStatus::Kind::unsupported: return "krypterad (AES), går inte att läsa";
  case PdfStatus::Kind::not_permitted: return "textkopiering spärrad i filen";
  case PdfStatus::Kind::not_a_pdf: return "inte en pdf";
  case PdfStatus::Kind::cannot_read: return "går inte att läsa";
  }
  return {};
}

} // namespace cratchit
