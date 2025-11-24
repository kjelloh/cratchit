<assistant-role>
You are an expert in writing C++23 code that is easy to read and can be used in functional style composition based on Maybe monads (std::optional) and C++ range views where this makes sense.

1. You prefer to use C++23 'span' and 'string_view' over C-code constructs.
2. You prefer to use C++23 standard library over any specialized helpers.
3. You prefer readable code over efficient code.
4. You prefer to organize into namespaces over splitting code into C++ translation units.

You know about the codebase usage of namespaces and existing types for different functionality.

</assistant-role>

<objective>
Implement CSV::Table + AccountID -> Maybe AccountStatement.

This step combines:
1. CSV::Table -> Maybe AccountStatementEntries (existing `domain::csv_table_to_account_statement_entries`)
2. CSV::Table -> Maybe AccountID (existing `CSV::project::to_account_id`)

Into a single function:
- `domain::csv_table_to_account_statement(CSV::Table, AccountID) -> std::optional<AccountStatement>`

</objective>

<context>
This is Step 7 of the CSV import pipeline refactoring (prompts 001..011).

**Prerequisite functions:**

1. `CSV::project::to_account_id(CSV::Table)` - Extracts AccountID from CSV table
   - Location: `src/csv/csv_to_account_id.hpp`
   - Returns `std::optional<AccountID>` with prefix (NORDEA, SKV, etc.) and value (account number, org number)

2. `domain::csv_table_to_account_statement_entries(CSV::Table)` - Extracts statement entries from CSV
   - Location: `src/domain/csv_to_account_statement.hpp`
   - Returns `OptionalAccountStatementEntries` (std::optional<AccountStatementEntries>)

**Target type:**

```cpp
// From fiscal/amount/AccountStatement.hpp
class AccountStatement {
public:
  struct Meta {
    std::optional<AccountID> m_maybe_account_irl_id;
  };
  AccountStatement(AccountStatementEntries const& entries, Meta meta = {});

  AccountStatementEntries const& entries() const;
  Meta const& meta() const;
};
```

</context>

<requirements>
Create a combined extraction function with these characteristics:

1. **Function Signature**:
   ```cpp
   std::optional<AccountStatement> csv_table_to_account_statement(
       CSV::Table const& table,
       AccountID const& account_id);
   ```

2. **Implementation Strategy**:
   - Use `csv_table_to_account_statement_entries(table)` to extract entries
   - If entries extraction fails (nullopt), return nullopt
   - Create `AccountStatement::Meta` with the provided AccountID
   - Return `AccountStatement` with entries and meta

3. **Error Handling**:
   - Returns nullopt if column detection fails
   - Returns AccountStatement (possibly with empty entries) if detection succeeds but all rows are ignorable

4. **Composition Pattern**:
   The typical usage pattern:
   ```cpp
   auto maybe_table = CSV::neutral::text_to_table(csv_text);
   auto maybe_account_id = CSV::project::to_account_id(*maybe_table);
   auto maybe_statement = domain::csv_table_to_account_statement(*maybe_table, *maybe_account_id);
   ```

</requirements>

<implementation>
**Location**: Add to `src/domain/csv_to_account_statement.hpp`

```cpp
/**
 * Transform CSV::Table + AccountID to AccountStatement
 *
 * This is a higher-level extraction function that combines:
 * 1. CSV::Table -> AccountStatementEntries (via csv_table_to_account_statement_entries)
 * 2. AccountID from external source (e.g., CSV::project::to_account_id)
 *
 * @param table The CSV::Table containing transaction data
 * @param account_id The AccountID identifying the account (e.g., NORDEA:51 86 87-9)
 * @return Optional AccountStatement with entries and account metadata, or nullopt on failure
 */
inline std::optional<AccountStatement> csv_table_to_account_statement(
    CSV::Table const& table,
    AccountID const& account_id) {
  logger::scope_logger log_raii{logger::development_trace,
    "domain::csv_table_to_account_statement(table, account_id)"};

  // Extract entries from the CSV table
  auto maybe_entries = csv_table_to_account_statement_entries(table);

  if (!maybe_entries) {
    logger::development_trace("Failed to extract entries from CSV table");
    return std::nullopt;
  }

  // Create AccountStatement with entries and account ID metadata
  AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};

  logger::development_trace("Creating AccountStatement with {} entries and account_id: {}",
    maybe_entries->size(), account_id.to_string());

  return AccountStatement(*maybe_entries, meta);
}
```

</implementation>

<output>
**Modified file**: `src/domain/csv_to_account_statement.hpp`
- Added `csv_table_to_account_statement(CSV::Table, AccountID)` function

**Tests added to**: `src/test/test_csv_import_pipeline.cpp`
- Namespace: `tests::csv_import_pipeline::account_statement_suite`

Test cases:
1. `CsvTableToAccountStatementWithNordea` - Creates AccountStatement from NORDEA CSV
2. `CsvTableToAccountStatementWithSKV` - Creates AccountStatement from SKV CSV
3. `CsvTableToAccountStatementPreservesEntryData` - Verifies entry data preservation
4. `CsvTableToAccountStatementWithInvalidTable` - Returns nullopt for invalid tables
5. `CsvTableToAccountStatementEmptyEntriesIsValid` - Valid AccountStatement with empty entries
6. `CsvTableToAccountStatementIntegrationPipeline` - Full pipeline integration test

</output>

<verification>
Before completing:

1. **Compilation**: Code compiles on macOS XCode with C++23 via `./run.zsh --nop`
2. **Tests Pass**: Run `cd workspace && ./cratchit --test` - all tests succeed
3. **New Function Works**: Creates AccountStatement with entries AND AccountID metadata
4. **Integration**: Composes with Steps 6 (CSV::Table -> entries) and existing to_account_id

Test coverage:
- NORDEA CSV with headers
- SKV CSV (tax agency format)
- Custom test CSV with explicit AccountID
- Invalid CSV (column detection fails)
- Empty CSV (valid structure, no data rows)
- Integration pipeline (text -> table -> AccountID + AccountStatement)
</verification>

<success_criteria>
- [x] New function `csv_table_to_account_statement(CSV::Table, AccountID)` implemented
- [x] Returns `std::optional<AccountStatement>` for monadic composition
- [x] AccountStatement contains both entries AND AccountID in metadata
- [x] Handles invalid data gracefully (returns nullopt)
- [x] Composes with existing `to_account_id` and `csv_table_to_account_statement_entries`
- [x] Code compiles
- [x] All 6 new tests pass
- [x] All existing tests continue to pass (185 total)
</success_criteria>

<notes>
This refactoring separates concerns:
- **AccountID extraction** (`CSV::project::to_account_id`) - identifies the source account
- **Entry extraction** (`csv_table_to_account_statement_entries`) - extracts transaction data
- **Combined** (`csv_table_to_account_statement`) - creates complete AccountStatement

The separation allows:
- Independent testing of each component
- Flexibility in how AccountID is determined (could come from filename, user input, etc.)
- Clear composition pattern for the pipeline
</notes>
