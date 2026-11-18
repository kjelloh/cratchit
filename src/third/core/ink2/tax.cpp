#include "core/ink2/tax.hpp"

#include "core/ink2/mapping.hpp"

#include <algorithm>
#include <format>

namespace cratchit::ink2 {

namespace {

bool used(Form const& form, std::string const& sru) {
  auto const field = form.fields.find(sru);
  return field != form.fields.end() && field->second.used;
}

std::string join(std::vector<std::string> const& texts) {
  std::string out;
  for (auto const& text : texts) {
    out += (out.empty() ? "" : ", ") + text;
  }
  return out;
}

} // namespace

TaxReturn build_tax_return(std::vector<SkvField> const& skv, Form const& form, Previous const& previous) {
  TaxReturn tax;
  // 4.1 and 4.2 are the year's result of INK2R
  if (used(form, "7450")) {
    tax.ink2s["7650"] = form.fields.at("7450").kronor;
  }
  if (used(form, "7550")) {
    tax.ink2s["7750"] = form.fields.at("7550").kronor;
  }
  // The INK2S fields made from the accounts
  for (auto const& [sru, field] : form.fields) {
    if (field.used && form_of(sru) == "INK2S") {
      tax.ink2s[sru] = field.kronor;
      if (field.kronor < 0) {
        tax.notes.push_back(std::format("INK2S {} is negative ({} kr): its accounts take the other way", sru,
                                        field.kronor));
      }
    }
  }
  if (previous.deficit != 0) {
    tax.ink2s["7763"] = previous.deficit;
  }
  // 4.15 or 4.16: the + fields less the - fields
  Kronor total = 0;
  for (auto const& field : fields_of(skv, "INK2S")) {
    if (field.sru == "7670" || field.sru == "7770") {
      continue;
    }
    auto const given = tax.ink2s.find(field.sru);
    if (given == tax.ink2s.end()) {
      continue;
    }
    if (field.sign == '+') {
      total += given->second;
    } else if (field.sign == '-') {
      total -= given->second;
    }
  }
  if (total >= 0) {
    tax.ink2s["7670"] = total;
    tax.ink2["7104"] = total;
  } else {
    tax.ink2s["7770"] = -total;
    tax.ink2["7114"] = -total;
  }
  // The yes/no marks are not in the books
  if (!previous.marks.empty()) {
    tax.ink2s_marks.insert(previous.marks.begin(), previous.marks.end());
    tax.notes.push_back(std::format("INK2S: the marks {} (uppdragstagare, revision) as last year ({})",
                                    join(previous.marks), previous.source));
  } else {
    tax.notes.push_back("INK2S: no marks for uppdragstagare (8040/8041) or revision (8044/8045): not in the books, "
                        "and not from last year's return");
  }
  if (used(form, "7321") || used(form, "7420") || used(form, "7525")) {
    tax.notes.push_back("INK2S: the books have periodiseringsfonder; 4.6a (schablonintäkt) is not computed");
  }
  return tax;
}

} // namespace cratchit::ink2
