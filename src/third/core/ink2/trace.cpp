#include "core/ink2/trace.hpp"

#include "core/date.hpp"
#include "core/ink2/output.hpp"

#include <algorithm>
#include <charconv>
#include <format>
#include <map>
#include <set>
#include <tuple>

namespace cratchit::ink2 {

namespace {

// A voucher's transaction on an account
struct Posting {
  sie::Voucher const* voucher = nullptr;
  Amount amount = 0; // as booked
};

int number_of(std::string_view number) {
  int value = 0;
  std::from_chars(number.data(), number.data() + number.size(), value);
  return value;
}

bool earlier(Posting const& left, Posting const& right) {
  return std::tuple(left.voucher->date, left.voucher->series, number_of(left.voucher->number)) <
         std::tuple(right.voucher->date, right.voucher->series, number_of(right.voucher->number));
}

bool is_balance_account(std::string_view account) {
  return account.starts_with('1') || account.starts_with('2');
}

std::string condition_text(Condition condition) {
  switch (condition) {
  case Condition::always:
    return "";
  case Condition::credit:
    return "Om netto +";
  case Condition::debit:
    return "Om netto -";
  case Condition::profit:
    return "+ 899x (vinst)";
  case Condition::loss:
    return "- 899x (förlust)";
  }
  return "";
}

class Tracer {
public:
  Tracer(sie::Document const& document, std::vector<Line> const& lines, Balances const& balances)
      : document_(document), lines_(lines), balances_(balances) {
    for (auto const& voucher : document.vouchers) {
      for (auto const& transaction : voucher.transactions) {
        if (transaction.kind != sie::Transaction::Kind::removed) {
          postings_[transaction.account].push_back({&voucher, transaction.amount});
        }
      }
    }
    for (auto& [account, list] : postings_) {
      std::ranges::stable_sort(list, earlier);
    }
    for (auto const& balance : document.opening) {
      if (balance.year == 0) {
        opening_[balance.account] += balance.amount;
      }
    }
  }

  std::string name_of(std::string_view account) const {
    auto const found = std::ranges::find(document_.accounts, account, &sie::Account::number);
    return found == document_.accounts.end() ? "" : found->name;
  }

  // An account with its balances, its vouchers and a remark
  void append_account(std::string& out, std::string const& account, std::string_view remark) const {
    auto const amount = balances_.contains(account) ? balances_.at(account) : Amount{0};
    auto head = std::format("  {} {}", account, name_of(account));
    if (is_balance_account(account)) {
      auto const opening = opening_.contains(account) ? opening_.at(account) : Amount{0};
      head += std::format("   IB {}   UB {}", format_amount(opening), format_amount(amount));
    } else {
      head += std::format("   RES {}", format_amount(amount));
    }
    out += head + (remark.empty() ? "" : std::format("   ({})", remark)) + '\n';
    if (auto const found = postings_.find(account); found != postings_.end()) {
      for (auto const& posting : found->second) {
        auto const& voucher = *posting.voucher;
        auto const id = std::format("{} {}", voucher.series, voucher.number);
        out += std::format("      {} {:<8} {:<48} {:>14}\n", format_iso_date(voucher.date), id,
                           voucher.text.substr(0, 48), format_amount(posting.amount));
      }
    }
  }

  // A field of the mapping with an amount and what it is made of
  void append_field(std::string& out, Line const& line, Field const& field, FieldAmount const& amount) const {
    out += std::format("\n{} {}  {}  {}\n", line.form, line.ink2, field.sru, line.text);
    out += std::format("  Mapping: {}; shown {}\n", field.bas, field.sign > 0 ? "as booked" : "negated");
    for (auto const& group : field.accounts.groups) {
      if (group.condition == Condition::always ||
          !std::ranges::any_of(amount.accounts, [&](auto const& entry) { return group.contains(entry.first); })) {
        continue;
      }
      Amount net = 0;
      for (auto const& [account, value] : balances_) {
        if (group.contains(account)) {
          net += value;
        }
      }
      out += std::format("  {}: the accounts of the group net {} ({}), so this field\n",
                         condition_text(group.condition), format_amount(net), net > 0 ? "a debit" : "a credit or 0");
    }
    Amount booked = 0;
    for (auto const& [account, value] : amount.accounts) {
      booked += value;
      std::string remark;
      if (!field.accounts.contains(account)) {
        remark = "not in the mapping's accounts: placed here by its #SRU";
      } else if (form_of(field.sru) == "INK2R") {
        std::vector<std::string> others;
        for (auto const& other_line : lines_) {
          for (auto const& other : other_line.fields) {
            if (&other != &field && form_of(other.sru) == "INK2R" && other.accounts.contains(account) &&
                std::ranges::none_of(field.accounts.groups,
                                     [](AccountGroup const& group) { return group.condition != Condition::always; })) {
              others.push_back(other.sru);
            }
          }
        }
        if (!others.empty()) {
          remark = std::format("the mapping has it also in {}; its #SRU chose this field", others.front());
        }
      }
      append_account(out, account, remark);
    }
    out += std::format("  Sum {} as booked, {} shown: {} kr\n", format_amount(booked), format_amount(amount.exact),
                       format_kronor(amount.kronor));
  }

private:
  sie::Document const& document_;
  std::vector<Line> const& lines_;
  Balances const& balances_;
  std::map<std::string, std::vector<Posting>> postings_;
  Balances opening_;
};

std::string value_of(std::map<std::string, Kronor> const& amounts, std::string const& sru) {
  auto const found = amounts.find(sru);
  return found == amounts.end() ? "" : format_kronor(found->second);
}

} // namespace

std::string format_trace(sie::Document const& document, std::vector<Line> const& lines,
                         std::vector<SkvField> const& skv, Balances const& balances, Form const& form,
                         TaxReturn const& tax, Previous const& previous) {
  Tracer const tracer(document, lines, balances);
  std::string out = "Trace of the return, in the order it is made: INK2R and INK2S from the accounts and their\n"
                    "vouchers of fiscal year 0, then INK2S and INK2 page 1 by their rules. Amounts as booked: debit\n"
                    "positive.\n";
  // The fields made from accounts
  std::set<std::string> on_a_field;
  for (std::string_view const wanted : {"INK2R", "INK2S"}) {
    out += std::format("\n{} from the accounts\n{}\n", wanted, std::string(wanted.size() + 18, '='));
    for (auto const& line : lines) {
      if (line.form != wanted || line.kind != Line::Kind::row) {
        continue;
      }
      for (auto const& field : line.fields) {
        auto const& amount = form.fields.at(field.sru);
        if (amount.used) {
          tracer.append_field(out, line, field, amount);
          for (auto const& [account, value] : amount.accounts) {
            if (wanted == "INK2R") {
              on_a_field.insert(account);
            }
          }
        }
      }
    }
  }
  // The fields made by rules
  out += "\nINK2S and INK2 page 1 by their rules\n====================================\n";
  auto const rule = [&out](std::string_view form_name, std::string_view number, std::string_view sru,
                           std::string_view text, std::string const& value) {
    out += std::format("\n{} {}  {}  {}\n  {}\n", form_name, number, sru, value, text);
  };
  if (tax.ink2s.contains("7650")) {
    rule("INK2S", "4.1", "7650", "Årets resultat, vinst = INK2R 3.26 (7450)", value_of(tax.ink2s, "7650"));
  }
  if (tax.ink2s.contains("7750")) {
    rule("INK2S", "4.2", "7750", "Årets resultat, förlust = INK2R 3.27 (7550)", value_of(tax.ink2s, "7750"));
  }
  if (tax.ink2s.contains("7763")) {
    rule("INK2S", "4.14a", "7763",
         std::format("Outnyttjat underskott från föregående år = last year's 4.16 ({})", previous.source),
         value_of(tax.ink2s, "7763"));
  }
  std::string sum;
  Kronor total = 0;
  for (auto const& field : fields_of(skv, "INK2S")) {
    auto const given = tax.ink2s.find(field.sru);
    if (field.sru == "7670" || field.sru == "7770" || given == tax.ink2s.end() ||
        (field.sign != '+' && field.sign != '-')) {
      continue;
    }
    total += field.sign == '+' ? given->second : -given->second;
    sum += std::format(" {} {} {}", field.sign, field.ink2, format_kronor(given->second));
  }
  auto const result = total >= 0 ? std::format("4.15 Överskott {}", format_kronor(total))
                                 : std::format("4.16 Underskott {}", format_kronor(-total));
  rule("INK2S", total >= 0 ? "4.15" : "4.16", total >= 0 ? "7670" : "7770",
       std::format("SKV's + fields less its - fields:{} = {}, so {}", sum.empty() ? " (none)" : sum,
                   format_kronor(total), result),
       total >= 0 ? value_of(tax.ink2s, "7670") : value_of(tax.ink2s, "7770"));
  if (tax.ink2.contains("7104")) {
    rule("INK2", "1.1", "7104", "Överskott av näringsverksamhet = INK2S 4.15 (7670)", value_of(tax.ink2, "7104"));
  }
  if (tax.ink2.contains("7114")) {
    rule("INK2", "1.2", "7114", "Underskott av näringsverksamhet = INK2S 4.16 (7770)", value_of(tax.ink2, "7114"));
  }
  if (!tax.ink2s_marks.empty()) {
    std::string marks;
    for (auto const& mark : tax.ink2s_marks) {
      marks += (marks.empty() ? "" : ", ") + mark;
    }
    out += std::format("\nINK2S marks {}: X, as last year's return ({})\n", marks, previous.source);
  }
  // What is on no field
  std::vector<std::string> left;
  for (auto const& [account, amount] : balances) {
    if (amount != 0 && !on_a_field.contains(account)) {
      left.push_back(account);
    }
  }
  out += "\nAccounts with an amount on no field of INK2R\n============================================\n";
  if (left.empty()) {
    out += "None.\n";
  }
  for (auto const& account : left) {
    tracer.append_account(out, account, "");
  }
  return out;
}

} // namespace cratchit::ink2
