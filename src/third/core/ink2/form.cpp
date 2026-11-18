#include "core/ink2/form.hpp"

#include <algorithm>
#include <format>
#include <optional>

namespace cratchit::ink2 {

namespace {

bool is_bas_account(std::string_view account) {
  return account.size() == 4 && std::ranges::all_of(account, [](char c) { return c >= '0' && c <= '9'; }) &&
         account.front() >= '1' && account.front() <= '8';
}

// Whether a group with this net (debit positive) meets its condition
bool meets(Condition condition, Amount net) {
  switch (condition) {
  case Condition::always:
    return true;
  case Condition::credit:
    return net <= 0;
  case Condition::debit:
    return net > 0;
  case Condition::profit:
    return net >= 0;
  case Condition::loss:
    return net < 0;
  }
  return false;
}

std::string join(std::vector<std::string> const& texts) {
  std::string out;
  for (auto const& text : texts) {
    out += (out.empty() ? "" : ", ") + text;
  }
  return out;
}

} // namespace

SruCodes sru_codes(sie::Document const& document) {
  SruCodes codes;
  for (auto const& account : document.accounts) {
    if (!account.sru.empty()) {
      codes[account.number] = account.sru;
    }
  }
  return codes;
}

Form build_form(std::vector<Line> const& lines, Balances const& balances, SruCodes const& sru) {
  Form form;
  std::vector<Field const*> fields;      // INK2R: each account in exactly one
  std::vector<Field const*> ink2s_fields; // INK2S: the accounts in them, besides their INK2R field
  for (auto const& line : lines) {
    for (auto const& field : line.fields) {
      (form_of(field.sru) == "INK2S" ? ink2s_fields : fields).push_back(&field);
      form.fields[field.sru];
    }
  }
  auto const field_of = [&](std::string_view code) -> Field const* {
    auto const found = std::ranges::find(fields, code, &Field::sru);
    return found == fields.end() ? nullptr : *found;
  };
  Amount outside = 0;
  std::vector<std::string> outside_accounts;
  std::vector<std::string> without_sru;
  for (auto const& [account, amount] : balances) {
    if (amount == 0) {
      continue;
    }
    if (!is_bas_account(account)) {
      outside += amount;
      outside_accounts.push_back(account);
      continue;
    }
    auto const code = sru.find(account);
    std::optional<std::string> const books_sru =
        code == sru.end() ? std::nullopt : std::optional<std::string>(code->second);
    if (!books_sru) {
      without_sru.push_back(account);
    }
    std::vector<Field const*> taken_by;
    bool books_agree = false; // the #SRU names a field that takes it, or the other field of its plus/minus pair
    for (auto const* field : fields) {
      bool takes = false;
      bool conditional = false;
      for (auto const& group : field->accounts.groups) {
        if (!group.contains(account)) {
          continue;
        }
        conditional = conditional || group.condition != Condition::always;
        Amount net = 0;
        for (auto const& [other, other_amount] : balances) {
          if (group.contains(other)) {
            net += other_amount;
          }
        }
        takes = takes || meets(group.condition, net);
      }
      if (takes) {
        taken_by.push_back(field);
      }
      if (books_sru && field->sru == *books_sru && (takes || conditional)) {
        books_agree = true;
      }
    }
    auto const sru_codes_of = [](std::vector<Field const*> const& taken) {
      std::vector<std::string> codes;
      for (auto const* field : taken) {
        codes.push_back(field->sru);
      }
      return join(codes);
    };
    if (taken_by.size() > 1) {
      auto const chosen = std::ranges::find_if(
          taken_by, [&](Field const* field) { return books_sru && field->sru == *books_sru; });
      if (chosen == taken_by.end()) {
        form.errors.push_back(std::format("Account {} ({}) is in more than one field: {}{}", account,
                                          format_amount(amount), sru_codes_of(taken_by),
                                          books_sru ? std::format(", and its #SRU {} is none of them", *books_sru)
                                                    : std::string(", and it has no #SRU to choose")));
        taken_by.clear();
      } else {
        form.notes.push_back(std::format("#SRU: account {} is in {} in the mapping; its #SRU chooses {}", account,
                                         sru_codes_of(taken_by), *books_sru));
        taken_by = {*chosen};
      }
    } else if (taken_by.empty()) {
      auto const* field = books_sru ? field_of(*books_sru) : nullptr;
      if (field) {
        form.notes.push_back(std::format("#SRU: account {} ({}) is in no field of the mapping; put in {} by its #SRU",
                                         account, format_amount(amount), field->sru));
        taken_by = {field};
      } else {
        form.errors.push_back(std::format("Account {} has an amount {} but is in no field of the mapping{}", account,
                                          format_amount(amount),
                                          books_sru ? std::format(", nor is its #SRU {}", *books_sru) : ""));
      }
    } else if (books_sru && !books_agree) {
      form.notes.push_back(std::format("#SRU: account {} has #SRU {} but the mapping puts it in {} (used)", account,
                                       *books_sru, taken_by.front()->sru));
    }
    for (auto const* field : taken_by) {
      auto& out = form.fields[field->sru];
      out.used = true;
      out.exact += field->sign * amount;
      out.accounts[account] = amount;
    }
  }
  for (auto const* field : ink2s_fields) {
    for (auto const& [account, amount] : balances) {
      if (amount != 0 && is_bas_account(account) && field->accounts.contains(account)) {
        auto& out = form.fields[field->sru];
        out.used = true;
        out.exact += field->sign * amount;
        out.accounts[account] = amount;
      }
    }
  }
  if (!outside_accounts.empty()) {
    form.notes.push_back(std::format("Left out, as not BAS accounts of class 1-8: {} (together {})",
                                     join(outside_accounts), format_amount(outside)));
  }
  if (!sru.empty() && !without_sru.empty()) {
    form.notes.push_back(std::format("#SRU: no #SRU in the books for {}; the mapping decides", join(without_sru)));
  }
  for (auto& entry : form.fields) { // not [sru, field]: sru is a parameter (MSVC C4457)
    entry.second.kronor = to_kronor(entry.second.exact);
  }
  auto const sums = sums_of(form);
  if (sums.assets != sums.equity_and_debt) {
    form.notes.push_back(std::format("Rounding: tillgångar (72xx) add up to {} kr but eget kapital och skulder (73xx) to "
                                     "{} kr, as each field is rounded on its own",
                                     sums.assets, sums.equity_and_debt));
  }
  if (sums.result != sums.year_result) {
    form.notes.push_back(std::format("Rounding: the fields of the resultaträkning add up to {} kr but årets resultat "
                                     "(3.26 less 3.27) is {} kr, as each field is rounded on its own",
                                     sums.result, sums.year_result));
  }
  return form;
}

Sums sums_of(Form const& form) {
  Sums sums;
  for (auto const& [sru, field] : form.fields) {
    if (sru == "7450") {
      sums.year_result += field.kronor;
    } else if (sru == "7550") {
      sums.year_result -= field.kronor;
    } else if (sru.starts_with("72")) {
      sums.assets += field.kronor;
    } else if (sru.starts_with("73")) {
      sums.equity_and_debt += field.kronor;
    } else if (sru.starts_with("74")) {
      sums.result += field.kronor;
    } else if (sru.starts_with("75")) {
      sums.result -= field.kronor;
    }
  }
  return sums;
}

} // namespace cratchit::ink2
