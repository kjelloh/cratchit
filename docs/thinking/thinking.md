# Thinking out loud on cracthit development

I find thinking out loud by writing to be a valuable tool to stay focused and arrive faster at viable solutions.

## 20260127

So let's implement:

* text::encoding::inferred::maybe::to_content_encoding
* text::encoding::inferred::maybe::to_istream_encoding
* text::encoding::inferred::maybe::to_file_at_path_encoding

It seems they depend on echother bottom up? Tah is to_file_at_path_encoding calls to_istream_encoding that calls to_content_encoding?

YES! That is what my AI friend has created for me. Now this breaks the and_then pipe-line I have aimed for.

Calling to_file_at_path_encoding actually opens the file and analyses the content.

But creating the istream and read the content is the task of the pipe line:

```c++
      auto result = persistent::in::maybe::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::maybe::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::maybe::to_with_threshold_step_f(100))
        .and_then(text::encoding::maybe::to_with_detected_encoding_step)
        ...

```

So where do we aactually have the call to to_file_at_path_encoding? Is it in to_with_detected_encoding_step?

* From try_parse_csv
* And thats it!
* Not even any tests!

wow, the scope of this mess just grows and grows!

Anyhow, it seems I should just remove this whole branch of code.

* Remove try_parse_csv
* Remove to_file_at_path_encoding

And fom where is to_istream_encoding called?

* From istream_to_decoding_in (TaggedAmountFramework!)
* And that is it!
* Not even any tests!

And istream_to_decoding_in is called ONLY from tas_from_statment_file. And tas_from_statment_file is marked as (TODO: Remove) and NOT called from anywhere!

So it seems I should:

* Remove tas_from_statment_file
* Remove istream_to_decoding_in
* Remove to_istream_encoding

Should I do this now or keep focusing on the nromalised API for encoding inference?

You know what? I SHOULD remove it! As it is not part of any active code but IS part of the 'encoing inference' API. Thus I shoudl remove it as part of the refactoring!

So we have:

* Remove tas_from_statment_file
* Remove istream_to_decoding_in
* Remove to_istream_encoding
* Remove try_parse_csv
* Remove to_file_at_path_encoding

Good. Let's do this! In what order though? It would be nice to remove one-by-one and at each step get code that still 'works'?

So what can we begin to remove as an isolated change?

* Revoved csv::parse::deprecated with no problem at all.
  - ParseCSVResult
  - try_parse_csv
  - encoding_caption

What next?

* Removed to_file_at_path_encoding with no repercussions.

What is next?

* Removed tas_from_statment_file with no repercussions.
* NOTE: Consider to also remove account_statements_to_tas?

In fact, all the 'original' pipe line can now be removed?

```c++
// TODO: Remove (Replaced by pipeline csv::path_to_tagged_amounts_shortcut in csv/import_pipeline.hpp)
AnnotatedMaybe<persistent::in::MaybeIStream> file_path_to_istream(std::filesystem::path const& statement_file_path) {
  AnnotatedMaybe<persistent::in::MaybeIStream> result{};
  result.m_value = persistent::in::to_maybe_istream(statement_file_path);
  if (!result.m_value) result.push_message("file_path_to_istream: Failed to create istream");
  return result;
}

// TODO: Remove (Replaced by pipeline csv::path_to_tagged_amounts_shortcut in csv/import_pipeline.hpp)
AnnotatedMaybe<text::encoding::MaybeDecodingIn> istream_to_decoding_in(persistent::in::MaybeIStream const& maybe_istream) {
  AnnotatedMaybe<text::encoding::MaybeDecodingIn> result{};

  auto maybe_encoding = maybe_istream
    .and_then([](std::istream& is) {
      return text::encoding::inferred::maybe::to_istream_encoding(is);
    });

  if (maybe_encoding) {
    result.m_value = text::encoding::to_decoding_in(
       maybe_encoding->defacto
      ,maybe_istream.value());
  }

  if (!result.m_value) result.push_message("istream_to_decoding_in: Failed to create a decoding in stream");
  return result;
}

// TODO: Remove (Replaced by pipeline csv::path_to_tagged_amounts_shortcut in csv/import_pipeline.hpp)
AnnotatedMaybe<CSV::FieldRows> decoding_in_to_field_rows(text::encoding::MaybeDecodingIn const& decoding_in) {
  AnnotatedMaybe<CSV::FieldRows> result{};
  result.push_message("decoding_in_to_field_rows: NOT YET IMPLEMENTED");
  return result;
}
// TODO: Remove (Replaced by pipeline csv::path_to_tagged_amounts_shortcut in csv/import_pipeline.hpp)
AnnotatedMaybe<CSV::Table> field_rows_to_table(CSV::FieldRows const& field_rows) {
  AnnotatedMaybe<CSV::Table> result{};
  result.push_message("field_rows_to_table: NOT YET IMPLEMENTED");
  return result;
}
```

I think I will leave them be for now an focus on the code that affects the encoding inference API.

So what is next?

* Removed istream_to_decoding_in with no repercussions

## 20260126

Now when I have slept on this I have a new approach. It seems I can first create the 'nomralised' API for encoding inference.

Thinking this way I can:

1. Create a new unit to_inferred_encoding and implement text::encoding::inferred namespace with the existing API
2. Then push down code that actually use ICU into the icu_facade namespace

DARN! I still fail to find a strategy to refactor in small steps. How can I move code to to_inferred_encoding step-by-step and be guided by the compiler to reports what needs to be attended to?

Maybe the encding should still be the base for to_inferred_encoding?

In that case I can make to_inferred_encoding a clone of the encoding unit and then prune both until they are separate units?

I did this and now both compile just fine (of course) but the linker reports duplicate symbols for all duplicated stuff.

So now I can attend to each duplicate and decide where to put it?

But maybe not. I would first like to make to_inferred_encoding include encoding.hpp to fix this design before moving on?

So having to_inferred_encoding.hpp include encoding.hpp only works fine for now. So now I can I start pruning until I have no duplocate symbols?

Or, maybe now is the time to first lift the whole 'inferr encoidng' API to become the 'normalised' API?

I decided that tghe next step is to 'empty out' the icu_facade into the 'normalised' API first. It seems I can do this by renaming the existing icu_facade to get existing access to fail to compile. I now have a compiler guided process to move the API?

1. Create the namespace 'inferred' for the normalised API.
2. rename existing icu_facade to icu_facade_deprecated in encoidng unit.
3. Move code to inferred and refactor call sites accodingly

Let's try!

I now get compiler errors like:

```sh
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.hpp:258:8: error: use of undeclared identifier 'icu_facade'
  258 |        icu_facade::EncodingDetectionResult const& detected_source_encoding
      |        ^
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/to_inferred_encoding.cpp:28:23: error: unknown type name 'EncodingDetectionResult'; did you mean 'icu_facade_deprecated::EncodingDetectionResult'?
   28 |         std::optional<EncodingDetectionResult> to_content_encoding(
      |                       ^~~~~~~~~~~~~~~~~~~~~~~
      |                       icu_facade_deprecated::EncodingDetectionResult
...
```

GREAT! Now I can use the compiler to guide my refactorings.

It turns out that the code worked better with the namespace 'inferred' before 'icu_facade'. In the hpp-file this enable icu_facade to referr to types in 'inferred'. And in cpp code in 'inferred' could still call 'icu_facade' due to declarations in hpp-file.

GOSH! There is no end to the mess in the code! Now I found:

```c++
  std::optional<EncodingDetectionResult> to_content_encoding(
    char const* data
    ,size_t length
    ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD);

  // But...

  template<typename ByteBuffer>
  std::optional<EncodingDetectionResult> to_detetced_encoding(
      ByteBuffer const& buffer
    ,int32_t confidence_threshold = DEFAULT_CONFIDENCE_THERSHOLD) {
```

My AI friends really lacks the ability to keep to a consistent architecture!

I now realised I should first refactor the hpp-files (declarations).

* In this way I get the compiler guide me to all call sites that need change.

So I emptied out the to_inferred_encoding.cpp and continued with:

* Moving from text::encoding::icu_facade_depracated to text::encoding::inferred

Now I get compiler erros like:

```sh
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/csv/parse_csv.hpp:15:25: error: no member named 'icu_facade' in namespace 'text::encoding'
   15 |         text::encoding::icu_facade::EncodingDetectionResult icu_detection_result;
      |         ~~~~~~~~~~~~~~~~^
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/csv/parse_csv.hpp:21:52: error: no member named 'icu_facade' in namespace 'text::encoding'
   21 |       std::string encoding_caption(text::encoding::icu_facade::EncodingDetectionResult const& detection_result);
      |                                    ~~~~~~~~~~~~~~~~^

```

In fact this approach worked well all the way. I could adress each compilation error and refactor each call site access to previous icu_facade to new inferred.

I ended upp with linker error:

```sh
Undefined symbols for architecture arm64:
  "text::encoding::inferred::maybe::to_content_encoding(char const*, unsigned long, int)", referenced from:
      std::__1::optional<MetaDefacto<text::encoding::inferred::EncodingDetectionMeta, text::encoding::EncodingID>> text::encoding::inferred::maybe::to_detetced_encoding<std::__1::vector<std::byte, std::__1::allocator<std::byte>>>(std::__1::vector<std::byte, std::__1::allocator<std::byte>> const&, int) in test_csv_import_pipeline.cpp.o
  "text::encoding::inferred::maybe::to_istream_encoding(std::__1::basic_istream<char, std::__1::char_traits<char>>&, int)", referenced from:
      istream_to_decoding_in(cratchit::functional::memory::OwningMaybeRef<std::__1::basic_istream<char, std::__1::char_traits<char>>> const&)::$_0::operator()(std::__1::basic_istream<char, std::__1::char_traits<char>>&) const in TaggedAmountFramework.cpp.o
  "text::encoding::inferred::maybe::to_file_at_path_encoding(std::__1::__fs::filesystem::path const&, int)", referenced from:
      CSV::parse::deprecated::try_parse_csv(std::__1::__fs::filesystem::path const&) in parse_csv.cpp.o
ld: symbol(s) not found for architecture arm64
```

Which told me that the only external dependancies where:

* text::encoding::inferred::maybe::to_content_encoding
* text::encoding::inferred::maybe::to_istream_encoding
* text::encoding::inferred::maybe::to_file_at_path_encoding

This verifies my assumption about how the API for encodining inference should work.

I implemented them as dummy (nullopt return). And now cratchit compiles with failing tests:

```sh
[  FAILED  ] 16 tests, listed below:
[  FAILED  ] MonadicCompositionFixture.PathToAccountIDedTable
[  FAILED  ] MonadicCompositionFixture.PathToAccountStatementTaggedAmountsRefactoring0
[  FAILED  ] MonadicCompositionFixture.PathToAccountStatementTaggedAmountsRefactoring1
[  FAILED  ] MonadicCompositionFixture.PathToAccountStatementTaggedAmountsRefactoring2
[  FAILED  ] MonadicCompositionFixture.PathToAccountStatementTaggedAmountsRefactoring3
[  FAILED  ] EncodingDetectionTestFixture.DetectUTF8
[  FAILED  ] EncodingDetectionTestFixture.DetectISO8859
[  FAILED  ] EncodingDetectionTests.ComposesWithFileIO
[  FAILED  ] BytesToUnicodeTests.IntegrationWithSteps1And2
[  FAILED  ] RuntimeEncodingTests.CompleteTranscodingPipeline
[  FAILED  ] EncodingPipelineTestFixture.ISO8859FileToUTF8Text
[  FAILED  ] EncodingPipelineTestFixture.CompleteIntegrationAllSteps
[  FAILED  ] CSVPipelineCompositionTestFixture.TranscodesISO8859ToUTF8ThenParsesCSV
[  FAILED  ] GenericStatementCSVTests.NORDEAStatementOk
[  FAILED  ] AccountStatementDetectionTests.GeneratedSingleRowMappingTest
[  FAILED  ] AccountStatementDetectionTests.GeneratedRowsMappingTest
```

Do I dare to check this code in?

If I check in this code it will be harder for me to back track?

On the other had, maybe this is the time to try to check in code even though I am not at a working app?

NOTE TO SELF: I have benn quite persistent to comment woth 'TODO' about deprecated and code to remove. So I can trust this is enough to clean out unwanted code later?

## 20260125

So time to make to_platform_encoded_string_step into a maybe version and base mondaic  version on that.

I implemneted the maybe variant ok. I think it is quite neat actually? I think I like how the 'transcoding' turned out as:

```c++
  // buffer -> unicode -> runtime encoding
  auto unicode_view = views::bytes_to_unicode(buffer, deteced_encoding);
  auto platform_encoding_view = views::unicode_to_runtime_encoding(unicode_view);
```

For now it seems like a good design to have the transcoding work on a view that generates code points over the charachter set?

* We get views::bytes_to_unicode to get a lazy range view to iterate over code points
  where our 'bytes' gets decoded into unicode using the provided deteced_encoding enum.
* We then get the runtime or 'platform' encoding as a lazy range view from views::unicode_to_runtime_encoding.

What is less good is that we pass two arguments to views::bytes_to_unicode? We already have a pair WithDetectedEncodingByteBuffer.

Would it be a good idea to pass the WithDetectedEncodingByteBuffer to views::bytes_to_unicode?

Where does WithDetectedEncodingByteBuffer live?

* We have text::encoding::WithDetectedEncodingByteBuffer
* We have text::encoding::WithDetectedEncodingByteBuffer::Meta being icu_facade::EncodingDetectionResult.

So we have not yet isolated a cratchit level EncodingDetectionResult that we can later free from icu_facade.

Anyhow, where does bytes_to_unicode and unicode_to_runtime_encoding live?

* text::encoding::views::bytes_to_unicode
* text::encoding::views::unicode_to_runtime_encoding

GERAT! The 'views' already lives in text::encoding.

After having thought for a while I have a proposal.

1. Implement a bridge to a 'normalised' encoding with a to_normalised_encoding(icu_facade::EncodingDetectionResult).
2. Then make a meta='normalised encoding' and defacto='byte buffer' pair to pass to bytes_to_unicode.

DARN! I have a 'catch 22' like identifier namespace problem.

* The existing and used EncodingDetectionResult lives in icu_facade.
* All existing detection functions also lives in icu_facade and returns this EncodingDetectionResult.
* There are no 'normalised' API for encoding detection.

To implement a 'normalised' EncodingDetectionResult I need also a 'normalised' API.

Should I do the work and define a normalised 'encode detection' API that uses icu_facade for now? Is it worth it?

I get an urge to break out the icu_facade to its own unit.

Well, I tried. And now I get the classic circular hpp-file dependancies again.

* I want icu_facade.hpp to include (know about) EncodingDetectionResult in normalised bytes_to_encode_id.hpp
* But I also want bytes_to_encode_id.hpp to use / leverage / know about icu_facade.hpp API

So what is the solution again? I need a common 'atomic' header with the normalised types like EncodingDetectionResult and EncodingID?

Maybe the low level unit is the existing encoding?

Yes, that can work. But then I need to make icu_facade::EncodingDetectionResult into a normalised one?

What goes into icu_facade::EncodingDetectionResult?

```c++
  struct EncodingDetectionMeta {
    CanonicalEncodingName canonical_name; // ICU canonical name (e.g., "UTF-8")
    std::string display_name;             // Human readable name
    int32_t confidence;                   // ICU confidence (0-100)
    std::string language;                 // Detected language (e.g., "sv" for Swedish)
    std::string detection_method;         // "ICU", "BOM", "Extension", "Default"
  };

  using EncodingDetectionResult = MetaDefacto<EncodingDetectionMeta,EncodingID>;
```

Well, the EncodingID is the 'normalised' enum. And the meta is ICU specific.

So what depends on what here? If I want a cratchit normalised API for encoding detection, I need it to be able to leverage some underlying library like the ICU (or a crachit local implemented library).

* normalised API -> uses ICU

For this to work I need ICU to only use its own types and 'stuff'! It can NOT use EncodingID!

I came upp with two new units:

```sh
	src/text/bytes_to_encode_id.cpp
	src/text/bytes_to_encode_id.hpp
	src/text/icu_facade.cpp
	src/text/icu_facade.hpp
```

I get a good feeling for this. And it seems I should aim for encoding.hpp -> bytes_to_encode_id.hpp -> icu_facade.hpp?

That is, encoding.hpp is the cratchit 'normalised' API. It depends on bytes_to_encode_id.hpp for 'normalised' encoding detection. It in turn uses icu_facade.hpp to implement this encoding detection.

To make this work I need to make icu_facade to be self-contained regarding the encoding code. But it may use cratchit logging and stuff that is not about encoding.

What could be the first step to get this ball rolling?

This is trycky! It seems I first need to kind-of reverse the icu_facade dependancy?

* How can I use the compiler to identify how icu_facade code depends on 'upstream' code?
* Can I first move icu_facade to the new unit and see if it compiles?

AHA! Yes, that could work. I can first add icu_facade as a stand alone unit and keep all the rest of the code as-is (NOT use icu_facade unit yet).

Doing this I can see the following code that depends 'the wrong way':

```c++

  // hpp
  EncodingID canonical_name_to_enum(CanonicalEncodingName const& canonical_name);
  using EncodingDetectionResult = MetaDefacto<EncodingDetectionMeta,EncodingID>;

  // cpp
  
```

I came up with the following scheme to use the compiler to identify upstream dependancies in icu_facae.

* I moved icu_facae hpp and cpp code to the new icu_facade hpp and cpp.
* I added icu_facade.cpp to CMakeLists
* I created namespace 'upstream' and added code there to make the unit comopile.
* Finaly I commented out the namespace levels 'text::encoding' to NOT get duplicate symbol names
  (icu_facade unit now places symbol names in top leve namespace icu_facade that does NOT clash with existing text::encoding::icu_facade)

I now got upstream code clrealy defined as:

```c++
      namespace upstream {
        // TODO: Refactor away this dependancy
        enum class EncodingID {
          Undefined,
          UTF8,
          UTF16BE,
          UTF16LE, 
          UTF32BE,
          UTF32LE,
          ISO_8859_1,
          ISO_8859_15,
          WINDOWS_1252,
          CP437,
          Unknown
        };
      } // upstream

      namespace upstream {
        // TODO: Refactor away this dependancy

        std::string enum_to_display_name(EncodingID) {
          return "enum_to_display_name dummy";
        }
      } // upstream

```

I now wanted to check in this new unit but I did not like the state of the code (I can't trust I remember to finalise the refactoring).

I tried:

```c++
        [[deprecated("Refactoring incomplete: this code is temporary")]] 
        std::string enum_to_display_name(EncodingID) {
          return "enum_to_display_name dummy";
        }
```

But I did not like this. It was good the code compiled. But this also meant I did not see the warnings as I had tests running by REPL so all looked fine in the restricted console winow output.

How can I reach a step where my code is in a state I can keep if I forget to refactor further?

I think I wait with checking in until I have icu_facade stand-alone.

So what is the semantic of a stand-alone icu_facade?

* upstream::EncodingID must go (be added by client code)
* We still need an upstream API that provides the EncodingID
* So what is left for icu_facade to provide?
* And how to I determine this?

Maybe I can now create the upstream API to see what the icu_facade can priovide?

Can we implement the 'normalised API in the new bytes_to_encode_id?

AHA!

* The 'file extension' -> 'detected encoding' is NOT an icu thing!
  So this we can already make 'normalised'
* And maybe the only thing we need icu_facade to return is 'canocical name'?
  This seems to be what the ICU code returns as a result when asked to detect coinrent?

```c++
  UCharsetDetector* detector = ucsdet_open(&status);
  const UCharsetMatch* match = ucsdet_detect(detector, &status);
  // ...
  const char* canonical_name = ucsdet_getName(match, &status);
  int32_t confidence = ucsdet_getConfidence(match, &status);
  const char* language = ucsdet_getLanguage(match, &status);

```

Hm, it seems I need to investigate how ICU actually identifies a character set? Is it the whoel match? Or is there an enum or string that canonically identifies the character set?

I have decided to tread carefully and first trim the EncodingDetectionMeta down.

* I do not need the language detection.
* I think I 'need' only 'canonical name' and 'confidence' for now?

By renaming the laguage field I discovered only one place where it was accessed (and then only if empty!). So that was an easy removal.

Then I discovered that actually removing the field gives me a compiler error regarding the count of members, not about what member is missing.

If I:

```c++
      struct EncodingDetectionMeta {
        CanonicalEncodingName canonical_name; // ICU canonical name (e.g., "UTF-8")
        std::string display_name;             // Human readable name
        int32_t confidence;                   // ICU confidence (0-100)
        // std::string language_;                 // Detected language (e.g., "sv" for Swedish)
        std::string detection_method;         // "ICU", "BOM", "Extension", "Default"
      };

```

It for one triggers a recompilation of a very large of files! And in the end the compiler errors are of the sort:

```sh
[ 30%] Building CXX object CMakeFiles/cratchit.dir/src/text/encoding.cpp.o
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.cpp:317:16: error: excess elements in struct initializer
  317 |               ,"ICU"            
      |                ^~~~~
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.cpp:449:20: error: excess elements in struct initializer
  449 |                   ,"ICU"
      |                    ^~~~~
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.cpp:487:16: error: excess elements in struct initializer
  487 |               ,"Cannot open file"
      |                ^~~~~~~~~~~~~~~~~~
```

So this is unhelpfull. I solved this by adding a constructor taking only the values I want to keep.

```c++
        EncodingDetectionMeta(
           CanonicalEncodingName canonical_name
          ,int32_t confidence
          ,std::string detection_method);

```

And now the compiler is more helpfull:

```sh
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.cpp:312:21: error: no matching constructor for initialization of 'Meta' (aka 'text::encoding::icu_facade::EncodingDetectionMeta')
  312 |             .meta = {
      |                     ^
  313 |                canonical_str
      |                ~~~~~~~~~~~~~
  314 |               ,display_name
      |               ~~~~~~~~~~~~~
  315 |               ,confidence
      |               ~~~~~~~~~~~
  316 |               ,language_str
      |               ~~~~~~~~~~~~~
  317 |               ,"ICU"            
      |               ~~~~~~
  318 |             }
      |             ~
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.hpp:202:9: note: candidate constructor not viable: requires 3 arguments, but 5 were provided
  202 |         EncodingDetectionMeta(
      |         ^
  203 |            CanonicalEncodingName canonical_name
      |            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  204 |           ,int32_t confidence
      |           ~~~~~~~~~~~~~~~~~~~
  205 |           ,std::string detection_method);
      |           ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.hpp:196:14: note: candidate constructor (the implicit copy constructor) not viable: requires 1 argument, but 5 were provided
  196 |       struct EncodingDetectionMeta {
      |              ^~~~~~~~~~~~~~~~~~~~~
/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding.hpp:196:14: note: candidate constructor (the implicit move constructor) not viable: requires 1 argument, but 5 were provided
  196 |       struct EncodingDetectionMeta {
      |              ^~~~~~~~~~~~~~~~~~~~~

```

It is interesting that it seems adding a constructor does not remove the implicit copy and move constructors? AT least, the compiler tells me it failed to apply them?

NO, this is not a good thing to do now.

* I remove 'display name' before I have an upstream replacement.
* I should actually remove 'detection method' also?
  This is known by the caller, not by the icu_facade.

AHA: The icu_facade actually ONLY performs the 'identify from data' part!

So maybe I catually only need to return 'canonical name' and 'confidence'?

This day has now come to an end and I decided this approach was a dead end for now.

I delete my icu_facade unit again and tries something else tomorrow. It was a insightfull exersice never the less :)

## 20260124

Next to make into a maybe variant is to_with_detected_encoding_step. I started by separating existing text::encoding::monadic::to_with_detected_encoding_step into hpp declaration and cpp definition.

I found out that if I have a function declaration as 'result_type to_with_detected_encoding_step(arg_type)' and a definition with the inline specifier 'inline result_type to_with_detected_encoding_step(arg_type)' - Then the linker will FAIL!

This stumped me at first. Then I specualted that the 'inline' specifier actually tells the compiler to place the function code at all call sites as-is. That is, NOT implement a call with stack-frame and everything. So when the declaration specifies a function to be called. But he definition specifies an ilined function - then the caallable function is no where to be found by the linke?

I asked my AI friends and they do NOT agree. The mechanism that fails is a bit different.

I got this answer snippet:

```text
* Non-inline function → exactly one definition with external linkage must exist
* Inline function → definitions may appear in multiple TUs, but all TUs that odr-use it must see a definition
```

And I got this answer snippet:

```text
  1. Declaration without inline (in header):
    - Promises external linkage
    - Tells the linker: "There will be exactly ONE definition of this function in some .cpp file"
  2. Definition with inline (in .cpp):
    - Has inline linkage (internal/weak linkage in modern C++)
    - Tells the compiler: "This definition can appear in multiple translation units"
    - The definition is NOT exported as an external symbol
  3. The Mismatch:
    - The linker looks for an external definition (because of the declaration)
    - But finds none, because the inline definition wasn't exported as external
    - Result: undefined reference error
```

So it seems thge mechanism is based on compiler and linker mechanosms called:

* 'external linkage'
* 'odr usgage' 
* 'internal/weak linkage'
* 'external symbol'

So a normal declaration defines an 'external symbol' and a call site will engage 'external linkage' to this symbol. For this to work there must exist a definition for this external symbol somewhere in the linked files.

So with the inline specifier the symbol becomes an 'internal' (weak) symbol. And the compiler must see the definition to generate the code itself? Or, the compiler tells the linker to look for the symbol in the object file and NOT in any of the linked object files?

I also googled and found snippets from the C++ standard that states:

```text
An inline function shall be defined in every translation unit in which it is odr-used and shall have exactly the same definition in every case.
...
An inline function with external linkage shall have the same address in all translation units.
```

Wow, this was a rabbit hole! I think I will have to live this for now. I feel overwhelmed by all the ways this is talked about on the web and by AI. I fail to find a consistent cause-and-effect explanation that gives me a form model of what is going on. But I get some clues.

* The 'inline' specifier causes the compiler to NOT 'create' an external symbol.
* Thus the linker suceeds only if the symbol is in the same tramslation unit (and thus object file)

Lets continue with our refactoring of the and_then chain for maybe variants of my step functions.

It turns out we have more incosnistencies to deal with now.

* The text::encoding::monadic::to_with_detected_encoding_step generates an OK message 
  with the detected encoidng spelled out in name and confidence.
* If we place this code in the maybe-variant we hide these results behind the maybe-wall that only returns a single enum
  that represents the detected encoding.
* We have semantic clashes in that the maybe and the enum both can represent 'non viable' encoding.

I tried to make WithDetectedEncodingByteBuffer result carry ALL meta-data from the detection.

* But then I got code leakage in that WithDetectedEncodingByteBuffer allowed assignment to only encoding member
  (passing WithDetectedEncodingByteBuffer with unititialied data).

I started to implement a to_assumed_encoding to create a fully WithDetectedEncodingByteBuffer for the default UTF-8 case.

* But then I introduced a new maybe case if this also failed.
* Not to mention, when and how can to_assumed_encoding fail?

I have to give this some more thought!

Ok, so here are some observations.

1. I want text::encoding::monadic::to_with_detected_encoding_step
   to be based on a text::encoding::maybe::to_with_detected_encoding_step.
2. To keep monadic OK message with detected encoding info I need to pass
   that back from the maybe variant.
3. But then this extra information will also leak out from the maybe version.

Is that bad? It feels bad. What options do I have?

* I thought for a while I could make the monadic version re-cerate the 
  missing info from the returned enum.
  But NO - this is not possible as I still loose the confidence value.

Now I have already decided I do not like the idea of 'conmfidence' in encoding detection. It feels leaky as a concept. I mean, what does it even mean? If I take a sample inoput and check the content for possible encodings and characters sets. The only think I can say is that an option is not violated. I can't say how probable it is that I am right? Let's say I find the sample matches encoding of pure 7-bit ASCII. The only think I then know is that the sample input does not violate being an encoding of a large set of character sets and encoings that maps to 7-bit ascci for that range of code points. It could still be the case that the inoput is UTF-8 or ISO 8859-1. The sample is just to small to know?

* Another option is to change the WithDetectedEncodingByteBuffer to contain the full detection result 'from below'.

Can I do this in a way to still keep the information that now leaks upsteram 'contained'?

* I suppose I could make the field I use in existing code (the encdoing field) front-and-center and make the rest a 'meta' section?

Hm, YES. That could work! Then the leak would be a new meta-section. And upstream code will not accidentally see it?

Lets try!

* I can basically use icu_facade::maybe::to_detetced_encoding as the maybe-bversion?
* Although keeping text::encoding::maybe::to_with_detected_encoding_step as a wrapper is good for code clarity.
* I tighten the EncodingDetectionResult tio be a MetaDefacto with defacto as the current enum.
  And meta as the stuff the monadic variant needs for its OK message.

I did the change:

```c++
    struct EncodingDetectionMeta {
      CanonicalEncodingName canonical_name; // ICU canonical name (e.g., "UTF-8")
      std::string display_name;             // Human readable name
      int32_t confidence;                   // ICU confidence (0-100)
      std::string language;                 // Detected language (e.g., "sv" for Swedish)
      std::string detection_method;         // "ICU", "BOM", "Extension", "Default"
    };

    using EncodingDetectionResult = MetaDefacto<EncodingDetectionMeta,DetectedEncoding>;
```

With the plan to refactor the code guided by compiler errors. This worked until I got confuded by:

```sh
... error: no matching function for call to object of type '(lambda at /Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/test/test_csv_import_pipeline.cpp:582:19)'
  343 |     _NOEXCEPT_(noexcept(static_cast<_Fp&&>(__f)(static_cast<_Args&&>(__args)...)))
```

at:

```c++
...
        .and_then([](auto buffer) {
          AnnotatedMaybe<text::encoding::icu_facade::EncodingDetectionResult> encoding_result;
          auto maybe_encoding = text::encoding::icu_facade::maybe::to_detetced_encoding(buffer);
          if (maybe_encoding) {
            encoding_result.m_value = *maybe_encoding;
            encoding_result.push_message(
              std::format("Detected encoding: {} (confidence: {})",
                         maybe_encoding->meta.display_name,
                         maybe_encoding->confidence));
          } else {
            encoding_result.push_message("Failed to detect encoding");
          }
          return encoding_result;
        });

```

How can I understand the compiler error and how to fix it?

After having chatted with my AI friends I get the notion that this is a 'red herring' caused by template 'subsistution is not a failure (SFINAE)' event. The lambda to and_then is treated as an 'auto type' and thus as a template. So according to SFINAE it is not an error for the lamda to not compile. And text::encoding::icu_facade::maybe::to_detetced_encoding does actually NOT compile with the change to the EncodingDetectionResult!

It is PAIN to be a C++ programmer!

I came up with the idea to change the compilation order to have the definition (the cpp file) be compiled before code that does template insrtantiation based on the changed code.

Then I thought about it again. And started to wonder what the function definition had to do with the template instantiation failure?

So I commented out the suspoicious code in the lamda. And the compiler error shifted to another location. By add code back in I identified the culprit as:

```c++
  encoding_result.push_message(
    std::format("Detected encoding: {} (confidence: {})",
                maybe_encoding->meta.display_name,
                maybe_encoding->confidence));
```

And sure enough, this can't comopile with the changed type! I fixed to 'maybe_encoding->meta.confidence' and carried on.

I made progress but all manual editing got tedious. I tried chatGPT to edit a code snippet for me. I prompted:

```text
I have '            return EncodingDetectionResult{
              .meta = {
                 "UTF-16LE"
                ,"UTF-16 Little Endian"
                ,100
                ,""
                ,"BOM"
              }
              ,.defacto = DetectedEncoding::UTF16LE
            };'. Can you help me format 'return {DetectedEncoding::Unknown, "", "Unknown", 0, "", "No BOM"};' the same way?
```

It returned:

```text
return EncodingDetectionResult{
  .meta = {
     "Unknown"
    ,"Unknown"
    ,0
    ,""
    ,"No BOM"
  }
  ,.defacto = DetectedEncoding::Unknown
};
```

Which is NOT correct!

So I conclude I can NOT trust my AI friends with changes that requires 100% adherence to code correctness!

After all it did not take me so long to refactor the code manually. But now I got suspisous about:

```c++
  auto encoding_result = icu_facade::maybe::to_detetced_encoding(buffer, confidence_threshold);
  ...
  result = WithDetectedEncodingByteBuffer{
      .meta = encoding_result->defacto
    ,.defacto = std::move(wt_buffer.defacto)
  };
```

How can '.meta = encoding_result->defacto' compile?

* WithDetectedEncodingByteBuffer is:

```c++
  using WithDetectedEncodingByteBuffer = MetaDefacto<DetectedEncoding,ByteBuffer>;
```

* And encoding_result->defacto is DetectedEncoding

Oh! I see. I have not changed WithDetectedEncodingByteBuffer yet!

Ok, time to have WithDetectedEncodingByteBuffer carry the whole EncodingDetectionResult as 'meta'.

I made the change:

```c++
  // using WithDetectedEncodingByteBuffer = MetaDefacto<DetectedEncoding,ByteBuffer>;
  using WithDetectedEncodingByteBuffer = MetaDefacto<icu_facade::EncodingDetectionResult,ByteBuffer>;
```

And got stumped by a template instantiation compilation error like:

```sh
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.2.sdk/usr/include/c++/v1/__type_traits/invoke.h:459:1: error: no type named 'type' in 'std::invoke_result<cratchit::functional::AnnotatedOptional<std::string> &, MetaDefacto<MetaDefacto<text::encoding::icu_facade::EncodingDetectionMeta, text::encoding::DetectedEncoding>, std::vector<std::byte>> &&>'
  459 | using invoke_result_t = typename invoke_result<_Fn, _Args...>::type;
      | ^~~~~

  ...

/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/text/encoding_pipeline.hpp:56:10: note: in instantiation of function template specialization 'cratchit::functional::AnnotatedOptional<MetaDefacto<MetaDefacto<text::encoding::icu_facade::EncodingDetectionMeta, text::encoding::DetectedEncoding>, std::vector<std::byte>>>::and_then<cratchit::functional::AnnotatedOptional<std::string> &>' requested here
   56 |         .and_then(monadic::to_platform_encoded_string_step); // #5
      |          ^
```

I feel it is time I learn to read these errors!

If I expand the code that is mine I can see:

```c++
      return persistent::in::path_to_byte_buffer_shortcut(file_path) // #1 + #2
        .and_then(monadic::to_with_threshold_step_f(confidence_threshold))
        .and_then(monadic::to_with_detected_encoding_step) // #4
        .and_then(monadic::to_platform_encoded_string_step); // #5
```

And I know the return type of to_with_detected_encoding_step has changed (as I changed the WithDetectedEncodingByteBuffer).

So how is to_platform_encoded_string_step declared?

Oh! I broke the declaration when I split it into hpp declaration and cpp definition!

Then I ran into this problem:

```c++
  // Default to UTF-8 on detection failure (permissive strategy)
  result = WithDetectedEncodingByteBuffer{
    .meta = DetectedEncoding::UTF8
    ,.defacto = std::move(wt_buffer.defacto)
  };
```

The previous code only passed on the default enum to signal UTF-8. Now I need to somehow provide also the meta-part.

```c++
    struct EncodingDetectionMeta {
      CanonicalEncodingName canonical_name; // ICU canonical name (e.g., "UTF-8")
      std::string display_name;             // Human readable name
      int32_t confidence;                   // ICU confidence (0-100)
      std::string language;                 // Detected language (e.g., "sv" for Swedish)
      std::string detection_method;         // "ICU", "BOM", "Extension", "Default"
    };
```

Or do I really? None of the upstream code actually uses it!

* Can I provide an empty one?
* Can I make the meta into a maybe?

If I provide an empty one I break the invariant that the defacto enum with the encoding must match the meta data for that enum. But then, do my existing AI generayed code even care? That is, have I already code that breaks this invariant?

If I make the meta into a maybe I probably don't solve the problem as all code that access the meta must then also handle the nullopt. But then, this is also maybe not a problem? Maybe every such case is paired with an enum 'Undefied or Unknown'?

But I extended the type to make maybe version signal mneta-data that variant version used. I did NOT extend it to provide meta-data upstream.

* So we know that for now it is fine to break the invariant upstream as it does not access the meta part.
* But future code may be tricked to access it and cause buds when the invariant is broken!

This was a curious dilemma. Where have I gone wrong? Or hove can I proceed correctly?

I wonder, how many hard coded 'default value' sites do I have? 

AHA! It seems there is only one!?

```c++
        else {
          // Default to UTF-8 on detection failure (permissive strategy)
          result = WithDetectedEncodingByteBuffer{
            .meta = DetectedEncoding::UTF8
            ,.defacto = std::move(wt_buffer.defacto)
          };
          result.push_message(
            std::format(
              "Encoding detection failed (confidence below threshold {}), defaulting to UTF-8"
              ,confidence_threshold));
        }

```

And from below we have the productions:

```c++
  // ...

          // UTF-8 BOM: EF BB BF
          if (bom_bytes[0] == 0xEF && bom_bytes[1] == 0xBB && bom_bytes[2] == 0xBF) {
            return EncodingDetectionResult{
              .meta = {
                 "UTF-8"
                ,"UTF-8"
                ,100
                ,""
                ,"BOM"
              }
              ,.defacto = DetectedEncoding::UTF8
            };

  // ...

        if (ext == ".csv") {
          return EncodingDetectionResult{
            .meta = {
              "UTF-8"
              ,"UTF-8"
              ,60
              ,""
              ,"Extension (.csv)"
            }
            ,.defacto = DetectedEncoding::UTF8
          };

  // ...

        return EncodingDetectionResult{
          .meta = {
            "UTF-8"
            ,"UTF-8"
            ,30
            ,""
            ,"Extension (default)"
          }
          ,.defacto = DetectedEncoding::UTF8
        };
        
```

So my AI friends has already generated hard coded meta-data.

It is also integersting that the propblem with the semantics of 'confidence' creeps up here again. My AI friends has stated 100% confidence from BOM detection, 60% confidence from '.csv' extension and 30% confidence for 'other file extension' assumed encoding?

This is LEAKY AF!

On the other hand, I don't make it worse? I did this:

```c++
          // Default to UTF-8 on detection failure (permissive strategy)
          result = WithDetectedEncodingByteBuffer{
            .meta = text::encoding::icu_facade::EncodingDetectionResult{
              .meta = {
                "UTF-8"
                ,"UTF-8"
                ,100
                ,""
                ,"Assumed"
              }
              ,.defacto = DetectedEncoding::UTF8
            }
            ,.defacto = std::move(wt_buffer.defacto)
          };

```

This makes the code compile for now. Also, no encoding tests fail so I asume we can settle woth this for now?

I now have implemented the maybe and monadic versions.

```c++

      std::optional<WithDetectedEncodingByteBuffer> to_with_detected_encoding_step(WithThresholdByteBuffer wt_buffer) {
        std::optional<WithDetectedEncodingByteBuffer> result{};

        auto& [confidence_threshold,buffer] = wt_buffer;
        auto encoding_result = icu_facade::maybe::to_detetced_encoding(buffer, confidence_threshold);

        if (encoding_result) {      
          result = WithDetectedEncodingByteBuffer{
             .meta = encoding_result.value()
            ,.defacto = std::move(buffer)
          };
        } 

        return result;
      }

      // ...

      AnnotatedMaybe<WithDetectedEncodingByteBuffer> to_with_detected_encoding_step(WithThresholdByteBuffer wt_buffer) {

        AnnotatedMaybe<WithDetectedEncodingByteBuffer> result{};

        auto const& [confidence_threshold,buffer] = wt_buffer;
        auto lifted = cratchit::functional::to_annotated_maybe_f(
           text::encoding::maybe::to_with_detected_encoding_step
          ,std::format(
              "Encoding detection failed (confidence below threshold {}), defaulting to UTF-8"
              ,confidence_threshold)
        );

        result = lifted(wt_buffer);

        if (result) {
          result.push_message(
            std::format("Detected encoding: {} (confidence: {}, method: {})",
                      result.value().meta.meta.display_name,
                      result.value().meta.meta.confidence,
                      result.value().meta.meta.detection_method)
          );
        }
        else {
          // Default to UTF-8 on detection failure (permissive strategy)
          // TODO 20260124 - Consider to remove this else?
          //                 It seems no test even triggers this else path?
          //                 Or, the detection logic already defaults to UTF-8 (Never nullopt)?
          result = WithDetectedEncodingByteBuffer{
            .meta = text::encoding::icu_facade::EncodingDetectionResult{
              .meta = {
                "UTF-8"
                ,"UTF-8"
                ,100
                ,""
                ,"Assumed"
              }
              ,.defacto = DetectedEncoding::UTF8
            }
            ,.defacto = std::move(wt_buffer.defacto)
          };
        }

        return result;

      } // to_with_detected_encoding_step

```

I then made the maybe version into a pure monadic composition as:

```c++
      std::optional<WithDetectedEncodingByteBuffer> to_with_detected_encoding_step(WithThresholdByteBuffer wt_buffer) {
        auto& [confidence_threshold,buffer] = wt_buffer;
        return icu_facade::maybe::to_detetced_encoding(buffer, confidence_threshold)
          .transform([buffer = std::move(buffer)](auto&& meta){
            return WithDetectedEncodingByteBuffer{
              .meta = std::forward<decltype(meta)>(meta)
              ,.defacto = std::move(buffer)
            };
          });
      } // to_with_detected_encoding_step
```

* I move-capture the buffer into the lambda
* I already move-from the meta into the lambda
* The lambda forwards the meta (to take advantage of any move-assignment)
* And the lamda already move the buffer to the result

It seems I have all the bells and whistles in place correctly? At least all test run without any excpetions or faults. Even in the debugger I get no catch on any faults or memory issues.

It is worth noting though that none of the tests triggered the else that hard codes the moinadic default to UTF-8.

* Does the detection code already default to UTF-8?
* Are there no tets on the fallback to UTF-8?

## 20260123

So next step is to make a maybe variant of to_with_threshold_step.

I studied the code and got confused by icu::to_detetced_encoding. I imagined this was from the ICU library. But it was not. It was my own icu namespace. Ok, so I renamed my 'namespace icu' to 'namespace icu_facade'.

But now my code fail on 'icu_facade::UnicodeString'. It turns out the ICU library DOES itself define an icu namespace! So good thing I renamed my own to not get confused in the future.

So I fixed all places where the code was using actual ICU code as icu:: ok.

So I feel I have to tread carefully in this step of refactoring. I started by clarifying existing 'maybe' returning functions in encoding unit. I introduced icu_facade::maybe and moved them there.

I continued to implement maybe-variant of to_with_threshold_step. ANd in the process I renamed 'to_annotated_nullopt' to 'to_annotated_maybe_f' ro reflect it takes a maybe-returing function and produces a function that returns an annotated maybe.

I managed to implement text::encoding::maybe::to_with_threshold_step_f so I could do:

```c++
      auto result = persistent::in::maybe::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::maybe::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::maybe::to_with_threshold_step_f(100));
```

I also managed to make monadic variant lift the maybe-variant as:

```c++
      AnnotatedMaybe<WithThresholdByteBuffer> ToWithThresholdF::operator()(ByteBuffer byte_buffer) const {
        auto lifted = cratchit::functional::to_annotated_maybe_f(
           text::encoding::maybe::to_with_threshold_step_f(confidence_threshold)
          ,"Failed to pair confidence_threshold with byte buffer"
        );
        return lifted(byte_buffer);
      }
      ToWithThresholdF to_with_threshold_step_f(int32_t confidence_threshold) {
        return ToWithThresholdF{confidence_threshold};
      }
```

That is, I made the indirection functor ToWithThresholdF use to_annotated_maybe_f to lift the maybe variant with an annotation and call it.

It would have been nice if I could implement to_with_threshold_step_f return the to_annotated_maybe_f from maybe variant directly. But then I don't know the return type any longer. And tgus can't separate declaration from the definition.

I wonder it there is some C++ template magic I can use to create the return type of to_annotated_maybe_f and use in declaration and definition of to_with_threshold_step_f?

I asked chatGPT and it sugested:

```c++
namespace cratchit::functional {

template<typename F>
using annotated_maybe_f_t =
  decltype(to_annotated_maybe_f(
    std::declval<F>(),
    std::declval<std::string>(),
    std::declval<std::string>()
  ));

} // namespace cratchit::functional

// and usage

namespace text::encoding::monadic {

  using MaybeStep =
    cratchit::functional::annotated_maybe_f_t<
      decltype(text::encoding::maybe::to_with_threshold_step_f(0))
    >;

  MaybeStep to_with_threshold_step_f(int32_t confidence_threshold);

} // namespace text::encoding::monadic
```

At first I thought this was a catch 22. But then I realised the 'monadic' variant uses the 'maybe' variant of to_with_threshold_step_f.

So I wrote this code for the monaic variant:

```c++

      // hpp
      using ToWithThresholdF =
        cratchit::functional::annotated_maybe_f_t<
          decltype(text::encoding::maybe::to_with_threshold_step_f(0))
        >;

      ToWithThresholdF to_with_threshold_step_f(int32_t confidence_threshold);    

      // cpp
      ToWithThresholdF to_with_threshold_step_f(int32_t confidence_threshold) {   
        auto lifted = cratchit::functional::to_annotated_maybe_f(
           text::encoding::maybe::to_with_threshold_step_f(confidence_threshold)
          ,"Failed to pair confidence_threshold with byte buffer"
        );
        return lifted;
      }
```

So at least I got rid of the functor indirection and made the ToWithThresholdF be the lifted function itself.

## 20260122

I think I have found a viable path though this mess of bloated and unorganised code I have.

With the test case PathToAccountStatementTaggedAmountsRefactoring3 I can focus on making the whole operation path -> tagged amounts as an and_then based on AnnotatedMaybe<T>.

So far I have managed to fill in previous gaps and now have:

```c++
      auto maybe_account_id_ed_table = persistent::in::monadic::path_to_istream_ptr_step(m_valid_file_path)
        .and_then(persistent::in::monadic::istream_ptr_to_byte_buffer_step)
        .and_then(text::encoding::monadic::to_with_threshold_step(100))
        .and_then(text::encoding::monadic::to_with_detected_encoding_step)
        .and_then(text::encoding::monadic::to_platform_encoded_string_step)
        .and_then(CSV::parse::monadic::csv_text_to_table_step)
        .and_then(account::statement::monadic::to_account_id_ed_step);
```

To get to tagged amounts I now need to tackle the 'tas::csv_table_to_tagged_amounts_shortcut(identified_table, account_id);'.

* It is a shortcut (meaning it aggregates steps)
* It takes two arguments (so can not work with my existing AnnotatedMaybe<T>::and_then(arg)).

* It seems we need to aggregate identified_table, account_id into one 'thing'. A MetaDefacto seems a viable approach.
* The refactor account::statement::maybe::csv_table_to_account_statement_step(table, account_id)

NO!

* The account::statement::maybe::csv_table_to_account_statement_step(table, account_id) is to be removed.
* I already have the next steps.

Satying really focused I found the steps I wanted to use and now have the tail:

```c++
    auto maybe_account_statement = maybe_account_id_ed_table
      .and_then(cratchit::functional::to_annotated_nullopt(
          account::statement::maybe::account_id_ed_to_account_statement_step
        ,"Account ID.ed table -> account statement failed"));

    auto maybe_tagged_amounts = maybe_account_statement
      .and_then(cratchit::functional::to_annotated_nullopt(
          tas::maybe::account_statement_to_tagged_amounts_step
        ,"Failed to transform Account Statement to Tagged Amounts"));

    ASSERT_TRUE(maybe_tagged_amounts) << "Expected succesfull tagged amounts";
```

I'm getting there! And it works to focus on the PathToAccountStatementTaggedAmountsRefactoring3 until I have the whole path -> tagged amounts based on AnnotatedMaybe<T>! Even with it I get lost over and over.

I have now stayed focused and made the AnnotatedMaybe<T> chain complete:

```c++
    auto maybe_tagged_amounts = persistent::in::monadic::path_to_istream_ptr_step(m_valid_file_path)
      .and_then(persistent::in::monadic::istream_ptr_to_byte_buffer_step)
      .and_then(text::encoding::monadic::to_with_threshold_step(100))
      .and_then(text::encoding::monadic::to_with_detected_encoding_step)
      .and_then(text::encoding::monadic::to_platform_encoded_string_step)
      .and_then(CSV::parse::monadic::csv_text_to_table_step)
      .and_then(account::statement::monadic::to_account_id_ed_step)
      .and_then(account::statement::monadic::account_id_ed_to_account_statement_step)
      .and_then(tas::monadic::account_statement_to_tagged_amounts_step);
```

Now I think it is time to clean upp so that we also have this chain on std::optional?

We do not yet have a  maybe varaint of path_to_istream_ptr_step(path).

So I have now implemented persistent::in::maybe::path_to_istream_ptr_step. I ran into test cases that now fails as they expect also successfull operations to inject side channel messages. So I made to_annotated_nullopt to take an optional message_on_value and made 'open file' log an 'opened' ok message to make CompleteIntegrationAllSteps test pass.

I am on the way OK. But we are in the weeds in more ways.

* Google tests also executes cratchit normal 'read environment' and sync with sie-files.
* And this sync FAILS with:

```sh
...
BEGIN: model_from_environment_and_md_filesystem 
BEGIN REFACTORED posted SIE digest

STAGE of cracthit entries FAILED when merging with posted (from external tool)
Entries in sie-file:sie_in/TheITfiedAB20250812_145743.se overrides values in cratchit staged entries
CONFLICTED! : No longer valid entry  A3 "Account:NORDEA Text:BG KONTOINS Message:5050-1030 SK5567828172" 20250516
    1630 "" -1998.00
    1920 "" 1998.00
 year id:current - ALL POSTED OK
END REFACTORED posted SIE digest
SIE year id:current --> Tagged Amounts = size:9
Account Statement Files --> Tagged Amounts = size:9
[       OK ] ModelTests.Environment2ModelSIEStageOverlapConflictTest (5 ms)
...
```

What is going on?

* Is this triggered by Environment2ModelSIEStageOverlapConflictTest?

YES! 

* So the test needs to be updated to do semthing with the 'conflict'?

Also, a normal start of cratchit seems ok enough for now:

```sh
...
BEGIN: model_from_environment_and_md_filesystem 
BEGIN REFACTORED posted SIE digest
 year id:-2 - ALL POSTED OK
 year id:-1 - ALL POSTED OK
 year id:current - ALL POSTED OK
END REFACTORED posted SIE digest
SIE year id:-1 --> Tagged Amounts = size:440
SIE year id:-2 --> Tagged Amounts = size:889
SIE year id:current --> Tagged Amounts = size:1073
Account Statement Files --> Tagged Amounts = size:1073
Init from "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/cratchit.env"
cratchit:>
```

Back to my refactoring. Next step is to implement a maybe variant of 'istream_ptr_to_byte_buffer_step'.

I am a bit unsatisfied wigh the code options I found for 'proper' reading of bytes from std::istream. I used charGPT and Claude Code to serve me code snippets and I chose one but added three options in the code for future refactoring.

It seems my AI coding friends are also confused aabout error handling in reading from std::istream. When I fed code back they changed their minds and proposed more safety nets and tweak.

I tweaked std::istream reading  with placeholder code and TODO about returning std::expected instead for better error tracking.

The code should be stable against error for now I suppose?

## 20260121

I am frustrated that fixing the account statement csv file parsing pipe line is so hard! I have a real problem to navigate the source code and see though the mess what is there, what is AI slop and what is redundancy.

I have come up with an approach to try and navigate to a slimmer and focused cleaned up design.

1. Identify the pipe line that is actually in use.
2. Write down this pipe line in a test case to fix it in code for refactoring stability.
3. Propose a new pipe line where Account ID is produced based on new account statement table meta analysis.
4. Wite down the new pipe line in a test case to fix it in code for refactoring stability.
5. Replace old pipe line with the new one.
6. Remove redunant code and test cases based on new pipe line test case.

### So where is the current pipe line?

I found one candidate in tas_from_statment_file:

```c++
OptionalTaggedAmounts tas_from_statment_file(std::filesystem::path const& statement_file_path) {

  if (true) {
    std::cout << "\nto_tagged_amounts(" << statement_file_path << ")";
  }

  if (true) {
    // Refactored to pipeline

    auto result = file_path_to_istream(statement_file_path)
      .and_then(istream_to_decoding_in)
      .and_then(decoding_in_to_field_rows)
      .and_then(field_rows_to_table)
      .and_then(table_to_account_statements)
      .and_then(account_statements_to_tas);
```

I am surprised this pipe line is not made up my xxx_step named functions.

I also found:

```c++
  TEST_F(MonadicCompositionFixture,PathToAccountIDedTable) {
    logger::scope_logger log_raii(logger::development_trace,"TEST_F(MonadicCompositionFixture,PathToAccountIDedTable)");

    auto maybe_account_id_ed_table = persistent::in::monadic::path_to_istream_ptr_step(m_valid_file_path)
      .and_then(persistent::in::monadic::istream_ptr_to_byte_buffer_step)
      .and_then([](auto const& byte_buffer){
        return text::encoding::monadic::to_with_threshold_step(100,byte_buffer);
      })
      .and_then(text::encoding::monadic::to_with_detected_encoding_step)
      .and_then(text::encoding::monadic::to_platform_encoded_string_step)
      .and_then(cratchit::functional::to_annotated_nullopt(
        CSV::parse::maybe::csv_text_to_table_step
        ,"Failed to parse csv into a valid table"))
      .and_then(cratchit::functional::to_annotated_nullopt(
          account::statement::maybe::to_account_id_ed_step
        ,"Failed to identify account statement csv table account id"
      ));

    ASSERT_TRUE(maybe_account_id_ed_table) << "Expected successful account ID identification";
  }
```

So what pipe line is used by first variant fron end?

So AccountStatementFileState uses CSV::parse::monadic::file_to_table.

And then it provides an option table -> maybe account statement as:

```c++
  auto annotated_statement = annotated_table
    .and_then(to_annotated_nullopt(
      account::statement::maybe::to_account_id_ed_step,
      "Unknown CSV format - could not identify account"))
    .and_then(to_annotated_nullopt(
      account::statement::maybe::account_id_ed_to_account_statement_step,
      "Could not extract account statement entries"));

```

That is table -> id:ed table -> statement using the 'annotated' pipeline.

I am starting to see some pieces in the puzzle.

* We have some steps based on std::optional and some on leverage 'AnnotatedMaybe'
* We are using steps that are not yet named xxx_step for some reason
* We have no test case for the whole pipline file path -> tagged amounts
* We have disorder and confison in file naming and locations
* We have simmilar disorder and confusion about test cases and locations of those.

So where can I place the test case to achor the file path -> tagged amounts full pipe line?

* TEST_F(MonadicCompositionFixture,PathToAccountIDedTable) is in test_csv_import_pipeline
* TEST(AccountStatementDetectionTests, DetectColumnsFromNordeaHeader) is in test_csv_table_identification
* TEST_F(MDTableToAccountStatementTestFixture, SKVNewerMDTableToAccountStatement) is in test_md_table_to_account_statement

Wow, it is really 'upp hill'!!

But the namespace namespace tests::csv_import_pipeline::monadic_composition_suite seems the best candidate for now. It is in test_csv_import_pipeline.

* So first front end has not yet any account statement -> tagged amounts in the ux

Now I found a TODO comment for 'tas_from_statment_file' in TaggedAmountFramework. It states the current pipe line is 'csv::path_to_tagged_amounts_shortcut in csv/import_pipeline.hpp'.

So tas_from_statment_file is not used? Yes, it is NOT called from anywhere!

So what does path_to_tagged_amounts_shortcut do?






## How can I parse account statement csv files in a somewhat generic manner [20260120]?

I decided to continue my thinking here on top (See thinging until now below).

I am back at the plan:

1. Extend 'ColumnMapping' to a 'CSV::TableMeta' with more meta-data from identification process
2. Introduce an array of sz_xxx texts to test to parse into account statement tables
3. Extend 'coumn mapping' to also identify BBAN, BG and PG account number fields
4. Constrain 'header' to be valid only if header has the same coumn count as the in-saldo,out-saldo and trans entry rows

Where I am in the process of replacing to_column_mapping with to_account_statement_table_meta.

I have now decided to clean up the failing tests before I continue.

```sh
[  FAILED  ] 4 tests, listed below:
[  FAILED  ] MonadicCompositionFixture.PathToAccountIDedTable
[  FAILED  ] GenericStatementCSVTests.NORDEAStatementOk
[  FAILED  ] AccountStatementDetectionTests.GeneratedSingleRowMappingTest
[  FAILED  ] AccountStatementDetectionTests.GeneratedRowsMappingTest
```

This is a MESS!!

The tests are AI slop hard coded to fail if not a NORDEA or SKV format. So If I make PathToAccountIDedTable pass by e.g., making to_account_id_ed_step return some generic ID, then I fail tests:

```sh
[  FAILED  ] AccountIdTests.UnknownCsvReturnsNullopt
[  FAILED  ] AccountIdTests.TableWithOnlyHeadingUnknownFormatReturnsNullopt
[  FAILED  ] FullPipelineTestFixture.ImportSimpleCsvFailsForUnknownFormat
[  FAILED  ] FullPipelineTableTests.ImportTableFailsForUnknownFormat
[  FAILED  ] GenericStatementCSVTests.NORDEAStatementOk
```

*sigh*

So what is the work around for now? I wonder where I broke these tests in the first case? I actually can't figure out where and when I could have made thes ID.ed mechanism stop working?

Maybe if I fix test GenericStatementCSVTests.NORDEAStatementOk? Why does it fail?

It calls account::statement::maybe::to_entries_mapped(*maybe_table). What the heck is that?

Now I am confused. Did I not just add something simmilar? Where is the latest mechanism I added?

DARN! I have the new code (src/domain/csv_to_account_statement.hpp):

```c++
  enum class FieldType {
      Unknown
    ,Empty
    ,Date
    ,Amount
    ,Text 
    ,Undefined
  };

  struct RowMap {
    std::map<FieldType,std::vector<unsigned>> ixs;
  };

  using RowsMap = std::vector<RowMap>;

```

And I have the 'older' code (src/csv/csv_to_account_id.hpp):

```c++
  enum class ColumnType {
      Undefined
    ,Empty
    ,Text
    ,Amount
    ,Date
    ,Unknown
  }; // ColumnType

  // ...

  using ColumnMap = std::map<int,ColumnType>;

  enum class EntryType {
    Undefined
    ,Emtpy
    ,Caption
    ,Heading
    ,Transaction
    ,Balance
    ,Unknown
  };
```

So we have compeeting code?

```c++
* ColumnMapping to_column_mapping(CSV::Table const& table)
```

And: 

```c++
* EntryMap to_entry_map(CSV::Table::Row const& row)
* EntryType to_entry_type(EntryMap const& entry_map)
* std::optional<CSV::MDTable<EntryMaps>> to_entries_mapped(CSV::Table const& table)
```

AND: to_entries_mapped is not even implemented yet!!

I asked Claude Code to help me find the two approaches in the git log. It came back to me with:

```sh
You have two similar but distinct functions for analyzing CSV table structure:

  1. to_entries_mapped (src/csv/csv_to_account_id.hpp:304-340)

  - Introduced: December 7, 2025 in commit 4a51390
  - Purpose: Maps each CSV table row to entry types (Transaction, Balance, Heading, etc.) with column type information
  - Status: Incomplete - returns empty result (return {}; // failed at line 339)
  - Location: account::statement::generic namespace

  2. to_column_mapping → to_account_statement_table_meta

  - Introduced: January 15, 2026 in commit 827f2ad
  - Evolved: Replaced with to_account_statement_table_meta on January 20, 2026 (commit f1fb929)
  - Purpose: Detects column mappings (date, amount, description columns) from CSV table
  - Status: Fully implemented and in use in the CSV import pipeline
  - Location: src/domain/csv_to_account_statement.cpp

  Key Commits

  # Original to_entries_mapped implementation
  git show 4a51390

  # Original to_column_mapping implementation  
  git show 827f2ad

  # Evolution to to_account_statement_table_meta
  git show 6543d17
  git show f1fb929

  The Similarity

  Both functions analyze CSV table structure, but:
  - to_entries_mapped: Classifies each row by type (Transaction/Balance/etc.) and identifies column types
  - to_column_mapping: Identifies which columns contain date/amount/description data
```

OK. So maybe the december approach with 'to_entries_mapped' is now DEAD? And I can remove it?

* It is not implemented.
* And it is not used.

## How can I parse account statement csv files in a somewhat generic manner?

20260114

The account statement csv file parsing is a MESS due to AI slop. That said, I still have not solved how to parse the 'new' SKV account statemend csv file.

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit/workspace/from_bank_or_skv % cat Bokförda\ transaktioner\ 556782-8172\ Alla\ typer\ 2025-10-01--2025-12-31.csv
"THE ITFIED AB";"556782-8172";"";""
"";"";"";""
"";"Ingående saldo 2025-10-01";"";"660"
"2025-10-04";"Intäktsränta";"1";"661"
"2025-11-12";"Moms juli 2025 - sept 2025";"1 997";"2 658"
"2025-11-14";"Utbetalning";"-1 997";"661"
"2025-12-06";"Intäktsränta";"1";"662"
"";"Utgående saldo 2025-12-31";"";"662"
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit/workspace/from_bank_or_skv % 
```

I now have a 'mapper' that logs:

```sh
[20:46:23.238]               TRACE: BEGIN detect_columns_from_data
[20:46:23.238]                 TRACE: row:﻿"THE ITFIED AB"^556782-8172^^ map: Unknown: 1 Empty: 2 3 Text: 0
[20:46:23.238]                 TRACE: row:^^^ map: Empty: 0 1 2 3
[20:46:23.238]                 TRACE: row:^Ingående saldo 2025-10-01^^660 map: Empty: 0 2 Date: 1 Amount: 3
[20:46:23.238]                 TRACE: row:2025-10-04^Intäktsränta^1^661 map: Date: 0 Amount: 2 3 Text: 1
[20:46:23.238]                 TRACE: row:2025-11-12^Moms juli 2025 - sept 2025^1 997^2 658 map: Date: 0 Amount: 2 3 Text: 1
[20:46:23.238]                 TRACE: row:2025-11-14^Utbetalning^-1 997^661 map: Date: 0 Amount: 2 3 Text: 1
[20:46:23.238]                 TRACE: row:2025-12-06^Intäktsränta^1^662 map: Date: 0 Amount: 2 3 Text: 1
[20:46:23.238]                 TRACE: row:^Utgående saldo 2025-12-31^^662 map: Empty: 0 2 Date: 1 Amount: 3
[20:46:23.238]                 TRACE: returns mapping.is_valid:false
[20:46:23.238]               TRACE: END detect_columns_from_data
```

* I still have the BOM (BOM not rempoved by file reader)
* The 'to_string' produce a starnge '^'-separated string (log uses the to_string for persistent cintent format)
* The 'Ingående saldo 2025-10-01' is parsed as a valid amount

That said, I DO get viable results for the entries of interest for now.

```sh
[20:46:23.238]                 TRACE: row:2025-10-04^Intäktsränta^1^661 map: Date: 0 Amount: 2 3 Text: 1
[20:46:23.238]                 TRACE: row:2025-11-12^Moms juli 2025 - sept 2025^1 997^2 658 map: Date: 0 Amount: 2 3 Text: 1
[20:46:23.238]                 TRACE: row:2025-11-14^Utbetalning^-1 997^661 map: Date: 0 Amount: 2 3 Text: 1
[20:46:23.238]                 TRACE: row:2025-12-06^Intäktsränta^1^662 map: Date: 0 Amount: 2 3 Text: 1
```

So how can I identify what amount is 'transaction' and what is 'saldo'?

```sh
2025-11-12^Moms juli 2025 - sept 2025^1 997^2 658 map: Date: 0 Amount: 2 3 Text: 1
```

* Date in column 0
* Text in column 1
* Amounts in coulmn 2 and 3.

Should I first identify a 'key' to pick out only the entries we are to transform into 'account statement entries'?

If so, I may be tempted to define a key as 'map: Date: 0 Amount: 2 3 Text: 1'?

But then, should I first find out what amount is what? That is, keep the transaction amount and ignore the saldo amount for now?

Should I also write tests to keep me focused and get reliable code?

YES, I should write tests. Let's do do this properly this time!

1. Design an algorithm that maps a csv table into a map that projects the table to row ixs to project and a 'key' that defines column ix for Heading (Text), Amount (transaction amount) and Date
2. Write tests for this algorithm sub-steps and final result tests on example csv input
3. Integrate the new algorithm into existing 'account statement csv file import' pipe line.

So what algorithm can I imagine to parse a csv table into account statment entries?

* Is the current pipe line row -> account statement -> tagged amount?

  - extract_entry_from_row(row,mapping) return an std::optional AccountStatementEntry.
  - extract_entry_from_row Tests InvalidDateHandledGracefully,InvalidAmountHandledGracefully,EmptyDescriptionHandledGracefully
  - csv_table_to_account_statement_entries(table) returns OptionalAccountStatementEntries
  - csv_table_to_account_statement_step and account_id_ed_to_account_statement_step calls csv_table_to_account_statement_entries

  WHAT A MESS! (AI slop)

  Ok, lets clear this up. What pipeline can we keep and focus on?

  Where exactly is the pipe line we actually use located? With an SKV and a NORDEA account stetment file in 'from_bank_or_skv' cratchit logs processing as:

  <pre>
      BEGIN File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv"
      [Pipeline] Successfully opened file: /Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv
      [Pipeline] Successfully read 321 bytes from stream
      [Pipeline] Detected encoding: UTF-8 (confidence: 100, method: ICU)
      [Pipeline] Successfully transcoded 321 bytes to 321 platform encoding bytes
      [Pipeline] Step 6 complete: CSV parsed successfully (8 rows)
      [Pipeline] Step 6.5 complete: AccountID detected: 'SKV::5567828172'
      [Pipeline] Pipeline failed at Steps 7-8: Domain transformation failed - Could not extract tagged amounts
    *Note* "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv" (produced zero entries)
    *Ignored* "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv" (File empty or failed to understand file content)
    END File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/Bokförda transaktioner 556782-8172 Alla typer 2025-10-01--2025-12-31.csv"

    BEGIN File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv"
      [Pipeline] Successfully opened file: /Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv
      [Pipeline] Successfully read 1194 bytes from stream
      [Pipeline] Detected encoding: UTF-8 (confidence: 100, method: ICU)
      [Pipeline] Successfully transcoded 1194 bytes to 1194 platform encoding bytes
      [Pipeline] Step 6 complete: CSV parsed successfully (12 rows)
      [Pipeline] Step 6.5 complete: AccountID detected: 'NORDEA::32592317244'
      [Pipeline] Pipeline complete: 11 TaggedAmounts created from 'PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv'
      Valid entries count:11
      Consumed account statement file moved to "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/consumed/PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv"
    END File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/PLUSGIROKONTO FTG 51 86 87-9 - 2026-01-12 11.22.11.csv"
</pre>

* The log 'Step 6.5 complete: AccountID detected:' is created in:
  - table_to_tagged_amounts_shortcut
  - csv_to_tagged_amounts_shortcut
  - path_to_account_statement_shortcut
  - path_to_tagged_amounts_shortcut

WHAT A MESS (AI Slop)

So which one is actually used?

It is 'path_to_tagged_amounts_shortcut' that is used!

model_with_dotas_from_account_statement_files
  -> dotas_from_account_statement_files
  -> tas_sequence_from_consumed_account_statement_files
  -> tas_sequence_from_consumed_account_statement_file
  -> path_to_tagged_amounts_shortcut

As part of 'path_to_tagged_amounts_shortcut' we have the pipe line:

* path to maybe_table
* account::statement::maybe::to_account_id_ed_step(*maybe_table)
* csv_table_to_tagged_amounts_shortcut(CSV::Table const& table,AccountID const& account_id)

Another short cut! Anyhow...

As part of 'csv_table_to_tagged_amounts_shortcut(CSV::Table const& table,AccountID const& account_id)' we have:

* csv_table_to_account_statement_step(table, account_id)
* account_statement_to_tagged_amounts_step(*maybe_statement)

After this we are DONE!

So the crux is in csv_table_to_account_statement_step.

it calls csv_table_to_account_statement_entries(table).

* Tries detect_columns_from_header(table.heading)
* If fails tries detect_columns_from_data(table.rows)
* Iterates extract_entry_from_row(row, mapping)

It seems the step we need to tweak is table.rows -> ColumnMapping?

Maybe we can do it like this.

1. Make detect_columns_from_data work on SKV file format using new mapping method?
  - Create a ColumnMapping for the entries we want to turn into account statement entries?
2. Make extract_entry_from_row ignore rows that does not work with the mapping?
  - We can do this in is_ignorable_row (ignore if mapping does now work)

Idea: Mark the code with bookmark '#ISSUE20260114_SKV_CSV' to keep track of where I am?

Here I added the 'rinse-and-repeat' python based mechanism. So how should I rig my project to make the rinse-and-repeat keep me on track to make the SKV csv account statement file parsing work?

* Should I make a test case for detect_columns_from_data?
* Do I already have test cases for csv_table_to_account_statement_step?
  YES!
  - CSVTable2AccountStatementTests::CsvTableToAccountStatementWithNordea
  - CSVTable2AccountStatementTests::CsvTableToAccountStatementWithSKV
  - CSVTable2AccountStatementTests::CsvTableToAccountStatementPreservesEntryData
  - CSVTable2AccountStatementTests::CsvTableToAccountStatementWithInvalidTable
  - CSVTable2AccountStatementTests::CsvTableToAccountStatementEmptyEntriesIsValid
  - CSVTable2AccountStatementTests::CsvTableToAccountStatementIntegrationPipeline

  - CSVTable2TaggedAmountsTests::IntegrationWithNordeaCsvData
  - CSVTable2TaggedAmountsTests::IntegrationWithSkvCsvData

YES. I renamed the google tests as above to be able to filter out and run those tests specifically.

Now, what do I need to test the 'detect_columns_from_data'?

* I already have google tests that calls detect_columns_from_data.
  - TEST(AccountStatementTests, DetectColumnsFromData)
  - TEST(AccountStatementTests, DetectBothAmountColumns)

* Added TEST(AccountStatementDetectionTests, DetectColumnsFromSKVNew)

20260115 I have now implemnted an algorithm to map an SKV csv account statement file to a 'ColumnMapping' in 'detect_columns_from_data'. It is not pritty but it works.

What more can I imagine to do before closing the claude-001-refactor-csv-import-pipeline branch?

* Consider to implement an architecture that allows to add more ways to parse an account statement file?
  - Consider to base the architecture on detect_columns_from_data -> ColumnMapping?
  - It seems the code already applies the ColumnMapping only to rows it 'fits' (so we can utilise this as part of how the alögorithm works)?
* Maybe we can create a map of candidate Tabkle Rows -> ColumnMapping functions?
  And have the detect_columns_from_data apply one by one until one succeeds?
  - Then we would be able to know how to extend future parsing with new projections as we add account statement files we can parse?

* Consider what we can do to the parse-csv-file-pipeline?
  - We want to get rid of all the multi-step 'shortcuts'?
  - Consider to single out the pipeline steps to keep by placing them in cpp-definition?
    The we can later refactor by removing all '..._chortcut' functions in hpp-files?

Here is an idea to clean up the pipe line.

1. Refactor all '..._shortcut' functions to return only a single 'and_then' functions aggregate of '_step' functions.
2. Then remove one '_shortcut' at a time and replace any calls to them with the proven 'amnd_then' aggregation.
  - Also remove all tests using '_shortcut' steps.

Well, back to the architecture for Table Rows -> ColumnMapping. What can we imagine to change to clarify the algorithm for future extension to be able to parse more types of account statement files?

* Lets keep the projection to ColumnMapping for now.

I introduced to_coumn_mapping(table) and used in csv_table_to_account_statement_entries.

Now we can imagine a vector of such to_coumn_mapping to have csv_table_to_account_statement_entries try?

## What is the next step to make a design where we can add Table -> ColumnMapping for future csv file formats?

I still hesitate to add a loop-up table of Table -> ColumnMapping functions.

* Should I keep Heading -> ColumnMapping functionality (seems fragile)?
* Should I extend Table Data -> ColumnMapping (more complex but may be more realiable and maintainable)?
* And what pipeline functions are actually 'in use' and what are duplicates (shortcuts and AI slop)?

Thinkig about this for a while I have concluded the following for the Table -> ColumnMapping

* Ensure every table has a Header (generate a row0,row1,... if none exist)
  I think this is already in place?
* Be able to identify entries 1xDate, 1xAmount, >1xText
* Be able to identify entries 1xDate,2xAmount (trans + saldo), > 1xText
* Allow the 2xAmount IFF the amounts tarns_1,saldo_1 fullfill saldo_0 + trans_1 = saldo_1

I imagine I can implememt this as follows:

1. Implement test cases for invented csv tables
2. Ensure we can parse invented tables to csv Table WITH header (extracted or generated)
3. Implement vector of f: Table -> ColumnMapping so we have enough f:s to parse all invented tables
4. Make **to_coumn_mapping(table)** try f:s in vector of f:s until one returns a mapping.

## What invented csv tables can we imagine to try?

Imagine some headless tables with Date,Text,Amount permutations?

* Date,Text,Amount
* Date,Amount,Text
* Text,Date,Amount
* Text,Amount,Date
* Amount,Date,Text
* Amount,Text,Date

Maybe a test case that generates theese permutations?

Imagine some headless tables with Date,Text,TransAmount,SaldoAmount permutations?

Maybe a test case that generates theese permutations?

## What algorithm can we imagine to generate invented csv tables for account statement parse testing?

1. We seem to need to generate with or without saldo amount?
  - So we can make the algorithm invent some random transactions and always also keep a running saldo.
  - Then inject or not inject the saldo amount depending on a flag.
2. It seem we need to generate some set of text fields?
3. It seem we should also generate random 'to' and 'from' fields with 'account numbers'?

What account number formats have I already implemented?

* account::BBAN
* account::giro::BG
* account::giro::PG

So we can imagine to hard code some BBAN,BG and PG in a std::variant set and randomly use

4. It seem we need an account number for the csv table we are generating?
  - The csv table account number can be a BBAN,BG or PG?
5. It seems we can use the same account number generator to create to and from account numbers?

WOW! This is COMPLEX!!

Anyhow, lerts start to create the new test cpp-unit and the **to_coumn_mapping(table)** as function im vector of options.

What test units do we have?

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % tree src/test 
src/test
├── data
│   └── account_statements_csv.hpp
├── test_amount_framework.cpp
├── test_amount_framework.hpp
├── test_annotated_optional.cpp
├── test_atomics.cpp
├── test_atomics.hpp
├── test_csv_import_pipeline.cpp
├── test_fixtures.cpp
├── test_fixtures.hpp
├── test_generic_account_statement_csv.cpp
├── test_giro_framework.cpp
├── test_integrations.cpp
├── test_integrations.hpp
├── test_md_table_to_account_statement.cpp
├── test_runner.cpp
├── test_runner.hpp
├── test_statement_to_tagged_amounts.cpp
├── test_swedish_bank_account.cpp
├── test_transcoding_views.cpp
└── zeroth
    ├── test_zeroth.cpp
    └── test_zeroth.hpp

3 directories, 21 files
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % 
```

* Reuse test_csv_import_pipeline?
* Reuse test_generic_account_statement_csv?
* Reuse test_md_table_to_account_statement?

I ended up adding new test_csv_table_identification.cpp and moved detection test code there.

I now have the account statement table mapping detection tests in test_csv_table_identification ok. I also test only the 'top' to_column_mapping(table).

So I turned to clean up the current test cases that fails.

It trn out I now have SIEEnvironmentTests.StageToPostedOkTest failing for some unknown reason.

To track this I started adding logging to see where things go wrong.

And in doing so I needed a first::to_string for optional DateRange. But when I added this my code stopped compiling. Now the compiler fails to find  to_string(Date) and to_string(Amount) when called from withing namespace first! WTF?!!

## How can I make all free function to_string overloads to be found by the compiler?

The oproblem seems to be that as long as namespace first has NO to_string overload, then the compiler finds to_string(Date) and to_string(Amount) when called from inside namespace first. But if there is one, then the compiler gives up when trying all to_string inside namespace first?

So where is to_string(Date) and to_string(Amount)?

It turns out I have placed them in global namespace.

But is this the recommended way to expose free functions? I mean, compiler will apply ADL (argument dependant lookup) to find *unqualified function names*. So if I call top_string(T t), then it WILL find any to_string in the nemaspace where T is defined.

* So I should keep my types and functions on these types in their namespace?
* And only fix call to overloaded functions at call sites inside namespaces to make them find functions in other name spaces?

But then, why does not ADL work inside a name space? That is, is it true that ADL is not applied if the local namespace already have a free function with unqualified name?

* I expected a call to 'to_string' to look first in the namespace where the argument type is defined.
* Why is this not so?

It seems I need to start at C++ compiler [Name lookup](https://en.cppreference.com/w/cpp/language/lookup.html)?

So 'name lookup' is the mechanism where the compiler does identifier -> declaration.

* The name is either 'unqualified' (no namespace path provided) or 'qualified' (namespace path provided)

  - to_string(date) is an unqualified named 'to_string'
  - first::to_string(dat) is a qualified named 'first::to_string'

* [Unqualified name lookup](https://en.cppreference.com/w/cpp/language/unqualified_lookup.html)
* [Qualified name lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup.html)

AHA! The unqualified name lookup searches scopes '...until it finds at least one declaration of any kind'! That is, it does NOT care if the declaration found suits the code using it! In effect, if at least one declaration is found, then no more scopes are considdered!

This is NOT what I expected. I assumed the compiler would consider ALL scopes to get a FULL set of viable declarations.

In this way the viable scopes is NOT affected by the content of the scopes. But the actual name lookup behavious DOES change depending on scope content!

*sigh*

So what options do I have to adress this issue?

* I want my types and functions on those types to recide in namespaces.
* I want any call site to see the applicable function(s).
* I want to be able to use overloads (e.g. to_string overloads for types)

It seems I have at least two options?

1. Always place overloads in global namespace?
2. Manually import namespaces with 'using' at call sites as required (I can't make the compiler find them)?

Are there other options I did not thing of?

I think I will add a cpp_compile_playground.cpp to have a space to have a dialog with the compiler with code to determine how it actually behaves.

I managed to replicate the compiler error!

1. Having a global namespace to_string
2. AND - having a first::to_string
3. CAUSES: A call from withing namespace first of to_string to fail (can't see wnated global to_string)

The problem is that when there is a to_string in current namespace, then only this is considered by name lookup.

So the conclusion is:

1. Put functions and processed types in the same namespace
2. Rely on ADL to make overload lookup work from call site namespace to namespace with deffined code.
3. Thus, it seems to be OK to bring in types to global namesapce as aliases 'using a_type = a_namespace::a_type'
   ADL seems to still find functions in the namespace of the argument types?

Decision: Move to_string(Date) and to_string(Amount) to the namespace of DAte and Amount.

WAIT! This does NOT work!

* Date is an **alias** for std::chrono::year_month_day!
* Thus, ADL will look into namespace std::chrono for a to_string (NOT the namespace where the alias is defined)
* So putting to_string in e.g. namespace first will NOT make the compiler find it!

gave this some more thought and realised:

* I can just make to_string(DateRange) in global namespace for now!
* And add comments about future 'better' options.

## Consider to extend ColumnMapping to a 'TableMeta' and make tests based on array of sz_xxx csv text to try to parse?

I have thought about account statement csv file parsing and identification a bit more. I am now tempted to try:

1. Extend 'ColumnMapping' to a 'CSV::TableMeta' with more meta-data from identification process
2. Introduce an array of sz_xxx texts to test to parse into account statement tables
3. Extend 'coumn mapping' to also identify BBAN, BG and PG account number fields
4. Constrain 'header' to be valid only if header has the same coumn count as the in-saldo,out-saldo and trans entry rows

I Imganine the TableMeta to contain:

* A boolean flag 'has columns header'
* row index of first and last account transaction row
* The 'transaction row column mapping'
* row index of 'in saldo' and 'out saldo' row
* a list of row indexes of unidentifed rows

So let's first refactor 'to_column:mapping -> ColumnMApping' to 'to_table_meta -> TableMeta' and adjust clienbt code accordingly. 

So I introduced to_account_statement_table_meta and replaced to_column_mapping in tests ok.

I then made MonadicCompositionFixture.PathToAccountIDedTable test pass by inactivating to_account_id_ed_step.

But now ALL tests that relies on correctly identifying NORDEA or SKV account stements fail!

So I think I activate old to_account_id_ed_step again for now.

DARN! Now I discover my statically instantikated loggers are instable!

## How can I make my singleton-like loggers JIT created and deterministically destroyed?

I currently have 'static' logger instances. This does not work well with the google test framework. It seems the framework is (may) be instantiated and run before my own loggers are properly instantiated. Thus my code somethimes break when I use my loggers in google test code.

It seems I have maybe two options.

1. Make my loggers be JIT instantiated somehow (ensure they are created on call if they are not yet created)
2. Somehow try to make the google test framework be deterministically instantiated after my statig loggers?

So where and when is my google test framework instantiated?

Hm... It seems in main(argc,argv) I trigger on argument '--test' or '--gtest_filter' and then calls tests::run_all.

This will do:

```c++
  ::testing::InitGoogleTest(&argc,argv);
  ::testing::AddGlobalTestEnvironment(fixtures::TestEnvironment::GetInstance());
  int result = RUN_ALL_TESTS();        
```

Here I used the lldb debugger and found:

```sh
(lldb) breakpoint set -E C++
Breakpoint 1: 2 locations.
(lldb) run --test
There is a running process, kill it and restart?: [Y/n] Y
Process 38799 exited with status = 9 (0x00000009) killed
Process 44456 launched: '/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/cratchit' (arm64)
Process 44456 exited with status = 9 (0x00000009) Terminated due to code signing error
(lldb
```

So it was in fact a code signing error all along?!

*sigh*

Back to 