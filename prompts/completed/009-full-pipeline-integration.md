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

This integration step creates a simple, documented interface that encapsulates all transformation steps, making it easy for consumers to import CSV files and get tagged amounts for bookkeeping.
</objective>

<context>
This is Step 9 of an 11-step refactoring (prompts 001..011).

**Complete Pipeline - Actual Implementation:**

```
file_path
  → AnnotatedMaybe<ByteBuffer>     (Step 1 - cratchit::io::read_file_to_buffer)
  → DetectedEncoding               (Step 2 - text::encoding::icu::detect_buffer_encoding)
  → unicode_code_points_view       (Step 3 - text::encoding::views::bytes_to_unicode)
  → runtime_encoded_view           (Step 4 - text::encoding::views::unicode_to_runtime_encoding)
  → AnnotatedMaybe<std::string>    (Step 5 - text::encoding::read_file_with_encoding_detection)
                                   [Integrates Steps 1-4 into single function]
  → std::optional<CSV::Table>      (Step 6 - CSV::neutral::text_to_table)
  → std::optional<AccountID>       (Step 6.5 - CSV::project::to_account_id)
  → std::optional<AccountStatement>(Step 7 - domain::csv_table_to_account_statement)
  → std::optional<TaggedAmounts>   (Step 8 - domain::account_statement_to_tagged_amounts)
```

**Actual File Locations and Functions:**

| Step | File | Function | Returns |
|------|------|----------|---------|
| 1 | `src/io/file_reader.hpp` | `cratchit::io::read_file_to_buffer(path)` | `AnnotatedMaybe<ByteBuffer>` |
| 2 | `src/text/encoding.hpp` | `text::encoding::icu::detect_buffer_encoding(buffer)` | `std::optional<EncodingDetectionResult>` |
| 3 | `src/text/transcoding_views.hpp` | `text::encoding::views::bytes_to_unicode(buffer, encoding)` | Lazy view |
| 4 | `src/text/transcoding_views.hpp` | `text::encoding::views::unicode_to_runtime_encoding(view)` | Lazy view |
| 5 | `src/text/encoding_pipeline.hpp` | `text::encoding::read_file_with_encoding_detection(path)` | `AnnotatedMaybe<std::string>` |
| 6 | `src/csv/neutral_parser.hpp` | `CSV::neutral::text_to_table(text)` | `std::optional<CSV::Table>` |
| 6.5 | `src/csv/csv_to_account_id.hpp` | `CSV::project::to_account_id(table)` | `std::optional<AccountID>` |
| 7 | `src/domain/csv_to_account_statement.hpp` | `domain::csv_table_to_account_statement(table, account_id)` | `std::optional<AccountStatement>` |
| 8 | `src/domain/account_statement_to_tagged_amounts.hpp` | `domain::account_statement_to_tagged_amounts(statement)` | `std::optional<TaggedAmounts>` |

**Convenience Composition (Steps 7+8):**
- `domain::csv_table_to_tagged_amounts(table, account_id)` → `std::optional<TaggedAmounts>`
- Location: `src/domain/account_statement_to_tagged_amounts.hpp`

**Key Includes for the Pipeline:**
```cpp
#include "io/file_reader.hpp"                              // Step 1
#include "text/encoding_pipeline.hpp"                      // Steps 1-5 integrated
#include "csv/neutral_parser.hpp"                          // Step 6
#include "csv/csv_to_account_id.hpp"                       // Step 6.5
#include "domain/csv_to_account_statement.hpp"             // Step 7
#include "domain/account_statement_to_tagged_amounts.hpp"  // Step 8
```

**Existing Test File:**
- `src/test/test_csv_import_pipeline.cpp` - Contains tests for Steps 1-7
- `src/test/test_statement_to_tagged_amounts.cpp` - Contains tests for Step 8

Read CLAUDE.md for project conventions.
</context>

<requirements>
Create a unified high-level API with these characteristics:

1. **Simple Top-Level Function**:
   ```cpp
   namespace cratchit::csv {
       auto import_file_to_tagged_amounts(
           std::filesystem::path const& file_path
       ) -> AnnotatedMaybe<TaggedAmounts>;
   }
   ```

2. **Monadic Composition Chain**:
   The pipeline should compose:
   ```cpp
   // Pseudocode showing the composition
   text::encoding::read_file_with_encoding_detection(file_path)  // Steps 1-5
     .and_then([](std::string& text) {
       return CSV::neutral::text_to_table(text);                 // Step 6
     })
     .and_then([](CSV::Table& table) {
       auto maybe_account_id = CSV::project::to_account_id(table); // Step 6.5
       return domain::csv_table_to_tagged_amounts(table, account_id); // Steps 7+8
     });
   ```

3. **Error Handling**:
   - File not found → clear error message
   - Encoding detection failure → defaults to UTF-8 (permissive strategy from Step 5)
   - CSV parsing failure → error message with details
   - AccountID detection failure → use empty AccountID (graceful fallback)
   - Invalid business data → error message with specifics
   - All errors/messages propagate through AnnotatedMaybe

4. **Message Aggregation**:
   - Preserve messages from `read_file_with_encoding_detection` (Steps 1-5)
   - Add messages for CSV parsing success/failure
   - Add messages for AccountID detection
   - Add messages for final transformation

5. **Integration Tests**:
   - Test complete pipeline with real CSV test data
   - Test error handling at each stage
   - Test with both NORDEA and SKV CSV formats
   - Test with various encodings (UTF-8, ISO-8859-1)
</requirements>

<implementation>
**API Design:**
```cpp
// File: src/csv/import_pipeline.hpp

#pragma once

#include "io/file_reader.hpp"
#include "text/encoding_pipeline.hpp"
#include "csv/neutral_parser.hpp"
#include "csv/csv_to_account_id.hpp"
#include "domain/csv_to_account_statement.hpp"
#include "domain/account_statement_to_tagged_amounts.hpp"
#include "functional/maybe.hpp"
#include "logger/log.hpp"
#include <filesystem>

namespace cratchit::csv {

/**
 * Import CSV file to TaggedAmounts - Complete Pipeline
 *
 * This function composes the entire CSV import pipeline:
 *   1. File I/O: Read file to byte buffer
 *   2. Encoding detection: Detect source encoding using ICU
 *   3. Transcoding: Bytes → Unicode → Platform encoding
 *   4. CSV parsing: Text → CSV::Table
 *   5. AccountID detection: Identify bank/SKV format and extract account ID
 *   6. Domain extraction: CSV::Table → AccountStatement
 *   7. Final transformation: AccountStatement → TaggedAmounts
 *
 * @param file_path Path to the CSV file to import
 * @return AnnotatedMaybe<TaggedAmounts> with result or error messages
 */
inline AnnotatedMaybe<TaggedAmounts> import_file_to_tagged_amounts(
    std::filesystem::path const& file_path) {
  logger::scope_logger log_raii{logger::development_trace,
    "cratchit::csv::import_file_to_tagged_amounts(file_path)"};

  AnnotatedMaybe<TaggedAmounts> result{};

  // Steps 1-5: File → Text (with encoding detection)
  auto text_result = text::encoding::read_file_with_encoding_detection(file_path);

  if (!text_result) {
    // Propagate file/encoding errors
    result.m_messages = std::move(text_result.m_messages);
    return result;
  }

  // Copy messages from file reading/encoding
  result.m_messages = text_result.m_messages;
  auto const& text = text_result.value();

  // Step 6: Text → CSV::Table
  auto maybe_table = CSV::neutral::text_to_table(text);

  if (!maybe_table) {
    result.push_message("CSV parsing failed: Could not parse text as CSV");
    return result;
  }

  result.push_message(std::format("CSV parsed successfully: {} rows",
    maybe_table->rows.size()));

  // Step 6.5: CSV::Table → AccountID
  auto maybe_account_id = CSV::project::to_account_id(*maybe_table);

  AccountID account_id;
  if (maybe_account_id) {
    account_id = *maybe_account_id;
    result.push_message(std::format("AccountID detected: {}",
      account_id.to_string()));
  } else {
    account_id = AccountID{"", ""};
    result.push_message("AccountID detection failed, using empty AccountID");
  }

  // Steps 7+8: CSV::Table + AccountID → TaggedAmounts
  auto maybe_tagged = domain::csv_table_to_tagged_amounts(*maybe_table, account_id);

  if (!maybe_tagged) {
    result.push_message("Domain transformation failed: Could not extract tagged amounts");
    return result;
  }

  result.m_value = std::move(*maybe_tagged);
  result.push_message(std::format("Pipeline complete: {} tagged amounts created",
    result.value().size()));

  return result;
}

} // namespace cratchit::csv
```

**File Organization:**
- Create `./src/csv/import_pipeline.hpp` - Public API (header-only)
</implementation>

<output>
**Create file**: `./src/csv/import_pipeline.hpp`
- High-level API header with `cratchit::csv::import_file_to_tagged_amounts()`

**Add comprehensive tests to**: `./src/test/test_csv_import_pipeline.cpp`
- Use namespace: `namespace tests::csv_import_pipeline::full_pipeline_suite { ... }`

**Example tests:**
```cpp
namespace tests::csv_import_pipeline {
  namespace full_pipeline_suite {

    TEST(FullPipelineTests, ImportNordeaCSVSuccess) {
      logger::scope_logger log_raii{logger::development_trace,
        "TEST(FullPipelineTests, ImportNordeaCSVSuccess)"};

      // Create temp file with test::data::sz_NORDEA_csv_20251120
      auto temp_path = std::filesystem::temp_directory_path() / "nordea_test.csv";
      {
        std::ofstream ofs(temp_path);
        ofs << test::data::sz_NORDEA_csv_20251120;
      }

      auto result = cratchit::csv::import_file_to_tagged_amounts(temp_path);

      ASSERT_TRUE(result.has_value());
      EXPECT_GT(result->size(), 0);

      // Verify all tagged amounts have required tags
      for (auto const& ta : *result) {
        EXPECT_TRUE(ta.tag_value("Text").has_value());
        EXPECT_TRUE(ta.tag_value("Account").has_value());
      }

      std::filesystem::remove(temp_path);
    }

    TEST(FullPipelineTests, ImportSKVCSVSuccess) {
      logger::scope_logger log_raii{logger::development_trace,
        "TEST(FullPipelineTests, ImportSKVCSVSuccess)"};

      // Create temp file with test::data::sz_SKV_csv_20251120
      auto temp_path = std::filesystem::temp_directory_path() / "skv_test.csv";
      {
        std::ofstream ofs(temp_path);
        ofs << test::data::sz_SKV_csv_20251120;
      }

      auto result = cratchit::csv::import_file_to_tagged_amounts(temp_path);

      ASSERT_TRUE(result.has_value());
      EXPECT_GT(result->size(), 0);

      std::filesystem::remove(temp_path);
    }

    TEST(FullPipelineTests, FileNotFoundError) {
      logger::scope_logger log_raii{logger::development_trace,
        "TEST(FullPipelineTests, FileNotFoundError)"};

      auto result = cratchit::csv::import_file_to_tagged_amounts("/nonexistent/path.csv");

      EXPECT_FALSE(result.has_value());
      EXPECT_GT(result.m_messages.size(), 0);

      // Verify error message mentions file not found
      bool found_error = false;
      for (auto const& msg : result.m_messages) {
        if (msg.find("not exist") != std::string::npos ||
            msg.find("not found") != std::string::npos) {
          found_error = true;
          break;
        }
      }
      EXPECT_TRUE(found_error) << "Expected 'not found' error message";
    }

    TEST(FullPipelineTests, MessagesPreserved) {
      logger::scope_logger log_raii{logger::development_trace,
        "TEST(FullPipelineTests, MessagesPreserved)"};

      // Create temp file with valid CSV
      auto temp_path = std::filesystem::temp_directory_path() / "messages_test.csv";
      {
        std::ofstream ofs(temp_path);
        ofs << test::data::sz_NORDEA_csv_20251120;
      }

      auto result = cratchit::csv::import_file_to_tagged_amounts(temp_path);

      ASSERT_TRUE(result.has_value());

      // Verify messages from various pipeline stages are present
      EXPECT_GT(result.m_messages.size(), 2)
        << "Expected messages from multiple pipeline stages";

      std::filesystem::remove(temp_path);
    }

    TEST(FullPipelineTests, EmptyFileHandledGracefully) {
      logger::scope_logger log_raii{logger::development_trace,
        "TEST(FullPipelineTests, EmptyFileHandledGracefully)"};

      auto temp_path = std::filesystem::temp_directory_path() / "empty_test.csv";
      {
        std::ofstream ofs(temp_path);
        // Empty file
      }

      auto result = cratchit::csv::import_file_to_tagged_amounts(temp_path);

      // Empty file should result in failure (no CSV content)
      EXPECT_FALSE(result.has_value()) << "Empty file should fail CSV parsing";

      std::filesystem::remove(temp_path);
    }

  } // namespace full_pipeline_suite
} // namespace tests::csv_import_pipeline
```
</output>

<verification>
Before completing:

1. **Compilation**: Code compiles on macOS XCode with C++23 via `./run.zsh --nop`
2. **Code Signing**: Execute `codesign -s - --force --deep workspace/cratchit` if needed
3. **Tests Pass**: Run `cd workspace && ./cratchit --test` - all tests succeed
4. **Pipeline Integration**: All steps compose correctly
5. **Error Propagation**: Failures at each stage handled correctly
6. **Message Preservation**: Success/error messages from all steps preserved

Test coverage:
- Complete successful import (file → tagged amounts)
- Error handling at each stage (file I/O, encoding, CSV parsing, domain logic)
- NORDEA and SKV CSV formats
- Edge cases (empty files, missing files)
</verification>

<success_criteria>
- [ ] High-level API created: `cratchit::csv::import_file_to_tagged_amounts()`
- [ ] Composes all Steps 1-8 successfully
- [ ] Returns AnnotatedMaybe with preserved messages
- [ ] Error handling verified at each pipeline stage
- [ ] Real-world CSV files import correctly (NORDEA, SKV)
- [ ] Edge cases handled gracefully
- [ ] Code compiles on macOS XCode with C++23
- [ ] Comprehensive integration tests pass
- [ ] API is simple and well-documented
- [ ] Ready for Steps 10-11 to refactor existing code to use this pipeline
</success_criteria>
