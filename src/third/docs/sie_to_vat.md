# cratchit --sie-to-vat

> In cratchit, the VAT return (momsrapport) of take 3 (session 60cae9fe) runs from
> the command line as `cratchit --sie-to-vat ...`, without cratchit's user
> interface. `cratchit --sie-to-vat --help` prints this manual. cratchit's own user
> manual is separate. The terminal output is in English; the files written are in
> Swedish, as they are for the company's records and for Skatteverket.

Makes the Swedish VAT return (momsdeklaration) of a calendar quarter from a
company's books read from SIE 4 files: the boxes of the form, the checks of the
books behind them, the voucher that clears the VAT, and the eSKD file that
Skatteverket's e-service *Lämna momsdeklaration* takes.

## Synopsis

    cratchit --sie-to-vat <file.se> [<last year's file.se>]
    cratchit --sie-to-vat <file.se> [<last year's file.se>] --period <YYYYQn>
    cratchit --sie-to-vat --help
    cratchit --sie-to-vat -h

The arguments may come in any order. A wrong command line prints the usage and
exits with 2.

## Last year's file

An SIE file holds the vouchers of its fiscal year 0 only. With a broken fiscal
year (e.g. May - April) a quarter can start in the fiscal year before: April -
June lies in two fiscal years. Give last year's SIE file too, and the quarter
counts the vouchers of both. The files are read as an SIE import into cratchit
would read them, the older year first; the years must follow each other.

## Listing the quarters

    cratchit --sie-to-vat TheITfiedAB20261019_105108.se

lists the calendar quarters that end in the fiscal year of the (latest) file,
newest first, each with its VAT in three steps:

    VAT quarters ending in 2026-05-01 - 2027-04-30 of The ITfied AB, 556782-8172 (today 2026-11-17):
                    Period     Cleared     Charged
      2026 Q4        -25.00        0.00        0.00  so far
      2026 Q3      -1076.92        0.00        0.00
      2026 Q2      -2292.07    -2292.00    -2292.00  cleared by M1, charged by A9

| Step    | What it is |
|---------|------------|
| Period  | The VAT the quarter's vouchers post on the VAT accounts (boxes 10-12, 30-32, 60-62 and 48). Clearing vouchers are left out. |
| Cleared | What clearing vouchers dated in the quarter move from the VAT accounts to 2650 (Redovisningskonto för moms) or 1650 (Momsfordran). A clearing voucher is any voucher with rows on 2650 or 1650 and on a VAT account, whatever its series. |
| Charged | What Skatteverket charged or credited the tax account (1630) for the quarter: a voucher with rows on 2650 or 1650 and on 1630, and none on a VAT account. A charge is matched to the oldest quarter cleared by the same amount that has no charge yet. |

The figures keep the sign of the VAT accounts: VAT to pay is negative (credit),
VAT to get back positive (debit). Cleared and Charged are in whole kronor. A
quarter running today is marked `so far`; quarters not yet started are left out.
A quarter that starts before the first file's fiscal year says to give last
year's file too.

## Writing a quarter

    cratchit --sie-to-vat TheITfiedAB20261019_105108.se --period 2026Q3

writes, in the working directory, the folder `<orgnr>_moms_<first day>_<last day>`:

    556782-8172_moms_2026-07-01_2026-09-30/
        556782-8172_moms_2026-07-01_2026-09-30_momsrapport.txt
        556782-8172_moms_2026-07-01_2026-09-30_verifikation.txt
        moms_202609.eskd
        556782-8172_moms_2026-07-01_2026-09-30.se

- `_momsrapport.txt` is the VAT return: each box with postings, in the form's
  order and under the form's sections, with its amount and its title on the
  form (from `momsrapport.csv`), the accounts behind it with their sums and the
  vouchers with their texts; box 49; the three steps; and the checks (below).
- `_verifikation.txt` is the voucher that clears the quarter's VAT, to book in
  series M: each VAT account gets minus what the quarter posted on it, box 49
  goes to 2650 (to pay) or 1650 (to get back), and 3740 (Öres- och
  kronutjämning) takes the öre. It is not booked: book it yourself. When the
  books already have the quarter's clearing voucher, the file says which.
- `moms_<YYYYMM>.eskd` is the eSKD file (eSKDUpload 6.0) with the organisation
  number, the period as the year and the quarter's last month, each box with
  postings under its tag (from `momsrapport_box_to_eskd.csv`) in whole kronor,
  and `MomsBetala` (box 49).
- `.se` is a copy of the (latest) SIE file.

Each run replaces the files. The return is made from the quarter's vouchers
without its VAT clearing voucher, so a quarter already cleared in the books
gives the return that was filed for it. For a quarter not yet ended only the
report is written.

## Checks

The report ends with checks of the books behind the return. They only warn.

| Check | What it finds |
|-------|---------------|
| Not fully booked | Drafts dated in the quarter (in an SIE file there are none). |
| 2650 and 1650 | A voucher posting 2650 or 1650 with rows on other accounts than VAT accounts, 1630, 3740 and memo accounts (9000-9999). Only a clearing or a charge voucher posts them. |
| Reverse charge and import | Per voucher: a purchase base (boxes 20-24, or 50) without output VAT (30-32, or 60-62), or the other way round; the output VAT must be the base times the box's rate, rounded towards zero to whole öre; box 48 must deduct it in full. |
| Registered after the clearing | A voucher dated in the quarter, registered after its clearing voucher: the return made then lacks it. |
| VAT accounts outside the return | Rows on accounts 2600-2699 (2650 aside) that report in no box. |

## Exit codes

| Code | Meaning |
|------|---------|
| 0    | Success (checks only warn). |
| 1    | An SIE file could not be read or imported, the quarter is not in the files' years, starts before them (give last year's file) or has not started, or a file could not be written. |
| 2    | Invalid command line arguments. |

## Not yet included

Monthly and yearly VAT periods, and booking the clearing voucher (book it in
cratchit or in another program).
