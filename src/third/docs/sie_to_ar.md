# cratchit_ar

> In cratchit, cratchit_ar (session 5b35b2b9) runs from the command line as
> `cratchit --sie-to-ar ...`, without cratchit's user interface. `cratchit
> --sie-to-ar --help` prints this manual. cratchit's own user manual is
> separate.

Makes the Swedish årsredovisning (annual report) of a company's books, read from an SIE 4 file.

## Synopsis

    cratchit --sie-to-ar [--trace] [--bas-year <year>] <file.se>
    cratchit --sie-to-ar --help
    cratchit --sie-to-ar -h

The options may come in any order. A wrong command line prints the usage and exits
with 2.

## The folder it writes

    cratchit --sie-to-ar _Resources/_sie/TheITfiedAB20250817_172409.se

writes, in the working directory, a folder named after the company's orgnr and the
fiscal year, with three files named after the folder:

    556782-8172_2024-05-01_2025-04-30/
        556782-8172_2024-05-01_2025-04-30_arsredovisning.txt
        556782-8172_2024-05-01_2025-04-30_rapport.txt
        556782-8172_2024-05-01_2025-04-30.se

- `_arsredovisning.txt` is the årsredovisning (see What the årsredovisning holds).
- `_rapport.txt` is the report on it: when and by which version it was made, the SIE
  file and the mapping used, its status (complete, not complete, PRELIMINÄR or errors),
  the errors, the notes (e.g. the year-end closing notes) and the K2 check.
- `.se` is a copy of the SIE file it was made from.

Each run replaces the files with the latest generation, so the folder is always one
copy away from an archive. **The årsredovisning is replaced on every run: copy it before
editing it**, and merge your edits into a new version by hand when the books change.

The terminal gets a short summary of what was written, and the errors:

    Folder .../556782-8172_2024-05-01_2025-04-30
      Wrote 556782-8172_2024-05-01_2025-04-30_arsredovisning.txt: not complete; 9 items TBD
        (replaced on every run: copy it before editing)
      Wrote 556782-8172_2024-05-01_2025-04-30_rapport.txt: 4 notes, the K2 check
      Copied the SIE file as 556782-8172_2024-05-01_2025-04-30.se

- The SIE copy is written when it is missing or differs from the file given. Given the
  copy itself, e.g. when run from the folder, nothing is copied.
- With errors (exit 1) the report is written, with the errors, but not the
  årsredovisning: an earlier one is left as it was, and so is its SIE copy, so that the
  two still belong together.
- An SIE file that cannot be read, or that is not a valid SIE 4 file, writes no folder
  at all; the errors go to the terminal.
- `--trace` writes no files: it prints to the terminal.

## What the årsredovisning holds

A complete årsredovisning for a K2 aktiebolag (BFNAR 2016:10), as plain text, in the
order the rules prescribe (p. 3.2). What the SIE file cannot provide is marked
`TBD [n]`; the K2 check at the end says, for each n, which rule requires it and what
to provide.

1. The front: company name, orgnr, that it is an årsredovisning for fiscal year 0, and
   the sentence on the board, the reporting currency and the unit (p. 3.7-3.9).
2. The förvaltningsberättelse (directors' report), with the headings of p. 4.9:
   - Verksamheten: Allmänt om verksamheten with the säte, and Väsentliga händelser
     under räkenskapsåret (TBD).
   - Flerårsöversikt (multi-year overview) of fiscal year 0 and the year before; the
     two years before those are TBD, and so is the comment when Nettoomsättning varies
     more than 30 %.
   - Förändringar i eget kapital (changes in equity) of fiscal year 0.
   - Resultatdisposition (the disposition of the funds) of fiscal year 0, with the
     board's proposal TBD.
3. The resultaträkning (income statement).
4. The balansräkning (balance sheet), with a Not column naming the note of a post.
5. Noter (notes) of fiscal year 0: Not 1 Redovisningsprinciper (the depreciation
   periods TBD), Not 2 Medelantal anställda when the books show personnel costs, and
   the notes of the posts of the balansräkning that have one in the mapping: for now
   Not 3 Inventarier, verktyg och installationer.
6. Underskrifter: whose årsredovisning for which year, then place, date and signatures
   (TBD).

The K2 check (see below) is in the report.

For the resultaträkning and balansräkning:

- The first column is fiscal year 0 of the SIE file (`#RAR 0`), the second the year
  before (`#RAR -1`), when the file has it.
- Amounts are in whole kronor, with a space between thousands, e.g. `-84 193`.
- A post is shown when any account in it has an amount in either year. Headings and
  sums are shown when a post under them is.


## Flerårsöversikt

Three key figures for fiscal year 0 and the year before, to compare the years at a
glance:

                                                                2022-05-01    2021-05-01
                                                               -2023-04-30   -2022-04-30

    Nettoomsättning (tkr)                                                2           336
    Resultat efter finansiella poster (tkr)                            -84           284
    Soliditet (%)                                                     96,2         100,0

- **Nettoomsättning** (net sales) and **Resultat efter finansiella poster** (result
  after financial items, before year-end appropriations and tax) are the lines of the
  resultaträkning, in thousands of kronor (tkr), rounded half away from zero.
- **Soliditet** (equity ratio) is how much of the assets the owners' capital stands
  for: (Summa eget kapital + 79,4 % of Summa obeskattade reserver) / Summa tillgångar,
  from the balansräkning, in percent with one decimal. The untaxed reserves count with
  79,4 % as 20,6 % of them is deferred tax. A `-` means there are no assets.
- An SIE file holds two years, so the overview shows two. The filed årsredovisningar
  show four.

## Resultatdisposition

The funds at the årsstämma's (annual general meeting's) disposal, and the board's
proposal for them:

    Till årsstämmans förfogande står följande medel
      Balanserat resultat                                          278 845
      Årets resultat                                               -84 193
    Summa                                                          194 653

    Styrelsen föreslår att medlen disponeras enligt följande
      Utdelas till aktieägare                                      TBD [6]
      Balanseras i ny räkning                                      TBD [6]
    Summa                                                          194 653

- **The funds** are the posts under Fritt eget kapital in the balansräkning (e.g.
  Balanserat resultat and Årets resultat), at the end of fiscal year 0. Their Summa
  equals Summa fritt eget kapital.
- **The proposal** (how much to pay out as dividend and how much to carry forward) is
  the board's decision and not in the SIE file, so its amounts are `TBD [n]` (see the
  K2 check). The SIE file only shows the dividend a year later, when the årsstämma's
  decision is booked (see Utdelning in Förändringar i eget kapital).

## Förändringar i eget kapital

The table shows how each part of the company's equity changed during fiscal year 0:
where it started, what happened to it, and where it ended. ÅRL 6 kap. 2 § asks for it
(in the förvaltningsberättelse for a K2 aktiebolag).

### How to read it

An example, for the fiscal year 2022-05-01 - 2023-04-30:

                                                    Balanserat         Årets
                                    Aktiekapital      resultat      resultat        Totalt

    Belopp vid årets ingång              100 800       119 680       284 165       504 645
    Utdelning                                         -125 000                    -125 000
    Balanseras i ny räkning                            284 165      -284 165             0
    Årets resultat                                                   -84 193       -84 193
    Belopp vid årets utgång              100 800       278 845       -84 193       295 453

- **Columns** are the posts under Eget kapital in the balansräkning, so each column
  ends where that post's line in the balansräkning does. Totalt is their sum.
  - **Aktiekapital**: the share capital. It only changes with a new share issue or a
    reduction.
  - **Balanserat resultat**: profits and losses kept from earlier years, together with
    shareholder contributions (2093).
  - **Årets resultat**: the result of a single year. At the start of the year it holds
    last year's result, at the end this year's.
- **Rows** read top to bottom, as the year went:
  1. **Belopp vid årets ingång**: equity at the start of the year. It equals the
     comparison column of the balansräkning (here 504 645).
  2. **Utdelning**: the dividend decided by the årsstämma (annual general meeting) and
     taken from Balanserat resultat (here 125 000).
  3. **Balanseras i ny räkning**: last year's result moved from Årets resultat into
     Balanserat resultat. It is a move within equity, so its Totalt is 0.
  4. **Årets resultat**: this year's result, the same as Årets resultat in the
     resultaträkning (here a loss of 84 193).
  5. **Belopp vid årets utgång**: equity at the end of the year. Each column equals
     its post in the balansräkning, and Totalt equals Summa eget kapital (295 453).
- Every column adds up: the rows from Belopp vid årets ingång down to the last change
  sum to Belopp vid årets utgång. In Balanserat resultat above:
  119 680 - 125 000 + 284 165 = 278 845. Each amount is rounded to kronor on its own,
  so a sum may differ by a krona from the shown amounts.
- A blank cell means the change did not touch that column. A change that is zero in
  every column is left out. For example, a dividend that was booked and then
  cancelled during the year gives no Utdelning row.

### How it is generated

From the SIE file alone, for fiscal year 0:

1. **Columns.** The posts under the heading `Eget kapital` in the mapping (see
   --bas-year), e.g. Aktiekapital (2081, 2083, 2084), Balanserat
   resultat (2090, 2091, 2093-2095, 2098) and Årets resultat (2099). A post becomes a
   column only when its accounts have an amount.
2. **Belopp vid årets ingång** is the opening balance (`#IB 0`) of each column's
   accounts. **Belopp vid årets utgång** is their closing balance (`#UB 0`). Both are
   shown as equity: credit positive.
3. **The changes** are the year's vouchers (`#VER`) that post to any of these accounts.
   The voucher's other accounts decide its row:

   | Row | The voucher's other accounts | Typical voucher |
   |---|---|---|
   | Erhållna aktieägartillskott | not 8999 or 2898, and all its equity amounts are on 2093 | bank 1920 against 2093 |
   | Utdelning | include 2898 Outtagen vinstutdelning | the årsstämma's dividend: 2098 against 2898 and 2091 |
   | Balanseras i ny räkning | none: only equity accounts | last year's result: 2099 against 2098 |
   | Årets resultat | include 8999 Årets resultat | the year-end booking: 2099 against 8999 |
   | Övriga förändringar | anything else | noted in the report with the voucher's series, number, date and text |

   A voucher adds its equity amounts to its row, each in its account's column.
   Amounts struck from a voucher (`#BTRANS`) do not count.
4. **The check.** In every column, Belopp vid årets ingång plus the changes must equal
   Belopp vid årets utgång, to the öre. If not, `cratchit --sie-to-ar` says which column and
   exits with 1. That would mean the SIE file's balances and vouchers do not agree.

To see the vouchers behind each row, run `cratchit --sie-to-ar --trace` (see below). Each
voucher on the equity accounts is in exactly one row.

## The Not column

When there are notes, the balansräkning has a `Not` column with the number of the note
that breaks a post down:

                                                         Not    2023-04-30    2022-04-30
          Inventarier, verktyg och installationer          3        52 321        38 836

The number comes from the note's title in the mapping, `Not <n> <post>`.

## Noter

A note breaks a post of the balansräkning down: how it changed during fiscal year 0,
from what it was at the start to what it is at the end. For now there is one, for the
post Inventarier, verktyg och installationer (equipment, tools and installations).
The note numbers are those of the filed årsredovisningar: Not 1 and Not 2 are text
notes not yet included.

### How to read it

An example, for the fiscal year ending 2023-04-30:

    Not 3 Inventarier, verktyg och installationer               2023-04-30

    Anskaffningsvärden
      Ingående anskaffningsvärden                                  220 215
      Inköp                                                         84 382
      Utgående anskaffningsvärden                                  304 597

    Avskrivningar
      Ingående avskrivningar                                      -165 970
      Årets avskrivningar                                          -70 897
      Utgående avskrivningar                                      -236 868

    Nedskrivningar
      Ingående nedskrivningar                                      -15 408
      Utgående nedskrivningar                                      -15 408

    Redovisat värde                                                 52 321

- **Anskaffningsvärden** (cost): what the company paid for the equipment it holds.
  It grows with **Inköp** (purchases, net of returns and credit notes) and shrinks
  with **Försäljningar och utrangeringar** (sold or scrapped equipment).
- **Avskrivningar** (accumulated depreciation, negative): the cost written off so far.
  It grows with **Årets avskrivningar** (this year's depreciation, the same amount as
  in the resultaträkning) and shrinks with **Återförda avskrivningar** (the
  depreciation of sold or scrapped equipment, taken out with it).
- **Nedskrivningar** (accumulated write-downs, negative): value written off beyond
  the plan, with **Årets nedskrivningar** and **Återförda nedskrivningar** the same way.
- Each section reads from its opening (Ingående) through the year's changes to its
  closing (Utgående) amount: here 220 215 + 84 382 = 304 597.
- **Redovisat värde** (book value) is the three closing amounts added up, and equals
  the post in the balansräkning (52 321).
- A change that is zero is left out. Each amount is rounded to kronor on its own, so
  a sum may differ by a krona from the shown amounts.

### How it is generated

From the SIE file alone, for fiscal year 0:

1. **The note and its sections are rows in the mapping** (see --bas-year), under the
   statement heading `Noter`. In `resources/ar/bas_to_ar_2026.csv`:

       1;Rubrik;;Noter
       2;Rubrik;;Not 3 Inventarier, verktyg och installationer
       3;Post;1220-1227, 1250-1257 eller 1260-1267;Anskaffningsvärden
       3;Post;1229, 1259 eller 1269;Avskrivningar
       3;Post;1228, 1258 eller 1268;Nedskrivningar
       2;Summa;;Redovisat värde

   `resources/ar/bas_to_ar_2024.csv` has the same rows over the accounts of its post
   (12x0-12x7, 12x8 and 12x9 of 1220-1279, but 1260).

   The split follows BAS: 12x0-12x7 hold the cost, 12x8 the accumulated write-downs
   and 12x9 the accumulated depreciation. The sections together hold exactly the
   accounts of the post in the balansräkning. The title `Not <n> <post>` names that post.
2. **Ingående** and **Utgående** are the opening (`#IB 0`) and closing (`#UB 0`)
   balances of the section's accounts, as booked: debit positive.
3. **The changes** are the year's vouchers that post to the section's accounts. A
   voucher is a sale or scrapping (Försäljningar och utrangeringar, Återförda
   avskrivningar or Återförda nedskrivningar) when it also posts to another section of
   the same note, or to 3970-3979 or 7970-7979 (gain or loss on disposal). Any other
   voucher is Inköp, Årets avskrivningar or Årets nedskrivningar.
4. **The checks.** Each section's opening balance and changes must add up to its
   closing balance, and Redovisat värde must equal the post in the balansräkning, to
   the öre. If not, `cratchit --sie-to-ar` says which and exits with 1.
5. A note is shown when any account in it has an amount in either year.

## --trace

Prints to the terminal, instead of writing the folder, what each amount of the
årsredovisning is made of: the BAS accounts and the vouchers behind it. Use it to check
an amount against the books, or to find out why a post holds what it does. It writes
no files; the errors and notes go to stderr. It is long (several hundred lines for a
small company), so page it or save it:

    cratchit --sie-to-ar --trace TheITfiedAB20230503_152345.se | less
    cratchit --sie-to-ar --trace TheITfiedAB20230503_152345.se > trace.txt

### How to read it

The trace follows the order of the report: Flerårsöversikt and Resultatdisposition
(with the lines of the statements they are computed from), Förändringar i eget
kapital, the resultaträkning, the balansräkning and the notes. Each shown amount is a
heading line, in whole kronor as on the årsredovisning, with what makes it up
indented below it in kronor and öre **as booked: debit positive, credit negative**.

In the resultaträkning and balansräkning, each post lists its accounts, and each
account its vouchers in fiscal year 0, by date:

    Redovisningsmedel                                               157 kr        161 kr
      1999 OBS                                                      156.75        161.25
        Ingående balans 2022-05-01                                  161.25
        A 68 2022-11-23 Reglera OBS-konto                          -161.25
        I 5 2023-04-30 Ej rapporterad Moms till OB-konto            156.75

- The post line has one column per year, like the report.
- The account line (number and `#KONTO` name) has the account's closing balance
  (`#UB`) for a balance account (class 1 and 2) or its result (`#RES`) for a result
  account, per year.
- Below it, for fiscal year 0: `Ingående balans` (`#IB 0`) for a balance account, then
  every voucher on the account, as series, number, date and text, with the amount
  booked on the account. The opening balance and the vouchers add up to the account's
  year 0 amount: here 161.25 - 161.25 + 156.75 = 156.75.
- The year before can only be traced to the accounts: the SIE file has its balances
  but not its vouchers.
- The post's amount is its accounts added up, with the sign the årsredovisning shows:
  class 1 as booked, all others negated (so a credit income of -636.05 is 636 kr of
  income).
- An account is listed when it has an amount in either year, an opening balance or a
  voucher, so an account that ends the year at 0 still shows how it got there.
- Sums are not traced again: they add up the posts above them.

In Förändringar i eget kapital, the first and last row list the equity accounts'
opening and closing balances. A change row lists its vouchers, each with its
transactions on the equity accounts:

    Utdelning                                                  -125 000 kr
      I 2 2022-11-24 Årsstämma resultatfördelning
        2098 Vinst eller förlust från föregående år              284164.89
        2091 Balanserad vinst eller förlust                     -159164.89

Here the dividend of 125 000 is the net of the two: 284 164.89 - 159 164.89 taken
from Balanserat resultat. The voucher's other transaction (2898, the dividend owed)
is not listed, as it is not equity.

In the notes, each section lists its rows the same way: the accounts' balances for
Ingående and Utgående, the vouchers for each change. A voucher's transactions show
the account with the transaction's own text when it has one, else the account's name,
so the depreciation of each item is visible:

      Årets avskrivningar                                       -70 897 kr
        I 6 2023-04-30 Planenliga Avskrivningar för 20220501
        - 20230430
          1229 Mac Pro 2013 (5 år)                                -4986.00

## --bas-year

    cratchit --sie-to-ar --bas-year 2026 TheITfiedAB20250817_172409.se

Says which BAS chart of accounts (kontoplan) the books follow, so that the accounts
go on the right lines of the årsredovisning. The default is **2024**. The SIE file
cannot tell: its `#KPTYP` names the kind of chart, and a program such as Fortnox
writes a generic `EUBAS97` whatever the year.

`cratchit --sie-to-ar` uses the newest built-in mapping that is not newer than the year:

| --bas-year | Mapping |
|---|---|
| 2024, 2025 (and the default) | `resources/ar/bas_to_ar_2024.csv` |
| 2026 and later | `resources/ar/bas_to_ar_2026.csv` |
| before 2024 | none: an error, exit 2 |

The mapping used is named in the report, in its K2 check and at the top of the
trace, e.g. `resources/ar/bas_to_ar_2024.csv (BAS 2024-2025; books following BAS 2024,
the default; see --bas-year)`.

The two mappings differ only where BAS 2026 changed the chart:

| Accounts | BAS 2024-2025 | BAS 2026 |
|---|---|---|
| 4000-4299 | Råvaror och förnödenheter | Handelsvaror (groups 40-42) |
| 4300-4899 | 4300-4799 Råvaror och förnödenheter, 48xx on no line | Råvaror och förnödenheter (groups 43-48) |
| 4930-4931 | Råvaror och förnödenheter | Förändring av lager ... |
| 1090-1099 | on no line | Övriga immateriella anläggningstillgångar |
| 1230-1249 | Inventarier, verktyg och installationer | Maskiner och andra tekniska anläggningar |
| 1260-1269 | 1261-1269 Inventarier, verktyg och installationer | Inventarier, verktyg och installationer |
| 1270-1279 | Inventarier, verktyg och installationer | on no line (gone from BAS) |
| 8990-8998 | on no line | outside the årsredovisning |

Books on the wrong year can put amounts on the wrong lines without any error, e.g.
purchases of handelsvaror on 4010 as Råvaror och förnödenheter. Check the mapping
named in the output.

## --help, -h

Prints this manual. Long output can be paged: `cratchit --sie-to-ar --help | less`.

## How the amounts are made

- **Source:** the closing balance (`#UB`) of balance accounts and the result (`#RES`)
  of result accounts, for each year.
- **Mapping:** the files `resources/ar/bas_to_ar_2024.csv` (the default) and
  `resources/ar/bas_to_ar_2026.csv` are built into the program; --bas-year chooses one.
  Each lists every line of the årsredovisning in order, with the columns
  `Nivå;Typ;BAS Range;ÅR`:
  - `Nivå`: the level, 1 for the statements themselves and deeper for their parts.
  - `Typ`: one of
    - `Rubrik`: a heading.
    - `Post`: a post with BAS accounts.
    - `Summa`: adds up the posts after the last `Rubrik` at its level or above.
    - `Utanför`: accounts that are not on the årsredovisning, e.g. 8990-8999 and class 9.
  - `BAS Range`: four digit accounts and ranges separated by `,`, `eller` or `och`, e.g.
    `1590-1619, 1630-1659 eller 1680-1689`. `(förutom ...)` excludes accounts, e.g.
    `4900-4999 (förutom 4910-4929 och 4980-4989)`.
  - `ÅR`: the text on the årsredovisning.
- **Where the mappings come from:** `resources/ar/bas_to_ar_2024.csv` has the BAS ranges
  of [BAS-kontoplan och din årsredovisning](https://www.arsredovisning-online.se/bas-kontoplan/)
  (unchanged since 2023-11-30). `resources/ar/bas_to_ar_2026.csv` has the same ranges
  corrected to the BAS 2026 chart of accounts:
  - Handelsvaror is 4000-4299 (groups 40-42), Råvaror och förnödenheter 4300-4899
    (groups 43-48) and 4910-4929.
  - The post Övriga immateriella anläggningstillgångar (1090-1099, e.g. tomträtter).
  - Maskiner och andra tekniska anläggningar is 1210-1219 and 1230-1249, Inventarier,
    verktyg och installationer 1220-1229 and 1250-1269 (127x is no longer in BAS).
  - 8990-8999 (the group Resultat) is outside the årsredovisning.

  In both, 8710 and 8750 (extraordinary items, which K2 does not have) are in Övriga
  rörelseintäkter and Övriga rörelsekostnader, 2099 is Årets resultat in the
  balansräkning, and the Noter rows are added. The post names follow K2's forms (p.
  3.5, 4.7): Färdiga varor (1450-1459) and Handelsvaror (1460-1469) are two posts,
  Kontokredit is the credit line, and Tecknat men ej inbetalt kapital is spelled as
  in K2. Every BAS 2026 account that a K2 aktiebolag may use is on a line; those
  marked "Ej K2" and the equity accounts of other company forms (2010-2072) are not.
- **Signs:** assets (class 1) are shown as booked. Everything else is negated, so
  income, equity and debt are positive and costs negative.
- **One line per account:** each account is on one line of the resultaträkning and
  balansräkning whatever its balance. Moving a receivable or a debt to its proper
  account is a closing entry in the books (see Year-end closing notes). Under the
  statement heading `Noter`, an account may also be on one line of a note, which
  breaks its post down (see Noter).
- **Rounding:** each line is rounded to whole kronor from its exact amount, half away
  from zero. A sum may therefore differ by a krona from the shown posts it adds.

## Errors and checks

`cratchit --sie-to-ar` exits with 1 and says why, on the terminal and in the report, when:

- The SIE file cannot be read or is not a valid SIE 4 file.
- An account has an amount but is on no line of the resultaträkning and
  balansräkning, or on more than one, or on more than one line of the notes.
- The balansräkning does not balance (Summa tillgångar differs from Summa eget
  kapital och skulder), to the öre, other than by a year's result that is not yet
  booked (see Books not yet closed).
- Årets resultat differs between resultaträkning and balansräkning, to the öre, other
  than for the same reason.
- In Förändringar i eget kapital, a column's opening balance and changes do not add
  up to its closing balance, to the öre.
- In a note, a section's opening balance and changes do not add up to its closing
  balance, Redovisat värde differs from its post in the balansräkning, or a section
  under `Noter` is not Anskaffningsvärden, Avskrivningar or Nedskrivningar.

A wrong command line prints the usage and exits with 2.

## Books not yet closed

`cratchit --sie-to-ar` can be run on books whose fiscal year is not yet closed, to see where
they stand and what the closing still needs. It takes the books as not closed when
nothing is booked to 8990-8999 in fiscal year 0 and the balansräkning is off by
exactly the year's result, as that is not booked to 2099. Then:

- A note in the report says so, instead of the errors for the balansräkning and Årets
  resultat:

      Closing: the books for 2025-05-01 - 2026-04-30 are not closed: the year's result
      -24989.49 is not booked to 2099 (nothing on 8999). The årsredovisning is a preview that
      shows it as booked (see --trace).

- The årsredovisning is made as if the year's result were booked: `cratchit --sie-to-ar` adds
  the closing entry Fortnox makes, 2099 against 8999 on the balance day, so that the
  balansräkning balances and Förändringar i eget kapital and the Resultatdisposition
  show the year's result. It is not a voucher of the books: `--trace` shows it as
  `* 2026-04-30 Ej bokfört: Årets resultat (simulerat av cratchit)`.
- The årsredovisning starts with
  `PRELIMINÄR: bokslutet är inte gjort, årets resultat är inte bokfört (TBD [1]).`, and
  the K2 check has the missing item Bokslut (BFL 6 kap. 1 §: the bookkeeping is closed
  with the årsredovisning).
- The exit code is 0. Any other difference is still an error.

The year-end closing notes below say what else the closing may need.

## Year-end closing notes

The årsredovisning shows the books as they are. For each year shown, `cratchit --sie-to-ar`
also notes in the report, as `Closing: ...`, the closing entries that BAS asks for at
the balance day and that the books lack. Notes do not change the exit code.

- `Moms (2610-2659) nets to a receivable`: the VAT not yet due is a receivable;
  book it to 1650 Momsfordran.
- `Skatteskulder (2500-2599) net to a receivable`: book it to 1640 Skattefordringar.
- `1630 Skattekonto has a credit balance`: book it to 2850, so that a debt is not
  netted against the assets.
- `Redovisningsmedel (1990-1999) ... is not matched by Avräkning för annans räkning
  (2830-2839)`: BAS 1990 is money held for others, booked against a debt on 2830. It is
  not a suspense account.
- `2999 OBS-konto is ...`: the suspense account must be zero at year end.
- `No depreciation (7800-7899) in the year ending ...`: an account of accumulated
  depreciation of anläggningstillgångar (10x9, 11x9, 12x9) has a balance, but the year
  has no depreciation. Depreciation (78xx against 12x9) is a closing entry.
- `2098 Vinst eller förlust från föregående år is ...`: last year's result is still on
  2098, so the årsstämma's decision on it (e.g. to 2091 Balanserat resultat, or a
  dividend) is not booked.

For example:

    Closing: Moms (2610-2659) nets to a receivable 1331.46 at 2023-04-30; BAS: move it to 1650 Momsfordran

## K2 check

The report ends with a check of the årsredovisning against the requirements of
BFNAR 2016:10, as in BFN:s vägledning Årsredovisning i mindre företag (K2), updated
2025-06-16. The terminal summary gives the number of missing items.

    K2 check
    ========
    ...
    Not complete: 9 items are missing, marked TBD [n] in the årsredovisning. Each breaks
    the rule given until it is provided.

    [1] Säte - BFNAR 2016:10 p. 5.2; ÅRL 2 kap. 5 §
        Provide: the municipality where the company has its registered office (säte).
        Template heading: ## Säte
    ...
    [5] Kommentar till nettoomsättningens förändring - BFNAR 2016:10 p. 5.5
        Found: Nettoomsättning went from 335825 kr to 1527 kr.
        Provide: why the Nettoomsättning varies more than 30 % between the years.
        Template heading: ## Kommentar till flerårsöversikten

- **Missing items** are required now. Each is `TBD [n]` in the årsredovisning, with the
  rule that requires it, what was found in the books (`Found`), what to provide and
  the heading it will have in the input template (not yet written by `cratchit --sie-to-ar`).
  - First, when the books are not closed: Bokslut (see Books not yet closed).
  - Always, as the SIE file cannot hold them: säte, Allmänt om verksamheten, Väsentliga
    händelser under räkenskapsåret, the Flerårsöversikt's two earlier years, the
    board's proposal, the datering and the underskrifter.
  - When the books show it applies: the comment on Nettoomsättning (it varies more
    than 30 % between the two years in the file), the depreciation periods (the
    resultaträkning has Av- och nedskrivningar or a post has a note), Medelantal
    anställda (the resultaträkning has Personalkostnader), and a note for each shown
    post under Immateriella or Materiella anläggningstillgångar that has none.
- **Items to confirm** are required only if something applies that the SIE file does
  not tell (`When`): the board's statement on a proposed dividend (ABL 18 kap. 4 §),
  ställda säkerheter, eventualförpliktelser, loans to board members and events after
  the balance day; and the items above that the books do not show as applying.
- An årsredovisning is complete when no item is missing: `Complete: nothing is
  missing.` The exit code does not change with missing items.

## Not yet included

The company's own information, marked TBD [n]: `cratchit --sie-to-ar` does not yet write or
read the input template that will provide it. Also: the Fastställelseintyg and anything
else Bolagsverket needs for filing, which BFN's vägledning does not cover.
