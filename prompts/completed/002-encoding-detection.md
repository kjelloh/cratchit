<assistant-role>
You are an expert in writing C++23 code that is easy to read and can be used in functional style composition based on Maybe monads based on std::optional and C++ range views where this makes sense.

  1. You prefer to use C++23 library over any specialised helpers
  2. You prefer readable code over efficient code
  3. You prefer to organise into name spaces over splitting code into C++ translation units

You know about the code base usage of namespaces for different functionality.

  * You know about namespace text for text related functionality
  * You know about namespace persistent::in for input from persistent storage (file system, streams)
  * You know about namespace text::encoding for charachter set encdoding functionality
  * You know about namespave text::encoding::icu for interfacting with the ICU library
</assistant-role>

<objective>
Implement encoding detection for raw byte buffers using the existing ICU library helpers. Create a monadic function: raw bytes → Maybe<DetectedEncoding> that integrates with the file I/O layer from Step 1.

This step enables the pipeline to automatically identify the text encoding of CSV files from different sources (banks, SKV) which may use different encodings.
</objective>

<context>
This is Step 2 of an 11-step refactoring. Step 1 provided file I/O with Maybe monadic composition.

The application already has ICU helper functions in the text::encoding::icu namespace and a DetectedEncoding enum. This step wraps those utilities in a composable, monadic API.

Read CLAUDE.md for project conventions.

**Examine existing encoding utilities:**
@src/text/encoding.hpp
@src/text/encoding.cpp

**Review the file I/O implementation from Step 1** to understand the byte buffer type and Maybe patterns used.
</context>

<requirements>
Create an encoding detection function with these characteristics:

1. **Detection Function**: `byte_buffer → Maybe<DetectedEncoding>`
   - Use existing ICU helpers from text::encoding::icu namespace
   - Return optional/Maybe with detected encoding
   - Handle detection failures gracefully (return empty optional)

2. **Confidence Threshold**: Consider detection confidence
   - Should low-confidence detections return empty optional?
   - What threshold makes sense for the use case?

3. **Composable with Step 1**: Chain with file I/O pipeline
   - `file_path → Maybe<byte_buffer> → Maybe<DetectedEncoding>`
   - Demonstrate the composed pipeline

4. **Integration with Existing Types**:
   - Use the existing DetectedEncoding enum
   - Leverage existing ICU helper functions
   - Follow existing patterns for encoding representation
</requirements>

<implementation>
**Design Considerations:**
- How do the existing text::encoding::icu functions work?
- Do they already return optional types, or do they throw/return error codes?
- Should this be a free function or a method on a class?
- What's the relationship between DetectedEncoding and ICU's encoding names?

**Error Handling:**
- Detection failure is expected (binary files, unknown encodings)
- Use Maybe/optional to represent "couldn't detect" vs "detected successfully"
- Consider whether to log detection failures for debugging

**File Organization:**
Add this function to the appropriate location in src/text/encoding.hpp/cpp based on existing code structure.
</implementation>

<output>
Modify these files:
- `./src/text/encoding.hpp` - Add encoding detection function signature
- `./src/text/encoding.cpp` - Implement detection using ICU helpers

**Add tests to:** `./src/test/test_csv_import_pipeline.cpp`
- Use namespace: `namespace tests::csv_import_pipeline::encoding_detection_suite { ... }`
- Follow patterns from @src/test/test_atomics.cpp
- Test encoding detection with various file encodings

Example tests:
```cpp
namespace tests::csv_import_pipeline {
    namespace encoding_detection_suite {
        TEST(EncodingDetectionTests, DetectUTF8) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingDetectionTests, DetectUTF8)"};
            // Test UTF-8 detection
        }

        TEST(EncodingDetectionTests, ComposesWithFileIO) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(EncodingDetectionTests, ComposesWithFileIO)"};
            // Test full pipeline: file → buffer → encoding
        }
    }
}
```
</output>

<verification>
Before completing:

1. **Compilation**: Code compiles on macOS XCode with C++23 via `./run.zsh`
2. **Tests Pass**: Run `./cratchit --test` to verify all tests pass
3. **Detection Works**: Test with files in different encodings (UTF-8, ISO-8859-1, etc.)
4. **Monadic Composition**: Chains cleanly with Step 1's file I/O
5. **Existing Utilities**: Properly leverages text::encoding::icu helpers
6. **DetectedEncoding**: Uses the existing enum correctly

Test coverage:
- UTF-8 file detection
- ISO-8859-1 file detection
- Windows-1252 file detection
- Composed pipeline with Step 1
</verification>

<success_criteria>
- [ ] Encoding detection function implemented
- [ ] Returns Maybe<DetectedEncoding>
- [ ] Uses existing ICU helpers from text::encoding::icu
- [ ] Composes with Step 1's file I/O pipeline
- [ ] Handles detection failures gracefully (returns empty optional)
- [ ] Code compiles on macOS XCode
- [ ] Tested with multiple encodings
- [ ] Follows existing codebase patterns
</success_criteria>
