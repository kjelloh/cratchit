#include "core/ar/requirements.hpp"

#include <algorithm>
#include <format>

namespace cratchit::ar {

namespace {

Requirement const& requirement(std::string_view id) {
  auto const& all = requirements();
  return *std::ranges::find(all, id, &Requirement::id);
}

// The exact amount of fiscal year 0 of the first line with text outside the notes, else 0
Amount year0_of(Statement const& statement, std::string_view text) {
  for (auto const& line : statement.lines) {
    if (!line.in_notes && line.kind != LineKind::heading && line.text == text) {
      return line.exact.front();
    }
  }
  return 0;
}

// Whether two years' Nettoomsättning differ by more than 30 % of the earlier one
bool varies_more_than_30_percent(Amount later, Amount earlier) {
  if (earlier == 0) {
    return later != 0;
  }
  auto const difference = later > earlier ? later - earlier : earlier - later;
  auto const base = earlier < 0 ? -earlier : earlier;
  return difference * 10 > base * 3;
}

} // namespace

std::vector<Requirement> const& requirements() {
  static std::vector<Requirement> const all{
      {"bokslut", "Bokslut: årets resultat bokfört",
       "BFL 6 kap. 1 § (the bookkeeping is closed with the årsredovisning; BFNAR 2016:10 kap. 1 kommentar)",
       "close the books: book the year's result to 2099 against 8999, and the other closing entries in the "
       "closing notes",
       ""},
      {"sate", "Säte", "BFNAR 2016:10 p. 5.2; ÅRL 2 kap. 5 §",
       "the municipality where the company has its registered office (säte)", "Säte"},
      {"verksamhet", "Allmänt om verksamheten", "BFNAR 2016:10 p. 4.9, 5.3, 5.6; ÅRL 6 kap. 1 §",
       "the business the company actually carries on, a change of company name, and circumstances that "
       "particularly affect it (e.g. only one customer)",
       "Allmänt om verksamheten"},
      {"handelser", "Väsentliga händelser under räkenskapsåret", "BFNAR 2016:10 p. 4.9, 5.4, 5.7; ÅRL 6 kap. 1 §",
       "important decided changes in the business, external factors that affected it, and whether the company "
       "can no longer be assumed to continue (or kontrollbalansräkning); \"Inga.\" if there were none",
       "Väsentliga händelser under räkenskapsåret"},
      {"flerar", "Flerårsöversikt, de två tidigare åren", "BFNAR 2016:10 p. 5.5; ÅRL 6 kap. 1 §",
       "Nettoomsättning (tkr), Resultat efter finansiella poster (tkr) and Soliditet (%) for the two fiscal years "
       "before those in the SIE file (e.g. from last year's årsredovisning)",
       "Flerårsöversikt, tidigare år"},
      {"omsattning", "Kommentar till nettoomsättningens förändring", "BFNAR 2016:10 p. 5.5",
       "why the Nettoomsättning varies more than 30 % between the years", "Kommentar till flerårsöversikten"},
      {"forslag", "Resultatdisposition, styrelsens förslag", "BFNAR 2016:10 p. 5.8; ÅRL 6 kap. 2 §",
       "how much of the funds to pay as dividend (Utdelas till aktieägare); the rest is Balanseras i ny räkning",
       "Utdelning"},
      {"yttrande", "Styrelsens yttrande om vinstutdelningen", "ABL 18 kap. 4 §; BFNAR 2016:10 p. 5.8 kommentar",
       "the board's motivated statement that a proposed dividend is försvarlig with regard to ABL 17 kap. 3 §",
       "Yttrande om vinstutdelning"},
      {"principer", "Not 1 Redovisningsprinciper, avskrivningar", "BFNAR 2016:10 p. 18.4; ÅRL 5 kap. 8 §",
       "the nyttjandeperiod (years) or avskrivningsprocent for each kind of anläggningstillgång", "Avskrivningstider"},
      {"anstallda", "Not 2 Medelantal anställda", "BFNAR 2016:10 p. 18.9; ÅRL 5 kap. 20 §",
       "the average number of employees during the year (BFNAR 2006:11); no note without employees",
       "Medelantal anställda"},
      {"not_post", "Not för anläggningstillgång", "BFNAR 2016:10 p. 18.10; ÅRL 5 kap. 8 §",
       "a note with ingående and utgående anskaffningsvärden, additions, disposals and the year's av- and "
       "nedskrivningar (rows under Noter in the mapping)",
       ""},
      {"sakerheter", "Ställda säkerheter", "BFNAR 2016:10 p. 18.13-18.14; ÅRL 5 kap. 14 §",
       "kind and amount of each security given (företagsinteckning, fastighetsinteckning, ...); \"Inga.\" if none",
       "Ställda säkerheter"},
      {"eventual", "Eventualförpliktelser", "BFNAR 2016:10 p. 18.15-18.19; ÅRL 5 kap. 15 §",
       "possible obligations, e.g. guarantees, and their amount; \"Inga.\" if none", "Eventualförpliktelser"},
      {"styrelselan", "Lån och säkerheter till styrelseledamöter m.fl.", "ÅRL 5 kap. 18 §",
       "loans, securities or guarantees given to a board member or the VD, with amount and terms; \"Inga.\" if none",
       "Lån till styrelseledamöter"},
      {"efter", "Väsentliga händelser efter räkenskapsårets slut", "BFNAR 2016:10 p. 18.20-18.22; ÅRL 5 kap. 22 §",
       "important events after the balance day; \"Inga.\" if none", "Väsentliga händelser efter räkenskapsårets slut"},
      {"datering", "Datering", "ÅRL 2 kap. 7 §; BFNAR 2016:10 kap. 3 kommentar",
       "the place and the date the årsredovisning's content was decided", "Ort och datum"},
      {"underskrifter", "Underskrifter", "ÅRL 2 kap. 7 §",
       "the name and role of every board member and the VD, and the date each signed", "Underskrifter"},
  };
  return all;
}

std::optional<std::string> Check::marker(std::string_view id) const {
  auto const found = std::ranges::find_if(missing, [id](CheckItem const& item) { return item.requirement->id == id; });
  return found == missing.end() ? std::nullopt : std::optional(std::format("TBD [{}]", found->number));
}

Check check_requirements(Statement const& statement, Overview const& overview, Notes const& notes,
                         std::optional<Amount> unbooked_result) {
  Check check;
  std::vector<CheckItem> confirm;
  auto const missing = [&check](std::string_view id, std::string detail = {}) {
    check.missing.push_back({&requirement(id), 0, std::move(detail)});
  };
  auto const to_confirm = [&confirm](std::string_view id, std::string detail = {}) {
    confirm.push_back({&requirement(id), 0, std::move(detail)});
  };

  if (unbooked_result) {
    missing("bokslut", std::format("the year's result {} is not booked to 2099; the årsredovisning shows it as booked",
                                   format_amount(-*unbooked_result)));
  }
  missing("sate");
  missing("verksamhet");
  missing("handelser");
  missing("flerar", "an SIE file holds two years; the vägledning asks for the year and the three before");
  auto const& sales = overview.net_sales;
  if (sales.size() >= 2 && varies_more_than_30_percent(sales[0], sales[1])) {
    missing("omsattning", std::format("Nettoomsättning went from {} kr to {} kr", to_kronor(sales[1]), to_kronor(sales[0])));
  } else {
    to_confirm("omsattning", "if it varies more than 30 % between any of the four years");
  }
  missing("forslag");
  bool const depreciates =
      !notes.notes.empty() || year0_of(statement, "Av- och nedskrivningar av materiella och immateriella "
                                                  "anläggningstillgångar") != 0;
  if (depreciates) {
    missing("principer", notes.notes.empty() ? "the resultaträkning has Av- och nedskrivningar"
                                             : "the company has anläggningstillgångar with notes");
  } else {
    to_confirm("principer", "if the company has anläggningstillgångar that are depreciated");
  }
  if (year0_of(statement, "Personalkostnader") != 0) {
    missing("anstallda", "the resultaträkning has Personalkostnader");
  } else {
    to_confirm("anstallda", "the books show no Personalkostnader, and without employees no note is needed");
  }
  // Every shown post under Immateriella or Materiella anläggningstillgångar needs a note
  auto const references = note_references(notes);
  bool in_fixed_assets = false;
  for (auto const& line : statement.lines) {
    if (line.in_notes) {
      break;
    }
    if (line.kind == LineKind::heading) {
      in_fixed_assets =
          line.text == "Immateriella anläggningstillgångar" || line.text == "Materiella anläggningstillgångar";
    } else if (line.kind == LineKind::sum) {
      in_fixed_assets = false;
    } else if (in_fixed_assets && line.shown && !references.contains(line.text)) {
      missing("not_post", std::format("no note for {}", line.text));
    }
  }
  missing("datering");
  missing("underskrifter");
  to_confirm("yttrande", "if the board proposes a dividend");
  to_confirm("sakerheter", "if the company has given securities");
  to_confirm("eventual", "if the company has possible obligations");
  to_confirm("styrelselan", "if there are such loans or securities");
  to_confirm("efter", "if there were such events");

  int number = 0;
  for (auto& item : check.missing) {
    item.number = ++number;
  }
  for (auto& item : confirm) {
    item.number = ++number;
  }
  check.to_confirm = std::move(confirm);
  return check;
}

} // namespace cratchit::ar
