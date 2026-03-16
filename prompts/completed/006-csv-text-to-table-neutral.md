<assistant-role>
You are an expert in writing C++23 code that is easy to read and can be used in functional style composition based on Maybe monads (std::optional) and C++ range views where this makes sense.

1. You prefer to use C++23 'span' and 'string_view' over C-code constructs.
2. You prefer to use C++23 standard library over any specialized helpers.
3. You prefer readable code over efficient code.
4. You prefer to organize into namespaces over splitting code into C++ translation units.

You are an expert in character set code points and encodings.

1. You are aware that C++ string literals are encoded in the encoding of the source code file.
2. Where reasonable you try to make testing of text encoding pipelines independent of source code file encoding(s) and stable cross-platform (macOS, Linux, Windows).

You are an expert in CSV file formatting.

1. You are aware that the new code must support quoted text in CSV files with proper escape handling.
2. You are aware that different CSV dialects use different delimiters (comma, semicolon).
3. You understand that CSV files may have quoted fields with embedded delimiters and newlines.

You know about the codebase usage of namespaces and existing types for different functionality.

1. You are aware of namespace 'CSV' for operations on comma separated values text.
2. You are aware that CSV::Table is currently used to represent a parsed CSV text/file.
3. You are aware that CSV::ParseCSVResult and CSV::try_parse_csv exist but MUST BE IGNORED for this implementation - they will be refactored to the new pipeline later.
4. You must create a NEW, NEUTRAL implementation that plugs into the encoding pipeline.

</assistant-role>

<objective>
Implement a neutral CSV parsing layer that transforms runtime encoded text (UTF-8 std::string) into structured CSV::Table using monadic composition.

Create: `runtime_encoded_text → Maybe<CSV::Table>`

This implementation must:
- Be completely independent of the existing CSV::try_parse_csv (ignore it)
- Create a new, clean, neutral CSV parser suitable for the encoding pipeline
- Work with the encoding pipeline from steps 1-5: `file_path → runtime_encoded_text`
- Return Maybe/optional for composability and graceful error handling
- Support quoted fields, multiple delimiters, and proper CSV parsing
</objective>

<context>
This is Step 6 in the CSV import refactoring pipeline. The previous steps (1-5) provide a complete encoding pipeline that produces UTF-8 runtime text from files with various encodings.

**Previous Pipeline**:
- Step 1: File I/O with Maybe monad (`file_path → Maybe<ByteBuffer>`)
- Step 2: Encoding detection (`ByteBuffer → Maybe<DetectedEncoding>`)
- Step 3: Bytes to Unicode view (lazy transcoding)
- Step 4: Unicode to runtime encoding view (UTF-8 output)
- Step 5: Integration (`file_path → Maybe<std::string>` via `read_file_with_encoding_detection`)

**This Step**: Add CSV parsing to complete the chain:
`file_path → Maybe<std::string> → Maybe<CSV::Table>`

Read CLAUDE.md for project conventions.

**Key Implementation Note**:
You MUST ignore the existing `CSV::try_parse_csv` function. We are building a NEW implementation that will eventually replace it. Do NOT modify or call the old implementation.

**Test Data Available**:
Examine `@src/test/data/account_statements_csv.hpp` for real CSV examples including:
- `sz_NORDEA_csv_20251120` - Nordea bank CSV with semicolon delimiter
- `sz_SKV_csv_older` - Older SKV format
- `sz_SKV_csv_20251120` - Newer SKV format with quotes

**Testing Location**:
Add tests to: `@src/test/test_csv_import_pipeline.cpp`
</context>

<requirements>
Create a NEW CSV parsing implementation with these characteristics:

1. **Parsing Function Signature**:
   - Function: `runtime_encoded_text → Maybe<CSV::Table>`
   - Input: `std::string` (UTF-8 encoded text from encoding pipeline)
   - Output: `std::optional<CSV::Table>` or similar Maybe monad
   - Namespace: Place in `CSV` namespace for consistency

2. **CSV Table Representation**:
   - Investigate what `CSV::Table` currently is in the codebase
   - If suitable, use it; if not, define a new clean representation
   - Consider: headers separate from data? vector<vector<string>>?
   - Ensure it's suitable for further pipeline stages (CSV → domain objects)

3. **CSV Parsing Features**:
   - Support semicolon (`;`) and comma (`,`) delimiters
   - Handle quoted fields: `"text with, comma"`
   - Handle escaped quotes within quoted fields: `"text with ""quote"""`
   - Handle quoted fields with embedded newlines
   - Detect delimiter automatically or accept as parameter
   - Return empty optional on parse failure (malformed CSV)

4. **Composability with Encoding Pipeline**:
   - Must chain seamlessly with `read_file_with_encoding_detection`
   - Demonstrate: `file_path → Maybe<text> → Maybe<CSV::Table>`
   - Use monadic composition (`.and_then()` or similar)

5. **Error Handling**:
   - Return empty optional for malformed CSV (don't throw)
   - Consider logging parse errors for debugging
   - Handle edge cases: empty files, only headers, malformed quotes

6. **Code Organization**:
   - Create appropriate header/implementation files in `src/csv/` if needed
   - Use namespace `CSV` for consistency with existing code
   - Keep the implementation separate from old `try_parse_csv`
</requirements>

<implementation>
**Design Approach**:

1. **Investigate Existing CSV::Table**:
   - Search for CSV::Table definition in the codebase
   - Understand its structure and suitability
   - If inadequate, propose and implement a better structure

2. **Parser Design**:
   - Create a NEW parsing function (e.g., `parse_csv_text`, `text_to_table`)
   - Do NOT call or modify `CSV::try_parse_csv`
   - Implement proper CSV RFC 4180 parsing with:
     * Field parsing with quote handling
     * Delimiter detection or explicit specification
     * Line ending handling (CRLF, LF)
     * Proper state machine for quoted vs unquoted fields

3. **API Design Options**:
   ```cpp
   namespace CSV {
     // Option A: Simple with default delimiter
     std::optional<Table> parse_text_to_table(std::string_view text);

     // Option B: Explicit delimiter
     std::optional<Table> parse_text_to_table(std::string_view text, char delimiter);

     // Option C: Auto-detect delimiter
     std::optional<Table> parse_text_to_table_auto(std::string_view text);
   }
   ```
   Choose based on the CSV examples in `account_statements_csv.hpp`.

4. **Integration Pattern**:
   ```cpp
   // Demonstrate composition
   auto csv_result = text::encoding::read_file_with_encoding_detection(file_path)
     .and_then([](const std::string& text) {
       return CSV::parse_text_to_table(text);
     });
   ```

5. **Incremental Development**:
   - Implement basic parsing first (unquoted fields, single delimiter)
   - Add tests, ensure compilation and passing
   - Add quoted field support
   - Add tests, ensure compilation and passing
   - Add delimiter detection
   - Add tests, ensure compilation and passing
   - Test with real CSV data from `account_statements_csv.hpp`
</implementation>

<output>
Create or modify these files:

- `./src/csv/csv_parser_neutral.hpp` - Header for new CSV parsing functions
- `./src/csv/csv_parser_neutral.cpp` - Implementation of CSV parsing
  (Or choose appropriate filenames that make it clear this is the NEW implementation)

**Add comprehensive tests to:** `./src/test/test_csv_import_pipeline.cpp`

Use namespace: `namespace tests::csv_import_pipeline::csv_parsing_suite { ... }`

Example test structure:
```cpp
namespace tests::csv_import_pipeline::csv_parsing_suite {

  TEST(CSVParsingTests, ParseSimpleCommaDelimited) {
    logger::scope_logger log_raii{logger::development_trace, "TEST(CSVParsingTests, ParseSimpleCommaDelimited)"};

    std::string csv_text = "Name,Amount\nJohn,100\nJane,200\n";
    auto result = CSV::parse_text_to_table(csv_text);

    ASSERT_TRUE(result.has_value()) << "Expected successful parse";
    // Verify table structure
  }

  TEST(CSVParsingTests, ParseSemicolonDelimited) {
    logger::scope_logger log_raii{logger::development_trace, "TEST(CSVParsingTests, ParseSemicolonDelimited)"};

    std::string csv_text = "Name;Amount\nJohn;100\nJane;200\n";
    auto result = CSV::parse_text_to_table(csv_text, ';');

    ASSERT_TRUE(result.has_value()) << "Expected successful parse";
  }

  TEST(CSVParsingTests, ParseQuotedFields) {
    logger::scope_logger log_raii{logger::development_trace, "TEST(CSVParsingTests, ParseQuotedFields)"};

    std::string csv_text = R"(Name,Description
"John Smith","Works at ""Acme"" Corp"
Jane,Simple)";
    auto result = CSV::parse_text_to_table(csv_text);

    ASSERT_TRUE(result.has_value()) << "Expected successful parse with quoted fields";
  }

  TEST(CSVParsingTests, ParseRealNordeaCSV) {
    logger::scope_logger log_raii{logger::development_trace, "TEST(CSVParsingTests, ParseRealNordeaCSV)"};

    std::string csv_text = sz_NORDEA_csv_20251120;
    auto result = CSV::parse_text_to_table(csv_text, ';');

    ASSERT_TRUE(result.has_value()) << "Expected successful parse of Nordea CSV";
    // Verify expected number of rows, columns, etc.
  }

  TEST(CSVParsingTests, ParseRealSKVCSV) {
    logger::scope_logger log_raii{logger::development_trace, "TEST(CSVParsingTests, ParseRealSKVCSV)"};

    std::string csv_text = sz_SKV_csv_20251120;
    auto result = CSV::parse_text_to_table(csv_text, ';');

    ASSERT_TRUE(result.has_value()) << "Expected successful parse of SKV CSV";
  }

  TEST(CSVParsingTests, MalformedCSVReturnsEmpty) {
    logger::scope_logger log_raii{logger::development_trace, "TEST(CSVParsingTests, MalformedCSVReturnsEmpty)"};

    std::string malformed = R"(Name,Amount
"Unclosed quote,100)";
    auto result = CSV::parse_text_to_table(malformed);

    EXPECT_FALSE(result.has_value()) << "Expected empty optional for malformed CSV";
  }

  TEST(CSVParsingTests, ComposesWithEncodingPipeline) {
    logger::scope_logger log_raii{logger::development_trace, "TEST(CSVParsingTests, ComposesWithEncodingPipeline)"};

    // Create temp CSV file
    auto temp_path = std::filesystem::temp_directory_path() / "test_compose.csv";
    {
      std::ofstream ofs(temp_path);
      ofs << "Name,Amount\nJohn,100\n";
    }

    // Complete pipeline: file → text → CSV table
    auto result = text::encoding::read_file_with_encoding_detection(temp_path)
      .and_then([](const std::string& text) {
        return CSV::parse_text_to_table(text);
      });

    ASSERT_TRUE(result.has_value()) << "Expected successful file → CSV pipeline";

    std::filesystem::remove(temp_path);
  }

  TEST(CSVParsingTests, EmptyFileReturnsEmptyTable) {
    logger::scope_logger log_raii{logger::development_trace, "TEST(CSVParsingTests, EmptyFileReturnsEmptyTable)"};

    std::string empty = "";
    auto result = CSV::parse_text_to_table(empty);

    // Decide: should empty input return empty table or nullopt?
    // Document the decision
  }

} // namespace csv_parsing_suite
} // namespace tests::csv_import_pipeline
```

**Update CMakeLists.txt if needed** to include new source files.
</output>

<verification>
Before completing, verify each step:

1. **Compilation**:
   - Run `./run.zsh` after each feature addition
   - Code must compile with C++23 on macOS XCode
   - Fix any compilation errors immediately

2. **Testing**:
   - Run tests from the `workspace/` directory: `cd workspace && ./cratchit --test`
   - If running produces no output, apply code signing fix: `codesign -s - --force --deep workspace/cratchit`
   - All new tests must pass
   - Existing tests must continue to pass

3. **Real CSV Data**:
   - Test with all three CSV examples from `account_statements_csv.hpp`
   - Verify Nordea CSV (semicolon delimiter) parses correctly
   - Verify both SKV CSVs parse correctly
   - Check row/column counts match expectations

4. **Pipeline Integration**:
   - Verify `file → text → CSV table` composition works
   - Ensure error handling propagates correctly
   - Empty optionals at any stage should short-circuit

5. **Error Handling**:
   - Malformed CSV returns empty optional (doesn't crash or throw)
   - Error cases are tested and handled gracefully
   - Consider adding logging for parse errors (optional)

6. **Incremental Progress**:
   - After implementing each feature, compile and test
   - Don't move to the next feature until current tests pass
   - This ensures the codebase remains in a working state

**Code Signing Note**: On macOS, if `./cratchit --test` produces no output and exits silently, this indicates a code signing failure. Run:
```bash
codesign -s - --force --deep workspace/cratchit
```
Then retry running the tests.
</verification>

<success_criteria>
- [ ] New CSV parsing function implemented (ignores old `try_parse_csv`)
- [ ] Function signature: `std::string → std::optional<CSV::Table>`
- [ ] Supports semicolon and comma delimiters
- [ ] Handles quoted fields with proper escape handling
- [ ] Parses all three real CSV examples from `account_statements_csv.hpp`
- [ ] Returns empty optional for malformed CSV (graceful error handling)
- [ ] Composes with encoding pipeline: `file → text → CSV table`
- [ ] Comprehensive tests in `test_csv_import_pipeline.cpp`
- [ ] All tests pass when run from `workspace/` directory
- [ ] Code compiles with `./run.zsh` on macOS XCode C++23
- [ ] Code signing applied if needed for test execution
- [ ] Each feature tested incrementally before moving to next
- [ ] Clear separation from old `try_parse_csv` implementation
- [ ] Follows existing codebase patterns (namespaces, Maybe monads, views)
</success_criteria>
