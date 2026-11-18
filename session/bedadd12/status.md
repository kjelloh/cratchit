# Status

Målet står i [session.md](session.md) och besluten i [beslut.md](beslut.md).

## Klart

* 2026-11-18: Rapporterna finns i cratchit third, i grenen `master`. Inget är committat.
  * **Kärnan:** `src/third/core/books/report.{hpp,cpp}` har `balance_report`, `result_report`, `voucher_list` och `account_ledger`.
    * Beräkningarna och testdatan kommer från take 3.
    * Texterna är på svenska.
    * Kolumnerna räknas i tecken (UTF-8), så att å, ä och ö inte förskjuter dem.
    * Take 3:s `core/books/columns` togs inte in. Det lilla som behövdes ligger i `report.cpp`.
  * **Gränssnittet:** `src/third/ui/reports_menu.{hpp,cpp}` har tre states.
    * `make_reports_menu` är *Rapporter*.
    * `make_report_view` visar en rapport hel, utan sidvisning och utan val.
    * `make_ledger_menu` frågar efter kontonumret till huvudboken.

    `ui/fiscal_year_menu.cpp` har fått valet `3 Rapporter`.
  * **Hjälpämnen:** `rapporter`, `rapport` och `huvudbok` (`ui/help_topics.hpp`), med egna stycken i användarmanualen.
  * **Användarmanualen** (`src/third/docs/anvandarmanual.md`) har avsnittet *Rapporter*, med *Läsa en rapport* och *Huvudbok för ett konto*, före *Årsflöde*. Räkenskapsårets avsnitt nämner `3 Rapporter`.
  * **Tester:**
    * `test/test_report.cpp` har 8 tester av rapporternas text: första året, ingående balanser som inte går ihop eller skiljer sig från källan, resultatet bokfört på 8999, struken och tillagd rad, och huvudbok med löpande saldo.
    * `test/test_reports_menu.cpp` har 4 tester av menyerna: valen, rapporternas innehåll, att en lång rapport (184 rader) visas hel utan val, och huvudbokens kontonummer.
    * I `test/test_ui.cpp` har räkenskapsårets val fått `3 Rapporter`.
  * **Provat på macOS:**
    * Med legacy (`build/Debug`): `cratchit_tests` 814 tester och `ctest` 1125 tester går igenom.
    * Utan legacy, som CI bygger: `ctest` 982 tester går igenom.
    * I det riktiga programmet, med `HOME` i en tillfällig mapp: en bokföring skapad med *Ny bokföring här > Från SIE* från en liten testfil. Sedan öppnades `2025 > 3 Rapporter` med tangenterna, och balansrapport, resultatrapport, verifikationslista och huvudbok för 1930 gav samma siffror som testerna.
  * **Inte provat:** rapporterna på en riktig bokföring med många konton och verifikationer. Inte heller provat på Windows och Linux; det sker i GitHub Actions vid nästa push.

* 2026-11-18: Rapporterna visas hela, efter att användaren hade provat dem.
  * Sidvisningen och `<`/`>` är borttagna ur `ReportState` i `ui/reports_menu.cpp`. Vyn är rapportens rader och inget annat, utan rubriken `rad 1–20 av 57`.
  * Testet av sidvisning är ersatt av `AReportIsShownWholeWithoutPaging`.
  * Manualens *Läsa en rapport* beskriver hela rapporter, med en hel balansrapport som exempel. Huvudboken nämner att `-` leder tillbaka till frågan om kontonummer.
  * Provat: `cratchit_tests` 814 tester går igenom, och i programmet visas verifikationslistan hel.

* 2026-11-18: Huvudboken visar alla konton när man trycker Enter utan kontonummer.
  * Kärnan: `general_ledger` i `core/books/report`. Den och `account_ledger` delar `entries_of` och `ledger_lines`.
  * Gränssnittet: `LedgerState` är ett textfält (`is_text_field`), och Enter på en tom rad leder till rapporten `Alla konton`.
  * Manualen: avsnittet heter nu *Huvudbok*.
  * Två nya tester, `GeneralLedgerHasEveryAccountWithRowsOrAnOpeningBalance` och `HuvudbokWithoutAnAccountShowsAllAccounts`. `cratchit_tests` 816 tester går igenom.
  * Provat i programmet: `4` och Enter visar alla fyra kontona, `-` går tillbaka till frågan, och `1930` och Enter visar ett konto.

* 2026-11-18: Rapportens rubrik har organisationsnumret.
  * `company_of` i `ui/reports_menu.cpp` läser `orgnr` ur `project.txt` och ger `Namn (orgnr)`, eller bara namnet när numret saknas.
  * Testerna kontrollerar båda fallen. `cratchit_tests` 817 tester går igenom.
  * Provat i programmet: resultatrapporten har `Test AB (556000-0001)` under rubriken.

* 2026-11-18: `3 Utskrift` i en bokförd verifikation.
  * Kärnan: `voucher_document` i `core/books/report`.
  * Gränssnittet:
    * `make_voucher_menu` tar `PrintVoucher print`, som `reverse`. Valet `3 Utskrift` finns bara när `print` är given och verifikationen inte är ett utkast; för ett utkast är `3` fortfarande *Använd mall*.
    * `voucher_printout` i `ui/reports_menu` läser bokföringen och ger utskriften med `company_of` i rubriken.
    * `ui/booked_menu.cpp` ger `print` när en verifikation öppnas. Utskriften visas med `make_report_view("Utskrift", …)`.
  * Manualen: avsnittet *Utskrift av en verifikation* under *Öppna en verifikation*, och verifikationens hjälptext nämner `3`. Exemplet med D19 visar nu också `2 Ändringsverifikation` och `3 Utskrift`. *Läsa en rapport* säger att `-` går tillbaka dit man kom ifrån.
  * Tester: `VoucherDocumentHasWhatTheLawAsksOfAVoucher`, `VoucherDocumentMarksStruckAndAddedRowsWithTheirOwnText` och `UtskriftOfABookedVoucherShowsItWhole`. Två tester i `test_ui.cpp` förväntar sig nu `3 Utskrift` bland valen. `cratchit_tests` 820 tester går igenom.
  * Provat i programmet: *Årsflöde > Bokfört*, `A2` och `3` visar utskriften, med serien `A  Redovisning`.

* 2026-11-18: En tom rad före och efter rapporter och utskrifter.
  * `ReportState::view` i `ui/reports_menu.cpp` lägger till dem. Rapporternas egna rader i `core/books/report` är oförändrade.
  * Testerna i `test_reports_menu.cpp` kontrollerar de tomma raderna. `cratchit_tests` 820 tester går igenom.
  * Manualens två exempel och *Läsa en rapport* visar dem.
  * Provat i programmet: utskriften av A2 har en tom rad under prompten och en före nästa prompt.

## Att känna till

* Balansrapportens *Beräknat resultat* är negativt för en vinst, eftersom rapporten visar debet positivt. Resultatrapportens *Årets resultat* är positivt för samma vinst.
* Resultatgrupperna följer bara kontots första siffra, utan BAS-kontoplanens undergrupper. Ett resultatkonto med annan första siffra (genom sin typ i `#KTYP`) hamnar under *Övriga resultatkonton*, och ett balanskonto utanför 1xxx–2xxx under *Övriga balanskonton*.
* Huvudboken för ett resultatkonto börjar alltid på noll.

## Nästa steg

* Användaren provar rapporterna på en riktig bokföring.
* Möjliga tillägg, inte beslutade:
  * en period inom året (till exempel ett kvartal eller en månad)
  * jämförelse med förra året
  * export till fil, av rapporter och av en verifikations utskrift
