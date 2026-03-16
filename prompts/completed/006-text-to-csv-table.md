<assistant-role>
You are an expert in writing C++23 code that is easy to read and can be used in functional style composition based on Maybe monads based on std::optional and C++ range views where this makes sense.

  1. You prefer to use C++23 'span' and 'string_view' over C-code constructs.
  2. You prefer to use C++23 library over any specialised helpers
  3. You prefer readable code over efficient code
  4. You prefer to organise into name spaces over splitting code into C++ translation units

You are an expert in character set code points and encodings.

  1. You are aware that C++ string literals are encoded in the encoding of the source code file.
  2. Where reasonable you try to make testing of text encoding pipe lines independent on source code file encoding(s) and stable cross platform (macOS, Linux, Windows).

You are an expert in CSV file formatting

  * You are aware that current code does not (but need to) support quoted text in CSV files.

You know about the code base usage of namespaces and existing types for different functionality.

  * You are aware of namespace 'CSV' for operations on comma separated values text.
  * You are aware that CSV::Table is currently used to represent a parsed CSV text / file.
  * You are aware that CSV::ParseCSVResult and CSV::try_parse_csv are now obsolete but that you may find reusable code and patterns there.

</assistant-role>

<objective>
Implement the CSV parsing layer that transforms runtime encoded text into structured CSV tables using monadic composition. Create: runtime encoded text → Maybe<csv_table>.

This begins the domain pipeline layer, converting text into structured data that can be transformed into business objects.
</objective>

<context>
This is Step 6 of an 11-step refactoring. Previous steps (1-5) provided a complete encoding pipeline: file path → runtime encoded text.

Now we build the domain layer, starting with CSV parsing. The existing try_parse_csv function in parse_csv.cpp needs to be refactored to work with the encoding-aware pipeline and return Maybe/optional results.

Read CLAUDE.md for project conventions.

**Examine existing CSV parsing code:**
@src/csv/parse_csv.cpp (focus on try_parse_csv around line 25 but be aware it is now obsolete)

**Review the encoding pipeline from Step 5** to understand the input format.
</context>

<requirements>
Create a CSV parsing function with these characteristics:

1. **Parsing Function**: `runtime_encoded_text → Maybe<csv_table>`
   - Parse text into structured CSV representation
   - Return optional/Maybe to handle malformed CSV
   - Gracefully handle parsing errors (wrong delimiter, unmatched quotes, etc.)

2. **CSV Table Representation**:
   - Determine appropriate structure: vector<vector<string>>? Custom type?
   - Should headers be separate from data rows?
   - What's the existing representation in the codebase?

3. **Refactor try_parse_csv**:
   - Adapt existing function to work with encoding-aware pipeline
   - Return Maybe/optional results instead of throwing or returning error codes
   - Maintain compatibility with existing functionality where possible

4. **Composability**:
   - Chain with encoding pipeline from Step 5
   - `file_path → Maybe<text> → Maybe<csv_table>`
   - Demonstrate the composed pipeline
</requirements>

<implementation>
**Design Considerations:**
- What does the current try_parse_csv implementation do?
- What CSV parsing library/approach is currently used?
- Should this handle different delimiters (comma, semicolon, tab)?
- How should quoted fields be handled?
- What about escaped quotes, multiline fields?

**CSV Parsing Strategy:**
Options to consider:
1. Refactor existing try_parse_csv to return optional
2. Wrap existing parser in monadic API
3. Implement new parser if existing one is inadequate

Choose based on the current implementation's quality and flexibility.

**Error Handling:**
CSV parsing failures are expected (malformed files). Consider:
- What level of detail is needed? (Just success/failure, or specific error info?)
- Should partially parsed data be returned?
- Log parsing errors for debugging?

**Bank/SKV CSV Format:**
- Do these have consistent formats?
- Are headers always present?
- What delimiters are used?
</implementation>

<output>
Modify these files:
- `./src/csv/parse_csv.cpp` - Refactor try_parse_csv for monadic API
- `./src/csv/parse_csv.hpp` - Update function signatures (if header exists)

**Add tests to:** `./src/test/test_csv_import_pipeline.cpp`
- Use namespace: `namespace tests::csv_import_pipeline::csv_parsing_suite { ... }`
- Test CSV parsing with various formats and error cases

Example tests:
```cpp
namespace tests::csv_import_pipeline {
    namespace csv_parsing_suite {
        TEST(CSVParsingTests, ParseWellFormedCSV) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(CSVParsingTests, ParseWellFormedCSV)"};
            // Test parsing well-formed CSV
        }

        TEST(CSVParsingTests, MalformedCSVReturnsEmpty) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(CSVParsingTests, MalformedCSVReturnsEmpty)"};
            // Test malformed CSV returns empty Maybe
        }

        TEST(CSVParsingTests, ComposesWithEncodingPipeline) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(CSVParsingTests, ComposesWithEncodingPipeline)"};
            // Test: file → text → CSV table
        }
    }
}
```
</output>

<verification>
Before completing:

1. **Compilation**: Code compiles on macOS XCode with C++23 via `./run.zsh`
2. **Tests Pass**: Run `./cratchit --test` - all tests succeed
3. **Parsing Works**: Successfully parses sample CSV files from banks/SKV
4. **Error Handling**: Malformed CSV returns empty Maybe, doesn't throw
5. **Integration**: Composes with encoding pipeline from Step 5
6. **Backward Compatibility**: Existing code using try_parse_csv still works (if applicable)

Test coverage:
- Well-formed CSV files
- Malformed CSV (missing quotes, wrong delimiter)
- Empty files
- Files with only headers
- Composition with encoding pipeline
</verification>

<success_criteria>
- [ ] CSV parsing function implemented with Maybe return type
- [ ] try_parse_csv refactored to work with encoding-aware pipeline
- [ ] Returns Maybe<csv_table> for monadic composition
- [ ] Handles malformed CSV gracefully (returns empty Maybe)
- [ ] Composes with encoding pipeline from Step 5
- [ ] Code compiles on macOS XCode
- [ ] Tested with sample CSV files from banks/SKV
- [ ] Follows existing codebase patterns
- [ ] Clear CSV table representation chosen and documented
</success_criteria>
