# Issue: What is missing to close branch origin/claude-001-refactor-csv-import-pipeline?

## Abstract

I want to merge origin/claude-001-refactor-csv-import-pipeline back to main branch. What is left to do?

## Potential Issues

* SKV csv -> tas FAILS (See Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv)

  - The Issue is that SKV csv-file populates columns with different data for header, saldo and transaction rows
  - Thus 'detect_columns_from_data' function fails to find a consistent mapping.

* A sample SKV csv file may look like this.

```sh
"THE ITFIED AB";"556782-8172";"";""
"";"";"";""
"";"Ingående saldo 2025-10-01";"";"660"
"2025-10-04";"Intäktsränta";"1";"661"
"2025-11-12";"Moms juli 2025 - sept 2025";"1 997";"2 658"
"2025-11-14";"Utbetalning";"-1 997";"661"
"2025-12-06";"Intäktsränta";"1";"662"
"";"Utgående saldo 2025-12-31";"";"662"
```

* A sample NORDEA csv file looks like this.

```sh
Bokföringsdag;Belopp;Avsändare;Mottagare;Namn;Ytterligare detaljer;Meddelande;Egna anteckningar;Saldo;Valuta;
2025/12/29;-1158,75;;5343-2795;LOOPIA AB (WEBSUPPORT);LOOPIA AB (WEBSUPPORT);62500016901740;Webhotell Q1;9911,33;SEK;
2025/12/19;-200,57;;;KORT             CLAUDE.AI SUBSC 26;KORT             CLAUDE.AI SUBSC 26;CLAUDE.AI SUBSC 2656;;11070,08;SEK;
2025/12/17;-2359,00;;824-3040;TELIA SVERIGE AB;TELIA SVERIGE AB;20578563254;Mobil + Data Q4;11270,65;SEK;
2025/12/03;-1,85;;;AVGIFTER NORDEA;AVGIFTER NORDEA;;;13629,65;SEK;
2025/12/02;10000,00;32592317244;;Insättning;Insättning;FRÅN SPAR;;13631,50;SEK;
2025/11/27;-615,00;;5020-7042;Fortnox Finans AB;Fortnox Finans AB;575901892218359;Fortnox Dec;3631,50;SEK;
2025/11/19;-198,08;;;KORT             CLAUDE.AI SUBSC 26;KORT             CLAUDE.AI SUBSC 26;CLAUDE.AI SUBSC 2656;;4246,50;SEK;
2025/11/18;1997,00;;;BG KONTOINS;BG KONTOINS;5050-1030 SK5567828172;;4444,58;SEK;
2025/11/05;-1,85;;;AVGIFTER NORDEA;AVGIFTER NORDEA;;;2447,58;SEK;
2025/10/20;-323,75;;5343-2795;LOOPIA AB (WEBSUPPORT);LOOPIA AB (WEBSUPPORT);62500012712646;sharedplanetcitizen org;2449,43;SEK;
2025/10/03;-3,70;;;AVGIFTER NORDEA;AVGIFTER NORDEA;;;2773,18;SEK;
```