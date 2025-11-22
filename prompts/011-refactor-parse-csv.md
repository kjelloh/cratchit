<objective>
Refactor the try_parse_csv function in parse_csv.cpp (line 25) to work seamlessly with the new encoding-aware pipeline and monadic composition patterns. This completes the refactoring effort.

This final step modernizes the CSV parsing interface to be fully compatible with the new pipeline architecture while maintaining any existing usage.
</objective>

<context>
This is Step 11 of an 11-step refactoring - the final step. Steps 1-9 created the complete pipeline, and Step 10 refactored TaggedAmountFramework.cpp.

This step refactors try_parse_csv to be the canonical CSV parsing interface for the new pipeline, completing the architectural transformation.

Read CLAUDE.md for project conventions.

**Examine the function to be refactored:**
@src/csv/parse_csv.cpp (focus on try_parse_csv around line 25)

**Review the CSV parsing layer from Step 6** which may have already modified this function.
</context>

<requirements>
Refactor try_parse_csv with these goals:

1. **Encoding-Aware Interface**:
   - Function should work with runtime encoded text (from Steps 1-5)
   - No assumptions about input encoding
   - Should compose naturally with encoding pipeline

2. **Monadic Return Type**:
   - Return Maybe/std::optional<csv_table> instead of throwing or error codes
   - Error handling through optional composition
   - Clear success/failure semantics

3. **Maintain Compatibility**:
   - Existing callers should continue to work (if any)
   - Provide migration path if breaking changes needed
   - Consider adding overloads if complete replacement isn't feasible

4. **Integration with Pipeline**:
   - Should integrate seamlessly with Step 6's CSV parsing layer
   - May need to reconcile any changes made in Step 6
   - Ensure consistency across the codebase
</requirements>

<implementation>
**Before Refactoring:**
1. **Understand Current Implementation**:
   - What does try_parse_csv currently do?
   - What are its parameters and return type?
   - How does it handle errors?
   - What CSV format assumptions does it make?
   - Is it used elsewhere in the codebase?

2. **Check Step 6 Changes**:
   - Was try_parse_csv already modified in Step 6?
   - If so, is this step about finishing that work?
   - Or ensuring consistency with the rest of the codebase?

**Refactoring Strategy:**
```cpp
// Old signature (example):
csv_table try_parse_csv(const std::string& content);
// Throws on error, assumes encoding already handled

// New signature:
std::optional<csv_table> try_parse_csv(std::string_view content);
// Or: Maybe<csv_table> try_parse_csv(std::string_view content);
// Returns optional, works with pipeline
```

**Backward Compatibility:**
If existing code depends on try_parse_csv:
1. Option 1: Update all callers to use new signature
2. Option 2: Keep old signature, add new one with different name
3. Option 3: Use [[deprecated]] on old signature, provide migration guide

Choose based on the scope of existing usage.

**Integration Points:**
- Should work with Step 5's encoding pipeline output
- Should be used by Step 6's CSV parsing layer
- Should support Step 7's statement extraction
</implementation>

<output>
Modify this file:
- `./src/csv/parse_csv.cpp` - Refactor try_parse_csv function
- `./src/csv/parse_csv.hpp` - Update function signature (if header exists)

**Update tests in:** `./src/test/test_csv_import_pipeline.cpp` (if needed)
- Use namespace: `namespace tests::csv_import_pipeline::parse_csv_refactor_suite { ... }`
- Add tests to verify refactored try_parse_csv maintains behavior

Example tests:
```cpp
namespace tests::csv_import_pipeline {
    namespace parse_csv_refactor_suite {
        TEST(ParseCSVRefactorTests, RefactoredFunctionWorks) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(ParseCSVRefactorTests, RefactoredFunctionWorks)"};
            // Verify refactored try_parse_csv works correctly
        }

        TEST(ParseCSVRefactorTests, CompleteRefactoringValidation) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(ParseCSVRefactorTests, CompleteRefactoringValidation)"};
            // Final validation: all 11 steps complete and working
        }
    }
}
```

Ensure:
- Encoding-aware (works with runtime encoded text)
- Returns Maybe/optional for monadic composition
- Integrates with complete pipeline
- Existing callers updated or backward compatibility maintained
</output>

<verification>
Before completing:

1. **Compilation**: All code compiles on macOS XCode with C++23 via `./run.zsh`
2. **All Tests Pass**: Run `./cratchit --test` - all tests (unit, integration) pass
3. **Pipeline Integration**: Works seamlessly with Steps 1-9
4. **Consistency**: CSV parsing is consistent across codebase
5. **No Regressions**: All existing functionality preserved
6. **Complete Refactoring**: All 11 steps are now complete

**Final validation - run comprehensive tests:**
- CSV parsing works correctly
- Encoding-aware pipeline functions end-to-end
- No regressions in existing features
- TaggedAmountFramework and parse_csv both use new pipeline
- All tests in test_csv_import_pipeline.cpp pass
- Complete monadic pipeline: file → raw bytes → encoding → unicode → runtime → CSV → statements → tagged amounts
</verification>

<success_criteria>
- [ ] try_parse_csv function successfully refactored
- [ ] Returns Maybe/std::optional for monadic composition
- [ ] Works with encoding-aware pipeline from Steps 1-5
- [ ] Integrates with complete pipeline (Steps 1-9)
- [ ] Code compiles on macOS XCode with C++23
- [ ] All tests pass (unit, integration, end-to-end)
- [ ] No regressions in existing functionality
- [ ] Backward compatibility maintained or migration path provided
- [ ] CSV parsing consistent across entire codebase
- [ ] **COMPLETE REFACTORING FINISHED**: All 11 steps done
- [ ] Full monadic pipeline: file path → raw bytes → encoding detection → Unicode → runtime encoding → CSV → statements → tagged amounts
</success_criteria>

<completion_notes>
Once this step is complete, the entire refactoring is finished:
- ✅ Steps 1-5: Encoding pipeline with lazy range views
- ✅ Steps 6-8: Domain pipeline with monadic composition
- ✅ Step 9: Full integration and testing
- ✅ Step 10: TaggedAmountFramework.cpp refactored
- ✅ Step 11: parse_csv.cpp refactored

The codebase now has a clean, composable, monadic architecture for CSV import with proper encoding handling, lazy evaluation, and readable error propagation.
</completion_notes>
