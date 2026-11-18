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
    * `make_report_view` visar en rapport sidvis med `pyroceph::tui::Pager`, som *Bokfört*.
    * `make_ledger_menu` frågar efter kontonumret till huvudboken.

    `ui/fiscal_year_menu.cpp` har fått valet `3 Rapporter`.
  * **Hjälpämnen:** `rapporter`, `rapport` och `huvudbok` (`ui/help_topics.hpp`), med egna stycken i användarmanualen.
  * **Användarmanualen** (`src/third/docs/anvandarmanual.md`) har avsnittet *Rapporter*, med *Läsa en rapport* och *Huvudbok för ett konto*, före *Årsflöde*. Räkenskapsårets avsnitt nämner `3 Rapporter`.
  * **Tester:**
    * `test/test_report.cpp` har 8 tester av rapporternas text: första året, ingående balanser som inte går ihop eller skiljer sig från källan, resultatet bokfört på 8999, struken och tillagd rad, och huvudbok med löpande saldo.
    * `test/test_reports_menu.cpp` har 4 tester av menyerna: valen, rapporternas innehåll, sidvisning med `<` och `>`, och huvudbokens kontonummer.
    * I `test/test_ui.cpp` har räkenskapsårets val fått `3 Rapporter`.
  * **Provat på macOS:**
    * Med legacy (`build/Debug`): `cratchit_tests` 814 tester och `ctest` 1125 tester går igenom.
    * Utan legacy, som CI bygger: `ctest` 982 tester går igenom.
    * I det riktiga programmet, med `HOME` i en tillfällig mapp: en bokföring skapad med *Ny bokföring här > Från SIE* från en liten testfil. Sedan öppnades `2025 > 3 Rapporter` med tangenterna, och balansrapport, resultatrapport, verifikationslista och huvudbok för 1930 gav samma siffror som testerna.
  * **Inte provat:** rapporterna på en riktig bokföring med många konton och verifikationer. Inte heller provat på Windows och Linux; det sker i GitHub Actions vid nästa push.

## Att känna till

* Balansrapportens *Beräknat resultat* är negativt för en vinst, eftersom rapporten visar debet positivt. Resultatrapportens *Årets resultat* är positivt för samma vinst.
* Resultatgrupperna följer bara kontots första siffra, utan BAS-kontoplanens undergrupper. Ett resultatkonto med annan första siffra (genom sin typ i `#KTYP`) hamnar under *Övriga resultatkonton*, och ett balanskonto utanför 1xxx–2xxx under *Övriga balanskonton*.
* Huvudboken för ett resultatkonto börjar alltid på noll.

## Nästa steg

* Användaren provar rapporterna på en riktig bokföring.
* Möjliga tillägg, inte beslutade:
  * en period inom året (till exempel ett kvartal eller en månad)
  * organisationsnumret i rapportens rubrik
  * jämförelse med förra året
  * export till fil
