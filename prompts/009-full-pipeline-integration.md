<assistant-role>
You are an expert in writing C++23 code that is easy to read and can be used in functional style composition based on Maybe monads (std::optional, AnnotatedMaybe) and C++ range views where this makes sense.

1. You prefer to use C++23 'span' and 'string_view' over C-code constructs.
2. You prefer to use C++23 standard library over any specialized helpers.
3. You prefer readable code over efficient code.
4. You prefer to organize into namespaces over splitting code into C++ translation units.

You know about the codebase usage of namespaces and existing types for different functionality.

1. You are aware that we are completing the CSV import pipeline: file → Maybe<TaggedAmounts>
2. You are aware that Steps 1-8 are complete and provide all transformation steps
3. You are creating the unified high-level API that composes all steps
</assistant-role>

<objective>
Create a comprehensive high-level API for the complete pipeline: file_path → Maybe<TaggedAmounts>

This integration step creates a simple, documented interface that encapsulates all 8 transformation steps, making it easy for consumers to import CSV files and get tagged amounts for bookkeeping.
</objective>

<context>
This is Step 9 of an 11-step refactoring (prompts 001..011).

**Complete Pipeline (from test_csv_import_pipeline.cpp):**
```
file_path
  → Maybe<byte_buffer>           (Step 1 - cratchit::io::read_file_to_buffer)
  → Maybe<DetectedEncoding>      (Step 2 - text::encoding::icu::detect_buffer_encoding)
  → unicode_code_points_view     (Step 3 - text::encoding::views::bytes_to_unicode)
  → runtime_encoded_view         (Step 4 - text::encoding::views::unicode_to_runtime_encoding)
  → Maybe<text>                  (Step 5 - text::encoding::read_file_with_encoding_detection)
  → Maybe<csv_table>             (Step 6 - CSV::neutral::text_to_table)
  → Maybe<account_statements>    (Step 7 - domain::csv_table_to_account_statement_entries)
  → Maybe<tagged_amounts>        (Step 8 - domain::account_statements_to_tagged_amounts)
```

**Existing Integration Points (from test file):**
- `text::encoding::read_file_with_encoding_detection()` - Combines Steps 1-5
- `CSV::neutral::text_to_table()` - Step 6
- `domain::csv_table_to_account_statement_entries()` - Step 7
- `domain::account_statements_to_tagged_amounts()` - Step 8 (from previous prompt)

**Review existing implementations:**
@src/io/file_reader.hpp (Step 1)
@src/text/encoding_pipeline.hpp (Steps 2-5)
@src/csv/neutral_parser.hpp (Step 6)
@src/domain/csv_to_account_statement.hpp (Step 7)
@src/domain/account_statement_to_tagged_amount.hpp (Step 8)
@src/test/test_csv_import_pipeline.cpp (all test suites)

Read CLAUDE.md for project conventions.
</context>

<requirements>
Create a unified high-level API with these characteristics:

1. **Simple Top-Level Function**:
   ```cpp
   namespace cratchit::csv {
       auto import_file_to_tagged_amounts(
           std::filesystem::path const& file_path
       ) -> AnnotatedMaybe<std::vector<TaggedAmount>>;
   }
   ```

2. **Monadic Composition Chain**:
   - Chain all 8 steps using .and_then()
   - Preserve error messages from each step
   - Short-circuit on any failure

3. **Error Handling**:
   - File not found → clear error message
   - Encoding detection failure → error message with details
   - CSV parsing failure → error message with location if possible
   - Invalid business data → error message with specifics
   - All errors propagate through AnnotatedMaybe

4. **Messages Documentation**:
   - Each step should contribute messages to the AnnotatedMaybe
   - Success messages: "File read successfully", "Detected UTF-8 encoding", etc.
   - Failure messages: specific and actionable

5. **Comprehensive Integration Tests**:
   - Test complete pipeline with real CSV files
   - Test error handling at each stage
   - Test with various encodings (UTF-8, ISO-8859-1)
   - Test with both bank and SKV CSV formats
</requirements>

<implementation>
**API Design:**
```cpp
namespace cratchit::csv {

    // Primary high-level API
    auto import_file_to_tagged_amounts(
        std::filesystem::path const& file_path
    ) -> AnnotatedMaybe<std::vector<TaggedAmount>>;

    // Optional: Alternative API that returns intermediate results
    struct ImportResult {
        std::string runtime_text;
        CSV::Table csv_table;
        std::vector<domain::AccountStatementEntry> account_statements;
        std::vector<TaggedAmount> tagged_amounts;
    };

    auto import_file_with_details(
        std::filesystem::path const& file_path
    ) -> AnnotatedMaybe<ImportResult>;

}
```

**Implementation Pattern:**
```cpp
auto import_file_to_tagged_amounts(
    std::filesystem::path const& file_path
) -> AnnotatedMaybe<std::vector<TaggedAmount>> {
    return text::encoding::read_file_with_encoding_detection(file_path)
        .and_then([](auto& text) -> AnnotatedMaybe<CSV::Table> {
            // Step 6: Parse CSV
            auto maybe_table = CSV::neutral::text_to_table(text);
            // ... handle result
        })
        .and_then([](auto& table) -> AnnotatedMaybe<std::vector<AccountStatementEntry>> {
            // Step 7: Extract account statements
            // ...
        })
        .and_then([](auto& statements) -> AnnotatedMaybe<std::vector<TaggedAmount>> {
            // Step 8: Generate tagged amounts
            // ...
        });
}
```

**Message Aggregation:**
Ensure messages from all steps are preserved in the final result. The AnnotatedMaybe.and_then() should accumulate messages.

**File Organization:**
- Create `./src/csv/import_pipeline.hpp` - Public API
- Create `./src/csv/import_pipeline.cpp` - Implementation
- Or add to existing appropriate location based on codebase conventions
</implementation>

<output>
Create or modify files:
- `./src/csv/import_pipeline.hpp` - High-level API header
- `./src/csv/import_pipeline.cpp` - Implementation composing Steps 1-8

**Add comprehensive tests to:** `./src/test/test_csv_import_pipeline.cpp`
- Use namespace: `namespace tests::csv_import_pipeline::full_pipeline_suite { ... }`

Example tests:
```cpp
namespace tests::csv_import_pipeline {
    namespace full_pipeline_suite {

        TEST(FullPipelineTests, ImportNordeaCSVSuccess) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, ImportNordeaCSVSuccess)"};
            // Create temp file with sz_NORDEA_csv_20251120
            // Call import_file_to_tagged_amounts()
            // Verify tagged amounts
        }

        TEST(FullPipelineTests, ImportSKVCSVSuccess) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, ImportSKVCSVSuccess)"};
            // Create temp file with sz_SKV_csv_20251120
            // Call import_file_to_tagged_amounts()
            // Verify tagged amounts
        }

        TEST(FullPipelineTests, FileNotFoundError) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, FileNotFoundError)"};
            // Test with non-existent file
            // Verify failure with appropriate error message
        }

        TEST(FullPipelineTests, ErrorMessagesPreserved) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, ErrorMessagesPreserved)"};
            // Verify that success messages from all steps are in result.m_messages
        }

        TEST(FullPipelineTests, ISO8859EncodingHandled) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, ISO8859EncodingHandled)"};
            // Create temp file with ISO-8859-1 encoding
            // Verify pipeline handles encoding correctly
        }

        TEST(FullPipelineTests, EmptyFileHandledGracefully) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, EmptyFileHandledGracefully)"};
            // Empty file → appropriate result (empty or error)
        }

        TEST(FullPipelineTests, MalformedCSVHandledGracefully) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, MalformedCSVHandledGracefully)"};
            // Malformed CSV → failure with clear error message
        }

        TEST(FullPipelineTests, MonadicShortCircuiting) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(FullPipelineTests, MonadicShortCircuiting)"};
            // Verify that failure at Step 1 doesn't run Steps 2-8
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
4. **Pipeline Integration**: All 8 steps compose correctly
5. **Error Propagation**: Failures at each stage handled correctly
6. **Message Preservation**: Success/error messages from all steps preserved

Test coverage:
- Complete successful import (file → tagged amounts)
- Error handling at each stage (file I/O, encoding, CSV parsing, domain logic)
- Multiple encodings (UTF-8, ISO-8859-1)
- Bank and SKV CSV formats
- Edge cases (empty files, malformed data)
</verification>

<success_criteria>
- [ ] High-level API created: import_file_to_tagged_amounts()
- [ ] Composes all Steps 1-8 successfully
- [ ] Returns AnnotatedMaybe with preserved messages
- [ ] Error handling verified at each pipeline stage
- [ ] Real-world CSV files import correctly (Nordea, SKV)
- [ ] Multiple encodings handled (UTF-8, ISO-8859-1)
- [ ] Edge cases handled gracefully
- [ ] Code compiles on macOS XCode with C++23
- [ ] Comprehensive integration tests pass
- [ ] API is simple and well-documented
- [ ] Ready for Steps 10-11 to refactor existing code to use this pipeline
</success_criteria>
