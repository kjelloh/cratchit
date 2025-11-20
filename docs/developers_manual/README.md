# cratchit developers manual

## git command reference (good to remember)

To get a 'proper' visual feed back how branches relates to master the 'git log' with '--graph' seems to do the job and show branches 'indented' in paralell with 'matsre' as I like to have it visualised?

```sh
git log --oneline --graph --decorate --all
```

## macOS BUILD issues

At times it seems the tool-chain builds the binary with an unacceptable 'code sign'?

Youy can check for this pn macOS in zsh-shell by running cratchit through the lldb debugger.

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % lldb workspace/cratchit
(lldb) target create "workspace/cratchit"
Current executable set to '/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/cratchit' (arm64).
(lldb) run
Process 90231 launched: '/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/cratchit' (arm64)
Process 90231 exited with status = 9 (0x00000009) Terminated due to code signing error
(lldb) quit
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit %
```

In case the error is 'Terminated due to code signing error' you can generate an acceptable one with the macOS code signing tool.

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % codesign -s - --force --deep workspace/cratchit

workspace/cratchit: replacing existing signature
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit %
```

Now macOS should allow the cratchit binary to execute again (fingers crossed).

Note: As of 20251103 I do not know exactly when and why this happens.

## SIE file handling

An SIE file contains registered financial events using Swedish BAS framework.

The cratchit app models and SIE file content with the SIEEnvironment class.

```cpp 
class SIEEnvironment {...}
```

As of this writing cratchit variant 'zeroth' handling of financial year is rudimentary and lacking in many ways.

* The envrionment stores one instance of the tag/type sie_file as an array of paths to imported sie-files

```sh
sie_file:0 "-1=sie_in/TheITfiedAB20250211_145723.se;current=sie_in/TheITfiedAB20250510_190029.se"
```

* The 'current' (also index 0) financial year is NOT checked nor 'shifted' when a new year starts.
* The cratchit app does NOT check if a registered financial event has a date withing the current fiscal year.
* The cratchit app keeps a single 'cratchit.se' SIE-file with staged entries. These are entries that are not in the corresponding imported SIE-file for the same 'year key'.
* The 'model->sie[year_key]' contains the SIE file contents for year key 'current',-1','-2',...
* The cratchit app reads sie-files into the model with the code:

```cpp
if (environment.contains("sie_file")) {
  auto const id_ev_pairs = environment.at("sie_file");
  if (id_ev_pairs.size() > 1) {
    std::cout << "\nDESIGN_INSUFFICIENCY: Expected at most one but found " << id_ev_pairs.size() << " 'sie_file' entries in environment file";
  }
  for (auto const& id_ev_pair : id_ev_pairs) {
    auto const& [id,ev] = id_ev_pair;
    for (auto const& [year_key,sie_file_name] : ev) {
      std::filesystem::path sie_file_path{sie_file_name};
      if (auto sie_environment = from_sie_file(sie_file_path)) {
        model->sie[year_key] = std::move(*sie_environment);
        model->sie_file_path[year_key] = {sie_file_name};
        prompt << "\nsie[" << year_key << "] from " << sie_file_path;
      }
      else {
        prompt << "\nsie[" << year_key << "] from " << sie_file_path << " - *FAILED*";
      }
    }
  }
}
```

* There is only one 'cratchit.se' to store new financial entries that are not in the imported sie-file for 'current'.

### SIE files indexed by financial year?

Maybe it would be better to organise SIE files indexed by an actual financial year?

20250806 - Started refactoring from SIEEnvironments_X -> SIEEnvironments_R. Where 'X' maps from int 0,-1,-2... relative index to SIEEnvironment and 'R maps Date -> SIEEnvironment (Date being forst day of financial year)

## Claude Code observations

I have used Claude Code to and from during this project with mixed results.

Basically Claude Code easilly gets confused with the complexity of this project and C++.

### Observations from the Claude code assistance on the git branch claude-001-refactor-csv-import-pipeline

For prompt './prompts/001-file-io-maybe-monad.md' Claude succeeded to implement 'src/io/file_reader.hpp' and tests 'src/test/test_csv_import_pipeline.cpp'. This took over half an hour though! It seems it basically struggled with the complexity of the code base.

It based the implementations on 'using IOResult = functional::AnnotatedOptional<T>;' which was an abstraction to high. I wanted it to base it on std::optional so that I copuld later create the AnnotatedOptional<T> on top of that. The AnnotatedOptional is my optional-like abtsraction with a user-message text side-channel for UI operation feed back pipe lines.

Also, the actual code in file_reader.hpp is old-school stream seek, C-API error code handling and what have you. And all the code is inline (no cpp-file with defintions). But it works so ok.

Conclusion: I decided to go ahead and let claude do its thing but work on better meta-prompting and prompting to guide it to code more to my liking. Also, I decided to do the refactoring fter the code is working as a separate activity.

For prompt ./prompts/002-encoding-detection.md I added an section <assistant-role> to try and guide Claude to create code more to my liking.

When I ran this prompt Claude actually detected that prompt had changed :). Great!

```sh
Perfect! I see the prompt has been updated with an <assistant-role> section that gives me context about the codebase conventions. Let me implement Step 2 directly (following the same approach as Step 1,
  rather than delegating to a sub-agent):
```

Claude was unable to understand why running catchit produced no output when macOS detected a code signing problem with the binary (and thus silently refused to run it). But Claude could apply the code signing command and contonue when I told it about the problem (See about the code signing problem above).

Claude implemented

```C++
      template<typename ByteBuffer>
      std::optional<EncodingDetectionResult> detect_buffer_encoding(
          ByteBuffer const& buffer
         ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD)
```

although there already was a
```C++ 
std::optional<EncodingDetectionResult> to_content_encoding(
         char const* data
        ,size_t length
        ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD);
```

Maybe this was due to my instructions that it is a C++23 expert that prefers modern C++? Anyhow, I will accept that for now.

It added '#include <cstring>' to the test unit. I am not sure why as I can find no C-style operation on null terminated strings or char[]? Also, the test unit compiles without the include (although that does not prove it is not still included as a side effect from other includes?).

It added a test for UTF-8 encoded strings but did this with:

```C++
        std::string utf8_content = "Name,Amount\nJohan Svensson,100.50\nÅsa Lindström,250.75\n";
```

This asumes the source code file is encoded in UTF-8. Otehrwise the Swedish characters will be encoded in the encoding used by the source file (so this shows a lack of meta-understanding on Claudes part).

Claude did a good job of this step though. It intergrated well with existing code and created good test covarge it seems.

It even surprised me with testing fo confidence:

```C++
      // Use high threshold to potentially get no match
      auto encoding = text::encoding::icu::detect_buffer_encoding(short_buffer, 95);
```

That was a pleasant surpice!

For prompt 003-xxx I edited the 

Claude Code started of with saying:

```sh
 Looking at the prompt, I need to:
  1. Create a lazy C++23 range view that transcodes raw bytes to Unicode code points
  2. Use the DetectedEncoding enum to select transcoding method
  3. Leverage existing encoding infrastructure (UTF8::istream, ISO_8859_1::istream, etc.)
  4. Ensure lazy evaluation
  5. Handle transcoding errors gracefully

  From the assistant role, I learned:
  - The codebase uses 'decode' for source encoding → Unicode
  - UTF-8 transcoding uses text::encoding::UTF8::ToUnicodeBuffer
  - There are existing encoding::xxx::istream types with line-by-line transcoding
```

This prompted me to clarify to NOT reuse the xxx::istream types.

At 14:00 I ran into a Claude limit:

```sh
Read(src/text/charset.cpp)
  ⎿  Read 338 lines
  ⎿  Session limit reached ∙ resets 3pm
     /upgrade to increase your usage limit.
```

So I decided to take a break and do something else for an hour. I went for a run and then took a coffee after a the shower. A well spent pause!

Claude now succeeded to perform as directed by prompt 003-xxx. It produced a nice summary:

```sh
Step 3 Complete! 🎉

  I've successfully implemented a lazy C++23 range view for transcoding raw bytes to Unicode code points. Here's what was accomplished:

  Implementation Summary:

  Created /src/text/transcoding_views.hpp:
  - Lazy range view that transcodes bytes → Unicode code points (char16_t) on-demand
  - Reuses existing transcoding logic without using the xxx::istream types:
    - UTF8::ToUnicodeBuffer for UTF-8 decoding (multi-byte, stateful)
    - charset::ISO_8859_1::iso8859ToUnicode for ISO-8859-1 (single-byte)
    - charset::CP437::cp437ToUnicode for CP437 (single-byte)
  - C++23 ranges compliant - uses std::ranges::view_interface
  - Composable with standard range algorithms (take, find, all_of, etc.)
  - Pipeable syntax support: buffer | bytes_to_unicode(encoding)

  Added 9 comprehensive tests - all passing ✅:
  1. UTF-8 simple ASCII transcoding
  2. UTF-8 multi-byte Swedish characters (Åsa)
  3. UTF-8 complex Swedish text (åäö)
  4. ISO-8859-1 transcoding
  5. ISO-8859-1 Swedish characters
  6. Lazy evaluation verification (no eager materialization)
  7. Composition with standard range algorithms
  8. Integration with Steps 1 & 2 (file → buffer → encoding → unicode)
  9. Empty buffer handling

  Test Results:

  [----------] 9 tests from BytesToUnicodeTests
  [       OK ] All 9 tests passed (0 ms total)
  [  PASSED  ] 82 out of 83 total tests

  The implementation successfully chains with Steps 1 & 2:
  file_path
    → read_file_to_buffer()           // Step 1
    → detect_buffer_encoding()        // Step 2  
    → bytes_to_unicode(buffer, encoding)  // Step 3
    → range<char16_t>

  Step 3 is complete and ready for Step 4!
```

But I observed some shortcomings.

In the test code Claude generated some pritty low level C-code.

```C++
      // Simple ASCII text (subset of UTF-8)
      cratchit::io::ByteBuffer buffer;
      std::string test_text = "Hello";
      buffer.resize(test_text.size());
      std::memcpy(buffer.data(), test_text.data(), test_text.size());
```
Given that ByteBuffer is std::vector this was quite dissapointing. But it works so ok.

Claude used the term 'transcoding' (which is correct) but I had asked it to use the term 'decoding' for in -> Unicode.

```C++
// Create transcoding view
      auto unicode_view = text::encoding::views::bytes_to_unicode(
        buffer,
        text::encoding::DetectedEncoding::UTF8
      );
```

But the design was what I had asked for. So ok.

Claude generated good test coverage and used hard coded hex-values to test for åäö-code points in supported encodings. This was good!

It still assumed UTF-8 encoded source code text though like:

```C++
      // Create test file with UTF-8 content
      auto temp_path = std::filesystem::temp_directory_path() / "cratchit_test_step3_integration.txt";
      {
        std::ofstream ofs(temp_path);
        ofs << "Åsa Lindström";  // UTF-8 encoded Swedish text
      }
```

But I will let that pass for now.

Claude created the lazy code-point-range views in src/text/transcoding_views.hpp. I looked through the file and got the feeling the code may be a little verbose and not as 'clean' as it could have been written by someone knowing how to write range views properly. But I don't have the knowledge to judge and this is where I am going to trust Claude to help me write code. At least I now have code that passes the tests and I can take it from here to learn and enhance?

I edited prompt 004-xxx to tell claude to prefer C++23 span and string_view over C-code constructs.

Claude succeeded with the task to create a range view over Unicode code points that outputs UTF-8 byte stream. I was actually a little surprised as this is NOT a code-point to code-point transform. Instead one Unicode code points generates 1..n UTF-8 bytes.

And Claude also re-used existing code I have as in:

```C++
        // Use std::ostringstream with UTF8::ostream to encode
        std::ostringstream oss;
        text::encoding::UTF8::ostream utf8_os{oss};
```

This was very good!

Claude did NOT try to adopt to any encoding detected by the RuntimeEncoding calss though. It simply picked up on my existing comments on 'hard coded' UTF-8 encoding for internal (platform) use.

Good enough for now though.

Claude then succeeded with prompt 005-xxx to assemble and text a complete transcoding chain raw in -> platform encoded std::string (hard coded to UTF-8). But here I obseerved some divergence from existing code.

Claude created:

```C++
cratchit::io::IOResult<std::string> read_file_with_encoding_detection(
    std::filesystem::path const& file_path,
    int32_t confidence_threshold = icu::DEFAULT_CONFIDENCE_THERSHOLD
  )
```

Where namespace 'cratchit::io' now is a paralell to existing namespace 'persistent::in'.

Also, cratchit::io::IOResult seems like a paralell type to cratchit::functional::AnnotatedOptional<T>?

Anyhow, looking though new file 'src/text/encoding_pipeline.hpp' and added tests in 'src/test/test_csv_import_pipeline.cpp' I feel content with the result. I do not have time to delve deep into all the code. I decide to trust the help Claude has provided and see where I end up.

The combination of having Claude generate code AND test makes me feel more secure to go with the code and be able to come back later to refactor it (fingers crossed)?

For prompt 006-xxx Claude failed to do what I wanted. It got to attracted to the existing code and created UTF-8 text -> Maybe Table, but failed to focus on a 'neutral' table. Instead it went for the SKV / NORDEA account statement tables right away (just as try_parse_csv does). So I decided to edit the 006-xxx prompt to try and make Claude to deviate from existing code and create a Maybe neutral table instead.

I used my custom '/create_prompt' and managed to get Claude to end up with this:

```sh
Write(prompts/006-csv-text-to-table-neutral.md)
  ⎿  Wrote 330 lines to prompts/006-csv-text-to-table-neutral.md                                                                                                                                   
     <assistant-role>
     You are an expert in writing C++23 code that is easy to read and can be used in functional style composition based on Maybe monads (std::optional) and C++ range views where this makes sense.
     1. You prefer to use C++23 'span' and 'string_view' over C-code constructs.
     2. You prefer to use C++23 standard library over any specialized helpers.
     3. You prefer readable code over efficient code.
     4. You prefer to organize into namespaces over splitting code into C++ translation units.
     You are an expert in character set code points and encodings.
     … +320 lines (ctrl+o to expand)

⏺ ---
  Prompt created successfully!

  ✓ Saved prompt to ./prompts/006-csv-text-to-table-neutral.md

  This prompt will guide Claude to:
  - Create a NEW, neutral CSV parser (ignoring the existing try_parse_csv)
  - Implement runtime_encoded_text → Maybe<CSV::Table>
  - Support semicolon/comma delimiters and quoted fields
  - Test with real CSV data from account_statements_csv.hpp
  - Compose with the encoding pipeline from steps 1-5
  - Handle macOS code signing requirements for test execution
  - Work incrementally: compile and test after each feature

```

When Claude ran the new prompt '006-' it created a new file 'src/csv/neutral_parser.hpp' and added tests to existing 'src/test/test_csv_import_pipeline.cpp'. So it did not pick up on the domain 'CSV::Table' and instead went with 'neutral csv'. Well, that works too I suppose. I mean, what would you get from parsing a CSV-file but a table?

Claude picked up on the test data I had crated in 'src/test/data/account_statements_csv.hpp' ok. That was good! I hate writing these complicated tests!

Running Claude I now got the fans on  my macBook Pro M3! So it uses some serious computation power locally too!

It seems that if I interupt Claude to repair the code signing on the binary (got stuck on no output), it starts over from reading the prompt again? I am not sure, but it did not just carry on from where I interupted it?

Claude REALLY struggled with this task (prompt)! I am not sure why but the token consumption was in the 2k over and over. At one time it even created a small C++ main pogram to test my test input data?

Claude finished the task but to my great dissapointment this time. I decided to check in the code and generated tests anyhow to see what I can refactor out from this mess?

1) It did implement the csv-file parsing but with lowe level C-style code! very bloated.
2) It did NOT implement detection of header row. Instead it always uses row 0 as header which potentially fails on SKV account statement csv files (first row is a saldo row, not a header).
3) It reimplemented encoding checks on table content. This is not needed as we already have tested the trasncoing pipe-line for this. We should focus only on CSV to table parsing and trust the encoding to be correct.

It feels as if Claude context window just got bloated and it fell back to create code by mimicking web code and forgot everytging about the context provided in the prompt?

I actually started a new session with Claude and asked it to evaluate how well it done against prompt 006. And it agreed it did a bad job! It also proposed a very comprehensive refactoring task in seven steps! I wonder - does Anthropic try to make me burn tokens to sell me more *smirk*!

I step prmpt 007-xxx Claude missed to take off from CSV::Table and instead provided functions that started off with text. I dissagreed and asked from CSV::Table -> Maybe Account statements.

Now Claude derailed again and went off and resued the existing 'heading ID' and what have you. Allthough I told it to implement a NEW table to statements projection. To small context window again?

I edited prompt 007- to try and make Claude stay on track (less text and more on point).