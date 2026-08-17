# Consider to investigate C++ string literal encoding in source file vs in compiled binary?

## 20260817

I suppose I can Write test cases to determine how string literals in source code ends up being encoded in compiled code?

* We can create source code files with different encodings?
  * UTF8 (very common for cross-web and cross-platform)
  * CP437 (for SIE-file encoding)
  * ISO_8859_1 (For DOS and simmilar OS?)
* We can define string literals denoted with different 'encodings' [cppreference: String literal](https://en.cppreference.com/cpp/language/string_literal)?

  * It seems we need to understand [Code unit and literal encoding](https://en.cppreference.com/cpp/language/charset#Code_unit_and_literal_encoding)

```text
Mapping from source file (other than a UTF-8 source file)(since C++23) characters to the basic character set(until C++23)translation character set(since C++23) during translation phase 1 is implementation-defined, so an implementation is required to document how the basic source characters are represented in source files.
```

  * So what should (1) '""' or (2) 'R"()"' become when compiled?

```cpp
  auto plain = "Hallå Världen";
  auto raw = R"("Hallå Världen")";
```

  * What should (3) 'L""' or (4) 'LR"()" become when compiled?
    * This is a 'simple' type widening, not a code point transformation?
  * A (5) 'u8""' or (6) 'u8R"()"' should encode source code code-points to UTF8?
  * A (7) 'u""' or (8) 'uR"()"' should encode source code code-points to UTF16?
  * A (9) 'U""' or (10) 'UR"()"' should encode source code code-points to UTF32?

I suppose we can start with just logging the content of the string literal to try and understand what has happened?

* Maybe the plain "" and R"()" can be used for us to log the encoding of the source code file?
* Then we can compare with other denotions like u8""?