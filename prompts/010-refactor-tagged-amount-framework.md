<objective>
Refactor TaggedAmountFramework.cpp lines 974-979 to use the new CSV import pipeline created in Steps 1-9. Replace existing implementation with clean pipeline integration.

This step modernizes existing code to use the new composable, monadic architecture while maintaining backward compatibility where needed.
</objective>

<context>
This is Step 10 of an 11-step refactoring. Steps 1-9 created and validated a complete pipeline: file path → tagged amounts.

Now we refactor the existing code in TaggedAmountFramework.cpp to use this new pipeline, replacing the old implementation with the modern, composable approach.

Read CLAUDE.md for project conventions.

**Examine the code to be refactored:**
@src/fiscal/amount/TaggedAmountFramework.cpp (focus on lines 974-979)

**Review the new pipeline API from Step 9** to understand the replacement interface.
</context>

<requirements>
Refactor the existing code with these goals:

1. **Replace Implementation**:
   - Identify what lines 974-979 currently do
   - Replace with new pipeline API
   - Simplify the code using monadic composition

2. **Maintain Functionality**:
   - Existing behavior should be preserved
   - Same inputs should produce same outputs
   - Don't break existing tests or dependent code

3. **Improve Maintainability**:
   - Code should be clearer and more readable
   - Error handling should be more explicit
   - Encoding issues should be handled automatically

4. **Integration Testing**:
   - Verify existing tests still pass
   - Add new tests if coverage is insufficient
   - Ensure no regressions in functionality
</requirements>

<implementation>
**Before Refactoring:**
1. **Understand Current Implementation**:
   - What exactly do lines 974-979 do?
   - How is file reading currently handled?
   - How is encoding currently handled?
   - How are CSV parsing and domain object creation done?
   - What error handling exists?

2. **Identify Dependencies**:
   - What code calls this section?
   - What does this section call?
   - Are there tests that verify this behavior?
   - What assumptions does dependent code make?

**Refactoring Strategy:**
```cpp
// Old approach (lines 974-979):
// [Manual file reading, encoding handling, CSV parsing, etc.]

// New approach:
auto maybe_amounts = import_csv_to_tagged_amounts(file_path)
    .or_else([]() {
        // Handle error - log, return default, throw?
        // Match existing error handling behavior
    });
```

**Backward Compatibility:**
- If existing code expects exceptions, adapt Maybe failures appropriately
- If existing code expects specific error codes, map Maybe failures to them
- Ensure the refactor is a drop-in replacement

**Testing Strategy:**
- Run existing tests to verify no regressions
- Add tests for new error handling paths if they differ
- Consider integration tests with real CSV files
</implementation>

<output>
Modify this file:
- `./src/fiscal/amount/TaggedAmountFramework.cpp` - Refactor lines 974-979

**Update tests in:** `./src/test/test_csv_import_pipeline.cpp` (if needed)
- Use namespace: `namespace tests::csv_import_pipeline::tagged_amount_framework_suite { ... }`
- Add tests to verify refactored code maintains behavior

Example tests:
```cpp
namespace tests::csv_import_pipeline {
    namespace tagged_amount_framework_suite {
        TEST(TaggedAmountFrameworkTests, RefactoredCodeMaintainsBehavior) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(TaggedAmountFrameworkTests, RefactoredCodeMaintainsBehavior)"};
            // Verify refactored code produces same results as before
        }
    }
}
```

Ensure:
- Code is clearer and more maintainable
- Uses the new pipeline from Steps 1-9
- Maintains backward compatibility
- Existing tests pass
</output>

<verification>
Before completing:

1. **Compilation**: Code compiles on macOS XCode with C++23 via `./run.zsh`
2. **Tests Pass**: Run `./cratchit --test` - all existing tests still pass
3. **Functionality Preserved**: Behavior matches old implementation
4. **Code Clarity**: New code is more readable and maintainable
5. **Error Handling**: Errors handled appropriately (matching old behavior or better)
6. **Integration**: Dependent code works without modification

Test coverage:
- Existing tests for TaggedAmountFramework pass
- Refactored lines 974-979 produce same results
- Integration with new pipeline works correctly
</verification>

<success_criteria>
- [ ] Lines 974-979 in TaggedAmountFramework.cpp successfully refactored
- [ ] Uses new CSV import pipeline from Steps 1-9
- [ ] Code compiles on macOS XCode
- [ ] All existing tests pass
- [ ] No regressions in functionality
- [ ] Error handling maintains or improves upon previous behavior
- [ ] Code is more readable and maintainable
- [ ] Backward compatibility maintained where needed
- [ ] Encoding issues handled automatically by pipeline
- [ ] Monadic composition makes control flow clearer
</success_criteria>
