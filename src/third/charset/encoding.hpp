#pragma once

namespace charset {

// The encodings that can be chosen at runtime, by make_decoder and make_transcoder
enum class Encoding { utf8, utf16be, utf16le, cp437 };

} // namespace charset
