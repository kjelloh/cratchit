#include "core/skv/vat_return/checks.hpp"

#include "core/skv/accounts.hpp"
#include "core/skv/vat_return/clearing.hpp"

#include <algorithm>
#include <format>
#include <map>
#include <set>

namespace cratchit::skv::vat_return {

namespace {

bool holds(Period const& period, Date date) {
  return period.first <= date && date <= period.last;
}

std::string name_of(books::Voucher const& voucher) {
  return std::format("{} {} \"{}\"", books::voucher_id(voucher.series, voucher.number), format_iso_date(voucher.event.date),
                     voucher.event.heading);
}

bool counted(books::Posting const& posting) {
  return posting.kind != books::Posting::Kind::removed;
}

bool is_memo_account(std::string_view account) {
  return account.size() == 4 && account.front() == '9';
}

// True when the voucher posts a VAT clearing account
bool posts_clearing_account(books::Voucher const& voucher) {
  return std::ranges::any_of(voucher.postings, [](books::Posting const& p) {
    return counted(p) && is_vat_clearing_account(p.account);
  });
}

// The accounts of the voucher's counted rows that no clearing or charge voucher posts
std::set<std::string> foreign_accounts(books::Voucher const& voucher) {
  std::set<std::string> accounts;
  for (auto const& p : voucher.postings) {
    if (counted(p) && !is_vat_account(p.account) && !is_vat_clearing_account(p.account) && p.account != tax_account &&
        p.account != rounding_account && !is_memo_account(p.account)) {
      accounts.insert(p.account);
    }
  }
  return accounts;
}

// A clearing voucher: posts a clearing account and VAT accounts, and nothing a clearing does not
bool is_clearing(books::Voucher const& voucher) {
  return posts_clearing_account(voucher) && foreign_accounts(voucher).empty() &&
         std::ranges::any_of(voucher.postings,
                             [](books::Posting const& p) { return counted(p) && is_vat_account(p.account); });
}

bool in_boxes(std::optional<Box> const& box, std::initializer_list<std::string_view> boxes) {
  return box && std::ranges::find(boxes, *box) != boxes.end();
}

std::string list_of(std::set<std::string> const& items) {
  std::string text;
  for (auto const& item : items) {
    text += (text.empty() ? "" : ", ") + item;
  }
  return text;
}

// Rule 3 for one kind of purchase: its bases and output VAT boxes; the output VAT it books
Amount check_pair(books::Voucher const& voucher, std::initializer_list<std::string_view> bases,
                  std::string_view bases_name, std::initializer_list<std::string_view> vats, std::string_view vats_name,
                  std::vector<std::string>& findings) {
  Amount base = 0;
  std::set<std::string> base_boxes;
  std::map<Box, Amount> vat;
  for (auto const& p : voucher.postings) {
    if (!counted(p)) {
      continue;
    }
    auto const box = box_of(p.account);
    if (in_boxes(box, bases)) {
      base += p.amount;
      base_boxes.insert(*box);
    } else if (in_boxes(box, vats)) {
      vat[*box] += p.amount;
    }
  }
  Amount vat_total = 0;
  for (auto const& [box, amount] : vat) {
    vat_total += amount;
  }
  auto const has_base = !base_boxes.empty();
  if (has_base && vat.empty()) {
    findings.push_back(std::format("{}: inköpsunderlag {} i ruta {}, men ingen utgående moms i rutorna {}", name_of(voucher),
                                   format_amount(base), list_of(base_boxes), vats_name));
  } else if (!has_base && !vat.empty()) {
    std::set<std::string> vat_boxes_used;
    for (auto const& [box, amount] : vat) {
      vat_boxes_used.insert(box);
    }
    findings.push_back(std::format("{}: utgående moms {} i ruta {}, men inget inköpsunderlag i rutorna {}", name_of(voucher),
                                   format_amount(-vat_total), list_of(vat_boxes_used), bases_name));
  } else if (has_base && vat.size() == 1) {
    auto const& [box, amount] = *vat.begin();
    auto const rate = rate_of(box).value_or(0);
    auto const magnitude = base < 0 ? -base : base;
    auto const expected = magnitude * rate / 100; // towards zero, to whole öre
    auto const booked = amount < 0 ? -amount : amount;
    if (booked != expected) {
      findings.push_back(std::format("{}: ruta {} har {}, men {} % av inköpsunderlaget {} är {}", name_of(voucher),
                                     box, format_amount(booked), rate, format_amount(magnitude),
                                     format_amount(expected)));
    }
  }
  return vat_total;
}

} // namespace

std::optional<int> rate_of(Box const& box) {
  if (box == "10" || box == "30" || box == "60") {
    return 25;
  }
  if (box == "11" || box == "31" || box == "61") {
    return 12;
  }
  if (box == "12" || box == "32" || box == "62") {
    return 6;
  }
  return std::nullopt;
}

std::vector<std::string> checks_of(books::Books const& books, Period period) {
  std::vector<std::string> findings;

  // 1. Not fully booked: drafts dated in the period (events and statement entries, as cratchit keeps them)
  std::vector<books::Draft const*> drafts;
  for (auto const& year : books.years) {
    for (auto const& draft : year.drafts) {
      if (holds(period, draft.event.date)) {
        drafts.push_back(&draft);
      }
    }
  }
  if (!drafts.empty()) {
    std::ranges::stable_sort(drafts, {}, [](auto const* d) { return d->event.date; });
    // The first one's heading, else the text of a posting (e.g. a statement entry's), else its date alone
    auto const& first = *drafts.front();
    auto text = first.event.heading;
    for (auto const& posting : first.postings) {
      if (!text.empty()) break;
      if (posting.kind != books::Posting::Kind::removed) text = posting.text;
    }
    findings.push_back(std::format("{} utkast daterade i perioden, inte bokförda än (det första {}{})", drafts.size(),
                                   format_iso_date(first.event.date),
                                   text.empty() ? std::string() : std::format(" \"{}\"", text)));
  }

  // The period's vouchers, from every fiscal year
  std::vector<books::Voucher const*> vouchers;
  for (auto const& year : books.years) {
    for (auto const& voucher : year.vouchers) {
      if (holds(period, voucher.event.date)) {
        vouchers.push_back(&voucher);
      }
    }
  }

  // 2 and 3, voucher by voucher
  for (auto const* voucher : vouchers) {
    if (posts_clearing_account(*voucher)) {
      if (auto const foreign = foreign_accounts(*voucher); !foreign.empty()) {
        findings.push_back(std::format("{}: konterar {} eller {} tillsammans med {}; bara en momsavräkning eller en "
                                       "debitering från Skatteverket konterar dem",
                                       name_of(*voucher), vat_liability_account, vat_receivable_account,
                                       list_of(foreign)));
      }
      continue; // a clearing or charge voucher is no purchase
    }
    auto const output = check_pair(*voucher, {"20", "21", "22", "23", "24"}, "20-24", {"30", "31", "32"}, "30-32",
                                   findings) +
                        check_pair(*voucher, {"50"}, "50", {"60", "61", "62"}, "60-62", findings);
    if (output != 0) {
      Amount deducted = 0;
      for (auto const& p : voucher->postings) {
        if (counted(p) && box_of(p.account) == Box{"48"}) {
          deducted += p.amount;
        }
      }
      if (deducted != -output) {
        findings.push_back(std::format("{}: ruta 48 drar av {}, men den utgående momsen på inköpen är {}; den dras "
                                       "av i sin helhet",
                                       name_of(*voucher), format_amount(deducted), format_amount(-output)));
      }
    }
  }

  // 4. Registered after the period's clearing voucher
  books::Voucher const* clearing = nullptr;
  for (auto const* voucher : vouchers) {
    if (is_clearing(*voucher) && voucher->registered && (!clearing || *voucher->registered < *clearing->registered)) {
      clearing = voucher;
    }
  }
  if (clearing) {
    for (auto const* voucher : vouchers) {
      if (voucher == clearing || !voucher->registered || *voucher->registered <= *clearing->registered ||
          posts_clearing_account(*voucher)) {
        continue;
      }
      if (std::ranges::any_of(voucher->postings,
                              [](books::Posting const& p) { return counted(p) && box_of(p.account); })) {
        findings.push_back(std::format("{}: registrerad {}, efter momsavräkningen {} (registrerad {}); "
                                       "momsdeklarationen som gjordes då saknar den",
                                       name_of(*voucher), format_iso_date(*voucher->registered),
                                       books::voucher_id(clearing->series, clearing->number),
                                       format_iso_date(*clearing->registered)));
      }
    }
  }

  // 5. VAT-range accounts outside the return
  std::map<std::string, std::vector<std::string>> outside;
  for (auto const* voucher : vouchers) {
    for (auto const& p : voucher->postings) {
      if (counted(p) && p.account.size() == 4 && p.account.starts_with("26") && p.account != vat_liability_account &&
          !box_of(p.account)) {
        auto& names = outside[p.account];
        auto const name = books::voucher_id(voucher->series, voucher->number);
        if (std::ranges::find(names, name) == names.end()) {
          names.push_back(name);
        }
      }
    }
  }
  for (auto const& [account, names] : outside) {
    std::string list;
    for (auto const& name : names) {
      list += (list.empty() ? "" : ", ") + name;
    }
    findings.push_back(std::format("{} har konteringar i perioden ({}), men redovisas inte i någon ruta", account,
                                   list));
  }
  return findings;
}

} // namespace cratchit::skv::vat_return
