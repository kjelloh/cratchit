<assistant-role>
You are an expert in writing C++23 code that is easy to read and can be used in functional style composition based on Maybe monads based on std::optional and C++ range views where this makes sense.

  1. You prefer to use C++23 'span' and 'string_view' over C-code constructs.
  1. You prefer to use C++23 library over any specialised helpers
  2. You prefer readable code over efficient code
  3. You prefer to organise into name spaces over splitting code into C++ translation units
  4. You apply the notion of 'decode' as decoding from a source text encoding to ICU Unicode and of 'encode' as encoding text from Unicode to a target encoding.
  5. You avoid to use 'transcode' as the generic term for the more specific 'decode' and encode where this makes sense.

You know about the code base usage of namespaces for different functionality.

  * You know about the namespace 'charset' with sub-namespaces for different character set aware functionality.
  * You are aware that UTF-8 transcoding is not yet in charset::UTF_8 but instead in text::encoding::UTF8 based on multi-byte ToUnicodeBuffer type.
  * You are aware of, but reuse only any code-point-transcoding, the existing types 'encoding::xxx::istream (xxx being supported character set).
  * You know about namespace text::encoding::icu for interfacting with the ICU library
</assistant-role>

<objective>
Create an integration checkpoint for the complete encoding pipeline. Compose Steps 1-4 into a cohesive, tested pipeline: file path → runtime encoded text.

This integration ensures all encoding layers work together correctly before building the CSV parsing layer on top.
</objective>

<context>
This is Step 5 of an 11-step refactoring - an integration checkpoint. Previous steps provided:
- Step 1: File I/O with Maybe monadic composition
- Step 2: Encoding detection
- Step 3: Lazy view bytes → Unicode code points
- Step 4: Lazy view Unicode → runtime encoding

This step composes all previous work into a single, well-tested pipeline that future steps can build upon.

Read CLAUDE.md for project conventions.

**Review implementations from Steps 1-4** to understand the complete pipeline.
</context>

<requirements>
Create an integrated encoding pipeline with these characteristics:

1. **High-Level API**:
   - Simple function: `file_path → Maybe<runtime_encoded_text>`
   - Hides the complexity of encoding detection and transcoding
   - Monadic composition throughout

2. **Complete Pipeline**:
   ```
   file_path
     → Maybe<byte_buffer>           (Step 1)
     → Maybe<DetectedEncoding>      (Step 2)
     → unicode_code_points_view     (Step 3)
     → runtime_encoded_view         (Step 4)
     → Maybe<string/text>
   ```

3. **Error Propagation**:
   - File I/O failures propagate through Maybe
   - Encoding detection failures (handle: default to UTF-8? fail?)
   - Transcoding errors handled gracefully

4. **Comprehensive Testing**:
   - Test files in different encodings (UTF-8, ISO-8859-1, Windows-1252)
   - Test with actual CSV files from banks/SKV if available
   - Test error cases (file not found, invalid encoding, etc.)
</requirements>

<implementation>
**High-Level API Design:**
Create a clean API that encapsulates the complexity:
```cpp
// Option 1: Simple function
auto maybe_text = read_file_with_encoding_detection(path);

// Option 2: Fluent builder
auto maybe_text = EncodingPipeline::from_file(path)
    .detect_encoding()
    .transcode_to_runtime()
    .execute();

// Choose based on codebase style
```

**Error Handling Strategy:**
Decide how to handle encoding detection failures:
- Return empty Maybe (fail safe)?
- Default to UTF-8 and proceed?
- Try UTF-8 first, then detect if that fails?

Document the chosen strategy and rationale.

**Testing Strategy:**
Create comprehensive tests covering:
- Happy path: UTF-8 file reads correctly
- Various encodings: ISO-8859-1, Windows-1252, etc.
- Error cases: file not found, binary file, corrupted encoding
- Large files: verify lazy evaluation doesn't cause memory issues
</implementation>

<output>
Create or modify files:
- `./src/text/encoding_pipeline.hpp` - High-level API (or add to existing file)
- `./src/text/encoding_pipeline.cpp` - Implementation composing Steps 1-4

**Add tests to:** `./src/test/test_csv_import_pipeline.cpp`
- Use namespace: `namespace tests::csv_import_pipeline::encoding_pipeline_integration_suite { ... }`
- Comprehensive integration tests for the complete encoding pipeline

Example tests:
```cpp
namespace tests::csv_import_pipeline {
    namespace encoding_pipeline_integration_suite {
        TEST(EncodingPipelineTests, UTF8FileToRuntimeText) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, UTF8FileToRuntimeText)"};
            // Test complete pipeline with UTF-8 file
        }

        TEST(EncodingPipelineTests, FileNotFoundHandledGracefully) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, FileNotFoundHandledGracefully)"};
            // Test error handling for missing file
        }

        TEST(EncodingPipelineTests, LazyEvaluationWithLargeFile) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingPipelineTests, LazyEvaluationWithLargeFile)"};
            // Verify lazy evaluation with large file
        }
    }
}
```

Include clear documentation of:
- How to use the pipeline
- Error handling behavior
- Performance characteristics (lazy evaluation)
</output>

<verification>
Before completing:

1. **Compilation**: All code compiles on macOS XCode with C++23 via `./run.zsh`
2. **Tests Pass**: Run `./cratchit --test` - all tests succeed
3. **Multiple Encodings**: Successfully reads UTF-8, ISO-8859-1, Windows-1252 files
4. **Error Handling**: File-not-found and invalid-encoding cases handled gracefully
5. **Memory Efficiency**: Large file doesn't cause excessive memory usage (lazy evaluation works)
6. **Monadic Composition**: Error propagation works correctly through Maybe chain

Test coverage:
- UTF-8, ISO-8859-1, Windows-1252 files
- File-not-found error handling
- Large file memory efficiency
- Complete pipeline integration
</verification>

<success_criteria>
- [ ] High-level API created for file path → runtime encoded text
- [ ] Composes Steps 1-4 successfully
- [ ] Monadic error propagation works throughout pipeline
- [ ] Comprehensive tests cover happy path and error cases
- [ ] Multiple encodings tested and working
- [ ] Code compiles on macOS XCode
- [ ] Tests pass
- [ ] Lazy evaluation verified (no excessive memory usage)
- [ ] Clear documentation of usage and behavior
- [ ] Ready for CSV parsing layer (Steps 6-8) to build upon
</success_criteria>
