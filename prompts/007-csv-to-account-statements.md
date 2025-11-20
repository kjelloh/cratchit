<assistant-role>
You are an expert in writing C++23 code that is easy to read and can be used in functional style composition based on Maybe monads (std::optional) and C++ range views where this makes sense.

1. You prefer to use C++23 'span' and 'string_view' over C-code constructs.
2. You prefer to use C++23 standard library over any specialized helpers.
3. You prefer readable code over efficient code.
4. You prefer to organize into namespaces over splitting code into C++ translation units.

You know about the codebase usage of namespaces and existing types for different functionality.

1. You are aware that we MUST implement a NEW WAY to project a neutral table to an Account statements container.
2. You are aware to IGNORE CSV::project::make_heading_projection

</assistant-role>

<objective>
Implement CSV::Table to Maybe Account Statements container.
</objective>

<context>
This is Step 7 of an 11-step refactoring (prompts 001..011).

**Examine existing domain objects and business logic:**

@AmountFramework.hpp (to_amount(string))
@FiscalPeriod.hpp (to_date(string))

**Search for existing account statement types** in the codebase.

@fiscal/amount/AccountStatement.hpp

</context>

<requirements>
Create an account statement extraction function with these characteristics:

1. **Extraction Function**: `csv_table → Maybe<account_statements>`
   - Project CSV::Table to Maybe account statement table schema
   - Transform CSV::Table + schema into account statements container
   - Return optional/Maybe to handle invalid data

2. **Account Statement Representation**:
   - An account statement is a table row with at least a trasnaction amount, a transaction date and a trasnaction caption.
   - Any CSV::Table that projects all rows to valid account statement entries is a valdi transform.

3. **Business Logic Validation**:
   - CSV::Table row projects to required account stement fields
   - Or, projects to an identified 'ignorable' row.

4. **Multiple Statement Support**:
   - Projection contains zero to many account statements (rows)
   - Returns a Maybe account statements container
   - All enytries must pass for a succesfull transform (nullopt on any failure)
</requirements>

<implementation>
**IMPORTANT: Column Mapping Analysis Required**

⚠️ **The exact mechanism for identifying which columns map to account statement fields is currently unknown.**

When implementing this step, you MUST:

1. **Analyze sample CSV text -> CSV::Table -> Account Statements**

@src/tst/data/account_statements_csv.hpp

2. **Be creative in column identification with headers**:
  - For CSV::Table with header, map keywords to required Account statement fileds 
  - For CSV::Table with no header, analyse column data for valid Dates, Amounts, captions

3. **Handle CSV tables WITHOUT header rows**:
   - **Filter out CSV::Table rows** that are 'ignorable' according identification rule
   - **Iterate over column data** and apply heuristics to detect column types
   - **"All valid dates" heuristic**: If all values in a column parse as valid dates → probably the date column
   - **"All valid amounts" heuristic**: If all values are numeric (with optional decimal separators, currency symbols) → probably transaction amount or saldo/balance column
   - **Text pattern heuristic**: Longest text fields → probably transaction descriptions
   - **Consider column position patterns**: Common formats often have predictable ordering (date, description, amount)
   - **Sample multiple rows** (not just first row) to ensure pattern consistency

4. **Distinguish between similar columns**:
   - Multiple amount columns? One might be transaction amount, another balance/saldo
   - Look for patterns: negative/positive values, magnitude differences, etc.

5. **Handle variations**: Different banks and SKV may use:
   - Different column orders
   - Different naming conventions
   - Presence or absence of headers
   - Different date formats

6. **Document your findings**: Explain what patterns you discovered and how the mapping works

This may require examining multiple sample files and implementing intelligent column detection logic that works both with and without headers.

**Design Considerations:**
- **How do we make CSV::Table -> Maybe Account Statemens Generic?**
- **How can we intelligently detect date, amount, and description columns?**
- **How do we handle CSV::Table without header?**
- **What heuristics can identify column types from data alone?**

**Validation Strategy:**
Consider validation approaches:
1. All-or-nothing: One invalid row fails entire parse
2. Permissive: Skip 'ignorable' rows, transform required ones

Choose based on business requirements. Document the rationale.

**Error Handling:**
- Missing required fields?
- Invalid date formats?
- Unparseable amounts?
- Unidentified transaction caption?

Use Maybe/optional to propagate failures through the pipeline.
</implementation>

<output>
Create or modify files:
- `./src/domain/account_statement.hpp` - Account statement type definition (or existing location)
- `./src/domain/csv_to_account_statement.cpp` - Extraction logic

**Add tests to:** `./src/test/test_csv_import_pipeline.cpp`
- Use namespace: `namespace tests::csv_import_pipeline::account_statement_suite { ... }`
- Test account statement extraction with column detection logic

Example tests:
```cpp
namespace tests::csv_import_pipeline {
    namespace account_statement_suite {
        TEST(AccountStatementTests, ExtractFromBankCSV) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, ExtractFromBankCSV)"};
            // Test extraction from bank CSV with headers
        }

        TEST(AccountStatementTests, DetectColumnsWithoutHeaders) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, DetectColumnsWithoutHeaders)"};
            // Test intelligent column detection for headless CSV
        }

        TEST(AccountStatementTests, InvalidDataHandledGracefully) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, InvalidDataHandledGracefully)"};
            // Test handling of invalid dates/amounts
        }
    }
}
```
</output>

<verification>
Before completing:

1. **Compilation**: Code compiles on macOS XCode with C++23 via `./run.zsh --nop` and outputs 'Cratchit - Compiles and runs'.
2. **Attend to code signing error**: Execute 'codesign -s - --force --deep workspace/cratchit' when compilation / run does not output required text.
2. **Tests Pass**: Run `cd workspace && ./cratchit --test` - all tests succeed
3. **Extraction Works**: Successfully extracts statements from sample CSV::Table
4. **Column Detection Works**: Intelligent detection of date/amount/description columns
5. **Validation Works**: Invalid data handled correctly (returns empty Maybe)
6. **Integration**: Composes with Steps 5 and 6

Test coverage:
- Bank CSV with headers
- SKV CSV without headers (column detection heuristics)
- CSV with missing required fields
- CSV with invalid dates/amounts
- Empty CSV (headers only) is OK (zero account statement entries)
</verification>

<success_criteria>
- [ ] Account statement extraction function implemented
- [ ] Returns Maybe<account_statements> for monadic composition
- [ ] Business logic validation applied
- [ ] Handles invalid data gracefully
- [ ] Composes with Step 6 (CSV::Table → statements)
- [ ] Code compiles
- [ ] Tested with sample data
- [ ] Account statement type appropriately defined
- [ ] Clear validation strategy documented
</success_criteria>
