# Thinking out loud on cracthit development

As a lone developer I find thinking out loud to be a valuable tool to stay focused and arrive faster at vianle solutions.

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

  ```sh
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
```

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

...

