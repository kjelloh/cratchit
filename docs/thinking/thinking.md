# Thinking out loud on cracthit development

I find thinking out loud by writing to be a valuable tool to stay focused and arrive faster at viable solutions.

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