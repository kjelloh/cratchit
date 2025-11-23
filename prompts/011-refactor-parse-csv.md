<assistant-role>
You are an expert in writing C++23 code that is easy to read and can be used in functional style composition based on Maybe monads (std::optional, AnnotatedMaybe) and C++ range views where this makes sense.

1. You prefer to use C++23 'span' and 'string_view' over C-code constructs.
2. You prefer to use C++23 standard library over any specialized helpers.
3. You prefer readable code over efficient code.
4. You prefer to organize into namespaces over splitting code into C++ translation units.

You know about the codebase usage of namespaces and existing types for different functionality.

1. You are aware that Steps 1-10 created and integrated a complete CSV import pipeline
2. You are aware that this is the FINAL step of the refactoring
3. You are aware that parse_csv.cpp should be consistent with the new architecture
</assistant-role>

<objective>
Refactor parse_csv.cpp to work seamlessly with the new encoding-aware pipeline and ensure consistency across the entire codebase for CSV handling.

This FINAL step completes the refactoring effort by ensuring parse_csv.cpp is fully integrated with the new architecture, creating a unified CSV parsing experience throughout the codebase.
</objective>

<context>
This is Step 11 of an 11-step refactoring (prompts 001..011) - THE FINAL STEP.

**Completed Pipeline Architecture:**
```
file_path
  → text::encoding::read_file_with_encoding_detection() [Steps 1-5]
  → CSV::neutral::text_to_table()                       [Step 6]
  → domain::csv_table_to_account_statement_entries()           [Step 7]
  → domain::account_statements_to_tagged_amounts()      [Step 8]
  → cratchit::csv::import_file_to_tagged_amounts()      [Step 9]
  → TaggedAmountFramework integrated                    [Step 10]
```

**Original Refactoring Target:**
The original prompt 001 identified parse_csv.cpp try_parse_csv function (line 25) as code that should:
- Work with the encoding-aware pipeline
- Return Maybe/optional for monadic composition
- Integrate seamlessly with the rest of the pipeline

**Current State:**
Step 6 created `CSV::neutral::text_to_table()` which may already address this. This step should:
1. Verify parse_csv.cpp is consistent with the new architecture
2. Refactor try_parse_csv if it's not already aligned
3. Ensure no duplicate or inconsistent CSV parsing exists
4. Clean up any deprecated code

**Files to Review:**
@src/csv/parse_csv.cpp (the original try_parse_csv function)
@src/csv/parse_csv.hpp
@src/csv/neutral_parser.hpp (Step 6 implementation)
@src/csv/neutral_parser.cpp
@src/csv/import_pipeline.hpp (Step 9 high-level API)
@src/test/test_csv_import_pipeline.cpp

Read CLAUDE.md for project conventions.
</context>

<requirements>
Refactor and finalize parse_csv.cpp with these goals:

1. **Analyze Current State**:
   - Compare try_parse_csv with CSV::neutral::text_to_table
   - Determine if they serve different purposes or are redundant
   - Identify any callers of try_parse_csv

2. **Alignment Options**:

   **Option A: Consolidate**
   If try_parse_csv and CSV::neutral::text_to_table serve the same purpose:
   - Deprecate or remove try_parse_csv
   - Update all callers to use CSV::neutral::text_to_table
   - Or make try_parse_csv a thin wrapper around the neutral parser

   **Option B: Differentiate**
   If they serve different purposes:
   - Document the difference clearly
   - Ensure both use consistent patterns (optional return, encoding-aware)
   - Consider if try_parse_csv should use the encoding pipeline

3. **Interface Consistency**:
   - Return Maybe/std::optional for monadic composition
   - Accept runtime-encoded text (UTF-8 strings from the encoding pipeline)
   - Follow established patterns from test_csv_import_pipeline.cpp

4. **Codebase Cleanup**:
   - Remove any deprecated CSV parsing code
   - Ensure consistent naming conventions
   - Update or remove outdated comments

5. **Final Validation**:
   - All CSV parsing in the codebase uses consistent patterns
   - The complete pipeline works end-to-end
   - All tests pass
</requirements>

<implementation>
**Analysis Required:**
1. Read parse_csv.cpp and understand try_parse_csv's current implementation
2. Compare with CSV::neutral::text_to_table from Step 6
3. Search for all usages of try_parse_csv in the codebase
4. Determine the best consolidation/alignment strategy

**Refactoring Strategy:**

If try_parse_csv should be a wrapper:
```cpp
namespace csv {
    // Backward-compatible wrapper
    [[deprecated("Use CSV::neutral::text_to_table instead")]]
    auto try_parse_csv(std::string_view content)
        -> std::optional<CSV::Table>
    {
        return CSV::neutral::text_to_table(std::string(content));
    }

    // Or update signature to match new pattern
    auto try_parse_csv(std::string_view content)
        -> std::optional<CSV::Table>
    {
        return CSV::neutral::text_to_table(std::string(content));
    }
}
```

If try_parse_csv should be removed:
- Update all callers to use CSV::neutral::text_to_table
- Remove try_parse_csv function
- Update any tests

**Migration Path:**
If there are many callers of try_parse_csv:
1. First, make it a wrapper around CSV::neutral::text_to_table
2. Mark it [[deprecated]]
3. Update callers incrementally
4. Eventually remove the wrapper

**Consistency Checks:**
- Verify CSV::Table type is used consistently
- Verify delimiter detection is consistent
- Verify quoting/escaping rules are consistent
- Verify error handling is consistent
</implementation>

<output>
Modify these files as needed:
- `./src/csv/parse_csv.cpp` - Refactor try_parse_csv function
- `./src/csv/parse_csv.hpp` - Update function signature if needed

**Update tests in:** `./src/test/test_csv_import_pipeline.cpp`
- Use namespace: `namespace tests::csv_import_pipeline::parse_csv_final_suite { ... }`
- Add tests to verify final integration

Example tests:
```cpp
namespace tests::csv_import_pipeline {
    namespace parse_csv_final_suite {

        TEST(ParseCSVFinalTests, TryParseCsvConsistentWithNeutralParser) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(ParseCSVFinalTests, TryParseCsvConsistentWithNeutralParser)"};
            // Test that try_parse_csv and CSV::neutral::text_to_table
            // produce the same results for the same input
            std::string csv_text = "A,B,C\n1,2,3\n";
            // Compare results from both functions
        }

        TEST(ParseCSVFinalTests, CompletePipelineValidation) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(ParseCSVFinalTests, CompletePipelineValidation)"};
            // Final end-to-end test: file → tagged amounts
            // Using real CSV data
        }

        TEST(ParseCSVFinalTests, AllCSVParsingUsesConsistentPatterns) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(ParseCSVFinalTests, AllCSVParsingUsesConsistentPatterns)"};
            // Verify CSV parsing returns optional
            // Verify encoding is handled correctly
        }

        TEST(ParseCSVFinalTests, NoRegressions) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(ParseCSVFinalTests, NoRegressions)"};
            // Run all existing tests
            // Verify complete refactoring hasn't broken anything
        }
    }
}
```
</output>

<verification>
Before completing:

1. **Compilation**: All code compiles on macOS XCode with C++23 via `./run.zsh --nop`
2. **Code Signing**: Execute 'codesign -s - --force --deep workspace/cratchit' if needed
3. **All Tests Pass**: Run `cd workspace && ./cratchit --test` - ALL tests pass
4. **Pipeline Integration**: Works seamlessly with Steps 1-9
5. **Consistency**: CSV parsing is consistent across codebase
6. **No Regressions**: All existing functionality preserved

**Final Validation Checklist:**
- [ ] CSV parsing returns optional/Maybe types
- [ ] Encoding-aware pipeline functions end-to-end
- [ ] No regressions in existing features
- [ ] TaggedAmountFramework uses new pipeline (Step 10)
- [ ] parse_csv.cpp is aligned with new architecture (Step 11)
- [ ] All tests in test_csv_import_pipeline.cpp pass
- [ ] Complete monadic pipeline works: file → text → CSV → statements → tagged amounts
</verification>

<success_criteria>
- [ ] parse_csv.cpp try_parse_csv function aligned with new architecture
- [ ] Returns Maybe/std::optional for monadic composition
- [ ] Works with encoding-aware pipeline from Steps 1-5
- [ ] Consistent with CSV::neutral::text_to_table from Step 6
- [ ] Code compiles on macOS XCode with C++23
- [ ] All tests pass (unit, integration, end-to-end)
- [ ] No regressions in existing functionality
- [ ] Backward compatibility maintained or migration path provided
- [ ] CSV parsing consistent across entire codebase
- [ ] **COMPLETE REFACTORING FINISHED**: All 11 steps done
</success_criteria>

<completion_notes>
Once this step is complete, the entire refactoring is finished:

- ✅ Steps 1-5: Encoding pipeline with lazy range views
  - File I/O with AnnotatedMaybe
  - Encoding detection with ICU
  - Transcoding views (bytes → Unicode → UTF-8)

- ✅ Steps 6-8: Domain pipeline with monadic composition
  - CSV::neutral::text_to_table (neutral CSV parsing)
  - domain::csv_table_to_account_statement_entries (column detection, extraction)
  - domain::account_statements_to_tagged_amounts (final domain objects)

- ✅ Step 9: Full integration
  - cratchit::csv::import_file_to_tagged_amounts() high-level API
  - Comprehensive integration tests

- ✅ Step 10: TaggedAmountFramework.cpp refactored
  - Uses new pipeline
  - Backward compatible

- ✅ Step 11: parse_csv.cpp finalized
  - Consistent with new architecture
  - Clean, maintainable codebase

**The codebase now has:**
- Clean, composable, monadic architecture for CSV import
- Proper encoding handling (UTF-8, ISO-8859-1, Windows-1252, etc.)
- Lazy evaluation where appropriate
- Readable error propagation via AnnotatedMaybe
- Comprehensive test coverage
- Simple high-level API for consumers
</completion_notes>
