<assistant-role>
You are an expert in writing C++23 code that is easy to read and can be used in functional style composition based on Maybe monads (std::optional) and C++ range views where this makes sense.

1. You prefer to use C++23 'span' and 'string_view' over C-code constructs.
2. You prefer to use C++23 standard library over any specialized helpers.
3. You prefer readable code over efficient code.
4. You prefer to organize into namespaces over splitting code into C++ translation units.

You are implementing a FOCUSED, SINGLE-PURPOSE step. Do NOT refactor existing code. Do NOT touch the current step 007 implementation. This step ONLY extracts AccountID from CSV::Table.
</assistant-role>

<objective>
Implement CSV::Table → Maybe<AccountID> extraction function.

This is a **preceding sub-step** to the current step 007. The purpose is to extract the AccountID (account identifier with prefix) from a CSV::Table, which will later be used to construct AccountStatement objects that hold AccountStatementEntries.

**IMPORTANT**: This step is ONLY about extracting the AccountID. Do NOT modify existing step 007 code. We will do that refactoring LATER.
</objective>

<context>
**The AccountID type already exists:**

@src/fiscal/amount/AccountStatement.hpp

```cpp
struct DomainPrefixedId {
  std::string m_prefix; // E.g., NORDEA,PG,BG,IBAN,SKV,
  std::string m_value; // E.g. a bank account, a PG account etc...
  std::string to_string() const {
    return std::format(
       "{}{}"
      ,((m_prefix.size()>0)?m_prefix:std::string{})
      ,m_value);
  }
};

using AccountID = DomainPrefixedId;
```

**The CSV::Table structure:**

@src/csv/csv.hpp

```cpp
struct Table {
  using Heading = FieldRow;  // FieldRow = std::vector<std::string>
  using Row = FieldRow;
  using Rows = std::vector<Row>;
  Heading heading;
  Rows rows;
};
```

**CSV formats to support:**

1. **NORDEA CSV** (bank account statements):
   - Has header row with Swedish column names
   - The "Avsändare" (sender) column contains the bank account number (e.g., "51 86 87-9")
   - Prefix should be: `"NORDEA"`
   - Value should be: the account number from "Avsändare" column

2. **SKV CSV** (tax agency account statements):
   - May NOT have a header row (or has minimal headers)
   - Contains organisation number in the data (format: 10 digits like "5567828172")
   - Prefix should be: `"SKV"`
   - Value should be: the organisation number if found, OR empty string if not found

3. **Unknown/Generic CSV**:
   - Prefix should be: `""` (empty)
   - Value should be: `""` (empty)
   - Returns a valid AccountID with empty values

**Sample CSV data to examine:**

@src/tst/data/account_statements_csv.hpp

</context>

<requirements>
Create an AccountID extraction function with these characteristics:

1. **Function Signature**:
   ```cpp
   std::optional<AccountID> to_account_id(CSV::Table const& table);
   ```

2. **Identification Logic** (step-by-step):

   **Step 1: Check if this is a NORDEA CSV**
   - Look for NORDEA-specific header keywords: "Bokföringsdag", "Avsändare", "Mottagare", "Saldo"
   - If NORDEA identified:
     - Find the "Avsändare" column
     - Extract the first non-empty value from that column (this is the account number)
     - Return `AccountID{"NORDEA", account_number}`

   **Step 2: Check if this is an SKV CSV**
   - Look for SKV-specific patterns in headers or data: "Ing saldo", "Utg saldo", "Obetalt"
   - If SKV identified:
     - Search the data rows for a 10-digit organisation number pattern
     - If found: Return `AccountID{"SKV", org_number}`
     - If not found: Return `AccountID{"SKV", ""}` (valid but without org number)

   **Step 3: Unknown/Fallback**
   - If neither NORDEA nor SKV can be identified
   - Return `AccountID{"", ""}` (empty prefix and value)

3. **Return Value Semantics**:
   - Return `std::optional<AccountID>`
   - Return `std::nullopt` ONLY if the CSV::Table is fundamentally invalid (empty, no rows)
   - Return a valid AccountID (possibly with empty values) for any valid CSV::Table

4. **Namespace**: Place the function in `namespace CSV::project`

</requirements>

<implementation>
**Think through this step-by-step:**

1. **First**: Read the sample CSV test data to understand the actual formats
2. **Second**: Identify distinctive patterns that differentiate NORDEA from SKV CSV files
3. **Third**: Implement header-based detection for NORDEA (specific Swedish column names)
4. **Fourth**: Implement content-based detection for SKV (organisation number pattern)
5. **Fifth**: Implement the fallback case

**Helper functions you may need:**

```cpp
// Check if a header row indicates NORDEA format
bool is_nordea_csv(CSV::Table::Heading const& heading);

// Check if table content indicates SKV format
bool is_skv_csv(CSV::Table const& table);

// Extract NORDEA account number from the Avsändare column
std::string extract_nordea_account(CSV::Table const& table);

// Search for 10-digit organisation number in SKV data
std::optional<std::string> find_skv_org_number(CSV::Table const& table);
```

**Pattern matching hints:**

- NORDEA account numbers look like: "51 86 87-9" (spaces and dash, 8-9 characters)
- SKV organisation numbers are exactly 10 digits: "5567828172"
- SKV files may have "Inbetalning från organisationsnummer XXXXXXXXXX" in a text field

**DO NOT:**
- Modify existing step 007 code
- Implement AccountStatementEntry extraction
- Change the AccountStatement class
- Add new fields to AccountID/DomainPrefixedId
</implementation>

<output>
Create or modify files:

- `./src/csv/csv_to_account_id.hpp` - The to_account_id function and helpers

**Add tests to:** `./src/test/test_csv_import_pipeline.cpp`
- Use namespace: `namespace tests::csv_import_pipeline::account_id_suite { ... }`
- Test AccountID extraction from different CSV formats

Example tests:
```cpp
namespace tests::csv_import_pipeline {
    namespace account_id_suite {
        TEST(AccountIdTests, ExtractNordeaAccountId) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, ExtractNordeaAccountId)"};
            // Test: NORDEA CSV → AccountID{"NORDEA", "51 86 87-9"}
        }

        TEST(AccountIdTests, ExtractSkvAccountIdWithOrgNumber) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, ExtractSkvAccountIdWithOrgNumber)"};
            // Test: SKV CSV with org number → AccountID{"SKV", "5567828172"}
        }

        TEST(AccountIdTests, ExtractSkvAccountIdWithoutOrgNumber) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, ExtractSkvAccountIdWithoutOrgNumber)"};
            // Test: SKV CSV without org number → AccountID{"SKV", ""}
        }

        TEST(AccountIdTests, UnknownCsvReturnsEmptyAccountId) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, UnknownCsvReturnsEmptyAccountId)"};
            // Test: Unknown CSV → AccountID{"", ""}
        }

        TEST(AccountIdTests, EmptyTableReturnsNullopt) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(AccountIdTests, EmptyTableReturnsNullopt)"};
            // Test: Empty CSV::Table → std::nullopt
        }
    }
}
```
</output>

<verification>
Before completing:

1. **Compilation**: Code compiles on macOS XCode with C++23 via `./run.zsh --nop` and outputs 'Cratchit - Compiles and runs'.
2. **Attend to code signing error**: Execute 'codesign -s - --force --deep workspace/cratchit' when compilation / run does not output required text.
3. **Tests Pass**: Run `cd workspace && ./cratchit --test` - all tests succeed
4. **NORDEA Detection Works**: Correctly identifies NORDEA CSV and extracts account number
5. **SKV Detection Works**: Correctly identifies SKV CSV and extracts org number when present
6. **Fallback Works**: Unknown CSV returns AccountID with empty values
7. **No Side Effects**: Existing step 007 code is UNCHANGED

Verify with test commands:
```bash
./run.zsh --nop  # Should compile successfully
cd workspace && ./cratchit --test --gtest_filter="*AccountId*"  # Run only AccountID tests
```
</verification>

<success_criteria>
- [ ] `to_account_id(CSV::Table) → std::optional<AccountID>` function implemented
- [ ] NORDEA CSV detected by header keywords, account extracted from Avsändare column
- [ ] SKV CSV detected by content patterns, org number extracted when present
- [ ] Unknown CSV returns AccountID{"", ""}
- [ ] Empty/invalid table returns std::nullopt
- [ ] Code compiles with C++23
- [ ] All AccountID tests pass
- [ ] Existing step 007 code is NOT modified
- [ ] Function is in `namespace CSV::project`
</success_criteria>
