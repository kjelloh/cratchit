<objective>
Refactor the CSV import pipeline to use a composable, monadic architecture with three distinct abstraction layers. This refactoring targets specific code in TaggedAmountFramework.cpp (lines 974-979) and the try_parse_csv function in parse_csv.cpp (line 25).

The goal is to create a readable, maintainable pipeline for importing CSV files from banks and the Swedish tax agency (SKV), where each transformation step uses monadic composition patterns that gracefully handle failures.

This matters because the current implementation likely has tightly coupled encoding detection, parsing, and domain object creation. Separating these concerns into composable layers will improve maintainability, testability, and make it easier to support additional CSV sources in the future.
</objective>

<context>
This is a C++23 bookkeeping application (Cratchit) that processes financial data from CSV files. The application must handle:
- Various text encodings from different sources (banks, SKV)
- CSV parsing with potential malformed data
- Transformation from raw CSV to domain objects (account statements, tagged amounts)

Read CLAUDE.md for project conventions and architecture patterns.

**Examine these files to understand the current implementation and available utilities:**

@src/fiscal/amount/TaggedAmountFramework.cpp (focus on lines 974-979)
@src/csv/parse_csv.cpp (focus on try_parse_csv function around line 25)
@src/text/encoding.hpp
@src/text/encoding.cpp
@src/text/RuntimeEncoding.hpp
@src/text/RuntimeEncoding.cpp

**Also search for existing "Maybe" type implementations** that can be reused for monadic composition.
</context>

<requirements>
Implement a three-layer composable pipeline architecture:

## Layer 1: File I/O with Optional-Based Error Handling
Create a pipeline: `file path → raw byte istream → raw byte buffer`
- Use `std::optional` monadic API (and_then, transform, or_else)
- Reuse existing "Maybe" types where they provide better ergonomics
- Handle file opening, reading failures gracefully through optional chaining

## Layer 2: Encoding Detection and Transcoding via Lazy Range Views
Create a pipeline: `raw bytes → maybe encoding → unicode code points → runtime encoding`

**Step 2a: Encoding Detection**
- Use existing ICU helper functions in `text::encoding::icu` namespace
- Return detection results as `std::optional<DetectedEncoding>` (or existing Maybe type)
- Leverage the existing `DetectedEncoding` enum and encoding wrapping streams

**Step 2b: Lazy Transcoding to Unicode**
- Implement C++23 range views to lazily transcode: `raw byte range → unicode code point range`
- Use the detected encoding from Step 2a
- Views should be composable with standard ranges algorithms

**Step 2c: Lazy Encoding to Runtime Format**
- Implement C++23 range views to lazily encode: `unicode code point range → runtime encoded text`
- Check RuntimeEncoding.hpp/cpp for runtime encoding detection, but UTF-8 can be used as a pragmatic default where needed
- Maintain lazy evaluation - no eager materialization unless necessary

## Layer 3: Domain Object Pipeline with Monadic Composition
Create a pipeline: `runtime encoded text → maybe csv_table → maybe account_statements → maybe tagged_amounts`

- Each transformation step uses monadic composition (and_then, transform)
- Failures at any step short-circuit the pipeline
- Clear separation between CSV parsing, business logic validation, and domain object construction
</requirements>

<implementation>
**Architecture Principles:**
1. **Composability First**: Each layer should be independently testable and composable
2. **Readability Over Performance**: Prioritize clear, understandable code. Optimization comes later.
3. **Monadic Error Handling**: Use std::optional (or existing Maybe types) to propagate failures without exceptions
4. **Lazy Evaluation**: Range views should not eagerly materialize data - process on-demand

**C++23 Ranges Considerations:**
- Use `std::ranges::views` for lazy transformations
- Test compilation with macOS XCode compiler to verify C++23 ranges support
- If specific range adaptors aren't available, note them and use workarounds or simpler alternatives
- Prefer standard library features, but pragmatically adjust if compiler support is incomplete

**Specific Refactoring Targets:**
- **TaggedAmountFramework.cpp lines 974-979**: Integrate this code into the new pipeline architecture
- **parse_csv.cpp try_parse_csv (line 25)**: Refactor to work with the encoding-aware pipeline and return optional results

**What to Avoid (and WHY):**
- **Avoid eager string materialization**: The pipeline should process data lazily because financial CSV files can be large, and materializing everything upfront wastes memory
- **Avoid throwing exceptions for expected failures**: Use optional/Maybe types because encoding detection failures, malformed CSV, and invalid data are expected scenarios, not exceptional ones
- **Avoid hardcoding encodings where possible**: Use the encoding detection mechanism because different banks and SKV may provide files in different encodings
- **Avoid tight coupling between layers**: Keep encoding, parsing, and domain logic separate because this makes the code testable and allows supporting new CSV formats easily

**Consider multiple implementation approaches:**
- Thoroughly analyze the existing code patterns before refactoring
- Consider how the existing "Maybe" types can be integrated with std::optional
- Explore different range view compositions to find the most readable approach
- Deeply consider the error handling strategy - what information needs to be preserved when failures occur?
</implementation>

<output>
Modify these files:
- `./src/fiscal/amount/TaggedAmountFramework.cpp` - Refactor lines 974-979 to use new pipeline
- `./src/csv/parse_csv.cpp` - Refactor try_parse_csv to work with encoding-aware pipeline

Create or modify these files for the new abstractions:
- `./src/text/transcoding_pipeline.hpp` - Header for encoding detection and transcoding views (if a new file is needed)
- `./src/text/transcoding_pipeline.cpp` - Implementation (if needed)
- Or integrate into existing files if that's more appropriate based on current code organization

**Important**: Determine the best file organization after examining the existing codebase structure. The goal is consistency with existing patterns, not necessarily new files.
</output>

<verification>
Before declaring complete, verify your work:

1. **Compilation Test**: Build the project with `./run.zsh` or `cmake --build .`
   - Confirm C++23 ranges features compile on macOS XCode
   - No compiler errors or warnings

2. **Pipeline Composition Test**: Create a simple test demonstrating the full pipeline:
   - file path → encoded CSV → parsed table → domain objects
   - Show how failures at each stage are handled monadically

3. **Code Readability Review**:
   - Can another developer understand the pipeline flow?
   - Are the three layers clearly separated?
   - Is the monadic composition evident?

4. **Integration Check**:
   - Does the refactored code integrate with existing SKV/bank CSV import workflows?
   - Are existing "Maybe" types used appropriately?
   - Is the DetectedEncoding enum leveraged correctly?

If any verification step fails, address the issues before completing.
</verification>

<success_criteria>
- [ ] TaggedAmountFramework.cpp lines 974-979 successfully refactored into pipeline architecture
- [ ] try_parse_csv function refactored to work with encoding-aware pipeline
- [ ] Three-layer architecture clearly implemented and separated
- [ ] C++23 range views used for lazy transcoding (with XCode compatibility)
- [ ] std::optional (and existing Maybe types) used for monadic composition
- [ ] Code compiles without errors on macOS XCode
- [ ] Pipeline is demonstrably composable and testable
- [ ] Code is readable and maintainable (prioritized over performance)
- [ ] Existing encoding detection utilities (DetectedEncoding, text::encoding::icu) are leveraged
- [ ] Error handling is graceful and propagates through the pipeline
</success_criteria>
