# Thinking out loud on cracthit development

I find thinking out loud by writing to be a valuable tool to stay focused and arrive faster at viable solutions.

## 20260219

I have now brute-forced the generic accounty statement projection code to statement_table_meta TU. It is NOT pritty. But at least it is now in its own TU for access by account::statement::monadic::to_statement_id_ed_step?

I have now made to_statement_id_ed_step call generic_like_to_statement_table_meta ok! Now some tests fail:

```sh
[  PASSED  ] 355 tests.
[  FAILED  ] 6 tests, listed below:
[  FAILED  ] AccountIdTests.UnknownCsvReturnsNullopt
[  FAILED  ] FullPipelineTestFixture.ImportSimpleCsvFailsForUnknownFormat
[  FAILED  ] FullPipelineTableTests.ImportTableFailsForUnknownFormat
[  FAILED  ] MDTableToAccountStatementTestFixture.NordeaMDTableToAccountStatement
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_csv_20251120_BOM_ed_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_BOM_ed_Ok
```

But I think I can be confident this is fixable?

Side note: Handle the BOM in to_with_detected_encoding_step?

* We need to mutate the buffer but it is worth the cost?
* The function can detect BOM and confirm the encoding matches?



## 20260218

I have now made ALL test pass.

```sh
[==========] 364 tests from 67 test suites ran. (501 ms total)
[  PASSED  ] 364 tests.
All tests PASSED
```

But I am still far from finished.

* I have an unknown amount of now redundant code
  - The whole old to_column_mapping is now obsolete?
* I have yet to move the account statement mapping to the to_statement_id_ed_step.
* I still feel the test coverage is not good enough
  - I lack test of the actual produced members of 'statement mapping'
* I am still unsure if BOM is removed
  - And if not, why it does not show up in the log output?

Maybe I should begin with detecting any BOM remaining in the table passed to statement_id_ed_to_account_statement_step?

So how can I proceed?

WAIT! All my new code is still NOT USED by the maybe pipeline! DARN! I am really struggling with this task??!

So what is actually happening in the pipe line as of now?

```c++
  // ...
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
  // ...
```

My GOD! It is still a mess!!

Maybe I should tread carefully and take a small chew of the elephant?

* Remove the fallback to 'to_column_map' in the new code?
* Try to move the 'new' code to the meta unit?
  - Move generic_like_to_account_id
  - Move generic_like_to_column_mapping
  - Move generic_like_to_statement_mapping
* But first add better test covarge of the new code?

Yes, I should take the opportunity to now make the tests have better coverage to be better prepared to move and use the new code as I intended.

You know what? Why are the 'generic_line_xxx' now returning optionals?

* generic_like_to_statement_mapping -> Optional OK
* generic_like_to_column_mapping -> Optional OK
* generic_like_to_account_id -> Optional OK

AHA! So it is these three I should enhance the test covarge on?!

I have now added test of members of 'Statement Mapping' produced by generic_like_to_statement_mapping for all test csv input

```sh
[==========] 361 tests from 67 test suites ran. (500 ms total)
[  PASSED  ] 361 tests.
All tests PASSED
```

I could not help myself and added a chekc for 'no BOM' in some parsed csv tables. I now have:

```sh
[  PASSED  ] 359 tests.
[  FAILED  ] 2 tests, listed below:
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_csv_20251120_BOM_ed_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_BOM_ed_Ok
```

So, the BOM is NOT removed!

I now made generic_like_to_column_mapping handle anonymous but ok statement table and removed fallback to 'older' to_column_mapping in generic_like_to_statement_table_meta (All tests still PASS ok).

## 20260217

I now have managed to brute force an algorithm that can parse both NORDEA and SKV current and older to a usable column map to extract statement entries. I am still baffelled how complicated and fiddly this still is? It feels I am missing something?

Never the less, I now aim and just push forward and see if I can see a pattern in the mess once it works.

I now want to move the types I used for the in- and out-saldo inference to live in the statement_table_meta TU.

```c++
  struct FoundSaldo {
    FoundSaldo(std::ptrdiff_t rix,Date date,Amount ta);
    using Value = std::pair<std::ptrdiff_t,TaggedAmount>;
    Value m_value;
  }; // FoundSaldo
  FoundSaldo::FoundSaldo(std::ptrdiff_t rix,Date date,Amount ta)
    : m_value(rix,TaggedAmount(date,to_cents_amount(ta))) {}

  struct FoundSaldos {
    FoundSaldo m_in_saldo;
    FoundSaldo m_out_saldo;
  }; // FoundSaldos
```

* But then statement_table_meta must know about TaggedAmount
* And when I include TaggedAmountFramework.hpp I get a circualr dependancy of header files!

I want to break this loop. What do I need to do?

```sh
[  1%] Building CXX object CMakeFiles/cratchit.dir/src/csv/statement_table_meta.cpp.o
In file included from /Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/csv/statement_table_meta.cpp:1:
In file included from /Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/csv/statement_table_meta.hpp:3:
In file included from /Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/fiscal/amount/TaggedAmountFramework.hpp:7:
In file included from /Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/csv/parse_csv.hpp:4:
In file included from /Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/csv/projections.hpp:4:
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/fiscal/amount/AccountStatement.hpp:34:19: error: use of undeclared identifier 'account'
   34 | using AccountID = account::statement::AccountID;
    ...
```

* TaggedAmountFramework.hpp includes parse_csv.hpp (Seems the wrong way around)?
* Or should I make an atomic TaggedAmount TU for the raw type?

Yes, I made TaggedAmount a separate unit to ioslarte type spcific functionality ok.

Ok, but the friction just refuses to go away! Now I have the delicate problem of producing an account ID for the older SKV statement table format. This is also a bit discouraging as I am pritty sure I will want to remove the whole 'account ID' mechanism as it will probably not be viable for most statement file content?

Maybe I can provide the map of row zero in the statement map?

* A valid statement file must have at least one row?
* For current SKV statement file row 0 contains the organisation number ok.
* And for older SKV I can detetc it to be an SKV statement table from row 0.
* And Current NORDEA detection also uses row 0 (then the heaader).

I now added row 0 map to statement map and account id is now determined for 'older' SKV using this info ok.

We are now down to:

```sh
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_NORDEA_0_1_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_BOM_ed_Ok
```

I have now pushed on and have:

```sh
[  PASSED  ] 358 tests.
[  FAILED  ] 3 tests, listed below:
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_2
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_3
```

This means I am now accepting failure to produce a column mapping for an emty account statement (this is OK).

But I fail to detect in and out saldo for current SKV statement file. And is also unable to produce an account id for an emtpy SKV statement.

* Maybe this is the time to implement detection of org-no fields?
  - Then I can use this to get a correct key for the first row and know it is an SKV account
* And if so, I may be tempted to also identify PG,BG and Bank account number fields in the statement table?

Let's try and see how much work it may be?

DARN! This exposed a dependancy and namespace confusion hell.

I moved only to_field_type to the meta unit and this worked out fine for now.

## 20260216

Implemented, valled and logged to_skv_in_out_saldos lambda. It seems to work on old SKV + stable for other text statement files.

Question is now:

* How should I expect in/out saldos and transaction range to relate?
* Should I require in/out saldos to 'embrace' any statement transaction entries?
* Should I care?

## 20260215

I now have this idea for generic statement table parsing.

1. Call a function to get optional in and out saldo
  - Return index + tagged amount pair for in and out saldo found
  - Reyurn nullopt if no in/out saldo found
2. If saldo is found
  - define transaction range based on in-salod index + 1 and out-saldo index -1
  - Else define trasnaction ramge as from first one Date,one or two Amount,one or more Text
3. produce common over transaction range.


## 20260213

Back to account statement sv file parsing.

The next step is to make the generic statement table parser be able to parse statements with in and out saldo like the 'old' SKV file was using.

We have generic_like_to_statement_mapping doing:

* A range from is_amount_and_saldo_entry_candidate to NOT is_amount_and_saldo_entry_candidate
* Check that we get at least one entry in the range

We now whant to also be able to handle:

* An optional single amount in-saldo and out-saldo entry
* A range of either is_single_amount_trans_candidate or is_amount_and_saldo_entry_candidate

Question is, whould the in-saldo and out-saldo predicates be different from xxx_trans_candidate?

* YES: It seems better to have a separate is_saldo_entry_candidate and some is_xxx_trans_candidate predicates?

We could imagine the following:

* First inferr if there are dedicated saldo_entries to surround the trans entry candidates?
* If the in- and out-saldo entries has the same rowmap as the trans entreis, then we can safelly expand?

I added a 'most permissive range of candidates' mechanism to generic_like_to_statement_mapping. And it succeeds on SKV 'old' statement table.

## 20260212 SIE roudntrip seems to be broken

I succeeded to create and send VAT Returns report to SKV.

I then imported the unposted SIE entries to Fortnox (In file 'cratchit_2025-05-01_2026-04-30.se')

```text
#GEN 20260212
#RAR 0 20250501 20260430
#VER A 26 20251003 "Account:NORDEA::32592317244 Text:AVGIFTER NORDEA | AVGIFTER NORDEA"
{
#TRANS 1920 {} -3.70 "" ""
#TRANS 6570 {} 3.70 "" ""
}
#VER A 27 20251004 "Account:SKV::5567828172 Text:IntÑktsrÑnta"
{
#TRANS 1630 {} 1.00 "" ""
#TRANS 8314 {} -1.00 "" ""
}
#VER A 28 20251105 "Account:NORDEA::32592317244 Text:AVGIFTER NORDEA | AVGIFTER NORDEA"
{
#TRANS 1920 {} -1.85 "" ""
#TRANS 6570 {} 1.85 "" ""
}
#VER A 29 20251112 "Account:SKV::5567828172 Text:Moms juli 2025 - sept 2025"
{
#TRANS 1630 {} 1997.00 "" ""
#TRANS 1650 {} -1997.00 "" ""
}
#VER A 30 20251114 "SKV Utbetalning Moms"
{
#TRANS 1630 {} -1997.00 "" ""
#TRANS 1920 {} 1997.00 "" ""
}
#VER A 31 20251119 "Account:NORDEA::32592317244 Text:KORT             CLAUDE.AI SUBSC 26 | KORT             CLAUDE.AI SUBSC 26"
{
#TRANS 1920 {} -198.08 "" ""
#TRANS 6230 {} 198.08 "" ""
}
#VER A 32 20251202 "Account:NORDEA::2025 Text:FRèN SPAR 518687-9 | FRèN SPAR"
{
#TRANS 1920 {} 10000.00 "" ""
#TRANS 1930 {} -10000.00 "" ""
}
#VER A 33 20251203 "Account:NORDEA::32592317244 Text:AVGIFTER NORDEA | AVGIFTER NORDEA"
{
#TRANS 1920 {} -1.85 "" ""
#TRANS 6570 {} 1.85 "" ""
}
#VER A 34 20251206 "Account:SKV::5567828172 Text:IntÑktsrÑnta"
{
#TRANS 1630 {} 1.00 "" ""
#TRANS 8314 {} -1.00 "" ""
}
#VER A 35 20251219 "Account:NORDEA::32592317244 Text:KORT             CLAUDE.AI SUBSC 26 | KORT             CLAUDE.AI SUBSC 26"
{
#TRANS 1920 {} -200.57 "" ""
#TRANS 6230 {} 200.57 "" ""
}
#VER A 36 20251231 "Account:NORDEA::2025 Text:RÑnta 2025"
{
#TRANS 1920 {} 328.68 "" ""
#TRANS 8310 {} -328.68 "" ""
}
#VER D 12 20251107 "Fortknox Faktura"
{
#TRANS 2440 {} -615.00 "" ""
#TRANS 5420 {} 492.00 "" ""
#TRANS 2641 {} 123.00 "" ""
#TRANS 3740 {} 0.00 "" ""
}
#VER D 13 20251117 "Telia Faktura"
{
#TRANS 2440 {} -2359.00 "" ""
#TRANS 6212 {} 1887.00 "" ""
#TRANS 2641 {} 471.25 "" ""
#TRANS 3740 {} 0.75 "" ""
}
#VER D 14 20251128 "Websupport Faktura"
{
#TRANS 2440 {} -1158.75 "" ""
#TRANS 6210 {} 927.00 "" ""
#TRANS 2641 {} 231.75 "" ""
}
#VER E 11 20251020 "Account:NORDEA::32592317244 Text:LOOPIA AB (WEBSUPPORT) | LOOPIA AB (WEBSUPPORT)"
{
#TRANS 1920 {} -323.75 "" ""
#TRANS 2440 {} 323.75 "" ""
}
#VER E 12 20251127 "Account:NORDEA::32592317244 Text:Fortnox Finans AB | Fortnox Finans AB"
{
#TRANS 1920 {} -615.00 "" ""
#TRANS 2440 {} 615.00 "" ""
}
#VER E 13 20251217 "Account:NORDEA::32592317244 Text:TELIA SVERIGE AB | TELIA SVERIGE AB"
{
#TRANS 1920 {} -2359.00 "" ""
#TRANS 2440 {} 2359.00 "" ""
}
#VER E 14 20251229 "Account:NORDEA::32592317244 Text:LOOPIA AB (WEBSUPPORT) | LOOPIA AB (WEBSUPPORT)"
{
#TRANS 1920 {} -1158.75 "" ""
#TRANS 2440 {} 1158.75 "" ""
}
```

I then exported from Fortnox back to cratchit.

But when I imported to Cratchit it reported conflicts!

```text
cratchit:>-sie sie_in/TheITfiedAB20260212_162605.se

Importing SIE to current year from "sie_in/TheITfiedAB20260212_162605.se"

STAGE of cracthit entries FAILED when merging with posted (from external tool)
Entries in sie-file:sie_in/TheITfiedAB20260212_162605.se overrides values in cratchit staged entries
CONFLICTED! : No longer valid entry  A26 "Account:NORDEA::32592317244 Text:AVGIFTER NORDEA | AVGIFTER NORDEA" 20251003
    "PlusGiro":1920 "" -3.70
    "Bankkostnader":6570 "" 3.70
OK - valid entry  A27 "Account:SKV::5567828172 Text:Intäktsränta" 20251004
    "Avräkning för skatter och avgifter (skattekonto)":1630 "" 1.00
    "Skattefria ränteintäkter":8314 "" -1.00
CONFLICTED! : No longer valid entry  A28 "Account:NORDEA::32592317244 Text:AVGIFTER NORDEA | AVGIFTER NORDEA" 20251105
    "PlusGiro":1920 "" -1.85
    "Bankkostnader":6570 "" 1.85
OK - valid entry  A29 "Account:SKV::5567828172 Text:Moms juli 2025 - sept 2025" 20251112
    "Avräkning för skatter och avgifter (skattekonto)":1630 "" 1997.00
    "Momsfordran":1650 "" -1997.00
OK - valid entry  A30 "SKV Utbetalning Moms" 20251114
    "Avräkning för skatter och avgifter (skattekonto)":1630 "" -1997.00
    "PlusGiro":1920 "" 1997.00
CONFLICTED! : No longer valid entry  A31 "Account:NORDEA::32592317244 Text:KORT             CLAUDE.AI SUBSC 26 | KORT             CLAUDE.AI SUBSC 26" 20251119
    "PlusGiro":1920 "" -198.08
    "Datakommunikation":6230 "" 198.08
CONFLICTED! : No longer valid entry  A32 "Account:NORDEA::2025 Text:FRÅN SPAR 518687-9 | FRÅN SPAR" 20251202
    "PlusGiro":1920 "" 10000.00
    "Företagskonto/checkkonto/affärskonto":1930 "" -10000.00
CONFLICTED! : No longer valid entry  A33 "Account:NORDEA::32592317244 Text:AVGIFTER NORDEA | AVGIFTER NORDEA" 20251203
    "PlusGiro":1920 "" -1.85
    "Bankkostnader":6570 "" 1.85
OK - valid entry  A34 "Account:SKV::5567828172 Text:Intäktsränta" 20251206
    "Avräkning för skatter och avgifter (skattekonto)":1630 "" 1.00
    "Skattefria ränteintäkter":8314 "" -1.00
CONFLICTED! : No longer valid entry  A35 "Account:NORDEA::32592317244 Text:KORT             CLAUDE.AI SUBSC 26 | KORT             CLAUDE.AI SUBSC 26" 20251219
    "PlusGiro":1920 "" -200.57
    "Datakommunikation":6230 "" 200.57
OK - valid entry  A36 "Account:NORDEA::2025 Text:Ränta 2025" 20251231
    "PlusGiro":1920 "" 328.68
    "Ränteintäkter från omsättningstillgångar":8310 "" -328.68
OK - valid entry  D12 "Fortknox Faktura" 20251107
    "Leverantörsskulder":2440 "" -615.00
    "Programvaror":5420 "" 492.00
    "Debiterad ingående moms":2641 "" 123.00
    "Öres- och kronutjämning":3740 "" 0.00
OK - valid entry  D13 "Telia Faktura" 20251117
    "Leverantörsskulder":2440 "" -2359.00
    "Mobiltelefon":6212 "" 1887.00
    "Debiterad ingående moms":2641 "" 471.25
    "Öres- och kronutjämning":3740 "" 0.75
OK - valid entry  D14 "Websupport Faktura" 20251128
    "Leverantörsskulder":2440 "" -1158.75
    "Telekommunikation":6210 "" 927.00
    "Debiterad ingående moms":2641 "" 231.75
CONFLICTED! : No longer valid entry  E11 "Account:NORDEA::32592317244 Text:LOOPIA AB (WEBSUPPORT) | LOOPIA AB (WEBSUPPORT)" 20251020
    "PlusGiro":1920 "" -323.75
    "Leverantörsskulder":2440 "" 323.75
CONFLICTED! : No longer valid entry  E12 "Account:NORDEA::32592317244 Text:Fortnox Finans AB | Fortnox Finans AB" 20251127
    "PlusGiro":1920 "" -615.00
    "Leverantörsskulder":2440 "" 615.00
CONFLICTED! : No longer valid entry  E13 "Account:NORDEA::32592317244 Text:TELIA SVERIGE AB | TELIA SVERIGE AB" 20251217
    "PlusGiro":1920 "" -2359.00
    "Leverantörsskulder":2440 "" 2359.00
CONFLICTED! : No longer valid entry  E14 "Account:NORDEA::32592317244 Text:LOOPIA AB (WEBSUPPORT) | LOOPIA AB (WEBSUPPORT)" 20251229
    "PlusGiro":1920 "" -1158.75
    "Leverantörsskulder":2440 "" 1158.75
 year id:current - ALL POSTED OK
cratchit:>
```

When I exited Cratchit I now had 'cratchit_2025-05-01_2026-04-30.se':

```text
#GEN 20260212
#RAR 0 20250501 20260430
```

I checked if Cratchit still had all the SIE entries as imported from Fortnox?

* First I got confused as the last entry was:

```text
 66 M2 "Momsrapport_2025-07-01-2025-09-30" 20250930
    "Debiterad ingående moms":2641 "" -1997.25
    "Momsfordran":1650 "" 1997.00
    "Öres- och kronutjämning":3740 "" 0.25
```
* DARN! I have yet to account M3!
* Because YES: Cratchit DOES now have all accounting OK in the imported SIE.

Anyhow, I registered M3 (from HAD ok). And then I Exported to Fortnox and back again OK.

* The import of the latest SIE from Fortnox (now with the additional M3) worked juste fine.
  - Cratchit reported NO conflicts.

So, what are the conflicts about??

## 20260212

So today I have to produce and deliver a VAR Returns report for Q4 of 2025 for my company activites.

So I have to pause my csv statement file parsing refactoring.

Instead I will take this opportunity to think about how Cratchit supports VAT Returns reporting.

The curreent mechanism is based on the flow:

1. Import bank and skv account statement files for the period -> Tagged Amounts

  - I have three statement files for this period

```sh
-rw-r--r--@  1 kjell-olovhogdahl  staff   321 Jan 12 11:24 Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv
-rw-r--r--@  1 kjell-olovhogdahl  staff  1194 Jan 12 11:22 PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv
-rw-r--r--@  1 kjell-olovhogdahl  staff   246 Jan 12 11:22 SPARKONTO FÖRETAG 3259 23 17244 - 2026-01-12 11.22.36.csv
```

  - The import works but redundantly also tries non-statement files

  ```sh
BEGIN: Processing files in "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv"

BEGIN File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/.DS_Store"
  ...
*Note* "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/.DS_Store" (produced zero entries)

*Note* "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/.DS_Store" (produced zero entries)
*Ignored* "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/.DS_Store" (File empty or failed to understand file content)
END File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/.DS_Store"

BEGIN File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv"
  ...
	[Pipeline] File /Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv opened OK
	[Pipeline] Successfully read 321 bytes from stream
	[Pipeline] Detected encoding: UTF-8 (confidence: 100, method: ICU)
	[Pipeline] Successfully transcoded 321 bytes to 321 platform encoding bytes
	[Pipeline] Step 6 complete: CSV parsed successfully (8 rows)
	[Pipeline] (4) Step 6.5 complete: AccountID detected: 'SKV::5567828172'
	[Pipeline] Pipeline complete: 4 TaggedAmounts created from 'Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv'
	Valid entries count:4
	Consumed account statement file moved to "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/consumed/Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv"
END File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv"

BEGIN File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv"
  ... calls skv_like_to_column_mapping then nordea_like_to_column_mapping
	[Pipeline] File /Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv opened OK
	[Pipeline] Successfully read 1194 bytes from stream
	[Pipeline] Detected encoding: UTF-8 (confidence: 100, method: ICU)
	[Pipeline] Successfully transcoded 1194 bytes to 1194 platform encoding bytes
	[Pipeline] Step 6 complete: CSV parsed successfully (12 rows)
	[Pipeline] (4) Step 6.5 complete: AccountID detected: 'NORDEA::32592317244'
	[Pipeline] Pipeline complete: 11 TaggedAmounts created from 'PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv'
	Valid entries count:11
	Consumed account statement file moved to "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/consumed/PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv"
END File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv"

  ...
```

2. List the Tagged Amount for the period and transform them to HAD:s (Heading Amount Date:s)
  - command:'-tas 20251001 20251231'
  - command:'-to_hads'
  - command:'-hads'

```text
  0   ?BAS? Account:NORDEA::2025 Text:Ränta 2025 328.68 20251231
  1   ?BAS? Account:NORDEA::32592317244 Text:LOOPIA AB (WEBSUPPORT) | LOOPIA AB (WEBSUPPORT) -1158.75 20251229
  2   ?BAS? Account:NORDEA::32592317244 Text:KORT             CLAUDE.AI SUBSC 26 | KORT             CLAUDE.AI SUBSC 26 -200.57 20251219
  3   ?BAS? Account:NORDEA::32592317244 Text:TELIA SVERIGE AB | TELIA SVERIGE AB -2359.00 20251217
  4   ?BAS? Account:SKV::5567828172 Text:Intäktsränta 1.00 20251206
  5   ?BAS? Account:NORDEA::32592317244 Text:AVGIFTER NORDEA | AVGIFTER NORDEA -1.85 20251203
  6   ?BAS? Account:NORDEA::32592317244 Text:Insättning | Insättning 10000.00 20251202
  7   ?BAS? Account:NORDEA::2025 Text:FRÅN SPAR 518687-9 | FRÅN SPAR -10000.00 20251202
  8   ?BAS? Account:NORDEA::32592317244 Text:Fortnox Finans AB | Fortnox Finans AB -615.00 20251127
  9   ?BAS? Account:NORDEA::32592317244 Text:KORT             CLAUDE.AI SUBSC 26 | KORT             CLAUDE.AI SUBSC 26 -198.08 20251119
  10   ?BAS? Account:NORDEA::32592317244 Text:BG KONTOINS | BG KONTOINS 1997.00 20251118
  11   ?BAS? Account:SKV::5567828172 Text:Utbetalning -1997.00 20251114
  12   ?BAS? Account:SKV::5567828172 Text:Moms juli 2025 - sept 2025 1997.00 20251112
  13   ?BAS? Account:NORDEA::32592317244 Text:AVGIFTER NORDEA | AVGIFTER NORDEA -1.85 20251105
  14   ?BAS? Account:NORDEA::32592317244 Text:LOOPIA AB (WEBSUPPORT) | LOOPIA AB (WEBSUPPORT) -323.75 20251020
  15   ?BAS? Account:SKV::5567828172 Text:Intäktsränta 1.00 20251004
  16   ?BAS? Account:NORDEA::32592317244 Text:AVGIFTER NORDEA | AVGIFTER NORDEA -3.70 20251003
```

3. Print out pdf vouchers
  - Invocies
  - Account statements
  - In this way I can mark them manually how they where anetered into the books
3. Enter period invoices and events that are not reflected in account statement yet as HAD:s
  - I have most invoices as pdf:s from emailed invoices.
  - I may have Receipts from purchases (but then those are also reflected in account statement)
  - command:'-'
  - command:'Heading Amount Date' -> Cracthit parses into HAD

```text
cratchit:>Telia Faktura 2359,00 2025-11-17

  ?BAS? Telia Faktura 2359.00 20251117
cratchit:>SKV Utbetalning Moms 1997,00 2025-11-14

  ?BAS? SKV Utbetalning Moms 1997.00 20251114
cratchit:>Websupport Faktura 1158,75 2025-11-28

  ?BAS? Websupport Faktura 1158.75 20251128
cratchit:>Websupport Faktura 1058,75 2026-01-11

  ?BAS? Websupport Faktura 1058.75 20260111
cratchit:>Websupport Faktura 236,25 2026-01-16

  ?BAS? Websupport Faktura 236.25 20260116
cratchit:>Websupport Faktura 323,75 2026-02-07

  ?BAS? Websupport Faktura 323.75 20260207
cratchit:>Postnord Faktura 4037,50 2026-01-12

  ?BAS? Postnord Faktura 4037.50 20260112
cratchit:>Fortknox Faktura 615,00 2025-11-07

  ?BAS? Fortknox Faktura 615.00 20251107
cratchit:> 
```

  - command:'-hads'

```text
  0   ?BAS? Websupport Faktura 323.75 20260207
  1   ?BAS? Websupport Faktura 236.25 20260116
  2   ?BAS? Postnord Faktura 4037.50 20260112
  3   ?BAS? Websupport Faktura 1058.75 20260111
  4   ?BAS? Account:NORDEA::2025 Text:Ränta 2025 328.68 20251231
  5   ?BAS? Account:NORDEA::32592317244 Text:LOOPIA AB (WEBSUPPORT) | LOOPIA AB (WEBSUPPORT) -1158.75 20251229
  6   ?BAS? Account:NORDEA::32592317244 Text:KORT             CLAUDE.AI SUBSC 26 | KORT             CLAUDE.AI SUBSC 26 -200.57 20251219
  7   ?BAS? Account:NORDEA::32592317244 Text:TELIA SVERIGE AB | TELIA SVERIGE AB -2359.00 20251217
  8   ?BAS? Account:SKV::5567828172 Text:Intäktsränta 1.00 20251206
  9   ?BAS? Account:NORDEA::32592317244 Text:AVGIFTER NORDEA | AVGIFTER NORDEA -1.85 20251203
  10   ?BAS? Account:NORDEA::32592317244 Text:Insättning | Insättning 10000.00 20251202
  11   ?BAS? Account:NORDEA::2025 Text:FRÅN SPAR 518687-9 | FRÅN SPAR -10000.00 20251202
  12   ?BAS? Websupport Faktura 1158.75 20251128
  13   ?BAS? Account:NORDEA::32592317244 Text:Fortnox Finans AB | Fortnox Finans AB -615.00 20251127
  14   ?BAS? Account:NORDEA::32592317244 Text:KORT             CLAUDE.AI SUBSC 26 | KORT             CLAUDE.AI SUBSC 26 -198.08 20251119
  15   ?BAS? Account:NORDEA::32592317244 Text:BG KONTOINS | BG KONTOINS 1997.00 20251118
  16   ?BAS? Telia Faktura 2359.00 20251117
  17   ?BAS? Account:SKV::5567828172 Text:Utbetalning -1997.00 20251114
  18   ?BAS? SKV Utbetalning Moms 1997.00 20251114
  19   ?BAS? Account:SKV::5567828172 Text:Moms juli 2025 - sept 2025 1997.00 20251112
  20   ?BAS? Fortknox Faktura 615.00 20251107
  21   ?BAS? Account:NORDEA::32592317244 Text:AVGIFTER NORDEA | AVGIFTER NORDEA -1.85 20251105
  22   ?BAS? Account:NORDEA::32592317244 Text:LOOPIA AB (WEBSUPPORT) | LOOPIA AB (WEBSUPPORT) -323.75 20251020
  23   ?BAS? Account:SKV::5567828172 Text:Intäktsränta 1.00 20251004
  24   ?BAS? Account:NORDEA::32592317244 Text:AVGIFTER NORDEA | AVGIFTER NORDEA -3.70 20251003
```
4. Check that Cratchit SIE is the same as external Fortnox SIE
  - If Not, Do SIE: Fortnox -> Cratchit (In this case the file TheITfiedAB20260212_113259.se)
  - command: 'cratchit:>-sie sie_in/TheITfiedAB20260212_113259.se'

```text
Importing SIE to current year from "sie_in/TheITfiedAB20260212_113259.se"
 year id:current - ALL POSTED OK
 cratchit:>
```
5. Transform each HAD to SIE account entry
  - I managed to do this in the 'zeroth' API
  - I noted some quirks and shortcomings below
  - I now have:

```text
cratchit:>-hads

	to_box_49_amount += [10]:0.00 = 0.00
	to_box_49_amount += [30]:0.00 = 0.00
	to_box_49_amount += [60]:0.00 = 0.00
	to_box_49_amount += [48]:826.00 = 826.00
Period:20251001...20260331
	to_box_49_amount += [10]:0.00 = 0.00
	to_box_49_amount += [30]:0.00 = 0.00
	to_box_49_amount += [60]:0.00 = 0.00
	to_box_49_amount += [48]:826.00 = 826.00
Period:20250701...20251231
*UPDATED*   ?BAS? Momsrapport 2026-01-01...2026-03-31 826.00 20260331
*UPDATED*   ?BAS? Momsrapport 2025-10-01...2025-12-31 826.00 20251231
Please select:
  0   ?BAS? Momsrapport 2026-01-01...2026-03-31 826.00 20260331
  1   ?BAS? Websupport Faktura 323.75 20260207
  2   ?BAS? Websupport Faktura 236.25 20260116
  3   ?BAS? Postnord Faktura 4037.50 20260112
  4   ?BAS? Websupport Faktura 1058.75 20260111
  5   ?BAS? Momsrapport 2025-10-01...2025-12-31 826.00 20251231
cratchit:had>
```
  - So VAT Returns aggregation  seems to work fine?

6. Generate the VAT Returns report to SKV
  - command: '-skv current'

```text
Till Skatteverket för år current
0: Arbetsgivardeklaration (TAX Returns)
1: Momsrapport (VAT Returns)
2: K10 (TAX Declaration Appendix Form)
3: INK1 + K10 (Swedish Tax Agency private TAX Form + Dividend Form
4: INK2 + INK2S + INK2R (Company Tax Returns form(s))
7. Enter the VAT Returns report as SIE Entry
```

  - command: '1: Momsrapport (VAT Returns)'

```text
0: Track Current Quarter 2026-01-01...2026-03-31
1: Report Previous Quarter 2025-10-01...2025-12-31
2: Check Quarter before previous 2025-07-01...2025-09-30
3: Check Previous two Quarters 2025-07-01...2025-12-31
```

  -command: '1: Report Previous Quarter 2025-10-01...2025-12-31'

```text
	to_box_49_amount += [10]:0.00 = 0.00
	to_box_49_amount += [30]:0.00 = 0.00
	to_box_49_amount += [60]:0.00 = 0.00
	to_box_49_amount += [48]:826.00 = 826.00
Previous Quarter 2025-10-01...2025-12-31 (to report)
VAT Returns for 2025-10-01...2025-12-31
Created "to_skv/moms_202512.eskd"
<!DOCTYPE eSKDUpload PUBLIC "-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV" "https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd">
<eSKDUpload Version="6.0">
  <OrgNr>556782-8172</OrgNr>
  <Moms>
   <Period>202512</Period>
   <ForsMomsEjAnnan>0</ForsMomsEjAnnan>
   <InkopVaruAnnatEg>0</InkopVaruAnnatEg>
   <MomsUlagImport>0</MomsUlagImport>
   <ForsTjSkskAnnatEg>0</ForsTjSkskAnnatEg>
   <MomsUtgHog>0</MomsUtgHog>
   <MomsInkopUtgHog>0</MomsInkopUtgHog>
   <MomsImportUtgHog>0</MomsImportUtgHog>
   <MomsIngAvdr>826</MomsIngAvdr>
   <MomsBetala>-826</MomsBetala>
  </Moms>
</eSKDUpload>
Created file "to_skv/periodisk_sammanstallning_25-4_20260212.csv" OK
SKV574008;
SE556782817201;25-4;Ville Vessla;555-244454;ville.vessla@foretaget.se
cratchit:skv:tax_return:period>
```

  - I now have:

```sh
drwxr-xr-x  3 kjell-olovhogdahl  staff   96 Aug 27 10:20 INK2
drwxr-xr-x  4 kjell-olovhogdahl  staff  128 Aug 26 12:40 K10
drwxr-xr-x  4 kjell-olovhogdahl  staff  128 Aug 26 18:52 SRU
-rw-r--r--  1 kjell-olovhogdahl  staff  616 Nov 11 17:11 moms_202409.eskd
-rw-r--r--@ 1 kjell-olovhogdahl  staff  623 Feb 11  2025 moms_202412.eskd
-rw-r--r--  1 kjell-olovhogdahl  staff  623 May 10  2025 moms_202503.eskd
-rw-r--r--  1 kjell-olovhogdahl  staff  621 Aug 12  2025 moms_202506.eskd
-rw-r--r--  1 kjell-olovhogdahl  staff  623 Nov 13 11:03 moms_202509.eskd
-rw-r--r--  1 kjell-olovhogdahl  staff  621 Feb 12 16:09 moms_202512.eskd
-rw-r--r--  1 kjell-olovhogdahl  staff   80 Nov 11 17:11 periodisk_sammanstallning_24-3_20251111.csv
-rw-r--r--@ 1 kjell-olovhogdahl  staff   80 Feb 11  2025 periodisk_sammanstallning_24-4_20250211.csv
-rw-r--r--  1 kjell-olovhogdahl  staff   80 May 10  2025 periodisk_sammanstallning_25-1_20250510.csv
-rw-r--r--  1 kjell-olovhogdahl  staff   80 Aug 12  2025 periodisk_sammanstallning_25-2_20250812.csv
-rw-r--r--  1 kjell-olovhogdahl  staff   80 Nov 12 23:12 periodisk_sammanstallning_25-3_20251112.csv
-rw-r--r--  1 kjell-olovhogdahl  staff   80 Nov 13 11:03 periodisk_sammanstallning_25-3_20251113.csv
-rw-r--r--  1 kjell-olovhogdahl  staff   80 Feb 12 16:09 periodisk_sammanstallning_25-4_20260212.csv
```

  - Now I can use the SKV Web interface to upload the file '/moms_202512.eskd'

7. Account the M3 VAT Returns report from HAD.

  - command: '-hads'
  - option: ' 5   ?BAS? Momsrapport 2025-10-01...2025-12-31 826.00 20251231'
  - option: 1 for yes in 'cratchit:had:vat:summary:ok?>'
  - For here it is '1'a for yes all the way!
  - NOTE: Ctachit actually generates THE SKV FILES ALSO!!
  - So it seems I do not need to do the (6) separatly above?

### Consider Cratchit Bank and SKV integration like Fortnox does?

It seems Fortnox offers intergration with SKV and Bank accounts to enable book keeping based on account statement entries?

* How does this work?
* HWat does the Bank and SKV require to allow the integration?
* Is there a fee or license requirement?
* Is it maybe based on BankID
* If so, how hard is it to implement in open source like Cratchit?

### VAT Returns report - Proposed enhancements

* Consider to make cratchit work on git repo folder?
  - Consider framework.env that defines workspace path?
  - framework.env is in runtime folder (with cratchit binary as 'installed')
  - Or, simply configure a 'current folder' to git repo folder from the shell (how does that even work)?
* Consider pdf-files -> HAD:s for vouchers?
  - Cratchit can create a HAD (seed) with a path to an invoice file
  - On macOS Cratchit can call the system 'open *path*' for conveniant Heading Amount and Date entry?
* Consider to list HAD:s in olde-to-new order?
  - Then the next HAD is always index 0?
* Consider to reject SIE file import if content and target fiscal yer does not match?
* Consider a mechanism that takes HAD:s and adds missing 'domino-events'?
  - A bank statement payment -> find accounted debt (or previous HAD)
  - A HAD Invoice -> Predict a future payment HAD
  - Do we need HADs to 'know' if they are Debt/Asset/Cost/Income?
* Consider to make Cratchit be able to re-create an account statement with the SIE accouning added?
  - Consider to remember how account statement entries was entered into SIE books
  - The have a function to re-create the account statement entries paierd with how they where accounted?
  - Then I can print out and keep as a record of how the books where made?
  - Then I would not have to manually write that down on the printed account statement?
* Consider Cratchit to inject the SIE accointing into the vocuher file names?
  - If the HAD can track the path.
  - Then it may be possible for Cratchit to rename the voucer file when the HAD is booked into SIE?
* Consider to output BAS account Saldo after each entered SIE?
  - I can then verify I am 'on track' while entereing into the books
* Consider to shorten the list of templates offered to account a HAD -> SIE
  - List into groups of same BAS accounts aggregate (accounts + signs)
* Consider to detect 'same HAD' from e.g., SKV and bank account transfers
  - The same amount (and text?) is out on one and in oth the other?
* Consider to show options for No/Yes when accepting generated VAT REturns SIE entry 'M'
* Consider to have Cratchit detect zero VAT Returns and remove any VAT Resturn HAD:s in the listing?
* Consider to add 'Print Entry' from the books?
  - Then I can print the generated VAT Returns entry (e.g., M3) for the books? 

### VAT Returns report ABSTRACT (Observations and notes)

* WOT! Cratchid DOES generate SKV files AND SIE entry for HAD Vat Resturns selection!
  - I though I had to do the files separate from the SIE entry?
  - But now when I try it seems Cratchit first generates the files, then generates the Mx SIE entry?
  - The MESS to keep track of what I have implemented or not...
* Cratchit does noty remove VAT Returns HAD:s on VAT Resturns aggretagtion == 0
  - That is, after having registered the VAT Resturns report, the HAD:s are still there.
* Crathit does not show out-saldo after booked (entered) SIE
  - It would be nice to confirm the Saldo when entering from account statement entry
* Current version of Cratchit dispose tagged amounts at exit
  - We must create HAD:s from imported statement files in the same session!
  - TODO: Consider to activate saving tagged amounts to environment again?
          Is the mechanism stable now after refactoring into 'prev' linked structure?
* Cratchit does not support HAD:s with foreign amounts
  - E.g., Anthropic invoices in $.
* Cratchit confusingly shows HAD prefixed with '?BAS?'
  - A left-over from unfinished mechanism to link to BAS account?
* Cratchit has NO PROTECTION against importing an SIE file to a fiscal year that does not match the content fiscal year!
* Command:'-balance' does not seem to work?
* Command: '-huvudbok' seems to work?
  - But lacks 'omslutning'
* The list of proposals (matched templates) is WAY too long!
* Command '-sie' now longer lists 'staged' (*) entries!
  - command '-si *' does though (so they are there!)
* I encountered failure to apply a template

```text
6  D1 "Faktura Fortnox" 20250508
	 gross = sort_code: 0x3 : "Leverantörsskulder":2440 "" -615.00
	 vat = sort_code: 0x6 : "Debiterad ingående moms":2641 "" 123.00
	 eu_vat cents = sort_code: 0x57 : "Öres- och kronutjämning":3740 "" 0.00
	 net = sort_code: 0x4 : "Programvaror":5420 "" 492.00
```

  - Is it because it contains 'öresavridning'?

```text
"cents" count:1
"eu_vat" count:1
"gross" count:1
"net" count:1
"vat" count:1
Failed to recognise the VAT type
vat_type = Undefined
Sorry, I encountered Undefined VAT type for  D1 "Faktura Fortnox" 20250508
	 gross = sort_code: 0x3 : "Leverantörsskulder":2440 "" -615.00
	 vat = sort_code: 0x6 : "Debiterad ingående moms":2641 "" 123.00
	 eu_vat cents = sort_code: 0x57 : "Öres- och kronutjämning":3740 "" 0.00
	 net = sort_code: 0x4 : "Programvaror":5420 "" 492.00
```

  - But this template worked:

  ```text
  28  D1 "Fortnox Faktura" 20230511
	 gross = sort_code: 0x3 : "Leverantörsskulder":2440 "" -521.00
	 vat = sort_code: 0x6 : "Debiterad ingående moms":2641 "" 104.25
	 cents = sort_code: 0x7 : "Öres- och kronutjämning":3740 "" -0.25
	 net = sort_code: 0x4 : "Programvaror":5420 "" 417.00
  ```
  - Became:

```text
"cents" count:1
"gross" count:1
"net" count:1
"vat" count:1
Template is a SWEDISH PURCHASE/sale
vat_type = Swedish VAT
Swedish VAT candidate  D _  "Fortknox Faktura" 20251107
    "Leverantörsskulder":2440 "" -615.00
    "Programvaror":5420 "" 492.00
    "Debiterad ingående moms":2641 "" 123.00
    "Öres- och kronutjämning":3740 "" 0.00
```

* SKV vat out payment and bank account in payment is not recognised as 'the same' event
* TELIA invoices applies a VAT algorithm that differs from what Cratchit applies from template
  - Cratchit does:

```text

35  D2 "Telia Faktura" 20250516
	 gross = sort_code: 0x3 : "Leverantörsskulder":2440 "" -2359.00
	 vat = sort_code: 0x6 : "Debiterad ingående moms":2641 "" 471.75
	 cents = sort_code: 0x7 : "Öres- och kronutjämning":3740 "" 0.25
	 net = sort_code: 0x4 : "Mobiltelefon":6212 "" 1887.00

--> Becomes

"cents" count:1
"gross" count:1
"net" count:1
"vat" count:1
Template is a SWEDISH PURCHASE/sale
vat_type = Swedish VAT
Swedish VAT candidate  D _  "Telia Faktura" 20251117
    "Leverantörsskulder":2440 "" -2359.00
    "Mobiltelefon":6212 "" 1887.20
    "Debiterad ingående moms":2641 "" 471.80
    "Öres- och kronutjämning":3740 "" 0.00
```

  - But TELIA invoice states (would require)

```text
Swedish VAT candidate  D _  "Telia Faktura" 20251117
    "Leverantörsskulder":2440 "" -2359.00
    "Mobiltelefon":6212 "" 1887.00
    "Debiterad ingående moms":2641 "" 471.75
    "Öres- och kronutjämning":3740 "" -0.25

```
  - Does TELIA prefers 0,25 adjustment?
  - Or NET amount without cents?
  - Or both? (Cuts off 0,20 from net amount and places -0,05 on VAT and the rest on rounding)
  - What is going on?

* Edit TELIA invoice transaction is a bit tedious?
  - command:'2 Edit account transactions'

    I get options 'Options:
    'Please enter index of entry to edit
    ,or <BAS account> <Amount> to add a new entry
    ;'-' to exit back to journal entry'

  - command: '1 "Mobiltelefon":6212 "" 1887.20'

    I get options: 'Options:
    Please Enter new Account
    ,new Amount (with decimal comma) or new transaction text
    ; '-' to exit back to candidate'

  - command:'1887,00'

* Cratchit does not stay in edit or explain what is wrong on 'entry does not balance'
  - I entered the wrong rounding in:

```text
"cents" count:1
"gross" count:1
"net" count:1
"vat" count:1
Template is a SWEDISH PURCHASE/sale
 D _  "Telia Faktura" 20251117
	 gross = sort_code: 0x3 : "Leverantörsskulder":2440 "" -2359.00
	 vat = sort_code: 0x6 : "Debiterad ingående moms":2641 "" 471.25
	 cents = sort_code: 0x7 : "Öres- och kronutjämning":3740 "" -0.25
	 net = sort_code: 0x4 : "Mobiltelefon":6212 "" 1887.00
SORRY - Failed to stage entry
```
  - BUT, the HAD remembered the cadidate (So I could select it again and edit it correctly)

```text
"cents" count:1
"gross" count:1
"net" count:1
"vat" count:1
Template is a SWEDISH PURCHASE/sale
 D _  "Telia Faktura" 20251117
	 gross = sort_code: 0x3 : "Leverantörsskulder":2440 "" -2359.00
	 vat = sort_code: 0x6 : "Debiterad ingående moms":2641 "" 471.25
	 cents = sort_code: 0x7 : "Öres- och kronutjämning":3740 "" 0.75
	 net = sort_code: 0x4 : "Mobiltelefon":6212 "" 1887.00
 D13 "Telia Faktura" 20251117
    "Leverantörsskulder":2440 "" -2359.00
    "Mobiltelefon":6212 "" 1887.00
    "Debiterad ingående moms":2641 "" 471.25
    "Öres- och kronutjämning":3740 "" 0.75 STAGED
```

* Cratchit is unhelpfukl when there are no prov ided template is applicable

  - It seems Cratchit supports BAS account = Amount?

* Cratchit does not show the options to accept generated M entry to SIE (From HAD)

  - command: '-hads'
  - option: '5   ?BAS? Momsrapport 2025-10-01...2025-12-31 826.00 20251231'
  - I guessed on 'Y' but that was wrong!

```text
cratchit:had:vat:summary:ok?>Y

ERROR - Expected Heading + Amount + Date
I Interpreted your input as,
	token: "Y"

Please check that your input matches my expectations?
	Heading = any text ("..." enclosure allowed)
	Amount = any positive or negative amount with optional ',' or '.' decimal point with one or two decimal digits
	Date = YYYYMMDD or YYYY-MM-DD
cratchit:had:vat:summary:ok?>

Options:
VAT Report Summary ok?
0: No
1: Yes
cratchit:had:vat:summary:ok?>
```


## 20260211

Pushed on without any 'thinking'.

I now have generic meta -> columns mapping -> account id for NORDE and SKV basic ok.

I can't help but document what Claude Code proposed and what I implemented for a 'to_org_no' lambda.

* I presented Claude Code with my type cpmmented and an empty to_org_no lambda.
* Claude proposed:

```c++
                auto to_org_no = [](std::string_view sv) -> std::optional<OrgNo> {
  429 -                  return std::nullopt;
  429 +                  // Extract only digits and hyphen
  430 +                  std::string cleaned;
  431 +                  for (char c : sv) {
  432 +                    if (std::isdigit(static_cast<unsigned char>(c)) || c == '-') {
  433 +                      cleaned += c;
  434 +                    }
  435 +                  }
  436 +  
  437 +                  if (cleaned.empty()) return std::nullopt;
  438 +  
  439 +                  // Extract all digits (remove hyphen)
  440 +                  std::string digits_only;
  441 +                  for (char c : cleaned) {
  442 +                    if (std::isdigit(static_cast<unsigned char>(c))) {
  443 +                      digits_only += c;
  444 +                    }
  445 +                  }
  446 +  
  447 +                  // Must be 10 or 12 digits total
  448 +                  if (digits_only.size() != 10 && digits_only.size() != 12) {
  449 +                    return std::nullopt;
  450 +                  }
  451 +  
  452 +                  // If hyphen exists, validate its position
  453 +                  auto hyphen_pos = cleaned.find('-');
  454 +                  if (hyphen_pos != std::string::npos) {
  455 +                    // For 10 digits: hyphen after 6 digits (NNNNNN-XXXX or YYMMDD-XXXX)
  456 +                    // For 12 digits: hyphen after 8 digits (YYYYMMDD-XXXX)
  457 +                    size_t expected_hyphen_pos = (digits_only.size() == 10) ? 6 : 8;
  458 +  
  459 +                    // Count digits before hyphen
  460 +                    size_t digits_before = 0;
  461 +                    for (size_t i = 0; i < hyphen_pos; ++i) {
  462 +                      if (std::isdigit(static_cast<unsigned char>(cleaned[i]))) {
  463 +                        digits_before++;
  464 +                      }
  465 +                    }
  466 +  
  467 +                    if (digits_before != expected_hyphen_pos) {
  468 +                      return std::nullopt;
  469 +                    }
  470 +                  }
  471 +  
  472 +                  // Split: value (first 6 or 8 digits) + control (last 4 digits)
  473 +                  size_t value_len = digits_only.size() - 4;
  474 +                  std::string value = digits_only.substr(0, value_len);
  475 +                  std::string control = digits_only.substr(value_len, 4);
  476 +  
  477 +                  return OrgNo{value, control};
  478                  };
```

* I looked at that and did NOT approve.
* Instead I implemented this:

```c++
  auto to_org_no = [](std::string_view sv) -> std::optional<OrgNo> {
    auto len = sv.size();
    int digits_count{};
    int hyphen_count{};
    for (auto ch  : sv) {
      if (std::isdigit(ch)) ++digits_count;
      if (ch == '-') ++hyphen_count;
    }

    if (true) logger::development_trace(
      "sv:{}:{} digits_count:{} hyphen_count:{}"
      ,sv
      ,len
      ,digits_count
      ,hyphen_count);

    if (hyphen_count>1) return {};
    if (digits_count + hyphen_count != len) return {};
    if (!(digits_count == 10 or digits_count == 12)) return {};

    if (true) logger::development_trace("org-no, candidate has acceptable counts");

    return OrgNo{
        .value = std::string(sv.substr(0,len-4-hyphen_count))
      ,.control = std::string(sv.substr(len-4,4))
    };
  }; // to_org_no
```

Now I can't help but thinking Claude presented something 'less structured'. But hey, I am biased of course!

Now asking Claude to compare it actually found a bug in my code. My code will accept any position for '-' which is a little weak.

* Claude proposed:

```c++
// Validate hyphen position if present
  if (hyphen_count == 1) {
    size_t expected_hyphen_pos = (digits_count == 10) ? 6 : 8;
    auto hyphen_pos = sv.find('-');
    if (hyphen_pos != expected_hyphen_pos) {
      if (true) logger::development_trace("hyphen at wrong position: {} expected: {}", hyphen_pos, expected_hyphen_pos);
      return {};
    }
  }
```

* While I implememnted:

```c++
  // ...
  for (size_t i=0;i<sv.size();++i) {
    auto ch = sv[i];
    if (std::isdigit(ch)) ++digits_count;
    if (ch == '-') {
      if (!(i==6 or i==8)) return {};
      ++hyphen_count;
    }
  }
  //...
```

Again, I feel my code leveraged the existing loop while Claude added a whole extra mechanism?

And of course, Claude Code praised my solution.

```text
 Verification:

  - "556782-8172" (10 digits, 11 chars): hyphen at position 6, len-5 = 11-5 = 6 ✓
  - "197001011234" (12 digits, 13 chars with hyphen): hyphen at position 8, len-5 = 13-5 = 8 ✓
  - "19700101-1234" (12 digits, 13 chars with hyphen): hyphen at position 8, len-5 = 13-5 = 8 ✓

  The code is clean, efficient, and correct. Well done! This is a textbook example of how to write robust validation logic for Swedish organization numbers.

```

I now have:

```sh
[  PASSED  ] 348 tests.
[  FAILED  ] 4 tests, listed below:
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_csv_older_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_NORDEA_0_1_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_BOM_ed_Ok
```

I think next is TableMetaBasedGeneric_sz_SKV_0_0_BOM_ed_Ok? The others are for 'older' SKV and 'empty' but correct NORDEA and SKV. These can wait?

On the other hand - Maybe I should stick to my plan and just carry on? I mean, I should get a beter statement parser mechanism, would I not?

The 'old' SKV statement is tricky!

```text
  ;Ingående saldo 2025-04-01;656;0;
  2025-04-05;Intäktsränta;1;;
  2025-05-12;Moms jan 2025 - mars 2025;1997;;
  2025-05-14;Utbetalning;-1997;;
  2025-06-01;Intäktsränta;1;;
  ;Utgående saldo 2025-06-30;658;0;
```

The first problem is that existing to_date will parse 'Ingående saldo 2025-04-01' to a date!

* to_date simply ignores any non-digits!
* So as long as 'what is left' is a valid date, it will accept it.
* Do I dare to fix this?

Lets make a plan to ensure we do not break anything?

* Make to_date only match some valid dates
* Run all existing test cases
* Run cratchit on some csv statement files and try to generate a VAT Returns?
* Is this sufficient?

Where do I even call to_date in the code?

AHA! I found the culprit!

```c++
OptionalDate to_date(std::string const& sYYYYMMDD_Candidate) {
  OptionalDate result{};
  try {
    if (sYYYYMMDD_Candidate.size()==8) {
      result = to_date(
        std::stoi(sYYYYMMDD_Candidate.substr(0,4))
        ,static_cast<unsigned>(std::stoul(sYYYYMMDD_Candidate.substr(4,2)))
        ,static_cast<unsigned>(std::stoul(sYYYYMMDD_Candidate.substr(6,2))));
    }
    else {
      // Handle "YYYY-MM-DD" "YYYY MM DD" etc.
      auto to_canonical_yyyymmdd = [](std::string_view sv) -> std::string {
        return std::string(sv);
      }
      std::string sDate = to_canonical_yyyymmdd(sYYYYMMDD_Candidate)
      if (sDate.size()==8) result = to_date(sDate);
    }
  }
  catch (std::exception const& e) {} // swallow silently (will result is nullopt)

  return result.and_then([](auto const& date) {
    return (date.ok()?OptionalDate(date):std::nullopt);
  });
}
```

I tried:

```c++
OptionalDate to_date(std::string const& sYYYYMMDD) {
  Date candidate{};
  if (true) {
    std::string digits{};
    std::string seps{};
    const std::set<unsigned char> valid_date_sep{
       '-'
      ,'/'
      ,' '
    };
    for (size_t i=0;i<sYYYYMMDD.size();++i) {
      auto ch = static_cast<unsigned char>(sYYYYMMDD[i]);
      if (::isdigit(ch)) digits.push_back(ch);
      if (valid_date_sep.contains(ch)) {
        if (i!=4 and i!=7) return {};
        seps.push_back(ch);
      }
    }
    if (!(digits.size()==8)) return {};
    if (!(seps.size()==0 or seps.size()==2)) return {};
    if (seps.size()==2 and seps[0] != seps[1]) return {};
    try {
      candidate = ::to_date(
        std::stoi(digits.substr(0,4))
        ,static_cast<unsigned>(std::stoul(digits.substr(4,2)))
        ,static_cast<unsigned>(std::stoul(digits.substr(6,2))));
    }
    catch (std::exception const& e) {
      logger::development_trace("to_date Failed: Exception:{}",e.what());
      logger::design_insufficiency("to_date Failed: Exception:{}",e.what());
    }
  }
  if (candidate.ok()) return candidate;
  return to_deprecate::to_date(sYYYYMMDD);
}
```

But then I got a slew ow tests failing!

```text
[  PASSED  ] 341 tests.
[  FAILED  ] 14 tests, listed below:
[  FAILED  ] AccountStatementTests.ExtractFromSKVCSVNewer
[  FAILED  ] AccountStatementTests.ExtractTransactionAmountNotSaldo
[  FAILED  ] AccountStatementTests.ThousandsSeparatorInSaldoDoesNotAffectTransaction
[  FAILED  ] CSVTable2AccountStatementTests.CsvTableToAccountStatementWithSKV
[  FAILED  ] FullPipelineTestFixture.ImportSkvFileSuccess
[  FAILED  ] FullPipelineTextTests.ImportSkvTextSuccess
[  FAILED  ] MDTableToAccountStatementTestFixture.SKVNewerMDTableToAccountStatement
[  FAILED  ] AccountStatementTableTestsFixture.MappingBasedSKVLike20251120OOk
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_0
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_1
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_2
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_NORDEA_0_1_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_BOM_ed_Ok
```

And ExtractFromSKVCSVNewer now seems to only try:

```text
  TRACE: BEGIN nordea_like_to_column_mapping
  ...
  TRACE: END nordea_like_to_column_mapping
 ```

I could not nake sense of this?

I found the error. The RowMap changed for SKV statement csv table. The previous to_date interpreted 'Utgående saldo 2025-09-30' as a Date and not a Text!

I now again have:

```text
[  PASSED  ] 349 tests.
[  FAILED  ] 6 tests, listed below:
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_0
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_1
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_2
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_NORDEA_0_1_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_BOM_ed_Ok
```

Phiew!!

## 20260208

I have now decided to split the test TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok into its four separarte tests.

I now have:

```sh
...
[  PASSED  ] 10 tests.
[  FAILED  ] 9 tests, listed below:
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_0
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_1
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok__sub_2
...
```

GREAT! I can now focus on making them PASS in the order they are repported.



### 10:00 - Consider Cratchit support for PTA files and processing?

I think it would be a good idea to make Cratchit be able to interact with 'Plain Text Accounting'?

* Be able to parse and produce PTA transaction entries to and from test files.
* Define a mapping from SIE-files to PTA test files?
  - BAS accounts mapped to ```text <accounting type>::<BAS account number> ```
  - Where ```text <accountint type> ``` is Tillgång, Skuld, Kostnad, Intäckt
* Maybe my existing 'csv file parsing piupe-line' can be reused as:

```c++
auto maybe_tagged_amounts = persistent::in::text::monadic::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_detected_encoding_step)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        // and_then to_pta_transactions_step
```

## 20260207 hledger, beancount and other text file based ledger aps

Youtube presented me with videos on hledger and beancount. The are command line text file based double accounting apps!

* [hledger: Robust, friendly, fast,plain text accounting](https://hledger.org)

*"You can import CSV from banks, or enter data via web/terminal interfaces or your favorite text editor. hledger provides powerful reports and scales smoothly from simple to sophisticated needs. It works well with version control, scripts, and LLMs."*

* [Beancount.io : Plain-Text Accounting. Powerful, Precise, Auditable](https://beancount.io) 

*"Write your finances like code—with AI-assisted insights. Beancount makes accounting transparent, scriptable, and ready for the age of LLMs. Automate reports, analyze trends, and build financial workflows that think with you."*

  - But they are a paid servcie?

I wonder if Beancount.io uses hledger under the hood?

* I found this journal entry in their demo

```text
2017-09-08 * "Cafe Modagor" "Eating out with Julie"
  Liabilities:US:Chase:Slate                       -33.71 USD
  Expenses:Food:Restaurant                          33.71 USD
```

* And an hledger journal entry is:

```text
2026-02-07 Min första transaktion
    NORDEA::Kortkonto          73,00 SEK
    expenses:carcharging      -73,00 SEK
```

YES! It is very much the same exact format?

DARN! Have I made cratchit in vain?!

There seems to be a whole community around  [Plain Text Accounting (PTA)](https://plaintextaccounting.org)?

It seems hledger is Haskel based while there is also C++ based [Ledger](https://ledger-cli.org)?

I installed 'brew install ledger' on my macOS.

* Oh BOY - Was it bloated!!
* Or NO - It seems to be only around 7MB?
  - ```text Uninstalling /opt/homebrew/Cellar/ledger/3.4.1_1... (186 files, 6.6MB)```
  - Altgough I counted also dependent code also uninstalled off around 120 MB
* Compared to Haskel based hledger around 340 MB?
  - ```text Uninstalling /opt/homebrew/Cellar/hledger/1.51.2... (16 files, 341.4MB) ```

It seems ledger lives on github at [Ledger: Command-Line Accounting](https://github.com/ledger/ledger)?

* But it has a long list of dependencies?
* First hunch is that it is kind-of bloated (unclean)
* Just as Cratchit has become for now.

I investigated ledger licence 'BSD Public License' and how it compares to Creative Commopns licenses.

* My AI friends told me the closest was Creative Commons Attribution (CC BY).
* BUT: *"...with an important caveat: Creative Commons licenses are not recommended for software, and BSD is a software license. So this is a conceptual alignment, not a drop-in replacement."*

What the heck?! Why have I missed this until now? My AI friends proposed the following short abstract:

```text
* CC licenses are for creative works, not software.
* Missing for software:
  - Explicit patent grants
  - Source vs binary distribution rules
  - Linking/derivative work clarity
  - DRM/store compatibility guidance
  - Software-specific attribution handling
  - Proper warranty/liability disclaimers
* BSD/MIT/Apache are designed for code; CC is fine for docs, media, art.

Rule of thumb: Code → software license, content → CC license.
```

So it seems I should change the licenses I currently have for my source code projects?

## 20260207

Time to dig in and take this mess head on to the next 'better' state. I think I see the light in the end of the tunnel

* Introduce a generic 'to column map' based on nordea_like_to_column_mapping and skv_line...
* Make it generate and return both ColumnMapping and AccountID (to now override and later move to to_statement_id_ed_step.
* Introduce TEST cases for generic 'to column map'
  - For existing test sz texts
  - For new 'single trans entry' sz text NORDEA
  - For new 'single trans entry' sz text SKV

Lets rig this up!

* I now introduced generic_like_to_column_mapping and rigged tests 'MappingBasedGeneric_xxx' with four new csv strings (two for NORDEA vs SKV minimal content and two SKV BOM-ed content)

```c++
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;

      // ...

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      ASSERT_TRUE(statement_table_meta.statement_mapping.column_mapping.is_valid()) << std::format("Expected Valid Mapping for {}",caption);

```

* I also made an effort to name tests in a universal way
* I moved to use TableMeta in statement_table_meta only
* I changed return type and renamed to TableMeta generic_like_to_statement_table_meta(CSV::Table const& table);
* I introduced fallback:

```c++
        std::optional<StatementMapping> generic_like_to_statement_mapping(CSV::Table const& table) {
          // TODO: Generalise code in skv_like_to_column_mapping and nordea_like_to_column_mapping
          //       table -> meta -> StatementMapping
          return {};
        }

        TableMeta generic_like_to_statement_table_meta(CSV::Table const& table) {
          TableMeta result{};
          if (auto maybe_statement_mapping = generic_like_to_statement_mapping(table)) {
            result.statement_mapping = *maybe_statement_mapping;
          }
          else {
            // Fallback while refactoring
            result.statement_mapping.column_mapping = to_column_mapping(table);
          }
          return result;
        }
```

OK, a lot of details. And I am already over my head. But fingers crossed I have rigged it good enough to start implementing generic_like_to_statement_table_meta based on generic_like_to_statement_mapping based on nordea/skv _like_to_statement_mapping?

DARN! Is STILL have no isolated test of only the NEW generic_like_to_statement_mapping!

Well, I can disable the fallback in generic_like_to_statement_table_meta while coding?

After some additional though I separated StatementMapping, ColumnMApping and AccountID as 'paralell' entities and placed them in TableMeta.

```c++
    struct TableMeta {
      StatementMapping statement_mapping;
      ColumnMapping column_mapping;
      AccountID account_id;
    }; // TableMeta

```

I also introuduce producers:

```c++
        std::optional<StatementMapping> generic_like_to_statement_mapping(CSV::Table const& table);
        std::optional<ColumnMapping> generic_like_to_column_mapping(CSV::MDTable<StatementMapping> const& mapped_table);
        std::optional<AccountID> generic_like_to_account_id(CSV::MDTable<StatementMapping> const& mapped_table);
        TableMeta generic_like_to_statement_table_meta(CSV::Table const& table);

        // ...

        TableMeta generic_like_to_statement_table_meta(CSV::Table const& table) {
          std::optional<StatementMapping> maybe_statement_mapping;
          std::optional<ColumnMapping> maybe_column_mapping;
          std::optional<AccountID> maybe_account_id;
          maybe_statement_mapping = generic_like_to_statement_mapping(table);
          if (maybe_statement_mapping) {
            CSV::MDTable<StatementMapping> mapped_table{*maybe_statement_mapping,table};
            maybe_column_mapping = generic_like_to_column_mapping(mapped_table);
            maybe_account_id = generic_like_to_account_id(mapped_table);
          }
          TableMeta result{
             .statement_mapping = maybe_statement_mapping.value_or(StatementMapping{})
            ,.column_mapping = maybe_column_mapping.value_or(to_column_mapping(table))
            ,.account_id = maybe_account_id.value_or(AccountID{})
          };
          return result;
        }

```

It is still convoluted and 'leaky'. But a step in the right direction I think?

* TableMeta is now the top contaoiner to pass.
* I have the seed for first StatementMapping and then ColumnMApping and AccountID from StatementMapping+Table.

Problems are:

* The generic_like_to_statement_table_meta is NOT a maybe (We break TableMeta if any member fails to manifest OK)
* Clinet code is still based on ColumnMapping.is_valid() (And so are all tests).

```c++
      // ...
      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      ASSERT_TRUE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected Valid Mapping for {}",caption);
      // ...
```

Maybe it is time to make separate tests for:

* generic_like_to_statement_mapping
* generic_like_to_column_mapping
* generic_like_to_account_id

AHA! I can make the tests call them separartly instead of calling generic_like_to_statement_table_meta?!

I now have the sevent tests that tries the seven string literal based statement tables to also call for separate production of maype_statement_mapping, maybe_column_mapping and maybe_account_id. I now have:

```sh
[  FAILED  ] 11 tests, listed below:
[  FAILED  ] MonadicCompositionFixture.PathToAccountIDedTable
[  FAILED  ] MonadicCompositionFixture.PathToAccountStatementTaggedAmountsRefactoring1
[  FAILED  ] MonadicCompositionFixture.PathToAccountStatementTaggedAmountsRefactoring2
[  FAILED  ] MonadicCompositionFixture.PathToAccountStatementTaggedAmountsRefactoring3
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_csv_20251120_BOM_ed_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_csv_older_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_NORDEA_0_1_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_BOM_ed_Ok
```

Which means my fallback does not work properly. No, I fixed the header names in generated test file match NORDEA expected names. I am confused when this test started to fail? Anyhow, I MUST focus on my clean-up task!

I now have:

```c++
[  FAILED  ] 7 tests, listed below:
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_csv_20251120_BOM_ed_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_csv_older_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_NORDEA_0_1_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_Ok
[  FAILED  ] AccountStatementTableTestsFixture.TableMetaBasedGeneric_sz_SKV_0_0_BOM_ed_Ok
```

GREAT! All tests but my refactoring tests fail. NOW I have a better grip on things!

Here is a plan:

* Enhance generic_like_to_statement_mapping until it correctly maps my test tables.
* Then Implement generic_like_to_column_mapping the same way
* Finally fill in the blank by implementing generic_like_to_account_id.

How far off is I now?

## 20260206

It just struck me - The 'to accound id:ed' step and mechanism and the to_column_mapping mechanism is the SAME step!

It is also the case the the to_column_mapping is not exposed as a maybe-step. So it seems we should be able to make the 'to account id:ed' step become the meta-analysis we now have in to_column_mapping?

So How can we test this proposal? What if:

* We rename to_account_id_ed_step to to_statement_mapped_step?
* We extend the transfer of ```c++ MDTable<AccountID> ````
  to transfer ```c++ MDTable<TableMeta> ```
* We can do this by moving the meta-code 
  from cvs_to_account_statement/account::statement::maybe::table
  to csv_to_account_id/account::statement *new* namespace meta?

I could for example try this:

1. Rename csv_to_account_id unit to table_to_mapped_statement unit
2. Add table_to_statement_mapped/account::statement::meta
3. Move meta-code from cvs_to_account_statement/account::statement::maybe::table
  - FieldType
  - FieldIx
  - RowMap
  - to_row_map, to_rows_map
  - ColumnMapping
  - TableMeta
  - to_account_statement_table_meta
  - 
4. Make TableMeta carry the new meta values produced by nordea_like_to_column_mapping
5. Make to_account_statement_table_meta produce the meta data
  - Also no longer produce ColumnMapping
6. Make csv_table_to_account_statement_entries use statement meta:
  - From current:

```c++
        AccountStatementEntries entries;

        for (auto const& row : table.rows) {
          auto maybe_entry = extract_entry_from_row(row, mapping);
          if (maybe_entry) {
            entries.push_back(*maybe_entry);
          }
        }
```

  - To instead iterate rix begin,end in statement meta

```c++
        AccountStatementEntries entries;

        for (auto rix = meta.rix_begin;rix<meta.rix_end;++rix ) {
          auto maybe_entry = extract_entry_from_row(rows[rix], meta.mapping);
          if (maybe_entry) {
            entries.push_back(*maybe_entry);
          }
          else {
            // DESIGN INSUFFICIENCY - Should succeed for valdi statement mapping (meta)
          }
        }
```

Can I do it without loosing track? Let's try!

You know what? I think I go for a new step from a new unit?

* New unit table_to_mapped_statement
* New table_to_mapped_statement_step
* New 'receiver step' cvs_to_account_statement/mapped_statement_to_manifetsed_step

The downside to this apporach is that I side-step existing tests and also need a new test for the new step?

ARGH! How hard can this be?!!!

NO, I think I should try and operate on a living patient after all!

Can I implement the changed data transfer without changing any names for now? That is keep:

```c++
  .and_then(account::statement::monadic::to_account_id_ed_step)
  .and_then(account::statement::monadic::account_id_ed_to_account_statement_step)
```

* But to_account_id_ed_step now returns the mapped statement table?
* And account_id_ed_to_account_statement_step takes this mapped statement table?

What would it look like to just do it without any (or as few as possible) name changes?

I already have:

```c++
      inline std::optional<CSV::MDTable<generic::EntryMaps>> to_entries_mapped(CSV::Table const& table) {
        return generic::to_entries_mapped(table);
      }

```

* I introduced namespace to_deprecate and moved existing NORDEA, SKV and generic 'to id:ed' code there
* I removed test_generic_account_statement_csv.cpp unit (now deprecated)

I then needed a new place for AccountID and the meta data now in csv_to_account_statement.

* I introuduced new unit statement_table_meta.hpp
* I moved DomainPrefixedId and AccountID there.
* I replaced passing of ```c++ CSV::MDTable<AccountID> ``` with ```c++  CSV::MDTable<account::statement::TableMeta> ```
* I adapted all call sites accordingly  

I now cleaned up and:

* Replaced all snippets 'account_id_ed' with 'statement_id_ed'
* Renamed src/csv/csv_to_account_id.cpp to src/csv/csv_to_statement_id_ed.cpp

OK. So whats next? I am now passing TableMeta.

```c++
        .and_then(account::statement::monadic::to_statement_id_ed_step)
        .and_then(account::statement::monadic::statement_id_ed_to_account_statement_step)
```

So now we want to:

* Remove the mechanism that produce and use the meta-data in statement_id_ed_to_account_statement_step. 
* And instead produce it at to_statement_id_ed_step and pass it on.

So far we have:

```c++
    struct TableMeta {
      AccountID account_id;
    }; // TableMeta

```

We need to:

* Extend the content.
* Have a function: Table -> TableMeta
* Put this function in statement_table_meta unit

We can clone the existing nordea_like_to_column_mapping into nordea_like_to_statement_mapping and return TableMeta?

* No, this does not work.
* We have the code that creates AccountID in csv_to_statement_id_ed.
* So, ok, we can have the production nordea_like_to_statement_mapping in csv_to_statement_id_ed?
  - Then statement_table_meta unit is just the types (not the production).

AHA! We need a StatementMapping type! With the stuff created by nordea_like_to_column_mapping.

I introduced an empty StatementMapping and provided in TableMeta. Then I tried to move the production of StatementMapping to csv_to_statement_id_ed TU. But that FAILED!

* I can't move the mechanism until it handles ALL statement types
* Because to_statement_id_ed_step returns nullopt on failure.
* Then statement_id_ed_to_account_statement_step is not called.
* So I cant 'fill in the blanks' for other statement tables there (we are cut short)

So I reversed. I have to make the meta-mecanism work for both NORDEA and SKV (that we currently support) before I can move to csv_to_statement_id_ed and statement_table_meta TU:s.

## 20260205

Today I think I want to start with two things based on yesterdays sucess.

* Make nordea_like_to_column_mapping detect forward and reversed dated transaction order tarsn and saldo amounts
* Secure WrappedDoubleAmount::Amount against 'epsilon garbage'

The idea for nordea_like_to_column_mapping mechanism is to:

* Always iterate as-is first-to-last candidate rows.
* Update both a 'newer-to-older' and 'older-to-newer' counters for trans_saldo and saldo_trans counters

I now succeeded to identify trans- vs saldo-columns for newer-to-older ordered statement entry candidates. I also made nordea_like_to_column_mapping bail out early on candidate match failure. In this way the code is stable against e.g. SKV-like account statements.

I now adressed the fragility of WrappedDoubleAmount::Amount against rouding errors. I first cleaned up the class into hpp/cpp design. And introduced private setter and getter for the actual aggergated double value. The idea was to protect the stored double though the setter to never obtain an invalid cents amount value.

I then asked Claude to help me with the prompt:

```text
> I want you to do as follows. First rename m_double_value to cause all access to generate compiler errors. I then want you to repeat the following procedure. 1. Compile the code. 2. Fix the first compilation 
error with a call to the setter or getter as required. 3. Repeat 1 and 2 until the code compiles. Then 4. Fix the linker error by implementing the setter and getter in the cpp-file. What are your prosed 
step-by-step strategy to do this? 
```

Claude code performed this task quite OK. Algthough maybe a bit too much to the letter? If I had done it myself I may have been tempted to actually dont use a getter.

I did not commit the code that Claude generated. Instead I asked chatGPT about it. And it seems I have to think about the folloiwng:

* Having a double based Amount will always cause cents drifting in arithmetic expressions.

```c++
  0.1 + 0.2 != 0.3

  Amount a = 0.01;
  for (int i=0; i<1000; ++i)
      a += 0.01;

  // Your rounding helps — but it does not eliminate intermediate precision drift.
  // With doubles, rounding after each step helps — but you still rely on floating behavior between operations.
```

* We don't know what rouding to apply!

```text
Rounding Strategy Needs Definition
You use:

  std::round(value * 100.0) / 100.0;

That is: round half away from zero

But financial systems often require:
* banker’s rounding (round half to even)
* configurable rounding mode
* currency-dependent precision

Consider making rounding policy explicit and configurable.
```

* Rounding for * or / with scalar may accumulate errors?

```text
Multiplication by Scalar — Rounding Timing

  Amount * scalar

If scalar has many decimals, rounding at each operation may accumulate error. Integer-cents storage avoids this entirely.
```

So this is tricky! I already adressed the rounding Issue in some report generating code. So I risk breaking that code if I hard code rounding on the stored double directly?

Decision: I reject the round-on-write for Amount for now.

But I should try to remember that I can refactor the existing Amount to use a cents amount internally directly. In this way I may be able to refactor to a safer state wihtout introducing say the WrappedCentsAmount, IntCentsAmount, UnitAndCents

I may come back to the comment in main to guide what to do?

```c++
      // TODO: Clean up the 'amount strong type' mess?
      //       For now we have a lot (to many) of experimental strong-type-amounts?
      //       But keep currency amounts strongly typed while being able to
      //       apply correct rounding to integer amounts (possibly applying different roundings for different domains?).
      //       E.g., Swedish tax agencly may define one way to round to integer amounts? While BAS accounding, banks
      //       invoices etc. may apply (call for) roduing with truncation, floor, ceil etc?

      // For now, go through UnitsAndCents as the intemediate candidate type.
      // CentsAmount -> UnitAndCents -> Amount -> C++ double -> C++ int (applying std::round)

```

I decided to remove also the rounding in Amount + and += operations. I don't trust this will not break code I already have that applies rouding. To still make the equal operations in trans- vs saldo-amount detection of nordea_like_to_column_mapping I instead made Amount::operator== be 'weaker' to not let rodung creap affect the result.

```c++
    bool Amount::operator==(Amount const& other) const {
      // return this->m_double_value == other.m_double_value;
      // Be 'kind' to not let floating point rouning errors affect the equality of cent-based currency amounts
      constexpr double EPS = 0.00001;
      return std::fabs(this->m_double_value - other.m_double_value) < EPS;      
    } // operator==
```

Now chatting with my AI friends again even this was NOT good! It is a learning experience to see how AI totally fails to understand what the end goal is and just riffs off on the current literal context.

I came to the conclusion that operato== should focus on 'cents-equal'.

```c++
    bool Amount::operator==(Amount const& other) const {
      // return this->m_double_value == other.m_double_value;
      // Hard code cents-based-equal
      return std::llround(m_double_value * 100) ==
            std::llround(other.m_double_value * 100);
    } // operator==
```

With this:

* My NORDEA like parsing still works.
* And I have not introduced any implicit rouding into existing Amount arithmetics.

So back to nordea_like_to_column_mapping. It seems what is missing is to determine what column and/or columns to use for the 'caption' of the transaction entry from the account statement.

What we can do next is to get the heading text of the common text columns? And then be tempted to match these to the known values in current NORDEA account statement and take it from there? In this way:

* We can use the common text column headers as a key to what account statement we have?
* Or even use the heading text of all the common columns as a key?

Lets try and see where this gets us?

I created a map from index -> text from row 0 and used as key to accept hard coded currently known NORDEA header content.

```c++
          std::vector<std::string> text_coumn_values{};
          std::map<FieldIx,std::string> text_columns_map{};
          for (auto ix : common.ixs.at(FieldType::Text)) {
            auto const& text = rows[0][ix];
            text_columns_map[ix] = text;
            text_coumn_values.push_back(text);
          }

          // ...

          const auto NORDEA_TEXT_MAP = std::map<FieldIx,std::string>{
             {4, "Namn"}
            ,{5, "Ytterligare detaljer"}
            ,{9, "Valuta"}};

          if (text_columns_map == NORDEA_TEXT_MAP) {
            result.description_column = 4;
            result.additional_description_columns.push_back(5);
          }

```

So now the mapping is actually complete! And I am down to only 5 failing tests.

```sh
[  PASSED  ] 336 tests.
[  FAILED  ] 5 tests, listed below:
[  FAILED  ] MonadicCompositionFixture.PathToAccountIDedTable
[  FAILED  ] MonadicCompositionFixture.PathToAccountStatementTaggedAmountsRefactoring1
[  FAILED  ] MonadicCompositionFixture.PathToAccountStatementTaggedAmountsRefactoring2
[  FAILED  ] MonadicCompositionFixture.PathToAccountStatementTaggedAmountsRefactoring3
[  FAILED  ] GenericStatementCSVTests.NORDEAStatementOk
```

But there is still much to do!

* Should I refactor out the meta-creation code from  nordea_like_to_column_mapping?
  - The key new thing seems to be generating the common rows map?
  - The identified transaction candidates range is also good to have?
* Should I make 'to account id:ed' apply this meta creation to identify NORDE?
  - And then extend it to identofy SKV?
* Or should I somehow inject account id after the mapping?
  - But then I have no way of doing this as I only pass ColumnMapping (not the meta data)
    along to the next step csv_table_to_account_statement_entries (from csv_table_to_account_statement_step)
* It seems the easiest next step is to pass the new meta as is introduced by TableMeta to_account_statement_table_meta?

AHA, I already have the architecture provided by TableMeta to_account_statement_table_meta?!

But where and how does 'account ID' end up?

1. It is provided to:

```c++

      std::optional<AccountStatement> account_id_ed_to_account_statement_step(CSV::MDTable<AccountID> const& account_id_ed);

      // Calling...

      std::optional<AccountStatement> csv_table_to_account_statement_step(
          CSV::Table const& table,
          AccountID const& account_id);

```

2. And injected to:

```c++
        AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};

        logger::development_trace("Creating AccountStatement with {} entries and account_id: {}",
          maybe_entries->size(), account_id.to_string());

        return AccountStatement(*maybe_entries, meta);

        // account_id_ed_to_account_statement_step

```

So some where in that chain it seems we should be able to inject account id based on our new meta data?

But first I think I will 'tear apart' **nordea_like_to_column_mapping** into meta-creating part and meta-using part?

NO. This was premature. I did get this far:

```c++
          struct TableMeta {
            std::ptrdiff_t begin_rix;
            std::ptrdiff_t end_rix;
            RowMap common;
          }; // TableMeta

          TableMeta table_meta{};          

```

The approach feels kind-of ok. But the code got a bit quirky.

* Does this code scale to be usable for SKV-like input also?
* For the above yes - But the header-based check for NORDEA will NOT.
  - SKV style inout does not use column header names.
* Can I use 'common' as key to inferr account id?
* I CAN already use common to identify a valid statement table
  - At least in combination with trans- vs saldo- checking.
* But what is a more secure way to identify NORDEA, SKV or others using the same mechanism?

So what is a better approach?

* I could make account id a mebre of TableMeta?
* Then I am free to invent whatever quirky mechanism to determine different statement tables?
* And I can always fall back to 'anonumous' account id for a valid statement table?

## 20260204

A new day and a new idea. If I add the row map for each transaction entry candidate with 'intersection'. So if I 'accumulate' or 'fold' each candidate entry row map, then the resulting row map will only contain the common map of all rows.

I implemented a 'fold' into a common row map and got for my NORDEA csv:

```text
  TRACE: common : Empty: 10 Date: 0 Amount: 1 8 Text: 4 5 9
```

Which is quite informative and also took care of intermittent OCR numbers getting confused for amounts.

Now it seems we can dissmiss the table if we don't have one date, two amounts and at least one text in common?

DARN! The NORDEA CSV file lists the transaction in falling date order!

I wonder if I mayy be tempted to actually create a new table woth only the columnn I have identified as candidfates for account statement entries?

* Then I can instantiate a normalised account statement csv table?
* In this table I know where date, amlunts and text columns are.
* And processing for transaction vs saldo amounts can be done more easy?
* On the other hand. I then create a nomrlaised table BEFORE I have idnetified which is transaction and which is saldo amount?
  - This is not a good 'nomrlaised' table.

No, I think it is better if we sort the CSV table as soon as we have identified valid candidates and know where the date is.

WAIT! We can't sort the table!

* We risk getting transaction on the same date in the wrong order.
* That is, we need to either keep the order or reverse the order!
* In this way the saldo will reflect the previous saldo + trans.
* Otherwise 'previous' saldo may end up in the wrong location!

WAIT. I have the core logic wrong anyhow. I implemented two running saldos. But I don't think I will be aböle to tell which one is transaction and which one is saldo from looking at those two? I will basically just get two salos that increment with the same amount each step.

So what do we need?

* I need the first and second amount as-is!
* Now I can check if one 'after' amount is the sum of one 'before' amount and the other 'after' amount.

I madew the change. I also introduced a flag to idnetify if the transactios are in falling date order. I now get a better matching:

```text
TRACE: first_trans_second_saldo_count:7 first_saldo_second_trans_count:0 undetermined_amounts_count:4
```

I still have undetermined matches though. I wonder if I am missing something here?

* What if I have 2 or more transactions on the same date?
* I still need to check them in reverse order to detect tarns vs saldo amount changes in the right order!

So maybe what I need to do is to process the list one time first to last. If I fail to make sense of ALL amounts, I then try the reverse order as-is.

In this way I duck the problem of failing to reverse a list that must be?

I hard coded traversing the entries in reverse order. I still initially got 4 undetermined matches. This turned out to be due to 'epsilon garbage' in my floating point based Amount types!

I asked Claude Code to find and fix the problem. And it actually did! I am impressed.

```c++
namespace WrappedDoubleAmount {

  class Amount {

    // Amount += Amount
    Amount operator+=(const Amount& other) {
      this->m_double_value += other.m_double_value;
      // Clean out roudning errors
      this->m_double_value = std::round(this->m_double_value * 100.0) / 100.0;
      // TODO: Replace this amount with something like IntCentsAmount?
      return *this;
    }

```

This is BAD!!!

* I can NOT trust any Amount arithmetic results to not have 'epsilon garbage'?!
* The fix is for '+' that uses '+='.
* But what about '-', '*' and '/'?

SHould I refactor into Amount as in IntCentsAmount? Is that even working?

I REALLY have made som very bad decisions when I started off cratchit with Double amounts!!

## 20260203

I am loosing my confidence that I will be able to clean this code up. That I will be able to figure out a design that I am satisfied with. It is super frustrating that every attempt I do to make the code better, just adds more mess and unclearity to it? What is going on?

Anyhow, I will struggle on and hope this is a hill to climb. And even if I do it inefficiently, I will be able to make progress to something better.

Now I am going to just add code to experimentally parse the NORDEA-like accountment statement file and see what comes out.

* I will add the code to detect_columns_from_data
* I will add code before the SKV parsing code

I now have introduced skv_like_to_column_mapping and nordea_like_to_column_mapping. I gutted out older code to its original content and placed in namespace to_deperecate. And I now have three 'to column mapping' options scanned by top to_column_mapping. Also added seed for testing to_column_mapping mechanism.

I now discovered I already had the test_csv_table_identification unit. So I moved my new test there.

* Now I at least managed to have related tests at the same place.
* These tests now adress CSV table identification based on the two mechanism I have introduced but barely use.
  1. account::statement::maybe::table::to_column_mapping(table);
  2. account::statement::maybe::table::to_account_statement_table_meta

Good. Some anchoring of lose pieces now done?

I now tweaked testing to gain more focus.

* Test prefix 'MappingBased' for test of account::statement::maybe::table::to_column_mapping
* Test prefix 'MetaBased' for test of account::statement::maybe::table::to_account_statement_table_meta

WAIT! Does those two mechanism use the same 'to column mapping'?

Jupp! 

*       csv_to_account_statement//account::statement::maybe::table::to_account_statement_table_meta 
* calls csv_to_account_statement//account::statement::maybe::table::to_column_mapping

```c++
        TableMeta to_account_statement_table_meta(CSV::Table const& table) {
          TableMeta result{};
          result.trans_row_mapping = to_column_mapping(table);
          return result;
        } // to_account_statement_table_meta

```

I now realise one problem why I have such a hard time to make sense of this MESS is that I am semi-dyslectic? I failed to see that both mechaniosm are in the SAME cpp unit AND name spae *sigh*!

But are we now using code form 'to account id:ed'?

NO. My memory is at fault here. The 'to account id' is based on hard coded parsing for NORDEA and SKV. No 'mapping' or 'meta analysing' going on here!

Ok, so we clarified that confusion, GOOD!

I wonder, can I add a new way to identify account id after meta analysis?

1. Inferr account ID based on analysed table and inject into account statement?
2. Remove 'account id_ed' step as discussed before? 

In this way I can first make 'GenericStatementCSVTests.NORDEAStatementOk' PASS. Then refactor away 'to account id:ed' step?

I think I shall first implement nordea_like_to_column_mapping.

I now made MappingBasedNordeaLikeOk call only nordea_like_to_column_mapping. Now I can implement it unitl the test pass.

FOCUS!

Found out that NORDEA csv table has an 'empty' 10:th column? This will be tideous (combinatorial explosion)!!

AHA - The NORDEA csv file uses ';' as an 'end marker' and NOT as a 'separator'

Should I trim the table from trailing empty columns?

DARN! The last row does NOT detect column 11 (why?). My code to require column 11 to be ampty fails due to this...

* csv_row_to_fields ok?
* Key::Path ok?

I asked Calude Code to fix csv_row_to_fields (it skipped last empty field if at end of string).

Aftert some work I now have nordea_like_to_column_mapping able to identify all 12 transaction entry candidates.

* I think I am to harch demanding 11 coumns with the last one empty
  - I can be much more lose to be able to parse variations on table format and still be safe?
* I get false positioves on OCR matrched as amount
  - Can I require an amount to be with decimals?
  - Can I detect an OCR as a 'largeer than life' integer?

## 20260202

I still feels STRONG resistance from the code against refactoring! I now want to try the approach to focus on function before form. So I know I have tests that fail for account statement file parse. And I have previosuly reasoned what processing steps I like to use.

So if I wait with the BOM and encoding confidence processing and take a look at the actual parsing of the table. Where is the code at currectly?

```c++
        .and_then(account::statement::monadic::to_account_id_ed_step)
        .and_then(account::statement::monadic::account_id_ed_to_account_statement_step)
```

I suspect the code of intereset happens in these two steps?

The account ID seems to be hard coded to look for NORDEA or SKV only?

```c++
        if (account::statement::NORDEA::is_account_statement_table(table)) {
          std::string account_number = account::statement::NORDEA::to_account_no(table).value_or("");
          logger::development_trace("to_account_id_ed_step: Detected NORDEA account: '{}'", account_number);
          return CSV::MDTable<AccountID>{AccountID{"NORDEA", account_number}, table};
        }

        if (account::statement::SKV::is_account_statement_table(table)) {
          auto maybe_org_number = account::statement::SKV::to_account_no(table);
          std::string org_number = maybe_org_number.value_or("");
          logger::development_trace("to_account_id_ed_step: Detected SKV account for org: '{}'", org_number);
          return CSV::MDTable<AccountID>{AccountID{"SKV", org_number}, table};
        }        
```

I found that the maybe version of account_id_ed_to_account_statement_step was defined in the header file. I moved definition to the cpp-file. But then I got a strange linkert error!

```sh
duplicate symbol 'account::statement::maybe::account_id_ed_to_account_statement_step(MetaDefacto<DomainPrefixedId, CSV::Table> const&)' in:
    /Users/kjell-olovhogdahl/Documents/GitHub/cratchit/build/Debug/CMakeFiles/cratchit.dir/src/domain/csv_to_account_statement.cpp.o
    /Users/kjell-olovhogdahl/Documents/GitHub/cratchit/build/Debug/CMakeFiles/cratchit.dir/src/main.cpp.o
ld: 1 duplicate symbols
```

WOT? From what I cpould see I hade only one definition? I asked Claude Code and got the answer:

```sh
⏺ Now I understand the issue. The problem is on line 8 of src/main.cpp:

  #include "zeroth/main.cpp" // zeroth variant while refactoring into 'this' variant

  This line is including a .cpp file, which is a very bad practice. When you include a .cpp file, all the code in that file and anything it pulls in gets compiled twice:
  1. Once as part of main.cpp
  2. Once when CMake compiles it as a separate translation unit

  The function account_id_ed_to_account_statement_step is properly:
  - Declared in src/domain/csv_to_account_statement.hpp:144
  - Defined in src/domain/csv_to_account_statement.cpp:428-453

  But since csv_to_account_statement.cpp is likely being compiled both:
  - As its own object file
  - Indirectly through includes when main.cpp includes zeroth/main.cpp

  This creates the duplicate symbol.
```

Which is totally WRONG! I told it so and to its credit, Claude Code now actually figured out what I alerady had started to suspect. That My change required a rebuild of older object files with the symbol 'inlined'.

```text
⏺ Found it! The git diff shows the issue. You recently refactored the function account_id_ed_to_account_statement_step:

  - Before: It was defined as inline in the header file csv_to_account_statement.hpp
  - After: You moved it to the .cpp file and changed it to a regular declaration in the header

  The problem is that old object files still contain the inline version! When it was inline in the header, it was compiled into every .o file that included that header (including main.cpp.o). Now you've moved
  it to the .cpp file, so it's also in csv_to_account_statement.cpp.o.

  Solution: Clean rebuild

```

I now struggled a while to tell cmake to do the rebuild. It was SUPER FRUSTRATIMNG!

Anyhow, Claude AI told me the solution that worked. Just remove the 'build/Debug' output folder.

```sh
rm -rf build/Debug && ./run.zsh --nop
```

Note: It turned out this actually requires also an 'init_toolchain' call to restore the cmake configuration.

Back to my original task! What does the code do?

```c++

        // account_id_ed_to_account_statement_step
        // ...
        auto maybe_entries = csv_table_to_account_statement_entries(table);

        // csv_table_to_account_statement_entries
        // ...
        auto mapping = table::to_column_mapping(table);

        for (auto const& row : table.rows) {
          auto maybe_entry = extract_entry_from_row(row, mapping);
          // ...

        ColumnMapping to_column_mapping(CSV::Table const& table) {
            for (auto project : detail::column_mapping_projectors) {
                ColumnMapping mapping = project(table);
                if (mapping.is_valid()) {
                    return mapping;
                }
            }
            return {};
        } // to_column_mapping
```

And drilling down I finally find:

```c++
          constexpr std::array<ColumnMappingFn, 1> column_mapping_projectors = {
              &to_column_mapping
          };

          // And ...

          ColumnMapping to_column_mapping(CSV::Table const& table) {
            ColumnMapping result{};
            result = table::detect_columns_from_header(table.heading);
            if (!result.is_valid()) {
              result = table::detect_columns_from_data(table.rows);
            }
            return result;
          } // to_column_mapping


          // And ...

        ColumnMapping detect_columns_from_data(CSV::Table::Rows const& rows) {
          ColumnMapping mapping;

          logger::scope_logger log_raii(logger::development_trace,"detect_columns_from_data");

          if (true) {
            // Refactored to use per row RowsMap analysis

            auto rows_map = to_rows_map(rows);
            // ...

        // And ...

        RowsMap to_rows_map(CSV::Table::Rows const& rows) {
          RowsMap result{};

          for (auto const& row : rows) {
            result.push_back(to_row_map(row));
          }

          return result;
        }


        // And ...

        RowMap to_row_map(CSV::Table::Row const& row) {
          RowMap result{};

          for (unsigned i=0;i<row.size();++i) {
            result.ixs[to_field_type(row[i])].push_back(i);
          }

          return result;
        }

        // And ...

        FieldType to_field_type(std::string const& s) {
          if (s.size()==0) {
            return FieldType::Empty;
          }
          else if (auto maybe_date = to_date(s)) {
            return FieldType::Date;
          }
          else if (auto maybe_amount = to_amount(s)) {
            return FieldType::Amount;
          }
          else if (text::functional::count_alpha(s) > 0) {
            return FieldType::Text;
          }
          return FieldType::Unknown;
        }
        
```

So deep, deep, deep down I have implemented a projector from each row to a vector of IDs (enums) for the types of fields identified.

So how do wer clean this MESS up?

* I would like to leverage detail::column_mapping_projectors to add something that can parse NORDEA
  (For now NORDEA is parsed based on identified header text?)
* The interesting code is in refactored part of detect_columns_from_data (for SKV parsing)

So how can I refactor this without breaking anthing while doing it?

Maybe I should just add a new to_column_mapping version?  And add to:

```c++
          constexpr std::array<ColumnMappingFn, 1> column_mapping_projectors = {
              &to_column_mapping
          };
```

If I add it before the one that is there now it will override existing code. But existing code is still there as long as my added code returns nullopt?

But I also have:

```c++
        struct TableMeta {
          ColumnMapping trans_row_mapping;
        }; // TableMeta

        // ColumnMapping to_column_mapping(CSV::Table const& table);
        TableMeta to_account_statement_table_meta(CSV::Table const& table);
```

This seems to be something I initiated but never did anything with?

This also clashes with 'csv_to_account_id' unit code:

```c++
    namespace generic {

      enum class ColumnType {
         Undefined
        ,Empty
        ,Text
        ,Amount
        ,Date
        ,Unknown
      }; // ColumnType

      // ...

      inline ColumnType to_column_type(std::string field)

      // ...
      using ColumnMap = std::map<int,ColumnType>;

      // ...
      enum class EntryType

      using EntryMap = MetaDefacto<EntryType,ColumnMap>;
      using EntryMaps = std::vector<EntryMap>;

      inline EntryType to_entry_type(EntryMap const& entry_map)

      // ...
      inline EntryMap to_entry_map(CSV::Table::Row const& row)

      // ...
      inline std::optional<CSV::MDTable<EntryMaps>> to_entries_mapped(CSV::Table const& table)


```

What a MESS! There, I said it again. Can I imagine a goald design and force what I have into it?

* I want to remove 'accoubnt id inference' from before parsing
  - I may be temped to inferr it while analysing the account statement table?
* I want to base the mechanism on the latst SKV account statement table parsing in 'from content'
  - See the injected 'refactored' code
* I am tempted to have a table -> meta-defacto-table with account statement meta data added.
* If so, then  this in fact replaces the 'to accoint id:ed step?
  - table -> 'to identified meta data + table' -> account statement

But I am still in the fog about how to get to this while having my existing code working?

AHA! I can implement a paralell pipe path (and remove the current one when it works!)

1. Implement paralell 'new' pipe

```c++
        .and_then(account::statement::monadic::to_with_statement_meta_step)
        .and_then(account::statement::monadic::with_statement_meta_to_account_statement_step)
```

2. Pick existing code into 'new' pipe
3. Mark 'old' code as 'deprecated' as I go
4. Remove all old code when new steps work

```c++
  // DEPRECATED...

        .and_then(account::statement::monadic::to_account_id_ed_step)
        .and_then(account::statement::monadic::account_id_ed_to_account_statement_step)

```

I can actually doi the same when I remove the 'confidence passing' steps.

1. Implement non 'confidence passing' pipe

```c++
//       auto maybe_tagged_amounts = persistent::in::text::monadic::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::text::monadic::istream_ptr_to_bommed_byte_buffer_step)
        .and_then(text::encoding::monadic::bommed_byte_buffer_to_with_detected_encoding_step)
//        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
```

2. Remove the no longer used 'confidence passing code

```c++

//       auto maybe_tagged_amounts = persistent::in::text::monadic::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
        .and_then(text::encoding::monadic::to_with_detected_encoding_step)
//        .and_then(text::encoding::monadic::to_platform_encoded_string_step)

```

## 20260131

I have been thinking about what I wnat and could do to enhance the account statement csv file parse and transform to tagged amounts.

* I can make the 'istream to buffer' to 'buffer to encoded text' pass a BOM
  - persistent::in::text::maybe::istream_ptr_to_byte_buffer_step -> BOMedBuffer
  - BOMedBuffer -> text::encoding::maybe::to_with_threshold_step_f(100)
  - BOMedBuffer + Threshold -> text::encoding::maybe::to_with_detected_encoding_step
* I could remove the passing of threhold around while I am at it
* I can make to_with_detected_encoding_step just transform a BOM to an Encoding ID and be done with it

I also want to adress the 'to accound ID:ed step

* I want to identify NORDEA based on content (Not header)
  - Mirror the analysis for identification of SKV account statement
  - All rows has the same column count
  - One header row and all other rows are statement rows
  - ALl Statement rows has Date in column 0, Amount in col 1 and caption in column '

I am tempted to use the NORDEA header to project a mappint to optional fields.

0. Bokföringsdag
  - Required Date.
1. Belopp
  - Required transaction amount
2. Avsändare
  - Optional 'from'
3. Mottagare
  - Optional 'to'
4. Namn
  - I am unsure what this is? Is it the name of the sender, receiver or what?
  - Maybe it is always the name of the 'other part'?
  - NO! This is in fact the required caption!
5. Ytterligare detaljer
  - Seems redundant?
6. Meddelande
  - This seems to be the OCR for my own payments of invoices
  - Make this into optional OCR?
  - We can then later project it to OCR if the content matches an OCR?
  - Consider some OCR data type and project mechanism?
7. Egna anteckningar
  - This is the txt I can enter myself for my own transactions
  - Make into an optional 'meta info' or 'own note'
8. Saldo
  - Some rows contains this saldo amount (bot not all do!)
  - Make into optional saldo?
9. Valuta
  - This is always SEK
  - Cratchit can warn if this is not SEK?
  - Or sghould I refuse to use the entry if this field is not empty and is not 'SEK'?

It seems to me I now know the following:

* It is a valid account statement table if:
  - There is a range of entries that all have a date,amount and some non empty text.
  - An entry is valid with two amounts (potential trasnaction amount + saldo amount)
* It is a valid range of transaction entries if:
  - All date are strict increasing or decerasing (both allowed)
  - Any optional saldo amount is the previous saldo amount + transaction amounts up to this one

It seems I would be tempted to implement a 'transaction entries checker'?

* Sort entries on date (normalise low to high)
* Loop through with a running saldo
  - Use an optional running saldo to handle any initial unkn own saldo

AHA! We need a mechanism to handle the case where all transaction entries have two amounts!

* For the first entry we initiate both amounts and saldo candidate and transaction amount candidates.
  - trans_amount_candidates = IndexDet (initiate with both first and second)
  - saldo_amount_candidates = IndexSet (initiate with both first and second)
* For second entry we check which second amount is the sum of the two first amounts plus one of the second entry amounts.
  - If we have only one valid amount, this MAY be an 'Saldo in or out' entry.
  - prev[first] + prev[second] + curr[first] = curr[second] ==> column first is transaction and secodn is Saldo
  - prev[second] + prev[first] +  curr[second] ==> Column first is Saldo and second is trasnaction amount
  - For a match we adjust trans_amount_candidates and saldo_amount_candidates to reflect the match
* Wow - it seems tricky to make an algorithm that can handle both 'saldo only' entries AND optional-saldo entries?
  - A saldo only entry may also be a transaction only entry.
* Consider to have a running total for both amount values
  - We always accumulate acc[first] and acc[second]
  - We have 'maybe trans column ix' and 'maybe saldo column ix'
  - If curr[first] + acc[first] = acc[second] then first is transaction and second ios Saldo
  - If curr[second] + acc[second] = acc[first] then second is transaction and first is saldo
  - Otherwise the entry is NOT a valid complement to the previous ones
* It seems we may be able to handle an amount column that can host BOTH transaction and saldo amout?!
  - The older SKV statement file uses column 2 as Saldo or Transaction depending on entry type
  - A saldo entry has empty date column column[date]=0
  - A saldo entry has the date baked into the caption column
  - A saldo entry has caption 'Ingående saldo' or 'Utgående saldo' + date on the form yyyy-mm-dd

Should I 'bite the bullet' and design an algorithm that can process also the older format of SKV stetment files?

It also seems the 'account ID inference' should be done AFTER having analysed the table?

* Only when we have a valid account statement table can we try and inferre an account ID?
* We can inferr NORDEA from either column ix for Date,amount,text and/or the header text for those columns?
  - column[date] = 0, column[amount]=1, column[caption]=4
  - head[date] = 'Bokföringsdag'
  - head[amount] = 'Belopp'
  - head[caption] = 'Namn'
* We can inferr SKV from identified content format *new*
  - column[date]=0,column[amount]=2, column[caption]=1
  - ALL rows have 4 columns
  - row[0].column[1] is a swedish organisation number 'xxxxxx-xxxx' (or 'xxxxxxxx-xxxx'?)
* We can inferr SKV from identified content format *older*
  - column[date]=0,column[amount]=2, column[caption]=1
  - ALL rows have  5 columns


So bottom line is that I am tempted to:

* Remove piping of 'confidence' (just hard code the constant for call to ICU)
* Pass BOM from 'istream read' to 'inferr encoding'
* Implement enhanced account statement table identification.
  - A valid range of statement entries
  - All entries of the same type:
    - trans_only_entry
    - trans_or_saldo_entry
    - trans_maybe_saldo_entry
  - Account ID inferred from valid statement table ONLY
    - From date,amount,caption column index
    - From heading text of date,amount,caption column
  - Be able to produce optional meta-data
    - Maybe 'our ornanisation number' (from SKV account statement)
    - Maybe 'from' identifier (BBAN,PG,BG)
    - Maybe 'to'  identifier (BBAN,PG,BG)
    - Maybe 'OCR' (E.g. NORDEA column 'Meddelande' if valid OCR)

I am starting to accept that this takes time beacuse it IS complicated. It may be worth doing though?

* To support more cratchit automation if need stuff like 'to', 'from' and 'OCR'.

Another side idea is that after having processed an account statement into HADs I can:

* Cross reference events to identify 'amount flows' and suggest 'event sameness'?
  - Two registered events (transactions) are 'the same' if only bank-days apart and same amount?
  - An 'out' Transaction amount flow with no intiating 'debt' is either a direct/card purchase, a subscrioption or a missing invoice.
  - We can suggest 'missing invoice' to enable user fill in what caused the debt.
  - We can look for a registered debt either in VAT accounts or the 'Leverantörsskulder'
* We can identify an in-flow of VAT to have identified steps?
  - An initiating report of VAT debt (the VAT returns report)
    - Registered in 'Momsfordran' or 'Momsskuld' BAS account
  - A VAT debt confirmation (In SKV account stement as an accepted VAT return report)
  - A VAT debt payment transaction (In SKV account statement as a VAT Payment transaction)
  - An in-amount transaction in bank account statement
  - This flow enables us to know to settle the BAS accounts VAT -> Bank Account
* We can require an out-flow to have identified steps?
  - An initiated debt (invoice) (BAS series F)
  - A debt settlement (payment) (BAS series E)

Can we imagine a seed to a general processing mechanism in cratchit that 'fills in the blanks'?

* Identify an in-flow or out-flow
* Identify missing steps
* fill in the steps as we go though account statements
* Propose HAD aggregates for event 'sameness' (account -> account)
* Propose HADs for missing steps
  - Proposed invoice
  - Proposed Payment
  - Proposed SKV VAT debt acceptance
  - Proposed SKV VAT payment

I wonder, can we even imagine a process that based on proposed 'gaps' in 'amount flows' is able to iterate until now more gaps are identified. ANd then have the user act on the identified gaps in the form of HADs?

Also, it seems we have two types of 'event aggregation'? 

* Voucher aggreation of 'same event' on debit/credit accounts?
  - SKV account <-> Bank Account (for VAT, Tax, fees) (cross account statement events)
  - Between two bank accounts (cross account statement events)
* Voucher aggregation of 'same flow' over time (voucher -> voucher -> voucher)? 
  - Debt event to payment event (event that caused account statement entry)
  - Payment event to settlement event (account statement entry that inferres BAS account settlement)
  ...

I got another idea, to project a statement table meta data to a hash. And then match the hash to an accoint ID? My thinking stems from the observation:

* We can inferr NORDEA from column ix for Date,amount,text and/or the header text for those columns?
  - column[date] = 0, column[amount]=1, column[caption]=4
  - head[date] = 'Bokföringsdag'
  - head[amount] = 'Belopp'
  - head[caption] = 'Namn'
* We can inferr SKV from identified content format *new*
  - column[date]=0,column[amount]=2, column[caption]=1
  - ALL rows have 4 columns
  - row[0].column[1] is a swedish organisation number 'xxxxxx-xxxx' (or 'xxxxxxxx-xxxx'?)
* We can inferr SKV from identified content format *older*
  - column[date]=0,column[amount]=2, column[caption]=1
  - ALL rows have  5 columns

If we can map the meta-data above to a unique hash value, then we can map this hash to an account ID.

* The column ix:s for date,amount,caption
* The heading text for column date,amount,caption
* the histogram for the column counts of statement entries
  - Or the column count that ALL statement entries have?
  - We require all actual sttement entries to all have the same column count?
  - YES. The three statement tables I know of all have all rows the same column count!
* So NO histogram but: The column count of ALL rows.

But for this to work we need a hash function that produces the same hash on all builds!

* std::hash produces different hash values on different platfroms and tool chains.

My AI friends tells me I can implement a constexpr FNV-1a hash.

Now how can I evaluate if my idea of table -> meta -> hash -> switch statement?

* I can first create a dummy constexpr hash function that takes std::span os std::bye.
* I can then create a proposed struct with the meta data I want as inout
* I can then create a constexpr function that transform the meta data into an std::array (I know the size)
* I can then create constants for meta-data instances I recognise
* I can then try to switch on runtime meta-data to compile time generated hash values.

I made some progress but decided to reverse and think about this some more. I do NOT want to check in an 'ansats' that I do not know I can use as a seed (and not end up as dead code and future confusion).

I did implement this header onluy hash OK (should I decide to come back and use this apporach).

```c++
#include <span>

namespace FNV1a {
  inline constexpr uint64_t OFFSET_BASIS_64 = 14695981039346656037ull;
  inline constexpr uint64_t PRIME_64        = 1099511628211ull;

  constexpr uint64_t hash64(std::span<const uint8_t> data,uint64_t hash = OFFSET_BASIS_64) {
      for (size_t i = 0; i < data.size(); ++i) {
          hash ^= data[i];
          hash *= PRIME_64;
      }

      return hash;
  } // hash64

  struct hasher64 {
      uint64_t value = OFFSET_BASIS_64;

      constexpr void update(std::span<const uint8_t> data)
      {
          for (uint8_t b : data)
          {
              value ^= b;
              value *= PRIME_64;
          }
      }

      constexpr uint64_t digest() const
      {
          return value;
      }
  };  // hasher64

} // FNV1a
```

And I started on this TEST:

```c++
#include "FNV1a.hpp"
#include <gtest/gtest.h>

namespace tests::FNV1a {

  namespace suite_0 {

    TEST(FNV1aTests,BasicConstexpr) {
      constexpr ::FNV1a::hasher64 hasher{};

      struct dummy_struct {
        int i;
        std::string s;
      }; // dummy_struct

      using ByteArray = std::array<uint8_t,16>;

      constexpr auto to_bytes = [](dummy_struct const& ds) -> ByteArray {

        return {};

      };

      constexpr dummy_struct ds{
        .i = 17
        ,.s = "Hello"
      };

      constexpr auto test_data = to_bytes(ds);

    }

  }

} // tests::FNV1a
```

But so I did not make it as far as seeing what the 'switch' statement based code would look like? What I still do NOT know:

* Is the meta-data -> hash sufficient to identify a unique 'account ID'
* Do I even need to know account ID?

You know what - Now when I think about it I think the whole idea to try and identify the account ID is a dead end? What exactly is it I want to determine?

* I asume I would like to map an account statement to a BAS account (for automation)?
  - E.g., Map bank statement entries to trasnactions to corresponding 1920 and 1930?
  - E.g. SKV account statement to VAT accounts (debt and settlement BAS accounts)
* But from what I can see the NORDEA statement files I have inspected does not provide enough information to know 1920 or 1930?
  - Or can I imagine some mechanism to perform this mapping?

GOOD! It is decided then. I can REMOVE the to_account_id_ed step all together!

So we have this going forward:

* Remove passing of detected coding 'confidence' though maybe and monadic pipe.
* Remove 'to account ID:ed' mechanism all together
* Pass BOM from istream reading to 'inferr encoding'


## 20260130

OK, so next (I think) is:

* Make the to_detetced_encoding calling to_content_encoding into one to_inferred_encoding.
* Introuduce MetaDefacto: maybe BOM,ByteBuffer (BOMedByteBuffer?)
* Make istream_ptr_to_byte_buffer_step return maybe BOM, ByteBuffer pair
  - istream_to_maybe_bom_ed_byte_buffer?
* Now to_with_detected_encoding_step can use the maybe BOM in received BOMedByteBuffer

It seems I may be tempted to mark the older bom_istream mechanism in faviour of the new pipe line?

But then, how does the bom_istream based decoding compare to the maybe-pipe-line?

* ISO_8859_1::istream - not used
* UTF8::istream
  - persistent::in::UTF8::istream utf8_K10_in
  - persistent::in::UTF8::istream utf8_ink1_in
  - Parse of SKV::SRU::INK2::Y_2024::INK2R_csv_to_sru_template
  - Parse SKV::SRU::INK2::Y_2024::INK2S_csv_to_sru_template
  - Parse SKV::SRU::INK2::Y_2024::INK2_csv_to_sru_template
  - Parse 'csv-file with SRU values' (provided by user in command -sru and -csv -sru)
  - Parse 'all files in skv_specs_path' (in skv_specs_mapping_from_csv_files but *no operation* yet)
* NORDEA::istream (is persistent::in::UTF8::istream)
  - csv file -> had (Command "-csv -had *path*")
* persistent::in::CP437::istream cp437_in (sie_from_stream)
  - cp437_in.getline(text::encoding::unicode::to_utf8{}))

Ok. I think I let that be for now. It is not clear to me how I can (or should) refactor that mechanism into using the new pipe line?

I now have only a single to_inferred_encoding (combined previous two + renamed).

I fight my 'loss aversion' (good to keep just in case) tendencies.

* Should I remove unused to_encoding_options?
* Should I remove unused to_bom_encoding?

Well, I now have removed to_encoding_options. And GOOD RIDDANCE. It was the right decision.

And now I also removed to_bom_encoding. When looking at the code I saw no reason to keep it. I need one that can try to read a set of known BOMs. Lets aim for that should I decide to do so.

And finally I now removed the 'emptied out' icu_facade_deprecated.

So step-by-step I am 'eating this elephant'. What is next?

* I could make istream_ptr_to_byte_buffer_step produce a MetaDefacto of the buffer paired with a maybe EncodingID from a read BOM?
* I could continue to make maybe-variants for:

```c++
  //   .and_then(CSV::parse::monadic::csv_text_to_table_step)
  //   .and_then(account::statement::monadic::to_account_id_ed_step)
  //   .and_then(account::statement::monadic::account_id_ed_to_account_statement_step)
  //   .and_then(tas::monadic::account_statement_to_tagged_amounts_step);
```

I am tempted to do the BOM reading mechanism now while I have the mechanism fresh in my head. But also, cratchit works without it (The SKV file gets garabaged with the BOM but I do not care about the heding text where the intrusive BOM is so...).

* Is there a compromise (e.g., mark with TODO-comments so I can pick up the ball later on)?
* How much work can I imagine it to be to implement the BOM mechanism?

## 20260129

So where are we?

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % git status
On branch claude-001-refactor-csv-import-pipeline
Your branch is up to date with 'origin/claude-001-refactor-csv-import-pipeline'.

Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git restore <file>..." to discard changes in working directory)
	modified:   CMakeLists.txt
	modified:   docs/thinking/thinking.md
	modified:   src/persistent/in/raw_text_read.cpp
	modified:   src/persistent/in/raw_text_read.hpp
	modified:   src/sie/SIEEnvironmentFramework.cpp
	modified:   src/text/encoding.cpp
	modified:   src/text/encoding.hpp
	modified:   src/text/to_inferred_encoding.hpp
	modified:   src/text/transcoding_views.hpp
	modified:   src/zeroth/main.cpp
	modified:   src/zeroth/main.hpp

Untracked files:
  (use "git add <file>..." to include in what will be committed)
	src/persistent/in/encoding_aware_read.cpp
	src/persistent/in/encoding_aware_read.hpp
	src/persistent/out/

no changes added to commit (use "git add" and/or "git commit -a")
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % ls -l src/persistent/out 
total 16
-rw-r--r--  1 kjell-olovhogdahl  staff  1454 Jan 29 10:05 encoding_aware_write.cpp
-rw-r--r--  1 kjell-olovhogdahl  staff   358 Jan 28 22:21 encoding_aware_write.hpp
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % 
```

I have now looked over the code so far and it seems legit.

* We have base unit 'encoding' withg mechanisms to encode some character sets.
  - This is the home of BOM and read and write of BOM
  - This ise the hoem of UTF8 encoding to and from unicode
  - In fact, the other character sets require no 'encoding' (they output their code points aas-is)
* We have 'raw_text_read but no raw_text_write
* We have encoding_aware_read and encoding_aware_write

I could not resist but implemented raw_text_write with bom_ostream declaration (No implementation yet though).

I think I am ready to commit this for now and go back to the main refacyorng track again?

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % git status
On branch claude-001-refactor-csv-import-pipeline
Your branch is up to date with 'origin/claude-001-refactor-csv-import-pipeline'.

Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git restore <file>..." to discard changes in working directory)
	modified:   CMakeLists.txt
	modified:   docs/thinking/thinking.md
	modified:   src/persistent/in/raw_text_read.cpp
	modified:   src/persistent/in/raw_text_read.hpp
	modified:   src/sie/SIEEnvironmentFramework.cpp
	modified:   src/text/encoding.cpp
	modified:   src/text/encoding.hpp
	modified:   src/text/to_inferred_encoding.hpp
	modified:   src/text/transcoding_views.hpp
	modified:   src/zeroth/main.cpp
	modified:   src/zeroth/main.hpp

Untracked files:
  (use "git add <file>..." to include in what will be committed)
	src/persistent/in/encoding_aware_read.cpp
	src/persistent/in/encoding_aware_read.hpp
	src/persistent/out/

no changes added to commit (use "git add" and/or "git commit -a")
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % ls -l src/persistent/out                   
total 32
-rw-r--r--  1 kjell-olovhogdahl  staff  1624 Jan 29 10:41 encoding_aware_write.cpp
-rw-r--r--  1 kjell-olovhogdahl  staff   318 Jan 29 10:36 encoding_aware_write.hpp
-rw-r--r--  1 kjell-olovhogdahl  staff   277 Jan 29 10:33 raw_text_write.cpp
-rw-r--r--  1 kjell-olovhogdahl  staff   539 Jan 29 10:39 raw_text_write.hpp
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % 
```

* Introduced encoding_aware_read unit
* Introduced encoding_aware_write unit
* Introduced raw_text_write unit
* Reanmed to persistent::in::UTF8::istream
* Renamed to persistent::out::UTF8::ostream
* Removed deprecated to_bom_encoding(path)
* Moved bom_istream to raw_text_read unit
* Moved Renamed to persistent::in::ISO_8859_1::istream
* Renamed to persistent::in::CP437::istream
* Introduced persistent::out::text::bom_ostream (no definition yet)

So what is next?

I now tackled the to_detetced_encoding template calling to_content_encoding.

* I moved ByteBuffer to encoding unit.
* I could now make to_detetced_encoding a normal function taking Bytebuffer directly.
* I also make text::encoding::inferred::to_content_encoding take an std::span.

```c++
  using BytesSpan = std::span<std::byte>;

  std::optional<EncodingDetectionResult> to_content_encoding(
    BytesSpan bytes
    ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD);

  std::optional<EncodingDetectionResult> to_detetced_encoding(
    ByteBuffer const& buffer
    ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD);

```

But now I get into 'constness preservation problems'. The 'ByteBuffer const& buffer' can not be implicitally converted to BytesSpan.

It turns out I need to think in the same way as for 'const iterators'. That is, I need a span over const members to preserve constness in the same way as const ByteBuffer.

Makes sense. And now I have:

```c++

  // hpp
  using ConstBytesSpan = std::span<std::byte const>;

  std::optional<EncodingDetectionResult> to_content_encoding(
      ConstBytesSpan bytes
    ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD);

  std::optional<EncodingDetectionResult> to_detetced_encoding(
      ByteBuffer const& buffer
    ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD);

  // cpp

  std::optional<EncodingDetectionResult> to_detetced_encoding(
    ByteBuffer const& buffer
    ,int32_t confidence_threshold) {

    return maybe::to_content_encoding(buffer,confidence_threshold);

    // ...

    std::optional<EncodingDetectionResult> to_content_encoding(
      ConstBytesSpan bytes
      ,int32_t confidence_threshold) {

      UErrorCode status = U_ZERO_ERROR;

      auto data = reinterpret_cast<char const*>(bytes.data());
      auto length = bytes.size();

  // ...

```

It seems I could now be tempted to replace to_content_encoding(buffer) with to_content_encoding(ConstBytesSpan)? As the buffer implicitaly converts to the span this should works just fine.

## 20260128

Can I (should I) make to_inferred_encoding dependent on file_reader ByteBuffer?

First I thought this could be a good idea. But then thinking about it, I should not need to make text::encoding dependent on persistent::in? That is, inferring encdoing from data in a buffer is semantically 'closed'. The crux is how we pass (or detect) a BOM that marks wjat encoding to expect.

1. Either we expect the data to to_content_encoding to be as-is (with or without BOM).
  - Then we can make to_content_encoding leverage the BOM if it is there.
  - But then we need a mechanism to ignore the BOM part (it is not part of the text)
2. Or we use the existing bom_istream (that is already in the encoding unit).
  - So the encoding unit is already tied to BOM stuff (but not to file_reader)

WHY IS THIS SO HARD!! I am so frustrated. The pieces keeps restisting to come together in a natural way?

* We have a text file that may or may not have a BOM
* We have a pipeline based on path -> istream -> ByteBuffer -> 'detected encoding byte buffer'
* We already have a bom_istream
* We have some AI generated to_bom_encoding(istream / path)

So the core decision now is to decide:

* What do we pass to to_content_encoding (pass maybe BOM + Buffer OR pass full buffer with potential BOM still in the data)
* Can we (do we want to) reuse bom_istream?

So one problem is that we kind-if have assumed path_to_istream_ptr_step to be about text files? But the namespace is namespace persistent::in::maybe does not convey this. Should it be named text::in::maybe? Also, tne unit name file_reader is ambigous.

OK, so the problem is that a BOM is a thing of text files, not any file. So the notion of a 'binary' file is not relevant for cratchit? Or at least, for now we read onoy text files. How can we clarify this?

I first renamed file_reader unit to raw_text_read unit to help me think.

I then moved existing code for assumed 'text files / streams' from  persistent::in to persistent::in::text.

OK. Better for now I think. At this point I also discovered that text files behave different also on different platyforms.

* Carrige return and Line feed are encoded differently!
* And this is in addition to text file character set encoding Issues and BOM mechanisms.

Ok, so where are we?

* We want to make to_detetced_encoding template and to_content_encoding function into one.
* We still need to decide where to put the BOM mechanism!

1. Read the file with bom_istream and pass maybe BOM -> EncodingID as side channel with ByteBuffer
2. Or remove existing bom_istream mechanism and make to_content_encoding detect and strip-off BOM -> EncodingID

How can I decide?

You know what. I keep get distracted and confused about what code is actually active now? For one I am hesistant to remove code that I may still want to use? On the other hand, maybe it is time to commit? I HAVE the new pipe line. Is it time to take the plunge?

It seems I KNOW I will not use the Annotated Maybe API in TaggedAmountFramework?

* Remove file_path_to_istream
* Remove decoding_in_to_field_rows
* Remove field_rows_to_table
* Remove table_to_account_statements
* Remove account_statements_to_tas

What about ```c++ using AccountStatements = std::vector<AccountStatement>; ```?

API removed but AccountStatements in use ok.

It seems we can now also remove to_decoding_in and DecodingIn type? It is a left-over from time when we 'assumed' an encoding and created an 'encdoding in' stream. Our new pipe line first read the stream raw. And only then inferr what encoding to apply.

I sucessfully removed identified code. Where are we now?

Can we now make to_detetced_encoding template and to_content_encoding function into one. That is, remove the template?

I now tried to rename the ByteBuffer to something that eludes to 'a text buffer with unknown, yet to know, encoding'.

* I tried AgnosticBuffer
* It worked well in most cases.
* EXCEPT: I had text cases and code that was specifically expected 'vector of bytes'!

So I reversed this approach for now. It seems a better apporach would be to:

* Add a new type when we are ready to do something for BOM mechanism handling?

Back to make to_detetced_encoding template and to_content_encoding function into one. That is, remove the template?

IF we make it into a function that takes ByteBuffer. Where shall ByteBuffer live?

* Now it lives in raw_text_read and persistent::in::text::ByteBuffer
* But do we want to_inferred_encoding depend on raw_text_read?

Maybe we can place ByteBuffer in encoding unit?

* It kind of feels ok for to_inferred_encoding to depend on encoding unit?
* Only - encoding unit still contains BOM and 'encdoing aware' streams.

But then, BOM and streams should now live in raw_text_read?

* It seems ok to have BOM in raw_text_read
* But encode aware istreams does NOT belong here!

GOSH! The friction is still HIGH!

Wait! I think I realised something:

* The BOM type and bom_istream belongs in raw_text_read OK.
* But the 'encoding aware istreams' needs a home of their own now!
  - SIE file reading still uses a encoding aware istream (I think).
  - But our new pipeline does not any longer (we have path -> bytes -> encoding aware bytes)
  - So the encoding unit shall now be concerned with, well 'encoding'
  - Maybe even be combined with transcoding_views?

So I did an 'evening shift' and brute forced the existing BOM mechanism into (kept in?) encoding unit. I also added units to communicate the split in/out and 'encode aware' vs 'raw' text. I now have:

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % git status
On branch claude-001-refactor-csv-import-pipeline
Your branch is up to date with 'origin/claude-001-refactor-csv-import-pipeline'.

Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git restore <file>..." to discard changes in working directory)
	modified:   CMakeLists.txt
	modified:   docs/thinking/thinking.md
	modified:   src/persistent/in/raw_text_read.cpp
	modified:   src/persistent/in/raw_text_read.hpp
	modified:   src/sie/SIEEnvironmentFramework.cpp
	modified:   src/text/encoding.cpp
	modified:   src/text/encoding.hpp
	modified:   src/text/transcoding_views.hpp
	modified:   src/zeroth/main.cpp
	modified:   src/zeroth/main.hpp

Untracked files:
  (use "git add <file>..." to include in what will be committed)
	src/persistent/in/encoding_aware_read.cpp
	src/persistent/in/encoding_aware_read.hpp
	src/persistent/out/

no changes added to commit (use "git add" and/or "git commit -a")
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % ls -l src/persistent/out
total 16
-rw-r--r--  1 kjell-olovhogdahl  staff  1419 Jan 28 21:53 encoding_aware_write.cpp
-rw-r--r--  1 kjell-olovhogdahl  staff   358 Jan 28 22:21 encoding_aware_write.hpp
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % 
```

I will NOT commit this to git until I have gone over the code again tomorrow. Is it 'good enough'?

## 20260127

So let's implement:

* text::encoding::inferred::maybe::to_content_encoding
* text::encoding::inferred::maybe::to_istream_encoding
* text::encoding::inferred::maybe::to_file_at_path_encoding

It seems they depend on echother bottom up? Tah is to_file_at_path_encoding calls to_istream_encoding that calls to_content_encoding?

YES! That is what my AI friend has created for me. Now this breaks the and_then pipe-line I have aimed for.

Calling to_file_at_path_encoding actually opens the file and analyses the content.

But creating the istream and read the content is the task of the pipe line:

```c++
      auto result = persistent::in::maybe::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::maybe::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::maybe::to_with_threshold_step_f(100))
        .and_then(text::encoding::maybe::to_with_detected_encoding_step)
        ...

```

So where do we aactually have the call to to_file_at_path_encoding? Is it in to_with_detected_encoding_step?

* From try_parse_csv
* And thats it!
* Not even any tests!

wow, the scope of this mess just grows and grows!

Anyhow, it seems I should just remove this whole branch of code.

* Remove try_parse_csv
* Remove to_file_at_path_encoding

And fom where is to_istream_encoding called?

* From istream_to_decoding_in (TaggedAmountFramework!)
* And that is it!
* Not even any tests!

And istream_to_decoding_in is called ONLY from tas_from_statment_file. And tas_from_statment_file is marked as (TODO: Remove) and NOT called from anywhere!

So it seems I should:

* Remove tas_from_statment_file
* Remove istream_to_decoding_in
* Remove to_istream_encoding

Should I do this now or keep focusing on the nromalised API for encoding inference?

You know what? I SHOULD remove it! As it is not part of any active code but IS part of the 'encoing inference' API. Thus I shoudl remove it as part of the refactoring!

So we have:

* Remove tas_from_statment_file
* Remove istream_to_decoding_in
* Remove to_istream_encoding
* Remove try_parse_csv
* Remove to_file_at_path_encoding

Good. Let's do this! In what order though? It would be nice to remove one-by-one and at each step get code that still 'works'?

So what can we begin to remove as an isolated change?

* Revoved csv::parse::deprecated with no problem at all.
  - ParseCSVResult
  - try_parse_csv
  - encoding_caption

What next?

* Removed to_file_at_path_encoding with no repercussions.

What is next?

* Removed tas_from_statment_file with no repercussions.
* NOTE: Consider to also remove account_statements_to_tas?

In fact, all the 'original' pipe line can now be removed?

```c++
// TODO: Remove (Replaced by pipeline csv::path_to_tagged_amounts_shortcut in csv/import_pipeline.hpp)
AnnotatedMaybe<persistent::in::MaybeIStream> file_path_to_istream(std::filesystem::path const& statement_file_path) {
  AnnotatedMaybe<persistent::in::MaybeIStream> result{};
  result.m_value = persistent::in::to_maybe_istream(statement_file_path);
  if (!result.m_value) result.push_message("file_path_to_istream: Failed to create istream");
  return result;
}

// TODO: Remove (Replaced by pipeline csv::path_to_tagged_amounts_shortcut in csv/import_pipeline.hpp)
AnnotatedMaybe<text::encoding::MaybeDecodingIn> istream_to_decoding_in(persistent::in::MaybeIStream const& maybe_istream) {
  AnnotatedMaybe<text::encoding::MaybeDecodingIn> result{};

  auto maybe_encoding = maybe_istream
    .and_then([](std::istream& is) {
      return text::encoding::inferred::maybe::to_istream_encoding(is);
    });

  if (maybe_encoding) {
    result.m_value = text::encoding::to_decoding_in(
       maybe_encoding->defacto
      ,maybe_istream.value());
  }

  if (!result.m_value) result.push_message("istream_to_decoding_in: Failed to create a decoding in stream");
  return result;
}

// TODO: Remove (Replaced by pipeline csv::path_to_tagged_amounts_shortcut in csv/import_pipeline.hpp)
AnnotatedMaybe<CSV::FieldRows> decoding_in_to_field_rows(text::encoding::MaybeDecodingIn const& decoding_in) {
  AnnotatedMaybe<CSV::FieldRows> result{};
  result.push_message("decoding_in_to_field_rows: NOT YET IMPLEMENTED");
  return result;
}
// TODO: Remove (Replaced by pipeline csv::path_to_tagged_amounts_shortcut in csv/import_pipeline.hpp)
AnnotatedMaybe<CSV::Table> field_rows_to_table(CSV::FieldRows const& field_rows) {
  AnnotatedMaybe<CSV::Table> result{};
  result.push_message("field_rows_to_table: NOT YET IMPLEMENTED");
  return result;
}
```

I think I will leave them be for now an focus on the code that affects the encoding inference API.

So what is next?

* Removed istream_to_decoding_in with no repercussions.

So next should now be to_istream_encoding?

* Removed to_istream_encoding with to reprecussions.

At this point I came to think I may be able to hit two birds withj the same stone kind-of.

* Current code does NOT read (nor remove) a BOM in the inout csv file
  - This can be seen in current parse of the SKV account statement file that has a BOM on my machine.
* So we could be tempted to detect and pass on the BOM when reading the raw text in.
* I have code that did this in the past (but my AI code does not use ii, has its own but still leave the BOM in place)

But let's finalise the clean up we are currently working on and come back to this when we are done.

What is next? Well, we are done with the removals we listed above.

So now we can go back and implement what is now in the 'inferr encoding' API:

Now I am CONFUSED! There is yet no text::encoding::inferred::monadic! Why NOT? ANd how does the code even compile?

AHA! I forgot. This API is a supporting API. It is not part of the and_then pipeline of csv file -> account stetement -> tagged amounts

But we have two functions where we need only one.

* ```c++ std::optional<EncodingDetectionResult> to_content_encoding.```
* ```c++         template<typename ByteBuffer>
        std::optional<EncodingDetectionResult> to_detetced_encoding(... ```

We should be just fine with one that takes ByteBuffer should we not?

* Consider to use persistent::in::Bytebuffer
* Consider to make to_detetced_encoding or to_content_encoding be the only one?

I am thinking like this:

* It is to_content_encoding that implements the mechanism.
* If we make it take persistent::in::Bytebuffer we can later pass an optional BOM discovered by the read mechanism.
* It should be OK to make the to_inferred_encoding unit depend on file_reader unit (Where Bytebuffer is defined)?

Let's try!

AHA, ok. We have not implemented text::encoding::inferred::maybe::to_content_encoding yet!

So let's do that first! So we can attend to the consequences of tying it to ByteBuffer in one go later.

* I first brought back icu_facade_ namespace as icu_facade (All external access is cleaned out now)
* Then I first implemented the new icu_facade version of to_content_encoding
* And the text::encoding::inferred::maybe::to_content_encoding using the icu_facade version.

I succeeded to implement the change and cratchit now PASS all tests related to the API change I made.

F**K!! I now discovered that I had NOT added the new to_inferred_encoding unit yet!

* ALL my carefull refactoring steps with intermediate check-ins will NOT compile :(

*SIGH* - SO much for trustworthy version control!

I wonder, is there a way to make git warn if I committ with untracked files present?

My AI friends tells me git has a 'pre-commit hook' in .git/hooks/pre-commit.

```text
If you want real cross-platform safety: Use the pre-commit framework
It is:
* Interpreter-stable
* Version-controlled
* CI-friendly
*Explicitly designed to solve this problem

This is the same solution used in LLVM, Chromium, and large C++ monorepos.
```

Ok, lets tread carefully now!

* I still have not cleaned up template to_detetced_encoding calling to_content_encoding
  - also to_content_encoding takes fragile C-like char* and length!
* I still have BOM code in old icu_facade_deprecated.
* I may now be tempted to make ByteBuffer somehow carry a maybe BOM from the reading phase?
* If so I may be tempted to keep this BOM code in new inferred encoding API?
* But then, Maybe I rather use my old BOM code?

So what do I do now and what do I come back to later?

* I am in the process of making a full and_then pipeline for maybe versions of existing monadic and_then pipe line.
* And I am tempted to solve the missing BOM-reading and passing in this pipe line while I'm at it?

I have come this far:

```c++

    // Done
    auto result = persistent::in::maybe::path_to_istream_ptr_step(m_valid_file_path)
      .and_then(persistent::in::maybe::istream_ptr_to_byte_buffer_step)
      .and_then(text::encoding::maybe::to_with_threshold_step_f(100))
      .and_then(text::encoding::maybe::to_with_detected_encoding_step)
      .and_then(text::encoding::maybe::to_platform_encoded_string_step);

    // Left to attend to
    //   .and_then(CSV::parse::monadic::csv_text_to_table_step)
    //   .and_then(account::statement::monadic::to_account_id_ed_step)
    //   .and_then(account::statement::monadic::account_id_ed_to_account_statement_step)
    //   .and_then(tas::monadic::account_statement_to_tagged_amounts_step);

```

So should I add BOM reading to istream_ptr_to_byte_buffer_step? And also extend ByteBuffer to pass optional BOM to to_with_detected_encoding_step?

I think I should first:

* finalise the clean up of template to_detetced_encoding calling to_content_encoding
* Decide if to keep or remove to_encoding_options
* Then study 

```c++
  EncodingDetectionResult to_bom_encoding(std::istream& is);
  EncodingDetectionResult to_bom_encoding(std::filesystem::path const& file_path);
  EncodingDetectionResult to_extension_heuristics_encoding(std::filesystem::path const& file_path);
```

You know what? I think I can remove to_extension_heuristics_encoding right now!

And so I did! No problems!

I now got another idea. Maybe it is time to implement 'continous integration'?

* This would secure against missing files in commits!
* I think GitHub have support?
* Maybe I can implement some local support too?
* Github may even support cross platform build and test CI?

But back to my clean-up task. I am tempted to keep to_encoding_options. I mean I have it (although untested and generated by Claude code). My intuition is that I rather use a 'raw file -> encoding options' than current 'raw file -> detected + confidence' mechanism.

* SHould I keep it?
* Should I have Claude Code generate tests for it?
* SHould I do this now or later?

I have to think about this now.

* Remember to come back to namespace CSV::project::deprecated::HeadingId to_csv_heading_id

I could not help to study BOM handling. ANd it seems to be my own hand rolled code that is used?

```c++
    using DecodingIn = std::variant<
      text::encoding::UTF8::istream
      ,text::encoding::ISO_8859_1::istream
      ,text::encoding::CP437::istream
    >;
    using MaybeDecodingIn = cratchit::functional::memory::OwningMaybeRef<DecodingIn>;
    MaybeDecodingIn to_decoding_in(
       EncodingID const& encoding
      ,std::istream& is);

  // And

     class bom_istream {
    public:
        std::istream& raw_in;
        std::optional<BOM> bom{};
        // ...

      namespace ISO_8859_1  {
      class istream : public bom_istream {

      namespace UTF8 {
      class istream : public bom_istream {

    namespace CP437 {
      class istream : public bom_istream {

```

So then - Wgy is NOT the suspected BOM in SKV-file consumed? 

* Is there an error in the code?
* Is the SKV file in fact not coming woth an UTF8 BOM (the only one currently supported)?

```c++
namespace text {
  namespace encoding {
      if (is >> bom.value) {
        if (bom.value == BOM::UTF8_VALUE) {      
```

So what IS the BOM value in the SKV file?

* According to VSCode it IS an UTF8 BOM in the SKV-file.

I can think of two options for BOM handling.

1. Make existing BOM handling work and pass optional BOM in extended ByteBuffer (add a Meta that carries optonal BOM)
2. Implement a version of existing to_bom_encoding that checks for BOM and removes it if found.
  - I can then integrate this into to_content_encoding?
  - Only, Tnen to_content_encoding must also mutate the buffer :(

I have to come back to this later!

So it seems the next step is in fact to continue to clean up template to_detetced_encoding calling to_content_encoding?

## 20260126

Now when I have slept on this I have a new approach. It seems I can first create the 'nomralised' API for encoding inference.

Thinking this way I can:

1. Create a new unit to_inferred_encoding and implement text::encoding::inferred namespace with the existing API
2. Then push down code that actually use ICU into the icu_facade namespace

DARN! I still fail to find a strategy to refactor in small steps. How can I move code to to_inferred_encoding step-by-step and be guided by the compiler to reports what needs to be attended to?

Maybe the encding should still be the base for to_inferred_encoding?

In that case I can make to_inferred_encoding a clone of the encoding unit and then prune both until they are separate units?

I did this and now both compile just fine (of course) but the linker reports duplicate symbols for all duplicated stuff.

So now I can attend to each duplicate and decide where to put it?

But maybe not. I would first like to make to_inferred_encoding include encoding.hpp to fix this design before moving on?

So having to_inferred_encoding.hpp include encoding.hpp only works fine for now. So now I can I start pruning until I have no duplocate symbols?

Or, maybe now is the time to first lift the whole 'inferr encoidng' API to become the 'normalised' API?

I decided that tghe next step is to 'empty out' the icu_facade into the 'normalised' API first. It seems I can do this by renaming the existing icu_facade to get existing access to fail to compile. I now have a compiler guided process to move the API?

1. Create the namespace 'inferred' for the normalised API.
2. rename existing icu_facade to icu_facade_deprecated in encoidng unit.
3. Move code to inferred and refactor call sites accodingly

Let's try!

I now get compiler errors like:

```sh
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.hpp:258:8: error: use of undeclared identifier 'icu_facade'
  258 |        icu_facade::EncodingDetectionResult const& detected_source_encoding
      |        ^
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/to_inferred_encoding.cpp:28:23: error: unknown type name 'EncodingDetectionResult'; did you mean 'icu_facade_deprecated::EncodingDetectionResult'?
   28 |         std::optional<EncodingDetectionResult> to_content_encoding(
      |                       ^~~~~~~~~~~~~~~~~~~~~~~
      |                       icu_facade_deprecated::EncodingDetectionResult
...
```

GREAT! Now I can use the compiler to guide my refactorings.

It turns out that the code worked better with the namespace 'inferred' before 'icu_facade'. In the hpp-file this enable icu_facade to referr to types in 'inferred'. And in cpp code in 'inferred' could still call 'icu_facade' due to declarations in hpp-file.

GOSH! There is no end to the mess in the code! Now I found:

```c++
  std::optional<EncodingDetectionResult> to_content_encoding(
    char const* data
    ,size_t length
    ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD);

  // But...

  template<typename ByteBuffer>
  std::optional<EncodingDetectionResult> to_detetced_encoding(
      ByteBuffer const& buffer
    ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD) {
```

My AI friends really lacks the ability to keep to a consistent architecture!

I now realised I should first refactor the hpp-files (declarations).

* In this way I get the compiler guide me to all call sites that need change.

So I emptied out the to_inferred_encoding.cpp and continued with:

* Moving from text::encoding::icu_facade_depracated to text::encoding::inferred

Now I get compiler erros like:

```sh
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/csv/parse_csv.hpp:15:25: error: no member named 'icu_facade' in namespace 'text::encoding'
   15 |         text::encoding::icu_facade::EncodingDetectionResult icu_detection_result;
      |         ~~~~~~~~~~~~~~~~^
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/csv/parse_csv.hpp:21:52: error: no member named 'icu_facade' in namespace 'text::encoding'
   21 |       std::string encoding_caption(text::encoding::icu_facade::EncodingDetectionResult const& detection_result);
      |                                    ~~~~~~~~~~~~~~~~^

```

In fact this approach worked well all the way. I could adress each compilation error and refactor each call site access to previous icu_facade to new inferred.

I ended upp with linker error:

```sh
Undefined symbols for architecture arm64:
  "text::encoding::inferred::maybe::to_content_encoding(char const*, unsigned long, int)", referenced from:
      std::__1::optional<MetaDefacto<text::encoding::inferred::EncodingDetectionMeta, text::encoding::EncodingID>> text::encoding::inferred::maybe::to_detetced_encoding<std::__1::vector<std::byte, std::__1::allocator<std::byte>>>(std::__1::vector<std::byte, std::__1::allocator<std::byte>> const&, int) in test_csv_import_pipeline.cpp.o
  "text::encoding::inferred::maybe::to_istream_encoding(std::__1::basic_istream<char, std::__1::char_traits<char>>&, int)", referenced from:
      istream_to_decoding_in(cratchit::functional::memory::OwningMaybeRef<std::__1::basic_istream<char, std::__1::char_traits<char>>> const&)::$_0::operator()(std::__1::basic_istream<char, std::__1::char_traits<char>>&) const in TaggedAmountFramework.cpp.o
  "text::encoding::inferred::maybe::to_file_at_path_encoding(std::__1::__fs::filesystem::path const&, int)", referenced from:
      CSV::parse::deprecated::try_parse_csv(std::__1::__fs::filesystem::path const&) in parse_csv.cpp.o
ld: symbol(s) not found for architecture arm64
```

Which told me that the only external dependancies where:

* text::encoding::inferred::maybe::to_content_encoding
* text::encoding::inferred::maybe::to_istream_encoding
* text::encoding::inferred::maybe::to_file_at_path_encoding

This verifies my assumption about how the API for encodining inference should work.

I implemented them as dummy (nullopt return). And now cratchit compiles with failing tests:

```sh
[  FAILED  ] 16 tests, listed below:
[  FAILED  ] MonadicCompositionFixture.PathToAccountIDedTable
[  FAILED  ] MonadicCompositionFixture.PathToAccountStatementTaggedAmountsRefactoring0
[  FAILED  ] MonadicCompositionFixture.PathToAccountStatementTaggedAmountsRefactoring1
[  FAILED  ] MonadicCompositionFixture.PathToAccountStatementTaggedAmountsRefactoring2
[  FAILED  ] MonadicCompositionFixture.PathToAccountStatementTaggedAmountsRefactoring3
[  FAILED  ] EncodingDetectionTestFixture.DetectUTF8
[  FAILED  ] EncodingDetectionTestFixture.DetectISO8859
[  FAILED  ] EncodingDetectionTests.ComposesWithFileIO
[  FAILED  ] BytesToUnicodeTests.IntegrationWithSteps1And2
[  FAILED  ] RuntimeEncodingTests.CompleteTranscodingPipeline
[  FAILED  ] EncodingPipelineTestFixture.ISO8859FileToUTF8Text
[  FAILED  ] EncodingPipelineTestFixture.CompleteIntegrationAllSteps
[  FAILED  ] CSVPipelineCompositionTestFixture.TranscodesISO8859ToUTF8ThenParsesCSV
[  FAILED  ] GenericStatementCSVTests.NORDEAStatementOk
[  FAILED  ] AccountStatementDetectionTests.GeneratedSingleRowMappingTest
[  FAILED  ] AccountStatementDetectionTests.GeneratedRowsMappingTest
```

Do I dare to check this code in?

If I check in this code it will be harder for me to back track?

On the other had, maybe this is the time to try to check in code even though I am not at a working app?

NOTE TO SELF: I have benn quite persistent to comment woth 'TODO' about deprecated and code to remove. So I can trust this is enough to clean out unwanted code later?

## 20260125

So time to make to_platform_encoded_string_step into a maybe version and base mondaic  version on that.

I implemneted the maybe variant ok. I think it is quite neat actually? I think I like how the 'transcoding' turned out as:

```c++
  // buffer -> unicode -> runtime encoding
  auto unicode_view = views::bytes_to_unicode(buffer, deteced_encoding);
  auto platform_encoding_view = views::unicode_to_runtime_encoding(unicode_view);
```

For now it seems like a good design to have the transcoding work on a view that generates code points over the charachter set?

* We get views::bytes_to_unicode to get a lazy range view to iterate over code points
  where our 'bytes' gets decoded into unicode using the provided deteced_encoding enum.
* We then get the runtime or 'platform' encoding as a lazy range view from views::unicode_to_runtime_encoding.

What is less good is that we pass two arguments to views::bytes_to_unicode? We already have a pair WithDetectedEncodingByteBuffer.

Would it be a good idea to pass the WithDetectedEncodingByteBuffer to views::bytes_to_unicode?

Where does WithDetectedEncodingByteBuffer live?

* We have text::encoding::WithDetectedEncodingByteBuffer
* We have text::encoding::WithDetectedEncodingByteBuffer::Meta being icu_facade::EncodingDetectionResult.

So we have not yet isolated a cratchit level EncodingDetectionResult that we can later free from icu_facade.

Anyhow, where does bytes_to_unicode and unicode_to_runtime_encoding live?

* text::encoding::views::bytes_to_unicode
* text::encoding::views::unicode_to_runtime_encoding

GERAT! The 'views' already lives in text::encoding.

After having thought for a while I have a proposal.

1. Implement a bridge to a 'normalised' encoding with a to_normalised_encoding(icu_facade::EncodingDetectionResult).
2. Then make a meta='normalised encoding' and defacto='byte buffer' pair to pass to bytes_to_unicode.

DARN! I have a 'catch 22' like identifier namespace problem.

* The existing and used EncodingDetectionResult lives in icu_facade.
* All existing detection functions also lives in icu_facade and returns this EncodingDetectionResult.
* There are no 'normalised' API for encoding detection.

To implement a 'normalised' EncodingDetectionResult I need also a 'normalised' API.

Should I do the work and define a normalised 'encode detection' API that uses icu_facade for now? Is it worth it?

I get an urge to break out the icu_facade to its own unit.

Well, I tried. And now I get the classic circular hpp-file dependancies again.

* I want icu_facade.hpp to include (know about) EncodingDetectionResult in normalised bytes_to_encode_id.hpp
* But I also want bytes_to_encode_id.hpp to use / leverage / know about icu_facade.hpp API

So what is the solution again? I need a common 'atomic' header with the normalised types like EncodingDetectionResult and EncodingID?

Maybe the low level unit is the existing encoding?

Yes, that can work. But then I need to make icu_facade::EncodingDetectionResult into a normalised one?

What goes into icu_facade::EncodingDetectionResult?

```c++
  struct EncodingDetectionMeta {
    CanonicalEncodingName canonical_name; // ICU canonical name (e.g., "UTF-8")
    std::string display_name;             // Human readable name
    int32_t confidence;                   // ICU confidence (0-100)
    std::string language;                 // Detected language (e.g., "sv" for Swedish)
    std::string detection_method;         // "ICU", "BOM", "Extension", "Default"
  };

  using EncodingDetectionResult = MetaDefacto<EncodingDetectionMeta,EncodingID>;
```

Well, the EncodingID is the 'normalised' enum. And the meta is ICU specific.

So what depends on what here? If I want a cratchit normalised API for encoding detection, I need it to be able to leverage some underlying library like the ICU (or a crachit local implemented library).

* normalised API -> uses ICU

For this to work I need ICU to only use its own types and 'stuff'! It can NOT use EncodingID!

I came upp with two new units:

```sh
	src/text/bytes_to_encode_id.cpp
	src/text/bytes_to_encode_id.hpp
	src/text/icu_facade.cpp
	src/text/icu_facade.hpp
```

I get a good feeling for this. And it seems I should aim for encoding.hpp -> bytes_to_encode_id.hpp -> icu_facade.hpp?

That is, encoding.hpp is the cratchit 'normalised' API. It depends on bytes_to_encode_id.hpp for 'normalised' encoding detection. It in turn uses icu_facade.hpp to implement this encoding detection.

To make this work I need to make icu_facade to be self-contained regarding the encoding code. But it may use cratchit logging and stuff that is not about encoding.

What could be the first step to get this ball rolling?

This is trycky! It seems I first need to kind-of reverse the icu_facade dependancy?

* How can I use the compiler to identify how icu_facade code depends on 'upstream' code?
* Can I first move icu_facade to the new unit and see if it compiles?

AHA! Yes, that could work. I can first add icu_facade as a stand alone unit and keep all the rest of the code as-is (NOT use icu_facade unit yet).

Doing this I can see the following code that depends 'the wrong way':

```c++

  // hpp
  EncodingID canonical_name_to_enum(CanonicalEncodingName const& canonical_name);
  using EncodingDetectionResult = MetaDefacto<EncodingDetectionMeta,EncodingID>;

  // cpp
  
```

I came up with the following scheme to use the compiler to identify upstream dependancies in icu_facae.

* I moved icu_facae hpp and cpp code to the new icu_facade hpp and cpp.
* I added icu_facade.cpp to CMakeLists
* I created namespace 'upstream' and added code there to make the unit comopile.
* Finaly I commented out the namespace levels 'text::encoding' to NOT get duplicate symbol names
  (icu_facade unit now places symbol names in top leve namespace icu_facade that does NOT clash with existing text::encoding::icu_facade)

I now got upstream code clrealy defined as:

```c++
      namespace upstream {
        // TODO: Refactor away this dependancy
        enum class EncodingID {
          Undefined,
          UTF8,
          UTF16BE,
          UTF16LE, 
          UTF32BE,
          UTF32LE,
          ISO_8859_1,
          ISO_8859_15,
          WINDOWS_1252,
          CP437,
          Unknown
        };
      } // upstream

      namespace upstream {
        // TODO: Refactor away this dependancy

        std::string enum_to_display_name(EncodingID) {
          return "enum_to_display_name dummy";
        }
      } // upstream

```

I now wanted to check in this new unit but I did not like the state of the code (I can't trust I remember to finalise the refactoring).

I tried:

```c++
        [[deprecated("Refactoring incomplete: this code is temporary")]] 
        std::string enum_to_display_name(EncodingID) {
          return "enum_to_display_name dummy";
        }
```

But I did not like this. It was good the code compiled. But this also meant I did not see the warnings as I had tests running by REPL so all looked fine in the restricted console winow output.

How can I reach a step where my code is in a state I can keep if I forget to refactor further?

I think I wait with checking in until I have icu_facade stand-alone.

So what is the semantic of a stand-alone icu_facade?

* upstream::EncodingID must go (be added by client code)
* We still need an upstream API that provides the EncodingID
* So what is left for icu_facade to provide?
* And how to I determine this?

Maybe I can now create the upstream API to see what the icu_facade can priovide?

Can we implement the 'normalised API in the new bytes_to_encode_id?

AHA!

* The 'file extension' -> 'detected encoding' is NOT an icu thing!
  So this we can already make 'normalised'
* And maybe the only thing we need icu_facade to return is 'canocical name'?
  This seems to be what the ICU code returns as a result when asked to detect coinrent?

```c++
  UCharsetDetector* detector = ucsdet_open(&status);
  const UCharsetMatch* match = ucsdet_detect(detector, &status);
  // ...
  const char* canonical_name = ucsdet_getName(match, &status);
  int32_t confidence = ucsdet_getConfidence(match, &status);
  const char* language = ucsdet_getLanguage(match, &status);

```

Hm, it seems I need to investigate how ICU actually identifies a character set? Is it the whoel match? Or is there an enum or string that canonically identifies the character set?

I have decided to tread carefully and first trim the EncodingDetectionMeta down.

* I do not need the language detection.
* I think I 'need' only 'canonical name' and 'confidence' for now?

By renaming the laguage field I discovered only one place where it was accessed (and then only if empty!). So that was an easy removal.

Then I discovered that actually removing the field gives me a compiler error regarding the count of members, not about what member is missing.

If I:

```c++
      struct EncodingDetectionMeta {
        CanonicalEncodingName canonical_name; // ICU canonical name (e.g., "UTF-8")
        std::string display_name;             // Human readable name
        int32_t confidence;                   // ICU confidence (0-100)
        // std::string language_;                 // Detected language (e.g., "sv" for Swedish)
        std::string detection_method;         // "ICU", "BOM", "Extension", "Default"
      };

```

It for one triggers a recompilation of a very large of files! And in the end the compiler errors are of the sort:

```sh
[ 30%] Building CXX object CMakeFiles/cratchit.dir/src/text/encoding.cpp.o
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.cpp:317:16: error: excess elements in struct initializer
  317 |               ,"ICU"            
      |                ^~~~~
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.cpp:449:20: error: excess elements in struct initializer
  449 |                   ,"ICU"
      |                    ^~~~~
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.cpp:487:16: error: excess elements in struct initializer
  487 |               ,"Cannot open file"
      |                ^~~~~~~~~~~~~~~~~~
```

So this is unhelpfull. I solved this by adding a constructor taking only the values I want to keep.

```c++
        EncodingDetectionMeta(
           CanonicalEncodingName canonical_name
          ,int32_t confidence
          ,std::string detection_method);

```

And now the compiler is more helpfull:

```sh
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.cpp:312:21: error: no matching constructor for initialization of 'Meta' (aka 'text::encoding::icu_facade::EncodingDetectionMeta')
  312 |             .meta = {
      |                     ^
  313 |                canonical_str
      |                ~~~~~~~~~~~~~
  314 |               ,display_name
      |               ~~~~~~~~~~~~~
  315 |               ,confidence
      |               ~~~~~~~~~~~
  316 |               ,language_str
      |               ~~~~~~~~~~~~~
  317 |               ,"ICU"            
      |               ~~~~~~
  318 |             }
      |             ~
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.hpp:202:9: note: candidate constructor not viable: requires 3 arguments, but 5 were provided
  202 |         EncodingDetectionMeta(
      |         ^
  203 |            CanonicalEncodingName canonical_name
      |            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  204 |           ,int32_t confidence
      |           ~~~~~~~~~~~~~~~~~~~
  205 |           ,std::string detection_method);
      |           ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.hpp:196:14: note: candidate constructor (the implicit copy constructor) not viable: requires 1 argument, but 5 were provided
  196 |       struct EncodingDetectionMeta {
      |              ^~~~~~~~~~~~~~~~~~~~~
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.hpp:196:14: note: candidate constructor (the implicit move constructor) not viable: requires 1 argument, but 5 were provided
  196 |       struct EncodingDetectionMeta {
      |              ^~~~~~~~~~~~~~~~~~~~~

```

It is interesting that it seems adding a constructor does not remove the implicit copy and move constructors? AT least, the compiler tells me it failed to apply them?

NO, this is not a good thing to do now.

* I remove 'display name' before I have an upstream replacement.
* I should actually remove 'detection method' also?
  This is known by the caller, not by the icu_facade.

AHA: The icu_facade actually ONLY performs the 'identify from data' part!

So maybe I catually only need to return 'canonical name' and 'confidence'?

This day has now come to an end and I decided this approach was a dead end for now.

I delete my icu_facade unit again and tries something else tomorrow. It was a insightfull exersice never the less :)

## 20260124

Next to make into a maybe variant is to_with_detected_encoding_step. I started by separating existing text::encoding::monadic::to_with_detected_encoding_step into hpp declaration and cpp definition.

I found out that if I have a function declaration as 'result_type to_with_detected_encoding_step(arg_type)' and a definition with the inline specifier 'inline result_type to_with_detected_encoding_step(arg_type)' - Then the linker will FAIL!

This stumped me at first. Then I specualted that the 'inline' specifier actually tells the compiler to place the function code at all call sites as-is. That is, NOT implement a call with stack-frame and everything. So when the declaration specifies a function to be called. But he definition specifies an ilined function - then the caallable function is no where to be found by the linke?

I asked my AI friends and they do NOT agree. The mechanism that fails is a bit different.

I got this answer snippet:

```text
* Non-inline function → exactly one definition with external linkage must exist
* Inline function → definitions may appear in multiple TUs, but all TUs that odr-use it must see a definition
```

And I got this answer snippet:

```text
  1. Declaration without inline (in header):
    - Promises external linkage
    - Tells the linker: "There will be exactly ONE definition of this function in some .cpp file"
  2. Definition with inline (in .cpp):
    - Has inline linkage (internal/weak linkage in modern C++)
    - Tells the compiler: "This definition can appear in multiple translation units"
    - The definition is NOT exported as an external symbol
  3. The Mismatch:
    - The linker looks for an external definition (because of the declaration)
    - But finds none, because the inline definition wasn't exported as external
    - Result: undefined reference error
```

So it seems thge mechanism is based on compiler and linker mechanosms called:

* 'external linkage'
* 'odr usgage' 
* 'internal/weak linkage'
* 'external symbol'

So a normal declaration defines an 'external symbol' and a call site will engage 'external linkage' to this symbol. For this to work there must exist a definition for this external symbol somewhere in the linked files.

So with the inline specifier the symbol becomes an 'internal' (weak) symbol. And the compiler must see the definition to generate the code itself? Or, the compiler tells the linker to look for the symbol in the object file and NOT in any of the linked object files?

I also googled and found snippets from the C++ standard that states:

```text
An inline function shall be defined in every translation unit in which it is odr-used and shall have exactly the same definition in every case.
...
An inline function with external linkage shall have the same address in all translation units.
```

Wow, this was a rabbit hole! I think I will have to live this for now. I feel overwhelmed by all the ways this is talked about on the web and by AI. I fail to find a consistent cause-and-effect explanation that gives me a form model of what is going on. But I get some clues.

* The 'inline' specifier causes the compiler to NOT 'create' an external symbol.
* Thus the linker suceeds only if the symbol is in the same tramslation unit (and thus object file)

Lets continue with our refactoring of the and_then chain for maybe variants of my step functions.

It turns out we have more incosnistencies to deal with now.

* The text::encoding::monadic::to_with_detected_encoding_step generates an OK message 
  with the detected encoidng spelled out in name and confidence.
* If we place this code in the maybe-variant we hide these results behind the maybe-wall that only returns a single enum
  that represents the detected encoding.
* We have semantic clashes in that the maybe and the enum both can represent 'non viable' encoding.

I tried to make WithDetectedEncodingByteBuffer result carry ALL meta-data from the detection.

* But then I got code leakage in that WithDetectedEncodingByteBuffer allowed assignment to only encoding member
  (passing WithDetectedEncodingByteBuffer with unititialied data).

I started to implement a to_assumed_encoding to create a fully WithDetectedEncodingByteBuffer for the default UTF-8 case.

* But then I introduced a new maybe case if this also failed.
* Not to mention, when and how can to_assumed_encoding fail?

I have to give this some more thought!

Ok, so here are some observations.

1. I want text::encoding::monadic::to_with_detected_encoding_step
   to be based on a text::encoding::maybe::to_with_detected_encoding_step.
2. To keep monadic OK message with detected encoding info I need to pass
   that back from the maybe variant.
3. But then this extra information will also leak out from the maybe version.

Is that bad? It feels bad. What options do I have?

* I thought for a while I could make the monadic version re-cerate the 
  missing info from the returned enum.
  But NO - this is not possible as I still loose the confidence value.

Now I have already decided I do not like the idea of 'conmfidence' in encoding detection. It feels leaky as a concept. I mean, what does it even mean? If I take a sample inoput and check the content for possible encodings and characters sets. The only think I can say is that an option is not violated. I can't say how probable it is that I am right? Let's say I find the sample matches encoding of pure 7-bit ASCII. The only think I then know is that the sample input does not violate being an encoding of a large set of character sets and encoings that maps to 7-bit ascci for that range of code points. It could still be the case that the inoput is UTF-8 or ISO 8859-1. The sample is just to small to know?

* Another option is to change the WithDetectedEncodingByteBuffer to contain the full detection result 'from below'.

Can I do this in a way to still keep the information that now leaks upsteram 'contained'?

* I suppose I could make the field I use in existing code (the encdoing field) front-and-center and make the rest a 'meta' section?

Hm, YES. That could work! Then the leak would be a new meta-section. And upstream code will not accidentally see it?

Lets try!

* I can basically use icu_facade::maybe::to_detetced_encoding as the maybe-bversion?
* Although keeping text::encoding::maybe::to_with_detected_encoding_step as a wrapper is good for code clarity.
* I tighten the EncodingDetectionResult tio be a MetaDefacto with defacto as the current enum.
  And meta as the stuff the monadic variant needs for its OK message.

I did the change:

```c++
    struct EncodingDetectionMeta {
      CanonicalEncodingName canonical_name; // ICU canonical name (e.g., "UTF-8")
      std::string display_name;             // Human readable name
      int32_t confidence;                   // ICU confidence (0-100)
      std::string language;                 // Detected language (e.g., "sv" for Swedish)
      std::string detection_method;         // "ICU", "BOM", "Extension", "Default"
    };

    using EncodingDetectionResult = MetaDefacto<EncodingDetectionMeta,DetectedEncoding>;
```

With the plan to refactor the code guided by compiler errors. This worked until I got confuded by:

```sh
... error: no matching function for call to object of type '(lambda at /Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/test/test_csv_import_pipeline.cpp:582:19)'
  343 |     _NOEXCEPT_(noexcept(static_cast<_Fp&&>(__f)(static_cast<_Args&&>(__args)...)))
```

at:

```c++
...
        .and_then([](auto buffer) {
          AnnotatedMaybe<text::encoding::icu_facade::EncodingDetectionResult> encoding_result;
          auto maybe_encoding = text::encoding::icu_facade::maybe::to_detetced_encoding(buffer);
          if (maybe_encoding) {
            encoding_result.m_value = *maybe_encoding;
            encoding_result.push_message(
              std::format("Detected encoding: {} (confidence: {})",
                         maybe_encoding->meta.display_name,
                         maybe_encoding->confidence));
          } else {
            encoding_result.push_message("Failed to detect encoding");
          }
          return encoding_result;
        });

```

How can I understand the compiler error and how to fix it?

After having chatted with my AI friends I get the notion that this is a 'red herring' caused by template 'subsistution is not a failure (SFINAE)' event. The lambda to and_then is treated as an 'auto type' and thus as a template. So according to SFINAE it is not an error for the lamda to not compile. And text::encoding::icu_facade::maybe::to_detetced_encoding does actually NOT compile with the change to the EncodingDetectionResult!

It is PAIN to be a C++ programmer!

I came up with the idea to change the compilation order to have the definition (the cpp file) be compiled before code that does template insrtantiation based on the changed code.

Then I thought about it again. And started to wonder what the function definition had to do with the template instantiation failure?

So I commented out the suspoicious code in the lamda. And the compiler error shifted to another location. By add code back in I identified the culprit as:

```c++
  encoding_result.push_message(
    std::format("Detected encoding: {} (confidence: {})",
                maybe_encoding->meta.display_name,
                maybe_encoding->confidence));
```

And sure enough, this can't comopile with the changed type! I fixed to 'maybe_encoding->meta.confidence' and carried on.

I made progress but all manual editing got tedious. I tried chatGPT to edit a code snippet for me. I prompted:

```text
I have '            return EncodingDetectionResult{
              .meta = {
                 "UTF-16LE"
                ,"UTF-16 Little Endian"
                ,100
                ,""
                ,"BOM"
              }
              ,.defacto = DetectedEncoding::UTF16LE
            };'. Can you help me format 'return {DetectedEncoding::Unknown, "", "Unknown", 0, "", "No BOM"};' the same way?
```

It returned:

```text
return EncodingDetectionResult{
  .meta = {
     "Unknown"
    ,"Unknown"
    ,0
    ,""
    ,"No BOM"
  }
  ,.defacto = DetectedEncoding::Unknown
};
```

Which is NOT correct!

So I conclude I can NOT trust my AI friends with changes that requires 100% adherence to code correctness!

After all it did not take me so long to refactor the code manually. But now I got suspisous about:

```c++
  auto encoding_result = icu_facade::maybe::to_detetced_encoding(buffer, confidence_threshold);
  ...
  result = WithDetectedEncodingByteBuffer{
      .meta = encoding_result->defacto
    ,.defacto = std::move(wt_buffer.defacto)
  };
```

How can '.meta = encoding_result->defacto' compile?

* WithDetectedEncodingByteBuffer is:

```c++
  using WithDetectedEncodingByteBuffer = MetaDefacto<DetectedEncoding,ByteBuffer>;
```

* And encoding_result->defacto is DetectedEncoding

Oh! I see. I have not changed WithDetectedEncodingByteBuffer yet!

Ok, time to have WithDetectedEncodingByteBuffer carry the whole EncodingDetectionResult as 'meta'.

I made the change:

```c++
  // using WithDetectedEncodingByteBuffer = MetaDefacto<DetectedEncoding,ByteBuffer>;
  using WithDetectedEncodingByteBuffer = MetaDefacto<icu_facade::EncodingDetectionResult,ByteBuffer>;
```

And got stumped by a template instantiation compilation error like:

```sh
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.2.sdk/usr/include/c++/v1/__type_traits/invoke.h:459:1: error: no type named 'type' in 'std::invoke_result<cratchit::functional::AnnotatedOptional<std::string> &, MetaDefacto<MetaDefacto<text::encoding::icu_facade::EncodingDetectionMeta, text::encoding::DetectedEncoding>, std::vector<std::byte>> &&>'
  459 | using invoke_result_t = typename invoke_result<_Fn, _Args...>::type;
      | ^~~~~

  ...

/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding_pipeline.hpp:56:10: note: in instantiation of function template specialization 'cratchit::functional::AnnotatedOptional<MetaDefacto<MetaDefacto<text::encoding::icu_facade::EncodingDetectionMeta, text::encoding::DetectedEncoding>, std::vector<std::byte>>>::and_then<cratchit::functional::AnnotatedOptional<std::string> &>' requested here
   56 |         .and_then(monadic::to_platform_encoded_string_step); // #5
      |          ^
```

I feel it is time I learn to read these errors!

If I expand the code that is mine I can see:

```c++
      return persistent::in::path_to_byte_buffer_shortcut(file_path) // #1 + #2
        .and_then(monadic::to_with_threshold_step_f(confidence_threshold))
        .and_then(monadic::to_with_detected_encoding_step) // #4
        .and_then(monadic::to_platform_encoded_string_step); // #5
```

And I know the return type of to_with_detected_encoding_step has changed (as I changed the WithDetectedEncodingByteBuffer).

So how is to_platform_encoded_string_step declared?

Oh! I broke the declaration when I split it into hpp declaration and cpp definition!

Then I ran into this problem:

```c++
  // Default to UTF-8 on detection failure (permissive strategy)
  result = WithDetectedEncodingByteBuffer{
    .meta = DetectedEncoding::UTF8
    ,.defacto = std::move(wt_buffer.defacto)
  };
```

The previous code only passed on the default enum to signal UTF-8. Now I need to somehow provide also the meta-part.

```c++
    struct EncodingDetectionMeta {
      CanonicalEncodingName canonical_name; // ICU canonical name (e.g., "UTF-8")
      std::string display_name;             // Human readable name
      int32_t confidence;                   // ICU confidence (0-100)
      std::string language;                 // Detected language (e.g., "sv" for Swedish)
      std::string detection_method;         // "ICU", "BOM", "Extension", "Default"
    };
```

Or do I really? None of the upstream code actually uses it!

* Can I provide an empty one?
* Can I make the meta into a maybe?

If I provide an empty one I break the invariant that the defacto enum with the encoding must match the meta data for that enum. But then, do my existing AI generayed code even care? That is, have I already code that breaks this invariant?

If I make the meta into a maybe I probably don't solve the problem as all code that access the meta must then also handle the nullopt. But then, this is also maybe not a problem? Maybe every such case is paired with an enum 'Undefied or Unknown'?

But I extended the type to make maybe version signal mneta-data that variant version used. I did NOT extend it to provide meta-data upstream.

* So we know that for now it is fine to break the invariant upstream as it does not access the meta part.
* But future code may be tricked to access it and cause buds when the invariant is broken!

This was a curious dilemma. Where have I gone wrong? Or hove can I proceed correctly?

I wonder, how many hard coded 'default value' sites do I have? 

AHA! It seems there is only one!?

```c++
        else {
          // Default to UTF-8 on detection failure (permissive strategy)
          result = WithDetectedEncodingByteBuffer{
            .meta = DetectedEncoding::UTF8
            ,.defacto = std::move(wt_buffer.defacto)
          };
          result.push_message(
            std::format(
              "Encoding detection failed (confidence below threshold {}), defaulting to UTF-8"
              ,confidence_threshold));
        }

```

And from below we have the productions:

```c++
  // ...

          // UTF-8 BOM: EF BB BF
          if (bom_bytes[0] == 0xEF && bom_bytes[1] == 0xBB && bom_bytes[2] == 0xBF) {
            return EncodingDetectionResult{
              .meta = {
                 "UTF-8"
                ,"UTF-8"
                ,100
                ,""
                ,"BOM"
              }
              ,.defacto = DetectedEncoding::UTF8
            };

  // ...

        if (ext == ".csv") {
          return EncodingDetectionResult{
            .meta = {
              "UTF-8"
              ,"UTF-8"
              ,60
              ,""
              ,"Extension (.csv)"
            }
            ,.defacto = DetectedEncoding::UTF8
          };

  // ...

        return EncodingDetectionResult{
          .meta = {
            "UTF-8"
            ,"UTF-8"
            ,30
            ,""
            ,"Extension (default)"
          }
          ,.defacto = DetectedEncoding::UTF8
        };
        
```

So my AI friends has already generated hard coded meta-data.

It is also integersting that the propblem with the semantics of 'confidence' creeps up here again. My AI friends has stated 100% confidence from BOM detection, 60% confidence from '.csv' extension and 30% confidence for 'other file extension' assumed encoding?

This is LEAKY AF!

On the other hand, I don't make it worse? I did this:

```c++
          // Default to UTF-8 on detection failure (permissive strategy)
          result = WithDetectedEncodingByteBuffer{
            .meta = text::encoding::icu_facade::EncodingDetectionResult{
              .meta = {
                "UTF-8"
                ,"UTF-8"
                ,100
                ,""
                ,"Assumed"
              }
              ,.defacto = DetectedEncoding::UTF8
            }
            ,.defacto = std::move(wt_buffer.defacto)
          };

```

This makes the code compile for now. Also, no encoding tests fail so I asume we can settle woth this for now?

I now have implemented the maybe and monadic versions.

```c++

      std::optional<WithDetectedEncodingByteBuffer> to_with_detected_encoding_step(WithThresholdByteBuffer wt_buffer) {
        std::optional<WithDetectedEncodingByteBuffer> result{};

        auto& [confidence_threshold,buffer] = wt_buffer;
        auto encoding_result = icu_facade::maybe::to_detetced_encoding(buffer, confidence_threshold);

        if (encoding_result) {      
          result = WithDetectedEncodingByteBuffer{
             .meta = encoding_result.value()
            ,.defacto = std::move(buffer)
          };
        } 

        return result;
      }

      // ...

      AnnotatedMaybe<WithDetectedEncodingByteBuffer> to_with_detected_encoding_step(WithThresholdByteBuffer wt_buffer) {

        AnnotatedMaybe<WithDetectedEncodingByteBuffer> result{};

        auto const& [confidence_threshold,buffer] = wt_buffer;
        auto lifted = cratchit::functional::to_annotated_maybe_f(
           text::encoding::maybe::to_with_detected_encoding_step
          ,std::format(
              "Encoding detection failed (confidence below threshold {}), defaulting to UTF-8"
              ,confidence_threshold)
        );

        result = lifted(wt_buffer);

        if (result) {
          result.push_message(
            std::format("Detected encoding: {} (confidence: {}, method: {})",
                      result.value().meta.meta.display_name,
                      result.value().meta.meta.confidence,
                      result.value().meta.meta.detection_method)
          );
        }
        else {
          // Default to UTF-8 on detection failure (permissive strategy)
          // TODO 20260124 - Consider to remove this else?
          //                 It seems no test even triggers this else path?
          //                 Or, the detection logic already defaults to UTF-8 (Never nullopt)?
          result = WithDetectedEncodingByteBuffer{
            .meta = text::encoding::icu_facade::EncodingDetectionResult{
              .meta = {
                "UTF-8"
                ,"UTF-8"
                ,100
                ,""
                ,"Assumed"
              }
              ,.defacto = DetectedEncoding::UTF8
            }
            ,.defacto = std::move(wt_buffer.defacto)
          };
        }

        return result;

      } // to_with_detected_encoding_step

```

I then made the maybe version into a pure monadic composition as:

```c++
      std::optional<WithDetectedEncodingByteBuffer> to_with_detected_encoding_step(WithThresholdByteBuffer wt_buffer) {
        auto& [confidence_threshold,buffer] = wt_buffer;
        return icu_facade::maybe::to_detetced_encoding(buffer, confidence_threshold)
          .transform([buffer = std::move(buffer)](auto&& meta){
            return WithDetectedEncodingByteBuffer{
              .meta = std::forward<decltype(meta)>(meta)
              ,.defacto = std::move(buffer)
            };
          });
      } // to_with_detected_encoding_step
```

* I move-capture the buffer into the lambda
* I already move-from the meta into the lambda
* The lambda forwards the meta (to take advantage of any move-assignment)
* And the lamda already move the buffer to the result

It seems I have all the bells and whistles in place correctly? At least all test run without any excpetions or faults. Even in the debugger I get no catch on any faults or memory issues.

It is worth noting though that none of the tests triggered the else that hard codes the moinadic default to UTF-8.

* Does the detection code already default to UTF-8?
* Are there no tets on the fallback to UTF-8?

## 20260123

So next step is to make a maybe variant of to_with_threshold_step.

I studied the code and got confused by icu::to_detetced_encoding. I imagined this was from the ICU library. But it was not. It was my own icu namespace. Ok, so I renamed my 'namespace icu' to 'namespace icu_facade'.

But now my code fail on 'icu_facade::UnicodeString'. It turns out the ICU library DOES itself define an icu namespace! So good thing I renamed my own to not get confused in the future.

So I fixed all places where the code was using actual ICU code as icu:: ok.

So I feel I have to tread carefully in this step of refactoring. I started by clarifying existing 'maybe' returning functions in encoding unit. I introduced icu_facade::maybe and moved them there.

I continued to implement maybe-variant of to_with_threshold_step. ANd in the process I renamed 'to_annotated_nullopt' to 'to_annotated_maybe_f' ro reflect it takes a maybe-returing function and produces a function that returns an annotated maybe.

I managed to implement text::encoding::maybe::to_with_threshold_step_f so I could do:

```c++
      auto result = persistent::in::maybe::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::maybe::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::maybe::to_with_threshold_step_f(100));
```

I also managed to make monadic variant lift the maybe-variant as:

```c++
      AnnotatedMaybe<WithThresholdByteBuffer> ToWithThresholdF::operator()(ByteBuffer byte_buffer) const {
        auto lifted = cratchit::functional::to_annotated_maybe_f(
           text::encoding::maybe::to_with_threshold_step_f(confidence_threshold)
          ,"Failed to pair confidence_threshold with byte buffer"
        );
        return lifted(byte_buffer);
      }
      ToWithThresholdF to_with_threshold_step_f(int32_t confidence_threshold) {
        return ToWithThresholdF{confidence_threshold};
      }
```

That is, I made the indirection functor ToWithThresholdF use to_annotated_maybe_f to lift the maybe variant with an annotation and call it.

It would have been nice if I could implement to_with_threshold_step_f return the to_annotated_maybe_f from maybe variant directly. But then I don't know the return type any longer. And tgus can't separate declaration from the definition.

I wonder it there is some C++ template magic I can use to create the return type of to_annotated_maybe_f and use in declaration and definition of to_with_threshold_step_f?

I asked chatGPT and it sugested:

```c++
namespace cratchit::functional {

template<typename F>
using annotated_maybe_f_t =
  decltype(to_annotated_maybe_f(
    std::declval<F>(),
    std::declval<std::string>(),
    std::declval<std::string>()
  ));

} // namespace cratchit::functional

// and usage

namespace text::encoding::monadic {

  using MaybeStep =
    cratchit::functional::annotated_maybe_f_t<
      decltype(text::encoding::maybe::to_with_threshold_step_f(0))
    >;

  MaybeStep to_with_threshold_step_f(int32_t confidence_threshold);

} // namespace text::encoding::monadic
```

At first I thought this was a catch 22. But then I realised the 'monadic' variant uses the 'maybe' variant of to_with_threshold_step_f.

So I wrote this code for the monaic variant:

```c++

      // hpp
      using ToWithThresholdF =
        cratchit::functional::annotated_maybe_f_t<
          decltype(text::encoding::maybe::to_with_threshold_step_f(0))
        >;

      ToWithThresholdF to_with_threshold_step_f(int32_t confidence_threshold);    

      // cpp
      ToWithThresholdF to_with_threshold_step_f(int32_t confidence_threshold) {   
        auto lifted = cratchit::functional::to_annotated_maybe_f(
           text::encoding::maybe::to_with_threshold_step_f(confidence_threshold)
          ,"Failed to pair confidence_threshold with byte buffer"
        );
        return lifted;
      }
```

So at least I got rid of the functor indirection and made the ToWithThresholdF be the lifted function itself.

## 20260122

I think I have found a viable path though this mess of bloated and unorganised code I have.

With the test case PathToAccountStatementTaggedAmountsRefactoring3 I can focus on making the whole operation path -> tagged amounts as an and_then based on AnnotatedMaybe<T>.

So far I have managed to fill in previous gaps and now have:

```c++
      auto maybe_account_id_ed_table = persistent::in::monadic::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step(100))
        .and_then(text::encoding::monadic::to_with_detected_encoding_step)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_account_id_ed_step);
```

To get to tagged amounts I now need to tackle the 'tas::csv_table_to_tagged_amounts_shortcut(identified_table, account_id);'.

* It is a shortcut (meaning it aggregates steps)
* It takes two arguments (so can not work with my existing AnnotatedMaybe<T>::and_then(arg)).

* It seems we need to aggregate identified_table, account_id into one 'thing'. A MetaDefacto seems a viable approach.
* The refactor account::statement::maybe::csv_table_to_account_statement_step(table, account_id)

NO!

* The account::statement::maybe::csv_table_to_account_statement_step(table, account_id) is to be removed.
* I already have the next steps.

Satying really focused I found the steps I wanted to use and now have the tail:

```c++
    auto maybe_account_statement = maybe_account_id_ed_table
      .and_then(cratchit::functional::to_annotated_nullopt(
          account::statement::maybe::account_id_ed_to_account_statement_step
        ,"Account ID.ed table -> account statement failed"));

    auto maybe_tagged_amounts = maybe_account_statement
      .and_then(cratchit::functional::to_annotated_nullopt(
          tas::maybe::account_statement_to_tagged_amounts_step
        ,"Failed to transform Account Statement to Tagged Amounts"));

    ASSERT_TRUE(maybe_tagged_amounts) << "Expected succesfull tagged amounts";
```

I'm getting there! And it works to focus on the PathToAccountStatementTaggedAmountsRefactoring3 until I have the whole path -> tagged amounts based on AnnotatedMaybe<T>! Even with it I get lost over and over.

I have now stayed focused and made the AnnotatedMaybe<T> chain complete:

```c++
    auto maybe_tagged_amounts = persistent::in::monadic::path_to_istream_ptr_step(m_valid_file_path)
      .and_then(persistent::in::monadic::istream_ptr_to_byte_buffer_step)
      .and_then(text::encoding::monadic::to_with_threshold_step(100))
      .and_then(text::encoding::monadic::to_with_detected_encoding_step)
      .and_then(text::encoding::monadic::to_platform_encoded_string_step)
      .and_then(CSV::parse::monadic::csv_text_to_table_step)
      .and_then(account::statement::monadic::to_account_id_ed_step)
      .and_then(account::statement::monadic::account_id_ed_to_account_statement_step)
      .and_then(tas::monadic::account_statement_to_tagged_amounts_step);
```

Now I think it is time to clean upp so that we also have this chain on std::optional?

We do not yet have a  maybe varaint of path_to_istream_ptr_step(path).

So I have now implemented persistent::in::maybe::path_to_istream_ptr_step. I ran into test cases that now fails as they expect also successfull operations to inject side channel messages. So I made to_annotated_nullopt to take an optional message_on_value and made 'open file' log an 'opened' ok message to make CompleteIntegrationAllSteps test pass.

I am on the way OK. But we are in the weeds in more ways.

* Google tests also executes cratchit normal 'read environment' and sync with sie-files.
* And this sync FAILS with:

```sh
...
BEGIN: model_from_environment_and_md_filesystem 
BEGIN REFACTORED posted SIE digest

STAGE of cracthit entries FAILED when merging with posted (from external tool)
Entries in sie-file:sie_in/TheITfiedAB20250812_145743.se overrides values in cratchit staged entries
CONFLICTED! : No longer valid entry  A3 "Account:NORDEA Text:BG KONTOINS Message:5050-1030 SK5567828172" 20250516
    1630 "" -1998.00
    1920 "" 1998.00
 year id:current - ALL POSTED OK
END REFACTORED posted SIE digest
SIE year id:current --> Tagged Amounts = size:9
Account Statement Files --> Tagged Amounts = size:9
[       OK ] ModelTests.Environment2ModelSIEStageOverlapConflictTest (5 ms)
...
```

What is going on?

* Is this triggered by Environment2ModelSIEStageOverlapConflictTest?

YES! 

* So the test needs to be updated to do semthing with the 'conflict'?

Also, a normal start of cratchit seems ok enough for now:

```sh
...
BEGIN: model_from_environment_and_md_filesystem 
BEGIN REFACTORED posted SIE digest
 year id:-2 - ALL POSTED OK
 year id:-1 - ALL POSTED OK
 year id:current - ALL POSTED OK
END REFACTORED posted SIE digest
SIE year id:-1 --> Tagged Amounts = size:440
SIE year id:-2 --> Tagged Amounts = size:889
SIE year id:current --> Tagged Amounts = size:1073
Account Statement Files --> Tagged Amounts = size:1073
Init from "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/cratchit.env"
cratchit:>
```

Back to my refactoring. Next step is to implement a maybe variant of 'istream_ptr_to_byte_buffer_step'.

I am a bit unsatisfied wigh the code options I found for 'proper' reading of bytes from std::istream. I used charGPT and Claude Code to serve me code snippets and I chose one but added three options in the code for future refactoring.

It seems my AI coding friends are also confused aabout error handling in reading from std::istream. When I fed code back they changed their minds and proposed more safety nets and tweak.

I tweaked std::istream reading  with placeholder code and TODO about returning std::expected instead for better error tracking.

The code should be stable against error for now I suppose?

## 20260121

I am frustrated that fixing the account statement csv file parsing pipe line is so hard! I have a real problem to navigate the source code and see though the mess what is there, what is AI slop and what is redundancy.

I have come up with an approach to try and navigate to a slimmer and focused cleaned up design.

1. Identify the pipe line that is actually in use.
2. Write down this pipe line in a test case to fix it in code for refactoring stability.
3. Propose a new pipe line where Account ID is produced based on new account statement table meta analysis.
4. Wite down the new pipe line in a test case to fix it in code for refactoring stability.
5. Replace old pipe line with the new one.
6. Remove redunant code and test cases based on new pipe line test case.

### So where is the current pipe line?

I found one candidate in tas_from_statment_file:

```c++
OptionalTaggedAmounts tas_from_statment_file(std::filesystem::path const& statement_file_path) {

  if (true) {
    std::cout << "\nto_tagged_amounts(" << statement_file_path << ")";
  }

  if (true) {
    // Refactored to pipeline

    auto result = file_path_to_istream(statement_file_path)
      .and_then(istream_to_decoding_in)
      .and_then(decoding_in_to_field_rows)
      .and_then(field_rows_to_table)
      .and_then(table_to_account_statements)
      .and_then(account_statements_to_tas);
```

I am surprised this pipe line is not made up my xxx_step named functions.

I also found:

```c++
  TEST_F(MonadicCompositionFixture,PathToAccountIDedTable) {
    logger::scope_logger log_raii(logger::development_trace,"TEST_F(MonadicCompositionFixture,PathToAccountIDedTable)");

    auto maybe_account_id_ed_table = persistent::in::monadic::path_to_istream_ptr_step(m_valid_file_path)
      .and_then(persistent::in::monadic::istream_ptr_to_byte_buffer_step)
      .and_then([](auto const& byte_buffer){
        return text::encoding::monadic::to_with_threshold_step(100,byte_buffer);
      })
      .and_then(text::encoding::monadic::to_with_detected_encoding_step)
      .and_then(text::encoding::monadic::to_platform_encoded_string_step)
      .and_then(cratchit::functional::to_annotated_nullopt(
        CSV::parse::maybe::csv_text_to_table_step
        ,"Failed to parse csv into a valid table"))
      .and_then(cratchit::functional::to_annotated_nullopt(
          account::statement::maybe::to_account_id_ed_step
        ,"Failed to identify account statement csv table account id"
      ));

    ASSERT_TRUE(maybe_account_id_ed_table) << "Expected successful account ID identification";
  }
```

So what pipe line is used by first variant fron end?

So AccountStatementFileState uses CSV::parse::monadic::file_to_table.

And then it provides an option table -> maybe account statement as:

```c++
  auto annotated_statement = annotated_table
    .and_then(to_annotated_nullopt(
      account::statement::maybe::to_account_id_ed_step,
      "Unknown CSV format - could not identify account"))
    .and_then(to_annotated_nullopt(
      account::statement::maybe::account_id_ed_to_account_statement_step,
      "Could not extract account statement entries"));

```

That is table -> id:ed table -> statement using the 'annotated' pipeline.

I am starting to see some pieces in the puzzle.

* We have some steps based on std::optional and some on leverage 'AnnotatedMaybe'
* We are using steps that are not yet named xxx_step for some reason
* We have no test case for the whole pipline file path -> tagged amounts
* We have disorder and confison in file naming and locations
* We have simmilar disorder and confusion about test cases and locations of those.

So where can I place the test case to achor the file path -> tagged amounts full pipe line?

* TEST_F(MonadicCompositionFixture,PathToAccountIDedTable) is in test_csv_import_pipeline
* TEST(AccountStatementDetectionTests, DetectColumnsFromNordeaHeader) is in test_csv_table_identification
* TEST_F(MDTableToAccountStatementTestFixture, SKVNewerMDTableToAccountStatement) is in test_md_table_to_account_statement

Wow, it is really 'upp hill'!!

But the namespace namespace tests::csv_import_pipeline::monadic_composition_suite seems the best candidate for now. It is in test_csv_import_pipeline.

* So first front end has not yet any account statement -> tagged amounts in the ux

Now I found a TODO comment for 'tas_from_statment_file' in TaggedAmountFramework. It states the current pipe line is 'csv::path_to_tagged_amounts_shortcut in csv/import_pipeline.hpp'.

So tas_from_statment_file is not used? Yes, it is NOT called from anywhere!

So what does path_to_tagged_amounts_shortcut do?






## How can I parse account statement csv files in a somewhat generic manner [20260120]?

I decided to continue my thinking here on top (See thinging until now below).

I am back at the plan:

1. Extend 'ColumnMapping' to a 'CSV::TableMeta' with more meta-data from identification process
2. Introduce an array of sz_xxx texts to test to parse into account statement tables
3. Extend 'coumn mapping' to also identify BBAN, BG and PG account number fields
4. Constrain 'header' to be valid only if header has the same coumn count as the in-saldo,out-saldo and trans entry rows

Where I am in the process of replacing to_column_mapping with to_account_statement_table_meta.

I have now decided to clean up the failing tests before I continue.

```sh
[  FAILED  ] 4 tests, listed below:
[  FAILED  ] MonadicCompositionFixture.PathToAccountIDedTable
[  FAILED  ] GenericStatementCSVTests.NORDEAStatementOk
[  FAILED  ] AccountStatementDetectionTests.GeneratedSingleRowMappingTest
[  FAILED  ] AccountStatementDetectionTests.GeneratedRowsMappingTest
```

This is a MESS!!

The tests are AI slop hard coded to fail if not a NORDEA or SKV format. So If I make PathToAccountIDedTable pass by e.g., making to_account_id_ed_step return some generic ID, then I fail tests:

```sh
[  FAILED  ] AccountIdTests.UnknownCsvReturnsNullopt
[  FAILED  ] AccountIdTests.TableWithOnlyHeadingUnknownFormatReturnsNullopt
[  FAILED  ] FullPipelineTestFixture.ImportSimpleCsvFailsForUnknownFormat
[  FAILED  ] FullPipelineTableTests.ImportTableFailsForUnknownFormat
[  FAILED  ] GenericStatementCSVTests.NORDEAStatementOk
```

*sigh*

So what is the work around for now? I wonder where I broke these tests in the first case? I actually can't figure out where and when I could have made thes ID.ed mechanism stop working?

Maybe if I fix test GenericStatementCSVTests.NORDEAStatementOk? Why does it fail?

It calls account::statement::maybe::to_entries_mapped(*maybe_table). What the heck is that?

Now I am confused. Did I not just add something simmilar? Where is the latest mechanism I added?

DARN! I have the new code (src/domain/csv_to_account_statement.hpp):

```c++
  enum class FieldType {
      Unknown
    ,Empty
    ,Date
    ,Amount
    ,Text 
    ,Undefined
  };

  struct RowMap {
    std::map<FieldType,std::vector<unsigned>> ixs;
  };

  using RowsMap = std::vector<RowMap>;

```

And I have the 'older' code (src/csv/csv_to_account_id.hpp):

```c++
  enum class ColumnType {
      Undefined
    ,Empty
    ,Text
    ,Amount
    ,Date
    ,Unknown
  }; // ColumnType

  // ...

  using ColumnMap = std::map<int,ColumnType>;

  enum class EntryType {
    Undefined
    ,Emtpy
    ,Caption
    ,Heading
    ,Transaction
    ,Balance
    ,Unknown
  };
```

So we have compeeting code?

```c++
* ColumnMapping to_column_mapping(CSV::Table const& table)
```

And: 

```c++
* EntryMap to_entry_map(CSV::Table::Row const& row)
* EntryType to_entry_type(EntryMap const& entry_map)
* std::optional<CSV::MDTable<EntryMaps>> to_entries_mapped(CSV::Table const& table)
```

AND: to_entries_mapped is not even implemented yet!!

I asked Claude Code to help me find the two approaches in the git log. It came back to me with:

```sh
You have two similar but distinct functions for analyzing CSV table structure:

  1. to_entries_mapped (src/csv/csv_to_account_id.hpp:304-340)

  - Introduced: December 7, 2025 in commit 4a51390
  - Purpose: Maps each CSV table row to entry types (Transaction, Balance, Heading, etc.) with column type information
  - Status: Incomplete - returns empty result (return {}; // failed at line 339)
  - Location: account::statement::generic namespace

  2. to_column_mapping → to_account_statement_table_meta

  - Introduced: January 15, 2026 in commit 827f2ad
  - Evolved: Replaced with to_account_statement_table_meta on January 20, 2026 (commit f1fb929)
  - Purpose: Detects column mappings (date, amount, description columns) from CSV table
  - Status: Fully implemented and in use in the CSV import pipeline
  - Location: src/domain/csv_to_account_statement.cpp

  Key Commits

  # Original to_entries_mapped implementation
  git show 4a51390

  # Original to_column_mapping implementation  
  git show 827f2ad

  # Evolution to to_account_statement_table_meta
  git show 6543d17
  git show f1fb929

  The Similarity

  Both functions analyze CSV table structure, but:
  - to_entries_mapped: Classifies each row by type (Transaction/Balance/etc.) and identifies column types
  - to_column_mapping: Identifies which columns contain date/amount/description data
```

OK. So maybe the december approach with 'to_entries_mapped' is now DEAD? And I can remove it?

* It is not implemented.
* And it is not used.

## How can I parse account statement csv files in a somewhat generic manner?

20260114

The account statement csv file parsing is a MESS due to AI slop. That said, I still have not solved how to parse the 'new' SKV account statemend csv file.

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit/workspace/from_bank_or_skv % cat Bokförda\ transaktioner\ 556782-8172\ Alla\ typer\ 2025-10-01--2025-12-31.csv
"THE ITFIED AB";"556782-8172";"";""
"";"";"";""
"";"Ingående saldo 2025-10-01";"";"660"
"2025-10-04";"Intäktsränta";"1";"661"
"2025-11-12";"Moms juli 2025 - sept 2025";"1 997";"2 658"
"2025-11-14";"Utbetalning";"-1 997";"661"
"2025-12-06";"Intäktsränta";"1";"662"
"";"Utgående saldo 2025-12-31";"";"662"
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit/workspace/from_bank_or_skv % 
```

I now have a 'mapper' that logs:

```sh
[20:46:23.238]               TRACE: BEGIN detect_columns_from_data
[20:46:23.238]                 TRACE: row:﻿"THE ITFIED AB"^556782-8172^^ map: Unknown: 1 Empty: 2 3 Text: 0
[20:46:23.238]                 TRACE: row:^^^ map: Empty: 0 1 2 3
[20:46:23.238]                 TRACE: row:^Ingående saldo 2025-10-01^^660 map: Empty: 0 2 Date: 1 Amount: 3
[20:46:23.238]                 TRACE: row:2025-10-04^Intäktsränta^1^661 map: Date: 0 Amount: 2 3 Text: 1
[20:46:23.238]                 TRACE: row:2025-11-12^Moms juli 2025 - sept 2025^1 997^2 658 map: Date: 0 Amount: 2 3 Text: 1
[20:46:23.238]                 TRACE: row:2025-11-14^Utbetalning^-1 997^661 map: Date: 0 Amount: 2 3 Text: 1
[20:46:23.238]                 TRACE: row:2025-12-06^Intäktsränta^1^662 map: Date: 0 Amount: 2 3 Text: 1
[20:46:23.238]                 TRACE: row:^Utgående saldo 2025-12-31^^662 map: Empty: 0 2 Date: 1 Amount: 3
[20:46:23.238]                 TRACE: returns mapping.is_valid:false
[20:46:23.238]               TRACE: END detect_columns_from_data
```

* I still have the BOM (BOM not rempoved by file reader)
* The 'to_string' produce a starnge '^'-separated string (log uses the to_string for persistent cintent format)
* The 'Ingående saldo 2025-10-01' is parsed as a valid amount

That said, I DO get viable results for the entries of interest for now.

```sh
[20:46:23.238]                 TRACE: row:2025-10-04^Intäktsränta^1^661 map: Date: 0 Amount: 2 3 Text: 1
[20:46:23.238]                 TRACE: row:2025-11-12^Moms juli 2025 - sept 2025^1 997^2 658 map: Date: 0 Amount: 2 3 Text: 1
[20:46:23.238]                 TRACE: row:2025-11-14^Utbetalning^-1 997^661 map: Date: 0 Amount: 2 3 Text: 1
[20:46:23.238]                 TRACE: row:2025-12-06^Intäktsränta^1^662 map: Date: 0 Amount: 2 3 Text: 1
```

So how can I identify what amount is 'transaction' and what is 'saldo'?

```sh
2025-11-12^Moms juli 2025 - sept 2025^1 997^2 658 map: Date: 0 Amount: 2 3 Text: 1
```

* Date in column 0
* Text in column 1
* Amounts in coulmn 2 and 3.

Should I first identify a 'key' to pick out only the entries we are to transform into 'account statement entries'?

If so, I may be tempted to define a key as 'map: Date: 0 Amount: 2 3 Text: 1'?

But then, should I first find out what amount is what? That is, keep the transaction amount and ignore the saldo amount for now?

Should I also write tests to keep me focused and get reliable code?

YES, I should write tests. Let's do do this properly this time!

1. Design an algorithm that maps a csv table into a map that projects the table to row ixs to project and a 'key' that defines column ix for Heading (Text), Amount (transaction amount) and Date
2. Write tests for this algorithm sub-steps and final result tests on example csv input
3. Integrate the new algorithm into existing 'account statement csv file import' pipe line.

So what algorithm can I imagine to parse a csv table into account statment entries?

* Is the current pipe line row -> account statement -> tagged amount?

  - extract_entry_from_row(row,mapping) return an std::optional AccountStatementEntry.
  - extract_entry_from_row Tests InvalidDateHandledGracefully,InvalidAmountHandledGracefully,EmptyDescriptionHandledGracefully
  - csv_table_to_account_statement_entries(table) returns OptionalAccountStatementEntries
  - csv_table_to_account_statement_step and account_id_ed_to_account_statement_step calls csv_table_to_account_statement_entries

  WHAT A MESS! (AI slop)

  Ok, lets clear this up. What pipeline can we keep and focus on?

  Where exactly is the pipe line we actually use located? With an SKV and a NORDEA account stetment file in 'from_bank_or_skv' cratchit logs processing as:

  <pre>
      BEGIN File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv"
      [Pipeline] Successfully opened file: /Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv
      [Pipeline] Successfully read 321 bytes from stream
      [Pipeline] Detected encoding: UTF-8 (confidence: 100, method: ICU)
      [Pipeline] Successfully transcoded 321 bytes to 321 platform encoding bytes
      [Pipeline] Step 6 complete: CSV parsed successfully (8 rows)
      [Pipeline] Step 6.5 complete: AccountID detected: 'SKV::5567828172'
      [Pipeline] Pipeline failed at Steps 7-8: Domain transformation failed - Could not extract tagged amounts
    *Note* "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv" (produced zero entries)
    *Ignored* "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv" (File empty or failed to understand file content)
    END File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv"

    BEGIN File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv"
      [Pipeline] Successfully opened file: /Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv
      [Pipeline] Successfully read 1194 bytes from stream
      [Pipeline] Detected encoding: UTF-8 (confidence: 100, method: ICU)
      [Pipeline] Successfully transcoded 1194 bytes to 1194 platform encoding bytes
      [Pipeline] Step 6 complete: CSV parsed successfully (12 rows)
      [Pipeline] Step 6.5 complete: AccountID detected: 'NORDEA::32592317244'
      [Pipeline] Pipeline complete: 11 TaggedAmounts created from 'PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv'
      Valid entries count:11
      Consumed account statement file moved to "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/consumed/PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv"
    END File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv"
</pre>

* The log 'Step 6.5 complete: AccountID detected:' is created in:
  - table_to_tagged_amounts_shortcut
  - csv_to_tagged_amounts_shortcut
  - path_to_account_statement_shortcut
  - path_to_tagged_amounts_shortcut

WHAT A MESS (AI Slop)

So which one is actually used?

It is 'path_to_tagged_amounts_shortcut' that is used!

model_with_dotas_from_account_statement_files
  -> dotas_from_account_statement_files
  -> tas_sequence_from_consumed_account_statement_files
  -> tas_sequence_from_consumed_account_statement_file
  -> path_to_tagged_amounts_shortcut

As part of 'path_to_tagged_amounts_shortcut' we have the pipe line:

* path to maybe_table
* account::statement::maybe::to_account_id_ed_step(*maybe_table)
* csv_table_to_tagged_amounts_shortcut(CSV::Table const& table,AccountID const& account_id)

Another short cut! Anyhow...

As part of 'csv_table_to_tagged_amounts_shortcut(CSV::Table const& table,AccountID const& account_id)' we have:

* csv_table_to_account_statement_step(table, account_id)
* account_statement_to_tagged_amounts_step(*maybe_statement)

After this we are DONE!

So the crux is in csv_table_to_account_statement_step.

it calls csv_table_to_account_statement_entries(table).

* Tries detect_columns_from_header(table.heading)
* If fails tries detect_columns_from_data(table.rows)
* Iterates extract_entry_from_row(row, mapping)

It seems the step we need to tweak is table.rows -> ColumnMapping?

Maybe we can do it like this.

1. Make detect_columns_from_data work on SKV file format using new mapping method?
  - Create a ColumnMapping for the entries we want to turn into account statement entries?
2. Make extract_entry_from_row ignore rows that does not work with the mapping?
  - We can do this in is_ignorable_row (ignore if mapping does now work)

Idea: Mark the code with bookmark '#ISSUE20260114_SKV_CSV' to keep track of where I am?

Here I added the 'rinse-and-repeat' python based mechanism. So how should I rig my project to make the rinse-and-repeat keep me on track to make the SKV csv account statement file parsing work?

* Should I make a test case for detect_columns_from_data?
* Do I already have test cases for csv_table_to_account_statement_step?
  YES!
  - CSVTable2AccountStatementTests::CsvTableToAccountStatementWithNordea
  - CSVTable2AccountStatementTests::CsvTableToAccountStatementWithSKV
  - CSVTable2AccountStatementTests::CsvTableToAccountStatementPreservesEntryData
  - CSVTable2AccountStatementTests::CsvTableToAccountStatementWithInvalidTable
  - CSVTable2AccountStatementTests::CsvTableToAccountStatementEmptyEntriesIsValid
  - CSVTable2AccountStatementTests::CsvTableToAccountStatementIntegrationPipeline

  - CSVTable2TaggedAmountsTests::IntegrationWithNordeaCsvData
  - CSVTable2TaggedAmountsTests::IntegrationWithSkvCsvData

YES. I renamed the google tests as above to be able to filter out and run those tests specifically.

Now, what do I need to test the 'detect_columns_from_data'?

* I already have google tests that calls detect_columns_from_data.
  - TEST(AccountStatementTests, DetectColumnsFromData)
  - TEST(AccountStatementTests, DetectBothAmountColumns)

* Added TEST(AccountStatementDetectionTests, DetectColumnsFromSKVNew)

20260115 I have now implemnted an algorithm to map an SKV csv account statement file to a 'ColumnMapping' in 'detect_columns_from_data'. It is not pritty but it works.

What more can I imagine to do before closing the claude-001-refactor-csv-import-pipeline branch?

* Consider to implement an architecture that allows to add more ways to parse an account statement file?
  - Consider to base the architecture on detect_columns_from_data -> ColumnMapping?
  - It seems the code already applies the ColumnMapping only to rows it 'fits' (so we can utilise this as part of how the alögorithm works)?
* Maybe we can create a map of candidate Tabkle Rows -> ColumnMapping functions?
  And have the detect_columns_from_data apply one by one until one succeeds?
  - Then we would be able to know how to extend future parsing with new projections as we add account statement files we can parse?

* Consider what we can do to the parse-csv-file-pipeline?
  - We want to get rid of all the multi-step 'shortcuts'?
  - Consider to single out the pipeline steps to keep by placing them in cpp-definition?
    The we can later refactor by removing all '..._chortcut' functions in hpp-files?

Here is an idea to clean up the pipe line.

1. Refactor all '..._shortcut' functions to return only a single 'and_then' functions aggregate of '_step' functions.
2. Then remove one '_shortcut' at a time and replace any calls to them with the proven 'amnd_then' aggregation.
  - Also remove all tests using '_shortcut' steps.

Well, back to the architecture for Table Rows -> ColumnMapping. What can we imagine to change to clarify the algorithm for future extension to be able to parse more types of account statement files?

* Lets keep the projection to ColumnMapping for now.

I introduced to_coumn_mapping(table) and used in csv_table_to_account_statement_entries.

Now we can imagine a vector of such to_coumn_mapping to have csv_table_to_account_statement_entries try?

## What is the next step to make a design where we can add Table -> ColumnMapping for future csv file formats?

I still hesitate to add a loop-up table of Table -> ColumnMapping functions.

* Should I keep Heading -> ColumnMapping functionality (seems fragile)?
* Should I extend Table Data -> ColumnMapping (more complex but may be more realiable and maintainable)?
* And what pipeline functions are actually 'in use' and what are duplicates (shortcuts and AI slop)?

Thinkig about this for a while I have concluded the following for the Table -> ColumnMapping

* Ensure every table has a Header (generate a row0,row1,... if none exist)
  I think this is already in place?
* Be able to identify entries 1xDate, 1xAmount, >1xText
* Be able to identify entries 1xDate,2xAmount (trans + saldo), > 1xText
* Allow the 2xAmount IFF the amounts tarns_1,saldo_1 fullfill saldo_0 + trans_1 = saldo_1

I imagine I can implememt this as follows:

1. Implement test cases for invented csv tables
2. Ensure we can parse invented tables to csv Table WITH header (extracted or generated)
3. Implement vector of f: Table -> ColumnMapping so we have enough f:s to parse all invented tables
4. Make **to_coumn_mapping(table)** try f:s in vector of f:s until one returns a mapping.

## What invented csv tables can we imagine to try?

Imagine some headless tables with Date,Text,Amount permutations?

* Date,Text,Amount
* Date,Amount,Text
* Text,Date,Amount
* Text,Amount,Date
* Amount,Date,Text
* Amount,Text,Date

Maybe a test case that generates theese permutations?

Imagine some headless tables with Date,Text,TransAmount,SaldoAmount permutations?

Maybe a test case that generates theese permutations?

## What algorithm can we imagine to generate invented csv tables for account statement parse testing?

1. We seem to need to generate with or without saldo amount?
  - So we can make the algorithm invent some random transactions and always also keep a running saldo.
  - Then inject or not inject the saldo amount depending on a flag.
2. It seem we need to generate some set of text fields?
3. It seem we should also generate random 'to' and 'from' fields with 'account numbers'?

What account number formats have I already implemented?

* account::BBAN
* account::giro::BG
* account::giro::PG

So we can imagine to hard code some BBAN,BG and PG in a std::variant set and randomly use

4. It seem we need an account number for the csv table we are generating?
  - The csv table account number can be a BBAN,BG or PG?
5. It seems we can use the same account number generator to create to and from account numbers?

WOW! This is COMPLEX!!

Anyhow, lerts start to create the new test cpp-unit and the **to_coumn_mapping(table)** as function im vector of options.

What test units do we have?

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % tree src/test 
src/test
├── data
│   └── account_statements_csv.hpp
├── test_amount_framework.cpp
├── test_amount_framework.hpp
├── test_annotated_optional.cpp
├── test_atomics.cpp
├── test_atomics.hpp
├── test_csv_import_pipeline.cpp
├── test_fixtures.cpp
├── test_fixtures.hpp
├── test_generic_account_statement_csv.cpp
├── test_giro_framework.cpp
├── test_integrations.cpp
├── test_integrations.hpp
├── test_md_table_to_account_statement.cpp
├── test_runner.cpp
├── test_runner.hpp
├── test_statement_to_tagged_amounts.cpp
├── test_swedish_bank_account.cpp
├── test_transcoding_views.cpp
└── zeroth
    ├── test_zeroth.cpp
    └── test_zeroth.hpp

3 directories, 21 files
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % 
```

* Reuse test_csv_import_pipeline?
* Reuse test_generic_account_statement_csv?
* Reuse test_md_table_to_account_statement?

I ended up adding new test_csv_table_identification.cpp and moved detection test code there.

I now have the account statement table mapping detection tests in test_csv_table_identification ok. I also test only the 'top' to_column_mapping(table).

So I turned to clean up the current test cases that fails.

It trn out I now have SIEEnvironmentTests.StageToPostedOkTest failing for some unknown reason.

To track this I started adding logging to see where things go wrong.

And in doing so I needed a first::to_string for optional DateRange. But when I added this my code stopped compiling. Now the compiler fails to find  to_string(Date) and to_string(Amount) when called from withing namespace first! WTF?!!

## How can I make all free function to_string overloads to be found by the compiler?

The oproblem seems to be that as long as namespace first has NO to_string overload, then the compiler finds to_string(Date) and to_string(Amount) when called from inside namespace first. But if there is one, then the compiler gives up when trying all to_string inside namespace first?

So where is to_string(Date) and to_string(Amount)?

It turns out I have placed them in global namespace.

But is this the recommended way to expose free functions? I mean, compiler will apply ADL (argument dependant lookup) to find *unqualified function names*. So if I call top_string(T t), then it WILL find any to_string in the nemaspace where T is defined.

* So I should keep my types and functions on these types in their namespace?
* And only fix call to overloaded functions at call sites inside namespaces to make them find functions in other name spaces?

But then, why does not ADL work inside a name space? That is, is it true that ADL is not applied if the local namespace already have a free function with unqualified name?

* I expected a call to 'to_string' to look first in the namespace where the argument type is defined.
* Why is this not so?

It seems I need to start at C++ compiler [Name lookup](https://en.cppreference.com/w/cpp/language/lookup.html)?

So 'name lookup' is the mechanism where the compiler does identifier -> declaration.

* The name is either 'unqualified' (no namespace path provided) or 'qualified' (namespace path provided)

  - to_string(date) is an unqualified named 'to_string'
  - first::to_string(dat) is a qualified named 'first::to_string'

* [Unqualified name lookup](https://en.cppreference.com/w/cpp/language/unqualified_lookup.html)
* [Qualified name lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup.html)

AHA! The unqualified name lookup searches scopes '...until it finds at least one declaration of any kind'! That is, it does NOT care if the declaration found suits the code using it! In effect, if at least one declaration is found, then no more scopes are considdered!

This is NOT what I expected. I assumed the compiler would consider ALL scopes to get a FULL set of viable declarations.

In this way the viable scopes is NOT affected by the content of the scopes. But the actual name lookup behavious DOES change depending on scope content!

*sigh*

So what options do I have to adress this issue?

* I want my types and functions on those types to recide in namespaces.
* I want any call site to see the applicable function(s).
* I want to be able to use overloads (e.g. to_string overloads for types)

It seems I have at least two options?

1. Always place overloads in global namespace?
2. Manually import namespaces with 'using' at call sites as required (I can't make the compiler find them)?

Are there other options I did not thing of?

I think I will add a cpp_compile_playground.cpp to have a space to have a dialog with the compiler with code to determine how it actually behaves.

I managed to replicate the compiler error!

1. Having a global namespace to_string
2. AND - having a first::to_string
3. CAUSES: A call from withing namespace first of to_string to fail (can't see wnated global to_string)

The problem is that when there is a to_string in current namespace, then only this is considered by name lookup.

So the conclusion is:

1. Put functions and processed types in the same namespace
2. Rely on ADL to make overload lookup work from call site namespace to namespace with deffined code.
3. Thus, it seems to be OK to bring in types to global namesapce as aliases 'using a_type = a_namespace::a_type'
   ADL seems to still find functions in the namespace of the argument types?

Decision: Move to_string(Date) and to_string(Amount) to the namespace of DAte and Amount.

WAIT! This does NOT work!

* Date is an **alias** for std::chrono::year_month_day!
* Thus, ADL will look into namespace std::chrono for a to_string (NOT the namespace where the alias is defined)
* So putting to_string in e.g. namespace first will NOT make the compiler find it!

gave this some more thought and realised:

* I can just make to_string(DateRange) in global namespace for now!
* And add comments about future 'better' options.

## Consider to extend ColumnMapping to a 'TableMeta' and make tests based on array of sz_xxx csv text to try to parse?

I have thought about account statement csv file parsing and identification a bit more. I am now tempted to try:

1. Extend 'ColumnMapping' to a 'CSV::TableMeta' with more meta-data from identification process
2. Introduce an array of sz_xxx texts to test to parse into account statement tables
3. Extend 'coumn mapping' to also identify BBAN, BG and PG account number fields
4. Constrain 'header' to be valid only if header has the same coumn count as the in-saldo,out-saldo and trans entry rows

I Imganine the TableMeta to contain:

* A boolean flag 'has columns header'
* row index of first and last account transaction row
* The 'transaction row column mapping'
* row index of 'in saldo' and 'out saldo' row
* a list of row indexes of unidentifed rows

So let's first refactor 'to_column:mapping -> ColumnMApping' to 'to_table_meta -> TableMeta' and adjust clienbt code accordingly. 

So I introduced to_account_statement_table_meta and replaced to_column_mapping in tests ok.

I then made MonadicCompositionFixture.PathToAccountIDedTable test pass by inactivating to_account_id_ed_step.

But now ALL tests that relies on correctly identifying NORDEA or SKV account stements fail!

So I think I activate old to_account_id_ed_step again for now.

DARN! Now I discover my statically instantikated loggers are instable!

## How can I make my singleton-like loggers JIT created and deterministically destroyed?

I currently have 'static' logger instances. This does not work well with the google test framework. It seems the framework is (may) be instantiated and run before my own loggers are properly instantiated. Thus my code somethimes break when I use my loggers in google test code.

It seems I have maybe two options.

1. Make my loggers be JIT instantiated somehow (ensure they are created on call if they are not yet created)
2. Somehow try to make the google test framework be deterministically instantiated after my statig loggers?

So where and when is my google test framework instantiated?

Hm... It seems in main(argc,argv) I trigger on argument '--test' or '--gtest_filter' and then calls tests::run_all.

This will do:

```c++
  ::testing::InitGoogleTest(&argc,argv);
  ::testing::AddGlobalTestEnvironment(fixtures::TestEnvironment::GetInstance());
  int result = RUN_ALL_TESTS();        
```

Here I used the lldb debugger and found:

```sh
(lldb) breakpoint set -E C++
Breakpoint 1: 2 locations.
(lldb) run --test
There is a running process, kill it and restart?: [Y/n] Y
Process 38799 exited with status = 9 (0x00000009) killed
Process 44456 launched: '/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/cratchit' (arm64)
Process 44456 exited with status = 9 (0x00000009) Terminated due to code signing error
(lldb
```

So it was in fact a code signing error all along?!

*sigh*

Back to 