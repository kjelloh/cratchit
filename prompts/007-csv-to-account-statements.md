<assistant-role>
You are an expert in writing C++23 code that is easy to read and can be used in functional style composition based on Maybe monads (std::optional) and C++ range views where this makes sense.

1. You prefer to use C++23 'span' and 'string_view' over C-code constructs.
2. You prefer to use C++23 standard library over any specialized helpers.
3. You prefer readable code over efficient code.
4. You prefer to organize into namespaces over splitting code into C++ translation units.

You know about the codebase usage of namespaces and existing types for different functionality.

1. You are aware that current pipeline produces a 'neutral' Maybe CSV::Table
2. You are aware that we MUST implement a NEW WAY to project a neutral table to an Account statements container.
3. You are aware of the existing type fiscal/amount/AccountStatement.hpp.

</assistant-role>

<objective>
Implement the business logic layer that transforms CSV tables into account statement domain objects using monadic composition. Create: csv_table → Maybe<account_statements>.

This step adds business logic validation and domain object construction to the pipeline, converting structured CSV data into meaningful business entities.
</objective>

<context>
This is Step 7 of an 11-step refactoring. Previous steps provided:
- Steps 1-5: Complete encoding pipeline (file → text)
- Step 6: CSV parsing (text → csv_table)

This step extracts account statements from CSV tables, applying business logic and validation specific to bank and SKV formats.

Read CLAUDE.md for project conventions and Swedish accounting requirements.

**Examine existing domain objects and business logic:**
@src/fiscal/amount/TaggedAmountFramework.cpp (focus on lines 974-979 for context)

**Search for existing account statement types** in the codebase.
</context>

<requirements>
Create an account statement extraction function with these characteristics:

1. **Extraction Function**: `csv_table → Maybe<account_statements>`
   - Parse CSV rows into account statement objects
   - Apply validation rules (required fields, format checks, etc.)
   - Return optional/Maybe to handle invalid data

2. **Account Statement Representation**:
   - What is an account statement in this domain?
   - What fields are required? (date, amount, description, account number, etc.)
   - Are there different types for bank vs SKV statements?

3. **Business Logic Validation**:
   - Required fields present and non-empty
   - Dates parse correctly and are valid
   - Amounts are valid numbers
   - Account numbers match expected format
   - Any domain-specific validation rules

4. **Multiple Statement Support**:
   - CSV may contain multiple account statements (rows)
   - Should this return a collection? Vector? Range?
   - How are invalid rows handled? Skip them? Fail entire parse?
</requirements>

<implementation>
**IMPORTANT: Column Mapping Analysis Required**

⚠️ **The exact mechanism for identifying which columns map to account statement fields is currently unknown.**

When implementing this step, you MUST:

1. **Analyze sample CSV files** from banks and SKV to understand their structure

2. **Be creative in column identification with headers**:
   - Look for headers that indicate date columns (e.g., "Date", "Datum", "Transaction Date")
   - Identify amount columns (e.g., "Amount", "Belopp", numeric values with currency symbols)
   - Find transaction description/heading columns (e.g., "Description", "Beskrivning", "Transaction")

3. **Handle CSV tables WITHOUT header rows**:
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
- What does the current implementation in TaggedAmountFramework.cpp do?
- Are there existing account statement classes/structs?
- Do bank and SKV CSV formats differ significantly?
- Should this be generic or format-specific?
- **How can we intelligently detect date, amount, and description columns?**
- **Should we support multiple column naming conventions?**
- **How do we handle CSV files without headers?**
- **What heuristics can identify column types from data alone?**

**Validation Strategy:**
Consider validation approaches:
1. All-or-nothing: One invalid row fails entire parse
2. Permissive: Skip invalid rows, return valid ones
3. Partial: Return what's valid + error info for invalid rows

Choose based on business requirements. Document the rationale.

**Swedish Accounting Compliance:**
According to CLAUDE.md, Swedish Accounting Act requires:
- Date of business transaction
- Date of voucher compilation
- Voucher number/identification
- Transaction description
- Amount and VAT
- Counterparty name

Ensure extracted statements meet these requirements where applicable.

**Error Handling:**
- Missing required fields?
- Invalid date formats?
- Unparseable amounts?
- Unknown account numbers?

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

1. **Compilation**: Code compiles on macOS XCode with C++23 via `./run.zsh`
2. **Tests Pass**: Run `./cratchit --test` - all tests succeed
3. **Extraction Works**: Successfully extracts statements from sample CSV
4. **Column Detection Works**: Intelligent detection of date/amount/description columns
5. **Validation Works**: Invalid data handled correctly (returns empty Maybe or skips rows)
6. **Integration**: Composes with Steps 5 and 6
7. **Business Rules**: Meets Swedish accounting requirements where applicable

Test coverage:
- Bank CSV with headers
- SKV CSV with headers
- CSV without headers (column detection heuristics)
- CSV with missing required fields
- CSV with invalid dates/amounts
- Empty CSV (headers only)
</verification>

<success_criteria>
- [ ] Account statement extraction function implemented
- [ ] Returns Maybe<account_statements> for monadic composition
- [ ] Business logic validation applied
- [ ] Handles invalid data gracefully
- [ ] Composes with Steps 5 and 6 (file → CSV → statements)
- [ ] Code compiles on macOS XCode
- [ ] Tested with sample bank and SKV CSV files
- [ ] Follows Swedish accounting requirements
- [ ] Account statement type appropriately defined
- [ ] Clear validation strategy documented
</success_criteria>
