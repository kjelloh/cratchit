<assistant-role>
You are an expert in writing C++23 code that is easy to read and can be used in functional style composition based on Maybe monads (std::optional, AnnotatedMaybe) and C++ range views where this makes sense.

1. You prefer to use C++23 'span' and 'string_view' over C-code constructs.
2. You prefer to use C++23 standard library over any specialized helpers.
3. You prefer readable code over efficient code.
4. You prefer to organize into namespaces over splitting code into C++ translation units.

You know about the codebase usage of namespaces and existing types for different functionality.

1. You are aware that we are finalizing the CSV import pipeline: file → Maybe<TaggedAmounts>
2. You are aware that Steps 1-7 are complete and provide: file → text → CSV::Table → AccountStatementEntries
3. You are aware that the final step transforms AccountStatementEntries → TaggedAmounts
</assistant-role>

<objective>
Implement the final transformation: AccountStatementEntries → Maybe<TaggedAmounts>

This completes the domain pipeline, creating the final domain objects needed for bookkeeping entries. Tagged amounts represent financial transactions with proper categorization and metadata suitable for SIE file generation.
</objective>

<context>
This is Step 8 of an 11-step refactoring (prompts 001..011).

**Current Pipeline State (from test_csv_import_pipeline.cpp):**
```
file_path
  → Maybe<byte_buffer>           (Step 1 - io::read_file_to_buffer)
  → Maybe<DetectedEncoding>      (Step 2 - text::encoding::icu::detect_buffer_encoding)
  → unicode_code_points_view     (Step 3 - text::encoding::views::bytes_to_unicode)
  → runtime_encoded_view         (Step 4 - text::encoding::views::unicode_to_runtime_encoding)
  → Maybe<text>                  (Step 5 - text::encoding::read_file_with_encoding_detection)
  → Maybe<csv_table>             (Step 6 - CSV::neutral::text_to_table)
  → Maybe<account_statements>    (Step 7 - domain::csv_table_to_account_statement_entries)
  → Maybe<tagged_amounts>        (Step 8 - THIS STEP)
```

**Examine existing tagged amount types and business logic:**
@src/fiscal/amount/TaggedAmountFramework.cpp
@src/fiscal/amount/TaggedAmountFramework.hpp
@src/fiscal/amount/HADFramework.hpp (transaction handling)
@src/fiscal/BASFramework.hpp (BAS account codes)
@src/fiscal/SKVFramework.hpp (SKV compliance)

**Examine the completed Step 7 implementation:**
@src/domain/csv_to_account_statement.hpp
@src/domain/csv_to_account_statement.cpp
@src/test/test_csv_import_pipeline.cpp (account_statement_suite namespace)

**Examine AccountStatementEntry structure used in Step 7:**
```cpp
struct AccountStatementEntry {
    Date transaction_date;
    Amount transaction_amount;
    std::string transaction_caption;
    // ... other fields
};
```

Read CLAUDE.md for project conventions and Swedish accounting requirements.
</context>

<requirements>
Create a tagged amount generation function with these characteristics:

1. **Generation Function**: `AccountStatementEntries → Maybe<TaggedAmounts>`
   - Transform account statement entries into tagged amounts
   - Apply business rules and categorization
   - Return AnnotatedMaybe<TaggedAmounts> to preserve transformation messages

2. **TaggedAmount Representation**:
   - Research existing TaggedAmount type in TaggedAmountFramework.hpp
   - Understand how tagged amounts relate to account statements
   - A tagged amount should contain: amount, tags/categories, date, description, account code(s)

3. **Transformation Logic**:
   - Map account statement amounts to tagged amounts
   - Determine appropriate tags/categories (may be placeholder/uncategorized initially)
   - Preserve all metadata from account statements
   - Consider: are amounts categorized automatically or manually later?

4. **Business Rules**:
   - Swedish accounting compliance (Bokföringslagen requirements)
   - BAS Framework integration (account codes from BASFramework.hpp)
   - Valid amount handling (non-zero amounts)
   - Proper date handling (transaction dates)

5. **Monadic Composition**:
   - Use AnnotatedMaybe<T> for return type (like other pipeline steps)
   - Enable .and_then() chaining with Step 7 output
   - Preserve error messages through pipeline
</requirements>

<implementation>
**Analysis Required:**
1. First, examine TaggedAmountFramework.cpp lines 974-979 to understand current usage
2. Research existing TaggedAmount type definition
3. Understand the relationship between AccountStatementEntry and TaggedAmount
4. Determine what "tagging" means in this domain (account codes? categories?)

**Design Decisions:**
- **Categorization Strategy**: If automatic categorization is complex, start simple:
  - Create "uncategorized" tagged amounts initially
  - Let a future step handle categorization rules
  - Focus on structural transformation first

- **Integration Points**:
  - Input: `std::vector<domain::AccountStatementEntry>` from Step 7
  - Output: `AnnotatedMaybe<std::vector<TaggedAmount>>` (or appropriate container type)
  - Namespace: Place in `domain::` namespace alongside Step 7

**Implementation Pattern (from Step 7):**
```cpp
namespace domain {
    // Transform account statements to tagged amounts
    auto account_statements_to_tagged_amounts(
        std::vector<AccountStatementEntry> const& statements
    ) -> AnnotatedMaybe<std::vector<TaggedAmount>>;
}
```

**Error Handling:**
- What happens if an account statement can't be converted?
- Should we fail the entire batch or skip invalid entries?
- Follow the pattern established in Step 7 (analyze csv_to_account_statement.cpp)
</implementation>

<output>
Create or modify files:
- `./src/domain/account_statement_to_tagged_amount.hpp` - Interface
- `./src/domain/account_statement_to_tagged_amount.cpp` - Implementation

**Add tests to:** `./src/test/test_csv_import_pipeline.cpp`
- Use namespace: `namespace tests::csv_import_pipeline::tagged_amount_suite { ... }`
- Follow the established test patterns from account_statement_suite

Example tests:
```cpp
namespace tests::csv_import_pipeline {
    namespace tagged_amount_suite {
        TEST(TaggedAmountTests, TransformFromAccountStatements) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(TaggedAmountTests, TransformFromAccountStatements)"};
            // Test basic transformation from account statements
            // Create sample AccountStatementEntries
            // Transform to TaggedAmounts
            // Verify output structure
        }

        TEST(TaggedAmountTests, PreservesTransactionMetadata) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(TaggedAmountTests, PreservesTransactionMetadata)"};
            // Verify date, amount, description preserved
        }

        TEST(TaggedAmountTests, ComposesWithStep7) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(TaggedAmountTests, ComposesWithStep7)"};
            // Test monadic composition with csv_table_to_account_statement_entries
            // CSV::Table → AccountStatements → TaggedAmounts
        }

        TEST(TaggedAmountTests, HandlesEmptyInput) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(TaggedAmountTests, HandlesEmptyInput)"};
            // Empty account statements → empty tagged amounts (success, not failure)
        }

        TEST(TaggedAmountTests, IntegrationWithRealCSV) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(TaggedAmountTests, IntegrationWithRealCSV)"};
            // Use sz_NORDEA_csv_20251120 or sz_SKV_csv_20251120
            // Full pipeline: CSV text → table → statements → tagged amounts
        }
    }
}
```
</output>

<verification>
Before completing:

1. **Compilation**: Code compiles on macOS XCode with C++23 via `./run.zsh --nop`
2. **Code Signing**: Execute 'codesign -s - --force --deep workspace/cratchit' if needed
3. **Tests Pass**: Run `cd workspace && ./cratchit --test` - all tests succeed
4. **Transformation Works**: Successfully creates tagged amounts from account statements
5. **Monadic Composition**: .and_then() chaining works with Step 7
6. **Metadata Preserved**: Date, amount, description carried through transformation

Test coverage:
- Transform account statements to tagged amounts
- Empty input handling
- Composition with Step 7
- Real CSV data integration (Nordea, SKV)
- Error handling for invalid entries (if applicable)
</verification>

<success_criteria>
- [ ] Tagged amount generation function implemented
- [ ] Returns AnnotatedMaybe<TaggedAmounts> for monadic composition
- [ ] Transforms account statements to tagged amounts correctly
- [ ] Preserves date, amount, description metadata
- [ ] Composes with Step 7 (csv_table_to_account_statement_entries → tagged amounts)
- [ ] Code compiles on macOS XCode with C++23
- [ ] Tested with account statement input
- [ ] Tested with real CSV data (Nordea, SKV)
- [ ] TaggedAmount type appropriately used or defined
- [ ] Follows established patterns from test_csv_import_pipeline.cpp
</success_criteria>
