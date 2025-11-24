<assistant-role>
You are an expert in writing C++23 code that is easy to read and can be used in functional style composition based on Maybe monads (std::optional, AnnotatedMaybe) and C++ range views where this makes sense.

1. You prefer to use C++23 'span' and 'string_view' over C-code constructs.
2. You prefer to use C++23 standard library over any specialized helpers.
3. You prefer readable code over efficient code.
4. You prefer to organize into namespaces over splitting code into C++ translation units.

You know about the codebase usage of namespaces and existing types for different functionality.

1. You are aware that Steps 1-9 created a complete, tested CSV import pipeline
2. You are aware that this step refactors existing code to use the new pipeline
3. You are aware that backward compatibility should be maintained where possible
</assistant-role>

<objective>
Refactor TaggedAmountFramework.cpp to use the new CSV import pipeline created in Steps 1-9.

This step modernizes the existing code in TaggedAmountFramework.cpp (originally referenced at lines 974-979) to use the new composable, monadic architecture while maintaining backward compatibility with existing callers.
</objective>

<context>
This is Step 10 of an 11-step refactoring (prompts 001..011).

**The New Pipeline (from Steps 1-9):**
```cpp
// High-level API from Step 9
cratchit::csv::import_file_to_tagged_amounts(file_path)
  → AnnotatedMaybe<std::vector<TaggedAmount>>
```

**Original Refactoring Target:**
The original prompt 001 identified TaggedAmountFramework.cpp lines 974-979 as code that should use the new pipeline. This code likely:
- Reads CSV files from banks/SKV
- Handles encoding manually
- Parses CSV data
- Creates tagged amounts

**Files to Review:**
@src/fiscal/amount/TaggedAmountFramework.cpp (focus on CSV import code)
@src/fiscal/amount/TaggedAmountFramework.hpp
@src/csv/import_pipeline.hpp (the new high-level API from Step 9)
@src/test/test_csv_import_pipeline.cpp (integration tests)

Read CLAUDE.md for project conventions.
</context>

<requirements>
Refactor the existing code with these goals:

1. **Identify Current Implementation**:
   - Find all CSV import code in TaggedAmountFramework.cpp
   - Understand what the current code does (file reading, encoding, parsing)
   - Identify callers of this code

2. **Replace with New Pipeline**:
   - Replace manual file reading → use import_file_to_tagged_amounts()
   - Replace manual encoding handling → handled by pipeline
   - Replace manual CSV parsing → handled by pipeline
   - Keep any post-processing logic that's specific to TaggedAmountFramework

3. **Maintain Functionality**:
   - Existing behavior should be preserved
   - Same inputs should produce same outputs
   - Don't break existing tests or dependent code

4. **Improve Error Handling**:
   - Old code may throw exceptions or use error codes
   - New code uses AnnotatedMaybe for clear error propagation
   - Adapt as needed to match existing interface expectations

5. **Backward Compatibility**:
   - If existing code expects exceptions, wrap AnnotatedMaybe appropriately
   - If existing code expects specific return types, adapt the result
   - Consider deprecating old interfaces if appropriate
</requirements>

<implementation>
**Analysis Steps:**
1. **Search for CSV-related code** in TaggedAmountFramework.cpp:
   - Look for file reading (std::ifstream, fopen, etc.)
   - Look for encoding detection/conversion
   - Look for CSV parsing
   - Look for tagged amount creation from CSV data

2. **Understand Dependencies**:
   - What functions call this CSV import code?
   - What types do they expect as return values?
   - Are there existing tests?

3. **Refactoring Strategy**:
```cpp
// Old approach (conceptual):
std::vector<TaggedAmount> import_csv_from_file(std::string_view path) {
    // Manual file reading
    // Manual encoding handling
    // Manual CSV parsing
    // Manual tagged amount creation
}

// New approach:
std::vector<TaggedAmount> import_csv_from_file(std::string_view path) {
    auto result = cratchit::csv::import_file_to_tagged_amounts(path);
    if (!result) {
        // Handle error based on existing interface expectations
        // Option 1: throw exception (if old code did)
        // Option 2: return empty vector with logged error
        // Option 3: return std::optional
    }
    return std::move(result.value());
}
```

4. **Preserve Business Logic**:
   - If TaggedAmountFramework has specific post-processing
   - If there are TaggedAmount-specific validations
   - Keep these, just change the data source to the new pipeline

**Testing Strategy:**
- Run existing tests to verify no regressions
- Add integration tests if coverage is insufficient
- Test that refactored code produces same results as before
</implementation>

<output>
Modify this file:
- `./src/fiscal/amount/TaggedAmountFramework.cpp` - Refactor CSV import code

**Update tests in:** `./src/test/test_csv_import_pipeline.cpp`
- Use namespace: `namespace tests::csv_import_pipeline::tagged_amount_framework_refactor_suite { ... }`
- Add tests to verify refactored code maintains behavior

Example tests:
```cpp
namespace tests::csv_import_pipeline {
    namespace tagged_amount_framework_refactor_suite {

        TEST(TaggedAmountFrameworkRefactorTests, RefactoredCodeProducesSameResults) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(TaggedAmountFrameworkRefactorTests, RefactoredCodeProducesSameResults)"};
            // Create test file with known CSV content
            // Call refactored TaggedAmountFramework function
            // Verify results match expected output
        }

        TEST(TaggedAmountFrameworkRefactorTests, ErrorHandlingMaintained) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(TaggedAmountFrameworkRefactorTests, ErrorHandlingMaintained)"};
            // Test error handling with missing file
            // Verify behavior matches original implementation's contract
        }

        TEST(TaggedAmountFrameworkRefactorTests, EncodingHandledAutomatically) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(TaggedAmountFrameworkRefactorTests, EncodingHandledAutomatically)"};
            // Create test files with different encodings
            // Verify pipeline handles encoding correctly
        }

        TEST(TaggedAmountFrameworkRefactorTests, ExistingTestsStillPass) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(TaggedAmountFrameworkRefactorTests, ExistingTestsStillPass)"};
            // Run any existing TaggedAmountFramework tests
            // Verify no regressions
        }
    }
}
```
</output>

<verification>
Before completing:

1. **Compilation**: Code compiles on macOS XCode with C++23 via `./run.zsh --nop`
2. **Code Signing**: Execute 'codesign -s - --force --deep workspace/cratchit' if needed
3. **Tests Pass**: Run `cd workspace && ./cratchit --test` - all existing tests still pass
4. **Functionality Preserved**: Behavior matches old implementation
5. **Code Clarity**: New code is more readable and maintainable
6. **Error Handling**: Errors handled appropriately (matching old behavior or better)
7. **Integration**: Dependent code works without modification

Test coverage:
- Existing tests for TaggedAmountFramework pass
- Refactored CSV import produces same results
- Error handling maintained
- Different encodings handled correctly
</verification>

<success_criteria>
- [ ] CSV import code in TaggedAmountFramework.cpp successfully refactored
- [ ] Uses new import_file_to_tagged_amounts() pipeline from Step 9
- [ ] Code compiles on macOS XCode with C++23
- [ ] All existing tests pass (no regressions)
- [ ] Functionality preserved (same inputs → same outputs)
- [ ] Error handling maintains or improves upon previous behavior
- [ ] Code is more readable and maintainable
- [ ] Backward compatibility maintained where needed
- [ ] Encoding issues handled automatically by pipeline
- [ ] Monadic composition makes control flow clearer
</success_criteria>
