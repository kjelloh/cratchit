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
Implement a lazy C++23 range view that transcodes raw bytes to Unicode code points using a detected encoding. Create: (raw bytes + DetectedEncoding) → unicode code points range view.

This step enables lazy, on-demand transcoding without materializing the entire file in memory. It's the first transcoding stage in the encoding pipeline.
</objective>

<context>
This is Step 3 of an 11-step refactoring. Previous steps provided:
- Step 1: File I/O with Maybe monadic composition
- Step 2: Encoding detection returning Maybe<DetectedEncoding>

This step creates a lazy range view that takes raw bytes and a detected encoding, and produces Unicode code points on-demand as the range is iterated.

Read CLAUDE.md for project conventions.

**Examine existing encoding utilities:**
@src/text/encoding.hpp
@src/text/encoding.cpp
@src/text/RuntimeEncoding.hpp

**Look for existing encoding wrapping streams** mentioned in the context.
</context>

<requirements>
Create a lazy range view with these characteristics:

1. **Range View Signature**:
   - Input: raw byte range + DetectedEncoding
   - Output: range of Unicode code points (char32_t or equivalent)
   - Lazy evaluation: transcode on-demand, not eagerly

2. **C++23 Ranges**:
   - Use std::ranges::views for the implementation
   - Test compilation on macOS XCode
   - If specific adaptors aren't available, use workarounds
   - Should compose with standard range algorithms

3. **Encoding Support**:
   - Use the DetectedEncoding enum to select transcoding method
   - Leverage existing encoding wrapping streams if available
   - Handle transcoding errors gracefully (invalid byte sequences)

4. **Composability**:
   - Chain with previous steps: file → buffer → encoding → unicode code points
   - Should be pipeable with | operator if possible
</requirements>

<implementation>
**Design Considerations:**
- How should this range view be implemented? Custom range adaptor? View class?
- What should happen on transcoding errors (invalid byte sequences)?
  - Skip invalid bytes? Replace with U+FFFD? Stop iteration?
- Should this buffer internally for performance, or truly read byte-by-byte?
- Can existing encoding wrapping streams be adapted for range views?

**C++23 Ranges Approach:**
Explore these options:
- Custom range adaptor using std::ranges::view_interface
- Transform view with stateful transcoder
- Combination of standard views (chunk, transform, join)

Choose the most readable approach given XCode's C++23 support.

**Error Handling:**
Invalid byte sequences are expected in malformed files. Consider:
- Returning optional<char32_t> per iteration?
- Filtering out invalid sequences?
- Logging warnings?
</implementation>

<output>
Create or modify files:
- `./src/text/transcoding_views.hpp` - Range view definitions (or add to existing file)
- `./src/text/transcoding_views.cpp` - Implementation if needed

**Add tests to:** `./src/test/test_csv_import_pipeline.cpp`
- Use namespace: `namespace tests::csv_import_pipeline::transcoding_views_suite { ... }`
- Test lazy range view for bytes → unicode code points

Example tests:
```cpp
namespace tests::csv_import_pipeline {
    namespace transcoding_views_suite {
        TEST(BytesToUnicodeTests, TranscodesUTF8) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(BytesToUnicodeTests, TranscodesUTF8)"};
            // Test UTF-8 bytes → unicode code points
        }

        TEST(BytesToUnicodeTests, LazyEvaluation) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(BytesToUnicodeTests, LazyEvaluation)"};
            // Verify lazy evaluation (no eager materialization)
        }
    }
}
```
</output>

<verification>
Before completing:

1. **Compilation**: Code compiles on macOS XCode with C++23 via `./run.zsh`
2. **Tests Pass**: Run `./cratchit --test` to verify all tests pass
3. **Lazy Evaluation**: Verify range is not eagerly materialized (test with large file)
4. **Correctness**: Test transcoding with known inputs/outputs in different encodings
5. **Composability**: Works with standard range algorithms (take, filter, etc.)
6. **Integration**: Chains with Steps 1 and 2

Test coverage:
- UTF-8 transcoding correctness
- ISO-8859-1 transcoding correctness
- Lazy evaluation (no eager materialization)
- Composition with Steps 1-2
</verification>

<success_criteria>
- [ ] Lazy range view implemented for bytes → Unicode code points
- [ ] Uses C++23 ranges (std::ranges::views)
- [ ] Compiles on macOS XCode
- [ ] Lazy evaluation (no eager materialization)
- [ ] Handles multiple encodings via DetectedEncoding
- [ ] Composable with standard range algorithms
- [ ] Integrates with Steps 1 and 2
- [ ] Transcoding errors handled gracefully
- [ ] Readable, maintainable implementation
</success_criteria>
