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
Implement a lazy C++23 range view that encodes Unicode code points to the runtime internal encoding. Create: unicode code points range → runtime encoded bytes/chars range view.

This completes the transcoding pipeline, converting from any input encoding through Unicode to the application's internal encoding representation.
</objective>

<context>
This is Step 4 of an 11-step refactoring. Previous steps provided:
- Step 1: File I/O with Maybe monadic composition
- Step 2: Encoding detection
- Step 3: Lazy view for bytes → Unicode code points

This step creates the second transcoding stage: Unicode code points → runtime internal encoding.

Read CLAUDE.md for project conventions.

**Examine runtime encoding utilities:**
@src/text/RuntimeEncoding.hpp
@src/text/RuntimeEncoding.cpp

**Review Step 3's implementation** to understand the Unicode code point representation.
</context>

<requirements>
Create a lazy range view with these characteristics:

1. **Range View Signature**:
   - Input: range of Unicode code points (char32_t or equivalent)
   - Output: range in runtime internal encoding (likely UTF-8 bytes or chars)
   - Lazy evaluation: encode on-demand

2. **Runtime Encoding Detection**:
   - Check RuntimeEncoding.hpp/cpp for runtime encoding detection
   - Pragmatically default to UTF-8 if detection is complex
   - Document the chosen approach and why

3. **C++23 Ranges**:
   - Use std::ranges::views for implementation
   - Test compilation on macOS XCode
   - Should compose with Step 3's Unicode view

4. **Composability**:
   - Chain with previous steps: file → buffer → encoding → unicode → runtime encoding
   - Pipeline should be readable and maintainable
</requirements>

<implementation>
**Design Considerations:**
- What is the runtime internal encoding? UTF-8, UTF-16, UTF-32?
- Should this be configurable or hardcoded?
- How should encoding errors be handled? (Some Unicode code points may be unrepresentable)
- Should this produce bytes (std::byte/uint8_t) or chars (char/char8_t)?

**Runtime Encoding Strategy:**
Explore options:
1. Use RuntimeEncoding.hpp detection mechanism
2. Default to UTF-8 (most common for modern C++ text)
3. Make it configurable at compile-time or runtime

Choose based on what's most practical given the existing codebase.

**Encoding Process:**
- Single code point may produce multiple bytes (UTF-8)
- Consider using std::views::join or similar for flattening
- Ensure lazy evaluation is maintained

**Readability Priority:**
Remember: readability over performance. Choose the clearest implementation.
</implementation>

<output>
Modify or create files:
- `./src/text/transcoding_views.hpp` - Add runtime encoding view
- `./src/text/transcoding_views.cpp` - Implementation

**Add tests to:** `./src/test/test_csv_import_pipeline.cpp`
- Use namespace: `namespace tests::csv_import_pipeline::runtime_encoding_suite { ... }`
- Test lazy range view for unicode → runtime encoding

Example tests:
```cpp
namespace tests::csv_import_pipeline {
    namespace runtime_encoding_suite {
        TEST(RuntimeEncodingTests, EncodesToRuntime) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(RuntimeEncodingTests, EncodesToRuntime)"};
            // Test unicode → runtime encoding
        }

        TEST(RuntimeEncodingTests, CompleteTranscodingPipeline) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(RuntimeEncodingTests, CompleteTranscodingPipeline)"};
            // Test: raw bytes → unicode → runtime encoding
        }
    }
}
```
</output>

<verification>
Before completing:

1. **Compilation**: Code compiles on macOS XCode with C++23 via `./run.zsh`
2. **Tests Pass**: Run `./cratchit --test` to verify all tests pass
3. **Lazy Evaluation**: Verify no eager materialization
4. **Correctness**: Test round-trip encoding/decoding
5. **Integration**: Chains with Steps 1-3 successfully
6. **Complete Pipeline**: File → raw bytes → Unicode → runtime encoding works end-to-end

Test coverage:
- Unicode to runtime encoding correctness
- Complete transcoding chain (various input encodings → runtime)
- Lazy evaluation maintained
</verification>

<success_criteria>
- [ ] Lazy range view implemented for Unicode → runtime encoding
- [ ] Uses C++23 ranges
- [ ] Compiles on macOS XCode
- [ ] Lazy evaluation maintained
- [ ] Runtime encoding determined (from RuntimeEncoding.hpp or defaulted to UTF-8)
- [ ] Composable with Step 3's Unicode view
- [ ] Full transcoding chain works: any encoding → Unicode → runtime encoding
- [ ] Readable, maintainable implementation
- [ ] Encoding errors handled gracefully
</success_criteria>
