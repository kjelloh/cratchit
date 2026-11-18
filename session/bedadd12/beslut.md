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
* **En rapport visas hel**, alla rader på en gång, utan sidvisning och bläddring. Terminalen skrollar, så det finns gott om plats (användarens val efter att ha provat). Det ersätter det första beslutet om 20 rader i taget med `<` och `>`.
* **En rapport och en utskrift har en tom rad före och efter sig** på skärmen, för luft (användarens val, förslag A av tre: tom rad, linje, eller båda). Det gäller alla dokument som visas hela: de fyra rapporterna och verifikationens utskrift.
* **Huvudboken frågar efter kontonumret** (bara siffror) och vägrar ett konto som varken finns i bokföringen eller har rader under året.
* **Huvudboken utan kontonummer visar alla konton** som har rader eller ingående balans under året, i nummerordning, ett avsnitt per konto (användarens önskan). Frågan om kontonummer är därför ett textfält, så att Enter på en tom rad når den i stället för att visa valen igen.
* **`3 Utskrift` i en bokförd verifikation** visar hela verifikationen som text på skärmen (användarens val: valet, att bara visa och inte spara, och bara bokförda verifikationer till att börja med). Den har det bokföringslagen kräver av en verifikation (5 kap. 7 §): id, företagets namn och organisationsnummer, räkenskapsåret, serien med namn, datum, text, belopp, registrerad och signatur, och raderna i kolumnerna Debet och Kredit med kontonamn, radens egen text och datum, märkning av strukna och tillagda rader, och summor. Den visas hel, som rapporterna.
* **Rapporterna läser bokföringen när de väljs** och visar det som är bokfört just då. Utkast kommer inte med.
* **Rapportens andra rad är bokföringens namn och organisationsnummer**, till exempel `Test AB (556000-0001)`, som i *Bekräfta* när en bokföring skapas från SIE (användarens önskan). Numret är `orgnr` i bokföringens `project.txt`. Saknas det står bara namnet.
