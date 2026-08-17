# Consider a character set descriptor with multi-data-point to unicode transformer?

## 20260817

Design a charchter set transform framework (possibly as combinators)?

I imagine the pump-through-code-point-queue transformer

* Imagine character set Cx.
* Let Cx define a set of code points (Cx::cp);
* Let Cx know how to map Cx::cp to Unicode and back
  * Unicode::cp code_point_to_unicode(Cx::cp)
  * Cx::cp unicode_to_code_point(Unicode::cp)
* Imagine a charchter set descriptor CSDx.
* Imagine the CSDx supports multi-byte encoding using a byte-queue CSDx::multi_byte_decoder .
  * MaybeUnicode CSDx::multi_byte_decoder.push(CX::cp)

Now I am still not fully there.

* The multi_byte_decoder may be required to come in flavours for different input copde point sizes?
  * To support UTF8-like multi bytes it shall take bytes and turn into Unicode::cp.
  * To support UTF16-like bulti-data-point it shall take a sequence uint16_t and turn into Unicode
  * But it is in fact not the size of the input-data-points that determine what to do.
  * The multi-data-point-decoder is determioned on the multi-data-point scheme.
  * Where UTF8,UTF16 etc are 'schemes'.
* We also need to descide (or support) what code-point size to support for Unicode::cp?
  * Now we can just go with the largest type to hold three bytes

```cpp
const char32_t CODE_POINT_MAX      = 0x0010ffff;
```

  * Or allow more slimmed applications down to first byte (E.g., ASCII), two bytes or full three bytes?

So then we have what we need to design a crahcter set descriptor?

* It defines copde_point_type (e.g., uint8_t, char, ...)
* It defines unicode_code_point_type (e.g., char,unit8_t,char16_t,char32_t,uint32_t,...)
* It define a multi_data_point_to_unicode queue
  * I suppose for consistency it should return expected maybe_uncide_code_point or error
  * Where error represents what can go wrong in the decoding?
  * So error can be 'invalid input data point'
  * Error can be (input data points encodes a too large unicode code point)
