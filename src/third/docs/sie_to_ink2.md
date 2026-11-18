# cratchit_ink2

> In cratchit, cratchit_ink2 (session 7d4475fb) runs from the command line as
> `cratchit --sie-to-ink2 ...`, without cratchit's user interface. `cratchit
> --sie-to-ink2 --help` prints this manual. cratchit's own user manual is
> separate.

Makes the Swedish inkomstdeklaration 2 for aktiebolag from a company's books read from an
SIE 4 file: INK2R, the räkenskapsschema (balance sheet and income statement), INK2S, the
skattemässiga justeringar, and INK2 page 1. Last year's return gives the underskott
(deficit) to carry over. The return is written as text with a report, and, when nothing
known is missing, as the SRU files SKV's e-service Filöverföring takes, which are also next
year's "last year's return". `--trace` shows how each amount is made, down to the vouchers.

## Synopsis

    cratchit --sie-to-ink2 [--trace] <file.se> <last year's BLANKETTER.SRU, or the folder of it>
    cratchit --sie-to-ink2 [--trace] <file.se> --previous-deficit <kronor>
    cratchit --sie-to-ink2 --help
    cratchit --sie-to-ink2 -h

The arguments may come in any order. The SIE file is told by its ending (`.se`, `.si` or
`.sie`, in any case), last year's return by `.sru` or by being a folder (then its file
`BLANKETTER.SRU`, in any case, is used). A wrong command line prints the usage and exits
with 2.

## Last year's return

INK2S 4.14a, Outnyttjat underskott från föregående år, is last year's 4.16 Underskott. It
is not in the books, so either last year's return or `--previous-deficit` is needed, never
both:

- **Last year's return** (its `BLANKETTER.SRU`, as filed with SKV): 4.14a is its INK2S 4.16
  (7770), or 0 when it had 4.15 Överskott. It must be the return of the same company (orgnr)
  for the fiscal year that ends the day before this one starts, or it is an error. Also
  taken from it, with notes:
  - the marks for uppdragstagare (8040/8041) and revision (8044/8045), as last year;
  - a check of this year's opening balances (`#IB`) against last year's balansräkning: a
    note for each field that differs.
- **`--previous-deficit <kronor>`**: last year's 4.16 Underskott in whole kronor, 0 or more
  (0 for a company's first year, or after an överskott). For when there is no last year's
  return, or when SKV's decision changed it. Nothing is checked and no marks are given; a
  note says so.

If SKV's decision changed last year's underskott, give that amount with
`--previous-deficit`, or a corrected `BLANKETTER.SRU`.

## The folder it writes

    cratchit --sie-to-ink2 _Resources/_sie/TheITfiedAB20250817_172409.se "_Resources/_ink2/deklaration 2024"

writes, in the working directory, a folder named after the company's orgnr and the fiscal
year, its files named after the folder:

    556782-8172_2024-05-01_2025-04-30/
        556782-8172_2024-05-01_2025-04-30_ink2_fält.txt
        556782-8172_2024-05-01_2025-04-30_ink2_rapport.txt
        BLANKETTER.SRU
        INFO.SRU
        556782-8172_2024-05-01_2025-04-30.se

- `_ink2_fält.txt`: the return as text (see What the return holds), with `TBD [n]` at each
  field the INK2 check finds missing.
- `_ink2_rapport.txt`: the report on it: when and by which version it was made, the SIE
  file, last year's return (or `--previous-deficit`), the mappings, the status (complete,
  not complete, PRELIMINÄR or errors), what became of the SRU files, the errors, the notes
  and the INK2 check (see below).
- `BLANKETTER.SRU`: the blocks INK2, INK2R and INK2S (e.g. `INK2R-2025P1`: the form, the
  year the fiscal year ends and its period, P1 for an end by 30 April, P2 by 30 June, P3 by
  31 August, P4 by 31 December). Each has `#IDENTITET` (the orgnr as SRU writes it, 16 and
  its ten digits, and the date and time of the run), `#NAMN`, the fiscal year (7011, 7012)
  and the fields given, in SKV's order. Amounts in whole kronor, negative with `-`.
- `INFO.SRU`: who files (`#ORGNR`, `#NAMN`, `#POSTNR`, `#POSTORT`). Postnummer and postort
  are taken from the SIE file's `#ADRESS` (e.g. `"17141 Solna"`), else from last year's
  `INFO.SRU` (next to its `BLANKETTER.SRU`), with a note saying which.
- The `.se` file is a copy of the SIE file the return is made from.

Which files a run writes:

| The return is | `_ink2_rapport.txt` | `_ink2_fält.txt` | SRU files |
|---|---|---|---|
| complete | written | written | written |
| not complete (items TBD) | written | written, with TBD [n] | not written |
| PRELIMINÄR (books not closed) | written | written, with TBD [n] | not written |
| with errors (exit 1) | written, with the errors | not written | not written |

- The SRU files are for filing, so they are written only when the return is complete. When
  they are not written, earlier ones in the folder are left, and the terminal and the
  report say they are of an earlier run: do not file them.
- A file not written is left as it was. The SIE copy goes with `_ink2_fält.txt`: it is
  written when missing, or when it differs and `_ink2_fält.txt` is written.
- Every file written replaces the one before: **copy a file before editing it**.
- An SIE file or last year's return that cannot be read writes no folder at all; the errors
  go to the terminal.
- SRU files are ISO 8859-1, as SKV wants. Test them with the test function of SKV's
  Filöverföring before filing them.

The terminal gets a short summary, and the errors:

    Folder .../556782-8172_2024-05-01_2025-04-30
      Wrote 556782-8172_2024-05-01_2025-04-30_ink2_fält.txt: complete
      Wrote 556782-8172_2024-05-01_2025-04-30_ink2_rapport.txt: 6 notes, the INK2 check
      Wrote BLANKETTER.SRU and INFO.SRU (replaced on every run)
      Copied the SIE file as 556782-8172_2024-05-01_2025-04-30.se

Next year, give this folder as last year's return:

    cratchit --sie-to-ink2 <next year's file.se> 556782-8172_2024-05-01_2025-04-30

## What the return holds

`_ink2_fält.txt`, for fiscal year 0 of the SIE file (`#RAR 0`):

1. The head: the company name and orgnr, the fiscal year with the fields 7011 and 7012 (its
   first and last day, as SKV writes them), and the status.
2. INK2 page 1: every field of the form, as SKV's field table gives it, with its number,
   text, SRU code and amount.
3. INK2R: each line of the form in the order of the mapping: its number on the form
   (2.1-2.50 for the balansräkning, 3.1-3.27 for the resultaträkning), its text, its SRU
   code (SKV's fältkod) and its amount in whole kronor. The amount is blank when the books
   have no account with an amount in the field, so `0` means accounts that add up to zero.
   Lines whose amount goes to a plus or a minus field by its sign (e.g. 3.2, 3.12,
   3.26/3.27) show both fields, marked `+` and `-`. Then the sums of the fields as shown:
   tillgångar against eget kapital och skulder, and the fields of the resultaträkning
   against årets resultat (3.26 less 3.27).
4. INK2S: every field of the form, with SKV's `+` (adds to överskott) or `-` (takes from
   it), its amount, and `X` for a mark given.

A field the INK2 check finds missing has `TBD [n]` after its amount; [n] is the item of the
INK2 check that says why and what to provide.

## The INK2 check

At the end of `_ink2_rapport.txt`. What the return needs that the books do not give:

- **Missing** (`TBD [n]` in the return; no SRU files while any is), when the books show it
  applies:
  1. Bokslut: the books are not closed (3.26/3.27 and the fields made from them).
  2. 4.6a Schablonintäkt: periodiseringsfonder (2110-2139) at the start of the year.
  3. 4.3b Nedskrivning av finansiella tillgångar som inte ska dras av: an amount on 3.17.
  4. 4.21 Pensionskostnader and 1.4/1.5 särskild löneskatt: pension costs (7410-7419).
  5. Uppdragstagare and revision (8040/8041, 8044/8045): no marks from last year's return.
  6. INFO.SRU's postnummer and postort: neither in `#ADRESS` nor in last year's INFO.SRU.

  The items are numbered in this order, as they apply.
- **To confirm**: what may apply and the books cannot tell, every year: 4.4b and 4.6e
  (costs and income not in the books), 4.13 (other adjustments, e.g. representation booked
  on deductible accounts), 4.14b-c (reductions of the underskott), 4.20 (loans from
  shareholders); and 4.14a when given by `--previous-deficit`.

Each item says which fields it is about, what the books show, and what to provide.

## How the amounts are made

Two files are built into the program:

- `resources/ink2/bas_2023_to_ink2_2026.csv`: which BAS accounts go to which field, and nothing
  else. Its INK2R part is BAS's table from BAS 2023 to INK2; its INK2S part (4.3a, 4.3c,
  4.5c) is cratchit's, from the accounts BAS 2025 names ej avdragsgill or skattefri. The
  accounts are written as BAS writes them, e.g. `12xx (exkl. 128x)` or `151x–155x, 158x`.
- `resources/ink2/skv_ink2_2026P4.csv`: SKV's fields of INK2, INK2R and INK2S (SKV's field tables
  `INK2*_SKV2002-34-01-26-04.xls`): code, number, text, SKV's `+`/`-`/`*` and whether an
  amount or a mark.

How fields depend on each other is in the program (see INK2S and INK2 page 1).

### INK2R

- Each field is the sum of its accounts at the end of fiscal year 0: the closing balance
  (`#UB`) of balance accounts and the result (`#RES`) of result accounts.
- Signs: tillgångar (72xx) are shown as booked; eget kapital och skulder (73xx) negated, so
  a debt is positive; the plus fields of the resultaträkning (74xx, income) negated and the
  minus fields (75xx, costs) as booked, so both are positive when they are what they say.
  A field may thus be negative, e.g. leverantörsskulder with a debit balance.
- `(Om netto +)` and `(Om netto -)`: the accounts before it go to this field only when they
  together add to (+, a credit) or take from (-, a debit) the year's result; zero counts as +.
- 3.26 Årets resultat, vinst (7450) and 3.27 förlust (7550): BAS's `+ 899x` and `– 899x`.
  899x carries the year's result to the balansräkning, so a debit on 899x is a profit.
- Each field is the exact sum of its accounts rounded to whole kronor, half away from zero.
  The sums of the fields may then differ by a krona or two; this is a note. SKV's field
  table for INK2R has no sum fields and no rule that they add up; its only rule is that
  7450 and 7550 are not both given.
- Accounts outside 1000-8999 (e.g. class 9 for internal accounting) are left out, with a note.

### The books' own SRU codes (#SRU)

An SIE file may give each account the INK2R code its bookkeeping program uses (`#SRU`). The
mapping decides, and the `#SRU` codes are checked against it and fill in where it cannot:

- An account the mapping puts in two fields (BAS lists 40xx-47xx under both 3.5 Råvaror och
  förnödenheter and 3.6 Handelsvaror, by the nature of the goods) goes to the one its `#SRU`
  names, with a note. When its `#SRU` names neither, or it has none, it is an error.
- An account in no field of the mapping goes to the field its `#SRU` names, with a note. When
  that is no field of INK2R (e.g. an older code), it is an error.
- An account whose `#SRU` names another field than the mapping is a note; the mapping is used.
  A `#SRU` naming the other field of a plus/minus pair agrees (e.g. 8999 with 7450 in a year
  with a loss, as `#SRU` has one code per account).
- One note lists the accounts with an amount that have no `#SRU`, when the file gives any.

### INK2S

- 4.1 Årets resultat, vinst (7650) is INK2R 3.26; 4.2 förlust (7750) is 3.27.
- 4.3a Skatt på årets resultat (89xx but 899x), 4.3c Andra bokförda kostnader som inte ska
  dras av (e.g. 6992, 8423) and 4.5c Andra bokförda intäkter som inte ska tas upp (8254,
  8314): their accounts by the mapping, shown positive. These accounts are also in their
  INK2R field.
- 4.14a Outnyttjat underskott från föregående år (7763): from last year (see Last year's
  return), when not 0.
- 4.15 Överskott (7670) or 4.16 Underskott (7770): the fields SKV marks `+` less those it
  marks `-`; 4.15 when that is 0 or more.
- The marks 8040/8041 and 8044/8045: as last year's return gave them.

### INK2 page 1

- 1.1 Överskott av näringsverksamhet (7104) is INK2S 4.15; 1.2 Underskott (7114) is 4.16.

## Books not closed

When the books of fiscal year 0 are not closed (nothing on 8990-8999, and the balance
accounts lack exactly the year's result), the return is a preview that shows the result as
if booked to 2099 against 8999: the status says PRELIMINÄR, the INK2 check has Bokslut as
TBD [1], and no SRU files are written.

## --trace

Prints to the terminal, instead of writing the folder, how each amount of the return is
made, from the vouchers up. It writes no files; the errors and notes go to stderr. It is
long, so page it or save it:

    cratchit --sie-to-ink2 --trace TheITfiedAB20250817_172409.se "deklaration 2024" | less

In the order the return is made:

1. **INK2R from the accounts**, then **INK2S from the accounts** (4.3a, 4.3c, 4.5c): each
   field with an amount:

       INK2R 2.4  7215  Maskiner, inventarier och övriga materiella anläggningstillgångar
         Mapping: 12xx (exkl. 128x); shown as booked
         1221 Inventarier   IB 162495.27   UB 195687.27
             2024-10-22 A 37     Account:NORDEA Text:KORT APPLE COM SE 26        23996.00
         ...
         Sum 65923.96 as booked, 65923.96 shown: 65 924 kr

   - The mapping's accounts, and whether the field shows them as booked or negated.
   - For a field with a condition (`Om netto +`, `+ 899x`, ...): what the accounts of the
     group net to, and so why the amount is in this field.
   - Each account: its opening and closing balance (a balance account) or its result, and
     every voucher of fiscal year 0 on it: date, series and number, text, amount as booked.
     An account placed by its `#SRU` says so. Unclosed books show the simulated closing
     voucher (series `*`).
   - The sum as booked, as shown, and in whole kronor.
2. **INK2S and INK2 page 1 by their rules**: 4.1/4.2 from 3.26/3.27, 4.14a from last year,
   4.15/4.16 as SKV's `+` fields less its `-` fields with each number, 1.1/1.2, and the
   marks.
3. **Accounts with an amount on no field of INK2R**, with their vouchers: those in no field
   (errors) and those left out (class 9).

Amounts in the trace are in kronor and öre, debit positive, as in SIE.

## Errors

- An SIE file that cannot be read, or that is not a valid SIE 4 file: the errors, exit 1,
  and no folder.
- Last year's return that cannot be read, has no INK2S, is of another company, or is not of
  the fiscal year before: the error, exit 1, and no folder.
- An account of class 1-8 with an amount that is in no field of the mapping, or in more than
  one (BAS lists 40xx-47xx under both 3.5 and 3.6), that its `#SRU` cannot place: exit 1;
  only the report is written, with the error (`--trace` shows the account's vouchers).
- A name with a character not in ISO 8859-1, or a file that cannot be written: exit 1.

## Notes

In `_ink2_rapport.txt` (with `--trace`, on stderr):

- `Closing:` closing entries BAS asks for at the balance day that the books lack (e.g. moms
  netting to a receivable, which BAS moves to 1650 Momsfordran); the return shows the books
  as they are.
- `#SRU:` what the books' own SRU codes chose, placed or disagree with, and accounts without one.
- `Opening balance:` a field of this year's opening balances that differs from last year's
  filed INK2R.
- `INK2S:` the marks taken from last year (or that there are none), a negative INK2S field,
  and periodiseringsfonder in the books (4.6a is not computed).
- `INFO.SRU:` where postnummer and postort are from.
- `Rounding:` the sums of the fields differ from rounding each field on its own.
- `Left out`: accounts that are not BAS accounts of class 1-8.

## Not yet included

- A way to give what the INK2 check asks for (e.g. the marks, 4.13) other than the books
  and last year's return.
- INK2S fields that need more than the books' accounts: 4.3b, 4.4, 4.5a-b, 4.6-4.13,
  4.14b-c, 4.17-4.22 (e.g. 4.6a schablonintäkt on periodiseringsfonder, 4.21
  pensionskostnader, 4.20 lån från aktieägare). INK2 page 1 but 1.1 and 1.2.

## Options

- `--trace`: print how each amount is made instead of writing the folder (see --trace).
- `--previous-deficit <kronor>`: last year's 4.16 Underskott, instead of last year's return
  (see Last year's return).
- `--help`, `-h`: print this manual.
