#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace walk_pdf {

// One of a stream's filters (PDF 32000-1:2008, 7.4 Filters) and its /DecodeParms
struct Filter {
  std::string name;        // e.g. "FlateDecode"
  int predictor = 1;       // /Predictor: 1 none, 2 TIFF, 10 and up PNG
  int colors = 1;          // /Colors
  int bits_per_component = 8; // /BitsPerComponent
  int columns = 1;         // /Columns
};

// Can walk_pdf apply this filter? FlateDecode, ASCIIHexDecode and
// ASCII85Decode it can; image filters such as DCTDecode it leaves alone.
bool can_decode(std::string_view filter);

// The result of applying a stream's filters
struct Decoded {
  std::string bytes;            // the data after the filters (as far as they went)
  std::optional<std::string> problem; // why decoding stopped early, if it did
};

// Applies the filters in order to a stream's (decrypted) data
Decoded decode(std::string data, std::vector<Filter> const& filters);

} // namespace walk_pdf
