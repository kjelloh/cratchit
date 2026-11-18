# Beslut

Beslut som gäller i den här sessionen. Målet står i [session.md](session.md) och läget i [status.md](status.md).

## 2026-11-18

* **Anteckningarna i den här sessionen skrivs på svenska** (användarens val). Koden och utvecklardokumentationen är fortfarande på engelska, och cratchit thirds gränssnitt och användarmanual på svenska.
* **Rapporterna kommer från take 3** (plaiground `session/60cae9fe`, commit `04d88d2`, `cratchit/src/core/books/report.*` och `test/test_report.cpp`). Datamodellen (`Books`, `YearBalances`, `balances`) är densamma i third, så beräkningarna följer med oförändrade.
* **Saldobalansen i take 3 delas i två rapporter** (användarens val):
  * **Balansrapport:** balanskontona med IB, förändring och UB, under *Tillgångar* (1xxx) och *Eget kapital och skulder* (2xxx), med summor. Sist *Beräknat resultat* (resultatkontonas summa) och *Summa*, som är noll när bokföringen går ihop.
  * **Resultatrapport:** resultatkontona i grupper efter BAS-kontoklass (3, 4, 5–6, 7, 8) med summor, och *Årets resultat*.
* **Verifikationslistan och huvudboken från take 3 följer också med** (användarens val).
* **Tecken:**
  * Balansrapporten visar beloppen som de är bokförda: debet positivt och kredit negativt.
  * Resultatrapporten visar intäkter positiva och kostnader negativa, så att en vinst är positiv, som i vanliga svenska bokföringsprogram.
  * Båda rapporterna säger under sig hur beloppen ska läsas.
* **Kontona 8990–8999** (där bokslutet bokför årets resultat) räknas inte in i *Årets resultat*. De visas efter det, med det som är *Kvar efter resultatbokningen* (noll när resultatet är bokfört).
* **Balansrapporten säger till när de ingående balanserna inte går ihop**, med summan och den vanligaste orsaken: förra årets resultat är inte bokfört på eget kapital än.
* **Rapporterna nås med `3 Rapporter` i räkenskapsåret** (användarens val), inte i Årsflöde, eftersom rapporter tas fram när som helst och Årsflöde följer årets arbetsordning. Under det: `1 Balansrapport`, `2 Resultatrapport`, `3 Verifikationslista`, `4 Huvudbok`.
* **En rapport visas en sida i taget**, 20 rader, med `<` och `>` som i *Bokfört*.
* **Huvudboken frågar efter kontonumret** (bara siffror) och vägrar ett konto som varken finns i bokföringen eller har rader under året.
* **Rapporterna läser bokföringen när de väljs** och visar det som är bokfört just då. Utkast kommer inte med.
* **Rapportens andra rad är bokföringens namn** (`Project::name`). Organisationsnumret står inte med än.
