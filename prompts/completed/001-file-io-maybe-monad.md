<objective>
Implement the foundation layer for file I/O using monadic composition patterns. Create a composable pipeline: file path → raw byte istream → raw byte buffer that uses Maybe/std::optional for error handling.

This is the first step in building a robust, composable CSV import pipeline. Getting this foundation right ensures all subsequent layers can rely on clean, functional error propagation.
</objective>

<context>
This is a C++23 bookkeeping application (Cratchit) that needs to read CSV files from various sources (banks, Swedish tax agency SKV). Files may not exist, may be unreadable, or may have access issues - these should be handled gracefully through monadic composition, not exceptions.

Read CLAUDE.md for project conventions and architecture patterns.

**Examine existing code patterns:**
@src/text/encoding.hpp
@src/text/encoding.cpp

**Search for existing "Maybe" type implementations** in the codebase that can be reused.
</context>

<requirements>
Create a composable file I/O abstraction with these characteristics:

1. **File Opening**: `file_path → Maybe<istream>`
   - Handle file not found, permission denied, etc.
   - Return optional/Maybe monad

2. **Stream to Buffer**: `istream → Maybe<byte_buffer>`
   - Read raw bytes into buffer
   - Handle read errors gracefully

3. **Composed Pipeline**: `file_path → Maybe<byte_buffer>`
   - Use monadic composition (and_then, transform, or_else)
   - Demonstrate chaining operations
   - Short-circuit on first failure

4. **Error Context**: Consider what error information should be preserved
   - File path for debugging?
   - Error type (not found vs permission denied)?
   - Use existing Maybe types if they provide better context than bare std::optional
</requirements>

<implementation>
**Design Considerations:**
- Should this be a free function, a class, or a namespace of utilities?
- What type should byte_buffer be? std::vector<std::byte>? std::span?
- Should the buffer be read all at once or support streaming?
- How do existing Maybe types in the codebase work? Can they be leveraged?

**C++23 Features:**
- Use modern C++23 idioms where supported
- Test compilation on macOS XCode

**File Organization:**
Examine the existing codebase structure to determine where this code should live. Options:
- Add to existing src/text/encoding.cpp if I/O utilities already exist there
- Create new file like src/io/file_reader.hpp/cpp
- Add to a utilities namespace

Choose based on existing patterns in the codebase.
</implementation>

<output>
Create or modify files as appropriate:
- Header file for file I/O abstractions (determine location based on codebase structure)
- Implementation file

**Create tests in:** `./src/test/test_csv_import_pipeline.cpp`
- Follow the test structure from @src/test/test_atomics.cpp
- Use namespace: `namespace tests::csv_import_pipeline::file_io_suite { ... }`
- Include Google Test headers: `<gtest/gtest.h>`
- Use `logger::scope_logger` for test logging
- Create tests demonstrating monadic composition

Example test structure:
```cpp
namespace tests::csv_import_pipeline {
    namespace file_io_suite {
        TEST(FileIOTests, ReadValidFile) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, ReadValidFile)"};
            // Test file reading with valid path
        }

        TEST(FileIOTests, ReadMissingFileReturnsEmpty) {
            logger::scope_logger log_raii{logger::development_trace, "TEST(FileIOTests, ReadMissingFileReturnsEmpty)"};
            // Test that missing file returns empty Maybe without throwing
        }
    }
}
```

Document the chosen approach and rationale.
</output>

<verification>
Before completing:

1. **Compilation**: Code compiles without errors on macOS XCode with `./run.zsh`
2. **Tests Pass**: Run with `./cratchit --test` to verify all tests pass
3. **Monadic Composition**: Tests demonstrate chaining with and_then/transform
4. **Error Handling**: Test shows that file-not-found returns empty optional without throwing
5. **Integration**: Can this be easily composed with the next step (encoding detection)?

Test coverage should include:
- Reading valid file succeeds
- Reading missing file returns empty Maybe
- Reading file with permissions issue handled gracefully
- Monadic composition chains correctly
</verification>

<success_criteria>
- [ ] File path → raw byte buffer pipeline implemented
- [ ] Uses Maybe/std::optional for monadic composition
- [ ] No exceptions thrown for expected failures (file not found, etc.)
- [ ] Code compiles on macOS XCode with C++23
- [ ] Composable with subsequent pipeline steps
- [ ] Consistent with existing codebase patterns
- [ ] Simple example/test demonstrates usage
</success_criteria>
