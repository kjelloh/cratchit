#pragma once

#include "core/amount.hpp"
#include "core/skv/vat_return/document.hpp"

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// The VAT return as SKV's eSKD file (eSKDUpload 6.0) gives it, in memory: the
// tags it fills in, in file order, and the file itself.
namespace cratchit::skv::vat_return::eskd {

// One element of the file, e.g. <MomsUtgHog>250</MomsUtgHog>
struct Field {
  std::string tag;
  Amount amount = 0; // the box amount as on the form, whole kronor

  bool operator==(Field const&) const = default;
};

struct Document {
  std::string orgnr;         // as in the file, e.g. 556000-0175
  std::string period;        // year and last month of the period, e.g. 202603
  std::vector<Field> fields; // in file order

  bool operator==(Document const&) const = default;
};

// The eSKD content of a VAT return: each box with postings, its amount as it
// is, in the tag of the built-in table, then MomsBetala (box 49, see vat_to_pay)
// always. A box without a tag is left out and logged as a design insufficiency.
Document document_of(vat_return::Document const& form, std::string_view orgnr);

// The file's text: the XML declaration (ISO-8859-1), eSKDUpload 6.0 with OrgNr,
// and Moms with Period and the fields, one element per line (LF). Amounts in
// whole kronor, a negative one with - straight before it. No DOCTYPE. The tags,
// digits and org number are ASCII, so the text is ISO-8859-1 as it is.
std::string xml_of(Document const& document);

// The file name for the period, e.g. moms_202603.eskd; with index 2 or more
// moms_202603_2.eskd
std::string file_name(std::string_view period, int index = 1);

// The first name of file_name in the folder that is not taken, from index 1 up
std::filesystem::path free_file(std::filesystem::path const& folder, std::string_view period);

// Writes xml_of the document to free_file in the folder, so an earlier file is
// never overwritten; the file written, or error text
std::expected<std::filesystem::path, std::string> write_file(std::filesystem::path const& folder,
                                                             Document const& document);

} // namespace cratchit::skv::vat_return::eskd
