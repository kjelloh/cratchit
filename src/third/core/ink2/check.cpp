#include "core/ink2/check.hpp"

#include <algorithm>
#include <format>

namespace cratchit::ink2 {

namespace {

bool is_account(std::string_view account) {
  return account.size() == 4 && std::ranges::all_of(account, [](char c) { return c >= '0' && c <= '9'; });
}

// The sum of the accounts first to last, four digit numbers compared as text
Amount sum_of(Balances const& balances, std::string_view first, std::string_view last) {
  Amount sum = 0;
  for (auto const& [account, amount] : balances) {
    if (is_account(account) && first <= account && account <= last) {
      sum += amount;
    }
  }
  return sum;
}

std::string join(std::vector<std::string> const& texts, std::string_view separator) {
  std::string out;
  for (auto const& text : texts) {
    out += (out.empty() ? "" : std::string(separator)) + text;
  }
  return out;
}

// Text wrapped at spaces to lines of at most width, each but the first indented
std::string wrapped(std::string_view text, std::size_t width, std::string_view indent) {
  std::string out;
  std::size_t line = 0;
  std::size_t position = 0;
  while (position < text.size()) {
    auto end = text.find(' ', position);
    if (end == std::string_view::npos) {
      end = text.size();
    }
    auto const word = text.substr(position, end - position);
    if (line != 0 && line + 1 + word.size() > width) {
      out += '\n' + std::string(indent);
      line = indent.size();
    } else if (line != 0) {
      out += ' ';
      ++line;
    }
    out += word;
    line += word.size();
    position = end + 1;
  }
  return out;
}

void append_item(std::string& out, CheckItem const& item, std::string_view label) {
  out += std::format("{} {}\n", label, wrapped(item.title + (item.fields.empty() ? "" : std::format(" (fält {})", join(item.fields, ", "))), 92,
                                               std::string(label.size() + 1, ' ')));
  if (!item.found.empty()) {
    out += std::format("    Found: {}\n", wrapped(item.found, 92, "    "));
  }
  out += std::format("    Provide: {}\n", wrapped(item.provide, 92, "    "));
}

} // namespace

Markers Check::markers() const {
  Markers out;
  for (auto const& item : missing) {
    for (auto const& field : item.fields) {
      auto& marker = out[field];
      marker += (marker.empty() ? "" : " ") + std::format("TBD [{}]", item.number);
    }
  }
  return out;
}

Check check_return(Form const& form, Balances const& opening, Balances const& balances, Previous const& previous,
                   bool previous_typed, std::optional<Amount> unbooked, bool postal_found) {
  Check check;
  auto const add = [&check](CheckItem item) {
    item.number = static_cast<int>(check.missing.size()) + 1;
    check.missing.push_back(std::move(item));
  };
  if (unbooked) {
    add({0, "Bokslut: årets resultat bokfört", {"7450", "7550", "7650", "7750", "7670", "7770", "7104", "7114"},
         std::format("the year's result {} is not booked to 2099 (nothing on 8999); the return shows it as booked",
                     format_amount(-*unbooked)),
         "close the books (book the year's result to 2099 against 8999, and the other closing entries), export a "
         "new SIE file and run again"});
  }
  if (auto const funds = sum_of(opening, "2110", "2139"); funds != 0) {
    add({0, "4.6a Beräknad schablonintäkt på periodiseringsfonder vid beskattningsårets ingång", {"7654"},
         std::format("periodiseringsfonder (2110-2139) of {} at the start of the year", format_amount(-funds)),
         "the schablonintäkt: the periodiseringsfonder at the start of the year times the statslåneränta of 30 "
         "November the year before (not computed by cratchit_ink2)"});
  }
  if (auto const field = form.fields.find("7521"); field != form.fields.end() && field->second.used) {
    add({0, "4.3b Bokförd nedskrivning av finansiella tillgångar som inte ska dras av", {"7652"},
         std::format("3.17 Nedskrivningar av finansiella anläggningstillgångar och kortfristiga placeringar is {} kr",
                     field->second.kronor),
         "the part of the nedskrivning that may not be deducted (e.g. on näringsbetingade andelar), or 0"});
  }
  if (auto const pensions = sum_of(balances, "7410", "7419"); pensions != 0) {
    add({0, "4.21 Pensionskostnader (som ingår i p. 3.8), and 1.4/1.5 underlag för särskild löneskatt",
         {"8022", "7132", "7133"},
         std::format("pension costs (7410-7419) of {}", format_amount(pensions)),
         "the pension costs and the underlag för särskild löneskatt på pensionskostnader"});
  }
  if (previous.marks.empty()) {
    add({0, "Uppdragstagare har biträtt vid upprättandet av årsredovisningen, och revision (Ja/Nej)",
         {"8040", "8041", "8044", "8045"}, "not in the books, and no marks from last year's return",
         "Ja or Nej for each: give last year's return that has them, or (not yet supported) the answers"});
  }
  if (!postal_found) {
    add({0, "INFO.SRU: postnummer och postort", {},
         "neither the SIE file's #ADRESS nor last year's INFO.SRU gives them",
         "the company's postal address in the books (#ADRESS, e.g. \"17141 Solna\"), or last year's INFO.SRU next "
         "to its BLANKETTER.SRU"});
  }
  check.to_confirm.push_back({0, "4.4b Andra ej bokförda kostnader, and 4.6e Andra ej bokförda intäkter",
                              {"7764", "7665"}, "",
                              "costs to deduct and income to tax that are not in the books, if any"});
  check.to_confirm.push_back(
      {0, "4.13 Andra skattemässiga justeringar av resultatet", {"7663", "7762"}, "",
       "other adjustments, if any: e.g. costs not to deduct booked on accounts for deductible ones (representation, "
       "gifts, fines), which 4.3c does not see"});
  check.to_confirm.push_back({0, "4.14b-c Reduktion av outnyttjat underskott", {"7671", "7672"}, "",
                              "a reduction for beloppsspärr, ackord, konkurs, koncernbidragsspärr or fusionsspärr, if "
                              "any (e.g. after a change of owner)"});
  check.to_confirm.push_back({0, "4.20 Lån från aktieägare (fysisk person) vid räkenskapsårets utgång", {"8026"}, "",
                              "the loans from shareholders who are natural persons, if any"});
  if (previous_typed) {
    check.to_confirm.push_back(
        {0, "4.14a Outnyttjat underskott från föregående år", {"7763"},
         std::format("{} kr, given by --previous-deficit", previous.deficit),
         "that it is last year's 4.16 as SKV decided it (Mina sidor or the slutskattebesked)"});
  }
  return check;
}

std::string format_check(Check const& check) {
  std::string out = "INK2 check\n==========\n";
  if (check.missing.empty()) {
    out += "\nComplete: nothing the books do not give is known to be missing.\n";
  } else {
    out += std::format("\nNot complete: {} {} missing, marked TBD [n] in the return. While any is missing the SRU "
                       "files are not written.\n\n",
                       check.missing.size(), check.missing.size() == 1 ? "item is" : "items are");
    for (auto const& item : check.missing) {
      append_item(out, item, std::format("[{}]", item.number));
    }
  }
  out += "\nTo confirm: these may apply, and the books cannot tell.\n\n";
  for (auto const& item : check.to_confirm) {
    append_item(out, item, "-");
  }
  return out;
}

} // namespace cratchit::ink2
