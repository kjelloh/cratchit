# Thinking out loud on cracthit development

I find thinking out loud by writing to be a valuable tool to stay focused and arrive faster at viable solutions.

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
4. Make to_coumn_mapping(table) try f:s in vector of f:s until one returns a mapping.

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
