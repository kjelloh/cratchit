<assistant-role>
You are an expert in writing C++23 code that is easy to read and can be used in functional style composition based on Maybe monads (std::optional, AnnotatedMaybe) and C++ range views where this makes sense.

1. You prefer to use C++23 'span' and 'string_view' over C-code constructs.
2. You prefer to use C++23 standard library over any specialized helpers.
3. You prefer readable code over efficient code.
4. You prefer to organize into namespaces over splitting code into C++ translation units.

You know about the codebase usage of namespaces and existing types for different functionality.

1. You are aware that we are finalizing the CSV import pipeline: file → Maybe<TaggedAmounts>
2. You are aware that Steps 1-7 are complete and provide: file → text → CSV::Table → AccountStatement
3. You are aware that the final step transforms AccountStatement → Maybe<TaggedAmounts>
</assistant-role>

<objective>
Implement the transformation: AccountStatement → Maybe<TaggedAmounts>

This step takes an AccountStatement (from prompt 007) and transforms each entry into a TaggedAmount with a defined tagging scheme. This is the bridge between raw account data and the domain model used for bookkeeping.
</objective>

<context>
This is Step 8 of the CSV import pipeline refactoring (prompts 001..011).

**Take-off point (from prompt 007):**

The `csv_table_to_account_statement(CSV::Table, AccountID)` function returns:
```cpp
std::optional<AccountStatement>
```

Where AccountStatement contains:
- `AccountStatementEntries entries()` - vector of AccountStatementEntry
- `Meta meta()` - contains `std::optional<AccountID> m_maybe_account_irl_id`

**AccountStatementEntry structure (from src/fiscal/amount/AccountStatement.hpp):**
```cpp
struct AccountStatementEntry {
  Date transaction_date;
  Amount transaction_amount;
  std::string transaction_caption;
  Tags transaction_tags;  // std::map<std::string, std::string>
};
```

**Target: TaggedAmount (from src/fiscal/amount/TaggedAmountFramework.hpp):**
```cpp
class TaggedAmount {
public:
  using Tags = std::map<std::string, std::string>;
  TaggedAmount(Date date, CentsAmount cents_amount, Tags tags = Tags{});

  Date const& date() const;
  CentsAmount const& cents_amount() const;
  Tags const& tags() const;
};
```

**Key files to examine:**
@src/domain/csv_to_account_statement.hpp (Step 7 - take-off point)
@src/fiscal/amount/AccountStatement.hpp (AccountStatementEntry definition)
@src/fiscal/amount/TaggedAmountFramework.hpp (TaggedAmount definition)
@src/fiscal/amount/AmountFramework.hpp (Amount, CentsAmount types)

Read CLAUDE.md for project conventions and Swedish accounting requirements.
</context>

<requirements>
Create a transformation function with these characteristics:

1. **Function Signature**:
   ```cpp
   namespace domain {
     std::optional<TaggedAmounts> account_statement_to_tagged_amounts(
         AccountStatement const& statement);
   }
   ```

2. **Transformation Logic**:
   For each `AccountStatementEntry` in `statement.entries()`, create a `TaggedAmount` with:
   - **Date**: `entry.transaction_date`
   - **Amount**: `entry.transaction_amount` converted to `CentsAmount`
   - **Tags**: A map with exactly two entries:
     - `"Text"` → `entry.transaction_caption` (the description)
     - `"Account"` → `statement.meta().m_maybe_account_irl_id->to_string()` (string representation of AccountID)

3. **Tagging Scheme**:
   ```cpp
   TaggedAmount::Tags tags = {
     {"Text", entry.transaction_caption},
     {"Account", account_id_string}
   };
   ```

   Where `account_id_string` is derived from the AccountStatement's meta.m_maybe_account_irl_id.
   - If AccountID has prefix "NORDEA:" and value "51 86 87-9", the tag value should be "NORDEA:51 86 87-9"
   - If no AccountID is present in meta, use empty string or "UNKNOWN"

4. **Amount Conversion**:
   - `Amount` (double/decimal) must be converted to `CentsAmount` (integer cents)
   - Use existing conversion functions if available, or implement: `cents = static_cast<CentsAmount>(amount * 100)`

5. **Return Type**:
   - Return `std::optional<TaggedAmounts>` (where `TaggedAmounts = std::vector<TaggedAmount>`)
   - Return `std::nullopt` if the transformation fundamentally fails
   - Return empty vector if statement has no entries (this is valid, not an error)
</requirements>

<implementation>
**Location**: Create new file `src/domain/account_statement_to_tagged_amounts.hpp`

```cpp
#pragma once

#include "fiscal/amount/AccountStatement.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"
#include "logger/log.hpp"
#include <optional>
#include <string>

namespace domain {

/**
 * Transform AccountStatement to TaggedAmounts
 *
 * Each AccountStatementEntry becomes a TaggedAmount with:
 * - Date: from entry.transaction_date
 * - Amount: from entry.transaction_amount (converted to CentsAmount)
 * - Tags:
 *   - "Text": entry.transaction_caption
 *   - "Account": statement account ID as string
 *
 * @param statement The AccountStatement containing entries and account metadata
 * @return Optional vector of TaggedAmounts, or nullopt on failure
 */
inline std::optional<TaggedAmounts> account_statement_to_tagged_amounts(
    AccountStatement const& statement) {
  logger::scope_logger log_raii{logger::development_trace,
    "domain::account_statement_to_tagged_amounts(statement)"};

  // Get account ID string from statement metadata
  std::string account_id_string = statement.meta().m_maybe_account_irl_id
    ? statement.meta().m_maybe_account_irl_id->to_string()
    : "";

  TaggedAmounts result;
  result.reserve(statement.entries().size());

  for (auto const& entry : statement.entries()) {
    // Convert Amount to CentsAmount
    // Note: Amount is typically a double, CentsAmount is integer cents
    CentsAmount cents = to_cents_amount(entry.transaction_amount);

    // Create tags with defined scheme
    TaggedAmount::Tags tags = {
      {"Text", entry.transaction_caption},
      {"Account", account_id_string}
    };

    result.emplace_back(entry.transaction_date, cents, std::move(tags));
  }

  logger::development_trace("Created {} tagged amounts from statement with account: {}",
    result.size(), account_id_string);

  return result;
}

} // namespace domain
```

**Note**: Verify the correct way to convert `Amount` to `CentsAmount` by examining `AmountFramework.hpp`. There may be an existing `to_cents_amount()` function or similar.
</implementation>

<output>
**Create file**: `./src/domain/account_statement_to_tagged_amounts.hpp`
- Contains `domain::account_statement_to_tagged_amounts(AccountStatement)` function

**Create test file**: `./src/test/test_statement_to_tagged_amounts.cpp`
- New test file specifically for AccountStatement → TaggedAmounts transformation
- Follow the same structure as test_csv_import_pipeline.cpp

**Test file structure:**
```cpp
#include "test_atomics.hpp"
#include "test_fixtures.hpp"
#include "logger/log.hpp"
#include "domain/account_statement_to_tagged_amounts.hpp"
#include "domain/csv_to_account_statement.hpp"
#include "fiscal/amount/AccountStatement.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"
#include "test/data/account_statements_csv.hpp"
#include <gtest/gtest.h>

namespace tests::statement_to_tagged_amounts {

TEST(StatementToTaggedAmountsTests, TransformSingleEntry) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmountsTests, TransformSingleEntry)"};

  // Create a simple AccountStatement with one entry
  AccountStatementEntries entries;
  entries.emplace_back(
    Date{std::chrono::year{2025}/std::chrono::January/15},
    Amount{100.50},
    "Test transaction"
  );

  AccountID account_id{"NORDEA:", "51 86 87-9"};
  AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
  AccountStatement statement(entries, meta);

  auto result = domain::account_statement_to_tagged_amounts(statement);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1);

  auto const& ta = result->at(0);
  EXPECT_EQ(ta.date(), entries[0].transaction_date);
  EXPECT_EQ(ta.cents_amount(), 10050);  // 100.50 * 100
  EXPECT_EQ(ta.tag_value("Text").value_or(""), "Test transaction");
  EXPECT_EQ(ta.tag_value("Account").value_or(""), "NORDEA:51 86 87-9");
}

TEST(StatementToTaggedAmountsTests, TransformMultipleEntries) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmountsTests, TransformMultipleEntries)"};

  // Create AccountStatement with multiple entries
  AccountStatementEntries entries;
  entries.emplace_back(
    Date{std::chrono::year{2025}/std::chrono::January/10},
    Amount{-500.00},
    "Payment to supplier"
  );
  entries.emplace_back(
    Date{std::chrono::year{2025}/std::chrono::January/15},
    Amount{1200.75},
    "Customer invoice payment"
  );
  entries.emplace_back(
    Date{std::chrono::year{2025}/std::chrono::January/20},
    Amount{-89.50},
    "Office supplies"
  );

  AccountID account_id{"NORDEA:", "12 34 56-7"};
  AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
  AccountStatement statement(entries, meta);

  auto result = domain::account_statement_to_tagged_amounts(statement);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->size(), 3);

  // Verify each tagged amount has correct tags
  for (auto const& ta : *result) {
    EXPECT_TRUE(ta.tag_value("Text").has_value());
    EXPECT_EQ(ta.tag_value("Account").value_or(""), "NORDEA:12 34 56-7");
  }

  // Verify amounts converted correctly
  EXPECT_EQ(result->at(0).cents_amount(), -50000);
  EXPECT_EQ(result->at(1).cents_amount(), 120075);
  EXPECT_EQ(result->at(2).cents_amount(), -8950);
}

TEST(StatementToTaggedAmountsTests, PreservesTransactionCaption) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmountsTests, PreservesTransactionCaption)"};

  std::string long_caption = "ACME Corporation Invoice #12345 - Consulting services for Q1 2025";

  AccountStatementEntries entries;
  entries.emplace_back(
    Date{std::chrono::year{2025}/std::chrono::February/1},
    Amount{5000.00},
    long_caption
  );

  AccountStatement statement(entries);

  auto result = domain::account_statement_to_tagged_amounts(statement);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1);
  EXPECT_EQ(result->at(0).tag_value("Text").value_or(""), long_caption);
}

TEST(StatementToTaggedAmountsTests, HandlesEmptyStatement) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmountsTests, HandlesEmptyStatement)"};

  AccountStatementEntries entries;  // Empty
  AccountStatement statement(entries);

  auto result = domain::account_statement_to_tagged_amounts(statement);

  ASSERT_TRUE(result.has_value());  // Success, not failure
  EXPECT_EQ(result->size(), 0);     // Empty vector
}

TEST(StatementToTaggedAmountsTests, HandlesMissingAccountID) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmountsTests, HandlesMissingAccountID)"};

  AccountStatementEntries entries;
  entries.emplace_back(
    Date{std::chrono::year{2025}/std::chrono::January/1},
    Amount{100.00},
    "Test"
  );

  // No meta/account ID
  AccountStatement statement(entries);

  auto result = domain::account_statement_to_tagged_amounts(statement);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1);

  // Account tag should be empty string when no AccountID
  EXPECT_EQ(result->at(0).tag_value("Account").value_or("MISSING"), "");
}

TEST(StatementToTaggedAmountsTests, HandlesNegativeAmounts) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmountsTests, HandlesNegativeAmounts)"};

  AccountStatementEntries entries;
  entries.emplace_back(
    Date{std::chrono::year{2025}/std::chrono::March/15},
    Amount{-1234.56},
    "Withdrawal"
  );

  AccountStatement statement(entries);

  auto result = domain::account_statement_to_tagged_amounts(statement);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->at(0).cents_amount(), -123456);
}

TEST(StatementToTaggedAmountsTests, HandlesSKVAccountID) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmountsTests, HandlesSKVAccountID)"};

  AccountStatementEntries entries;
  entries.emplace_back(
    Date{std::chrono::year{2025}/std::chrono::January/1},
    Amount{50000.00},
    "Tax payment"
  );

  AccountID account_id{"SKV:", "559000-1234"};
  AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
  AccountStatement statement(entries, meta);

  auto result = domain::account_statement_to_tagged_amounts(statement);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->at(0).tag_value("Account").value_or(""), "SKV:559000-1234");
}

TEST(StatementToTaggedAmountsTests, IntegrationWithRealCSVData) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmountsTests, IntegrationWithRealCSVData)"};

  // Use the test data from account_statements_csv.hpp
  // This tests the full pipeline: CSV text → table → statement → tagged amounts

  // Parse CSV to table
  auto maybe_table = CSV::neutral::text_to_table(test::data::sz_NORDEA_csv_20251120);
  ASSERT_TRUE(maybe_table.has_value());

  // Extract account ID
  auto maybe_account_id = CSV::project::to_account_id(*maybe_table);
  ASSERT_TRUE(maybe_account_id.has_value());

  // Create AccountStatement
  auto maybe_statement = domain::csv_table_to_account_statement(*maybe_table, *maybe_account_id);
  ASSERT_TRUE(maybe_statement.has_value());

  // Transform to TaggedAmounts
  auto result = domain::account_statement_to_tagged_amounts(*maybe_statement);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->size(), 0);

  // Verify all tagged amounts have the required tags
  for (auto const& ta : *result) {
    EXPECT_TRUE(ta.tag_value("Text").has_value()) << "Missing 'Text' tag";
    EXPECT_TRUE(ta.tag_value("Account").has_value()) << "Missing 'Account' tag";
  }
}

} // namespace tests::statement_to_tagged_amounts
```
</output>

<verification>
Before completing:

1. **Compilation**: Code compiles on macOS XCode with C++23 via `./run.zsh --nop`
2. **Code Signing**: Execute `codesign -s - --force --deep workspace/cratchit` if needed
3. **Tests Pass**: Run `cd workspace && ./cratchit --test` - all tests succeed
4. **Transformation Works**: Successfully creates TaggedAmounts from AccountStatement
5. **Tags Correct**: Each TaggedAmount has "Text" and "Account" tags with correct values
6. **Amount Conversion**: Amounts correctly converted from Amount to CentsAmount

Verify the Amount → CentsAmount conversion:
- Check `src/fiscal/amount/AmountFramework.hpp` for existing conversion functions
- Ensure cents are calculated correctly (multiply by 100, handle rounding if needed)
</verification>

<success_criteria>
- [ ] New file `src/domain/account_statement_to_tagged_amounts.hpp` created
- [ ] Function `domain::account_statement_to_tagged_amounts(AccountStatement)` implemented
- [ ] Returns `std::optional<TaggedAmounts>`
- [ ] Each TaggedAmount has tag "Text" = transaction_caption
- [ ] Each TaggedAmount has tag "Account" = AccountID.to_string()
- [ ] Amount correctly converted to CentsAmount
- [ ] Date preserved from entry.transaction_date
- [ ] New test file `src/test/test_statement_to_tagged_amounts.cpp` created
- [ ] Tests follow same structure as test_csv_import_pipeline.cpp
- [ ] All tests pass including integration test with real CSV data
- [ ] Empty statement returns empty vector (not nullopt)
- [ ] Missing AccountID handled gracefully (empty string tag)
- [ ] Code compiles on macOS XCode with C++23
</success_criteria>

<notes>
This transformation completes the domain bridge:
- **Input**: AccountStatement (from Step 7) - raw account data with entries
- **Output**: TaggedAmounts - domain objects ready for bookkeeping

The tagging scheme provides:
- **"Text"**: Human-readable description for voucher/journal entries
- **"Account"**: Source account identification for audit trail

Future steps may add more tags (BAS account codes, VAT codes, etc.) but this step establishes the foundational transformation from account statements to the TaggedAmount domain model.
</notes>
