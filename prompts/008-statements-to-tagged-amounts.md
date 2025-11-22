<objective>
Implement the final transformation in the domain pipeline: account statements → Maybe<tagged_amounts>. This creates the complete monadic chain from raw files to business domain objects.

Tagged amounts are the final domain objects needed for bookkeeping entries, representing financial transactions with proper categorization and metadata.
</objective>

<context>
This is Step 8 of an 11-step refactoring. Previous steps provided:
- Steps 1-5: Complete encoding pipeline (file → text)
- Step 6: CSV parsing (text → csv_table)
- Step 7: Domain extraction (csv_table → account_statements)

This step completes the domain transformation pipeline by converting account statements into tagged amounts, which are used for actual bookkeeping entries.

Read CLAUDE.md for project conventions and Swedish accounting requirements.

**Examine existing tagged amount code:**
@src/fiscal/amount/TaggedAmountFramework.cpp (especially lines 974-979)

**Search for TaggedAmount type definitions** and related business logic.
</context>

<requirements>
Create a tagged amount generation function with these characteristics:

1. **Generation Function**: `account_statements → Maybe<tagged_amounts>`
   - Transform account statements into tagged amounts
   - Apply final validation and business rules
   - Return optional/Maybe to handle transformation failures

2. **Tagged Amount Representation**:
   - What is a tagged amount in this domain?
   - What fields/metadata does it contain?
   - How does it relate to account statements?
   - Are there existing TaggedAmount types to use?

3. **Transformation Logic**:
   - What determines the "tags" on amounts?
   - Are amounts categorized automatically or manually?
   - Do bank vs SKV statements create different tagged amounts?
   - How are VAT and other metadata handled?

4. **Business Rules**:
   - Swedish accounting compliance (dates, amounts, descriptions per Bokföringslagen)
   - BAS Framework integration (account codes, categories)
   - SKV Framework compliance (tax reporting requirements)
   - Any validation that ensures bookkeeping entries are valid
</requirements>

<implementation>
**Design Considerations:**
- What does the current implementation in TaggedAmountFramework.cpp lines 974-979 do?
- Are there existing functions for creating tagged amounts?
- How are tags/categories assigned?
- Is there automatic categorization logic (keywords, patterns)?

**Swedish Accounting Frameworks:**
According to CLAUDE.md, integrate with:
- **BAS Framework**: Account codes and categories (BASFramework.hpp)
- **SKV Framework**: Tax agency compliance (SKVFramework.hpp)
- **HAD Framework**: Transaction handling (HADFramework.hpp)

Review these frameworks to understand how tagged amounts should be structured.

**Validation Strategy:**
Final validation before creating tagged amounts:
- All required fields present
- Amounts are valid and non-zero
- Categories/tags are valid per BAS framework
- Dates are in correct fiscal period
- Counterparty information is complete

**Error Handling:**
Use Maybe/optional for transformation failures:
- Missing categorization information?
- Invalid account codes?
- Failed business rule validation?
</implementation>

<output>
Modify or create files:
- `./src/fiscal/amount/tagged_amount_generator.cpp` - Transformation logic (or appropriate location)
- `./src/fiscal/amount/tagged_amount_generator.hpp` - Interface

**Add tests to:** `./src/test/test_csv_import_pipeline.cpp`
- Use namespace: `namespace tests::csv_import_pipeline::tagged_amount_suite { ... }`
- Test tagged amount generation from account statements

Example tests:
```cpp
namespace tests::csv_import_pipeline {
    namespace tagged_amount_suite {
        TEST(TaggedAmountTests, GenerateFromBankStatements) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(TaggedAmountTests, GenerateFromBankStatements)"};
            // Test generating tagged amounts from bank statements
        }

        TEST(TaggedAmountTests, BASFrameworkIntegration) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(TaggedAmountTests, BASFrameworkIntegration)"};
            // Test integration with BAS framework
        }

        TEST(TaggedAmountTests, CompletePipeline) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(TaggedAmountTests, CompletePipeline)"};
            // Test: file → CSV → statements → tagged amounts
        }
    }
}
```
</output>

<verification>
Before completing:

1. **Compilation**: Code compiles on macOS XCode with C++23 via `./run.zsh`
2. **Tests Pass**: Run `./cratchit --test` - all tests succeed
3. **Transformation Works**: Successfully creates tagged amounts from statements
4. **Business Rules**: Complies with Swedish accounting requirements
5. **Framework Integration**: Works with BAS, SKV, HAD frameworks
6. **Complete Pipeline**: Full chain works end-to-end (file → tagged amounts)

Test coverage:
- Bank statement CSV → tagged amounts
- SKV CSV → tagged amounts
- Statements with missing metadata
- Statements requiring categorization
- Edge cases (zero amounts, future dates)
- Complete monadic pipeline test
</verification>

<success_criteria>
- [ ] Tagged amount generation function implemented
- [ ] Returns Maybe<tagged_amounts> for monadic composition
- [ ] Transforms account statements to tagged amounts correctly
- [ ] Business rules and validation applied
- [ ] Integrates with BAS, SKV, HAD frameworks
- [ ] Composes with Steps 5-7 (complete pipeline works)
- [ ] Code compiles on macOS XCode
- [ ] Tested with bank and SKV CSV files
- [ ] Follows Swedish accounting requirements
- [ ] TaggedAmount type appropriately used or defined
- [ ] Complete monadic chain demonstrated: file → text → CSV → statements → tagged amounts
</success_criteria>
