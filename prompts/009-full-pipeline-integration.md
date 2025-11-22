<objective>
Create a comprehensive integration checkpoint for the complete pipeline: file path → tagged amounts. Test the full monadic composition, verify error handling at each stage, and ensure the system is production-ready.

This integration step validates that all 8 previous steps work together seamlessly before refactoring the existing code to use the new pipeline.
</objective>

<context>
This is Step 9 of an 11-step refactoring - the final integration checkpoint. Previous steps provided:
- Steps 1-5: Complete encoding pipeline (file → runtime encoded text)
- Steps 6-8: Complete domain pipeline (text → CSV → statements → tagged amounts)

This step ensures everything works together with comprehensive testing and documentation before Steps 10-11 refactor existing code to use this new pipeline.

Read CLAUDE.md for project conventions.

**Review all previous implementations** to understand the complete pipeline architecture.
</context>

<requirements>
Create comprehensive integration testing and documentation:

1. **High-Level API**:
   - Simple function: `file_path → Maybe<tagged_amounts>`
   - Encapsulates all 8 transformation steps
   - Clean, documented interface for consumers

2. **Complete Pipeline Verification**:
   ```
   file_path
     → Maybe<byte_buffer>           (Step 1)
     → Maybe<DetectedEncoding>      (Step 2)
     → unicode_code_points_view     (Step 3)
     → runtime_encoded_view         (Step 4)
     → Maybe<text>                  (Step 5)
     → Maybe<csv_table>             (Step 6)
     → Maybe<account_statements>    (Step 7)
     → Maybe<tagged_amounts>        (Step 8)
   ```

3. **Error Handling Verification**:
   - Test failure at each stage propagates correctly through Maybe chain
   - File not found (Step 1)
   - Encoding detection failure (Step 2)
   - Transcoding error (Steps 3-4)
   - CSV parsing failure (Step 6)
   - Invalid business data (Steps 7-8)

4. **Comprehensive Test Suite**:
   - Real-world bank CSV files
   - Real-world SKV CSV files
   - Various encodings (UTF-8, ISO-8859-1, Windows-1252)
   - Malformed data at each stage
   - Performance tests (large files, lazy evaluation)
   - Edge cases (empty files, headers only, etc.)

5. **Documentation**:
   - Architecture overview
   - Usage examples
   - Error handling behavior
   - Performance characteristics
   - Integration guide for existing code
</requirements>

<implementation>
**High-Level API Design:**
Create the simplest possible interface:
```cpp
// Option 1: Single function
auto result = import_csv_to_tagged_amounts(file_path);

// Option 2: Pipeline builder
auto result = CsvImportPipeline::from_file(file_path)
    .detect_encoding()
    .parse_csv()
    .extract_statements()
    .generate_tagged_amounts()
    .execute();

// Choose based on what's most readable and maintainable
```

**Testing Strategy:**
Create comprehensive tests organized by layer:
1. **Integration Tests**: Full pipeline with real CSV files
2. **Error Propagation Tests**: Verify failures at each stage
3. **Performance Tests**: Large files, memory usage
4. **Edge Case Tests**: Empty files, malformed data, etc.

**Performance Verification:**
- Lazy evaluation prevents excessive memory usage
- Large CSV files (10k+ rows) process efficiently
- No unnecessary string materializations

**Documentation Structure:**
```markdown
# CSV Import Pipeline

## Overview
[Architecture diagram and explanation]

## Usage
[Simple examples]

## Error Handling
[How failures propagate]

## Performance
[Lazy evaluation, memory characteristics]

## Integration Guide
[How existing code should use this]
```
</implementation>

<output>
Create or modify files:
- `./src/csv_import_pipeline.hpp` - High-level API (or appropriate location)
- `./src/csv_import_pipeline.cpp` - Implementation composing Steps 1-8
- `./docs/csv_import_pipeline.md` - Architecture and usage documentation

**Add comprehensive tests to:** `./src/test/test_csv_import_pipeline.cpp`
- Use namespace: `namespace tests::csv_import_pipeline::full_integration_suite { ... }`
- Comprehensive end-to-end integration tests

Example tests:
```cpp
namespace tests::csv_import_pipeline {
    namespace full_integration_suite {
        TEST(FullPipelineTests, ImportRealBankCSV) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, ImportRealBankCSV)"};
            // Test complete pipeline with real bank CSV
        }

        TEST(FullPipelineTests, MalformedCSVHandledGracefully) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, MalformedCSVHandledGracefully)"};
            // Test error handling for malformed CSV
        }

        TEST(FullPipelineTests, LargeFileMemoryEfficiency) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, LargeFileMemoryEfficiency)"};
            // Test memory efficiency with large file
        }

        TEST(FullPipelineTests, ErrorPropagationAtEachStage) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, ErrorPropagationAtEachStage)"};
            // Test failures at each pipeline stage
        }
    }
}
```

Ensure clear separation between:
- Public API (simple interface)
- Internal implementation (all the steps)
- Test coverage (comprehensive)
- Documentation (clear guide)
</output>

<verification>
Before completing, verify EVERYTHING:

1. **Compilation**: All code compiles on macOS XCode with C++23 via `./run.zsh`
2. **All Tests Pass**: Run `./cratchit --test` - integration, error handling, performance, edge cases all pass
3. **Real CSV Files**: Successfully imports actual bank and SKV CSV files
4. **Error Propagation**: Failures at each stage handled correctly
5. **Performance**: Large files process efficiently without excessive memory
6. **Lazy Evaluation**: Verify ranges are truly lazy (profile if needed)
7. **Documentation**: Complete, accurate, and helpful
8. **API Simplicity**: Can a developer use this without understanding internals?

Test coverage:
- Complete successful import (file → tagged amounts)
- Error handling at each stage (file I/O, encoding, CSV parsing, domain logic)
- Large file memory efficiency
- Multiple encodings (UTF-8, ISO-8859-1, Windows-1252)
- Bank and SKV CSV formats
</verification>

<success_criteria>
- [ ] High-level API created for file path → tagged amounts
- [ ] Composes all Steps 1-8 successfully
- [ ] Comprehensive integration tests pass
- [ ] Real-world CSV files import correctly
- [ ] Error handling verified at each pipeline stage
- [ ] Performance tests pass (lazy evaluation, large files)
- [ ] Edge cases handled gracefully
- [ ] Code compiles on macOS XCode with C++23
- [ ] Architecture documentation complete and clear
- [ ] Usage examples provided
- [ ] Integration guide written for existing code
- [ ] Ready for Steps 10-11 to refactor existing code to use this pipeline
- [ ] Monadic composition works seamlessly throughout entire pipeline
</success_criteria>
