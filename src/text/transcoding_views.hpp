#pragma once

#include "encoding.hpp"
#include "charset.hpp"
#include "io/file_reader.hpp"
#include <ranges>
#include <iterator>
#include <optional>
#include <variant>

namespace text::encoding::views {

  // Unicode replacement character for invalid byte sequences
  constexpr char16_t REPLACEMENT_CHARACTER = 0xFFFD;

  // Forward declarations
  template<std::ranges::view ByteRange>
  class bytes_to_unicode_view;

  namespace detail {

    // Transcoding state for different encodings
    // UTF-8 needs stateful multi-byte decoding
    struct UTF8State {
      text::encoding::UTF8::ToUnicodeBuffer buffer;

      std::optional<char16_t> decode_byte(std::byte b) {
        return buffer.push(static_cast<uint8_t>(b));
      }
    };

    // ISO-8859-1 is stateless: one byte = one code point
    struct ISO8859_1State {
      std::optional<char16_t> decode_byte(std::byte b) {
        return charset::ISO_8859_1::iso8859ToUnicode(static_cast<char>(b));
      }
    };

    // CP437 is stateless: one byte = one code point (via lookup)
    struct CP437State {
      std::optional<char16_t> decode_byte(std::byte b) {
        return charset::CP437::cp437ToUnicode(static_cast<char>(b));
      }
    };

    // Variant holding the appropriate decoder state
    using DecoderState = std::variant<UTF8State, ISO8859_1State, CP437State>;

    // Create decoder state based on detected encoding
    inline DecoderState make_decoder_state(DetectedEncoding encoding) {
      switch (encoding) {
        case DetectedEncoding::UTF8:
          return UTF8State{};
        case DetectedEncoding::ISO_8859_1:
        case DetectedEncoding::ISO_8859_15:
        case DetectedEncoding::WINDOWS_1252:
          // Note: Windows-1252 is a superset of ISO-8859-1 for 0x00-0xFF range
          return ISO8859_1State{};
        case DetectedEncoding::CP437:
          return CP437State{};
        default:
          // Default to UTF-8 for unknown encodings
          return UTF8State{};
      }
    }

    // Iterator for lazy transcoding
    template<std::ranges::view ByteRange>
    class transcoding_iterator {
    public:
      using iterator_category = std::input_iterator_tag;
      using value_type = char16_t;
      using difference_type = std::ptrdiff_t;
      using pointer = char16_t const*;
      using reference = char16_t;

      transcoding_iterator() = default;

      transcoding_iterator(
          std::ranges::iterator_t<ByteRange> current,
          std::ranges::sentinel_t<ByteRange> end,
          DecoderState decoder)
        : m_current(current)
        , m_end(end)
        , m_decoder(std::move(decoder))
        , m_current_codepoint(std::nullopt)
      {
        // Prime the iterator by reading the first code point
        advance_to_next_codepoint();
      }

      // Dereference operator - return current code point
      char16_t operator*() const {
        return m_current_codepoint.value_or(REPLACEMENT_CHARACTER);
      }

      // Pre-increment operator
      transcoding_iterator& operator++() {
        advance_to_next_codepoint();
        return *this;
      }

      // Post-increment operator
      transcoding_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
      }

      // Equality comparison
      bool operator==(transcoding_iterator const& other) const {
        // Check if both are at true end (no pending codepoint and at end of bytes)
        bool this_at_end = !m_current_codepoint.has_value() && (m_current == m_end);
        bool other_at_end = !other.m_current_codepoint.has_value() && (other.m_current == other.m_end);

        // If both at end, they're equal
        if (this_at_end && other_at_end) {
          return true;
        }

        // If one is at end and other isn't, they're not equal
        if (this_at_end != other_at_end) {
          return false;
        }

        // Both have data - compare underlying byte positions
        return m_current == other.m_current;
      }

      bool operator!=(transcoding_iterator const& other) const {
        return !(*this == other);
      }

      // Sentinel comparison (for end iterator)
      bool operator==(std::ranges::sentinel_t<ByteRange> const& sentinel) const {
        return m_current == sentinel;
      }

      bool operator!=(std::ranges::sentinel_t<ByteRange> const& sentinel) const {
        return m_current != sentinel;
      }

    private:
      std::ranges::iterator_t<ByteRange> m_current;
      std::ranges::sentinel_t<ByteRange> m_end;
      DecoderState m_decoder;
      std::optional<char16_t> m_current_codepoint;

      // Advance through bytes until we have a complete code point
      void advance_to_next_codepoint() {
        m_current_codepoint.reset();

        // Keep reading bytes until we get a code point or reach the end
        while (m_current != m_end && !m_current_codepoint) {
          std::byte current_byte = *m_current;
          ++m_current;

          // Decode the byte using the appropriate decoder
          auto maybe_codepoint = std::visit(
            [current_byte](auto& decoder) {
              return decoder.decode_byte(current_byte);
            },
            m_decoder
          );

          if (maybe_codepoint) {
            m_current_codepoint = maybe_codepoint;
          }
          // For UTF-8, nullopt means "need more bytes", so continue
          // For single-byte encodings, nullopt shouldn't happen
        }

        // If we reached the end without getting a code point,
        // check if UTF-8 decoder has partial data (invalid sequence at end)
        if (!m_current_codepoint && m_current == m_end) {
          // Iterator is now at end, m_current_codepoint remains nullopt
          // This is fine - the end iterator comparison will work
        }
      }
    };

  } // namespace detail

  // Lazy range view that transcodes bytes to Unicode code points
  template<std::ranges::view ByteRange>
  class bytes_to_unicode_view
    : public std::ranges::view_interface<bytes_to_unicode_view<ByteRange>>
  {
  public:
    bytes_to_unicode_view() = default;

    bytes_to_unicode_view(ByteRange byte_range, DetectedEncoding encoding)
      : m_byte_range(std::move(byte_range))
      , m_encoding(encoding)
    {}

    auto begin() {
      return detail::transcoding_iterator<ByteRange>(
        std::ranges::begin(m_byte_range),
        std::ranges::end(m_byte_range),
        detail::make_decoder_state(m_encoding)
      );
    }

    auto end() {
      // Create an end iterator (pointing to the underlying end)
      return detail::transcoding_iterator<ByteRange>(
        std::ranges::end(m_byte_range),
        std::ranges::end(m_byte_range),
        detail::make_decoder_state(m_encoding)  // Dummy decoder state for end iterator
      );
    }

  private:
    ByteRange m_byte_range;
    DetectedEncoding m_encoding;
  };

  // Range adaptor for piping syntax
  namespace adaptor {
    struct bytes_to_unicode_adaptor {
      DetectedEncoding encoding;

      template<std::ranges::viewable_range ByteRange>
      auto operator()(ByteRange&& byte_range) const {
        return bytes_to_unicode_view<std::views::all_t<ByteRange>>(
          std::views::all(std::forward<ByteRange>(byte_range)),
          encoding
        );
      }
    };

    // Piping support: byte_range | bytes_to_unicode(encoding)
    template<std::ranges::viewable_range ByteRange>
    auto operator|(ByteRange&& byte_range, bytes_to_unicode_adaptor adaptor) {
      return adaptor(std::forward<ByteRange>(byte_range));
    }

    // Factory function for the adaptor
    inline bytes_to_unicode_adaptor bytes_to_unicode(DetectedEncoding encoding) {
      return {encoding};
    }
  }

  // Convenience function to create view directly
  template<std::ranges::viewable_range ByteRange>
  auto bytes_to_unicode(ByteRange&& byte_range, DetectedEncoding encoding) {
    return bytes_to_unicode_view<std::views::all_t<ByteRange>>(
      std::views::all(std::forward<ByteRange>(byte_range)),
      encoding
    );
  }

  // ============================================================================
  // Step 4: Unicode Code Points → Runtime Encoding
  // ============================================================================

  namespace detail {

    // Iterator that encodes Unicode code points (char16_t) to UTF-8 bytes
    template<std::ranges::view UnicodeRange>
    class unicode_to_utf8_iterator {
    public:
      using iterator_category = std::input_iterator_tag;
      using value_type = char;
      using difference_type = std::ptrdiff_t;
      using pointer = char const*;
      using reference = char;

      unicode_to_utf8_iterator() = default;

      unicode_to_utf8_iterator(
          std::ranges::iterator_t<UnicodeRange> current,
          std::ranges::sentinel_t<UnicodeRange> end)
        : m_current(current)
        , m_end(end)
        , m_buffer_index(0)
      {
        // Prime the iterator by encoding the first code point
        advance_to_next_byte();
      }

      // Dereference operator - return current UTF-8 byte
      char operator*() const {
        return m_current_buffer[m_buffer_index];
      }

      // Pre-increment operator
      unicode_to_utf8_iterator& operator++() {
        advance_to_next_byte();
        return *this;
      }

      // Post-increment operator
      unicode_to_utf8_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
      }

      // Equality comparison
      bool operator==(unicode_to_utf8_iterator const& other) const {
        // Both at end if current == end and buffer is consumed
        bool this_at_end = (m_current == m_end) && (m_buffer_index >= m_current_buffer.size());
        bool other_at_end = (other.m_current == other.m_end) && (other.m_buffer_index >= other.m_current_buffer.size());

        if (this_at_end && other_at_end) {
          return true;
        }

        if (this_at_end != other_at_end) {
          return false;
        }

        // Both have data - compare underlying positions
        return (m_current == other.m_current) && (m_buffer_index == other.m_buffer_index);
      }

      bool operator!=(unicode_to_utf8_iterator const& other) const {
        return !(*this == other);
      }

    private:
      std::ranges::iterator_t<UnicodeRange> m_current;
      std::ranges::sentinel_t<UnicodeRange> m_end;
      std::string m_current_buffer;  // Buffer for UTF-8 encoded bytes from current code point
      size_t m_buffer_index;         // Current position in buffer

      // Encode one Unicode code point to UTF-8 and store in buffer
      void encode_codepoint_to_buffer(char16_t cp) {
        m_current_buffer.clear();
        m_buffer_index = 0;

        // Use std::ostringstream with UTF8::ostream to encode
        std::ostringstream oss;
        text::encoding::UTF8::ostream utf8_os{oss};

        // Cast char16_t to char32_t for UTF8::ostream
        utf8_os << static_cast<char32_t>(cp);
        m_current_buffer = oss.str();
      }

      // Advance to the next UTF-8 byte
      void advance_to_next_byte() {
        // If we have bytes left in current buffer, advance within it
        if (m_buffer_index + 1 < m_current_buffer.size()) {
          ++m_buffer_index;
          return;
        }

        // Buffer consumed, need to encode next code point
        if (m_current != m_end) {
          char16_t codepoint = *m_current;
          ++m_current;
          encode_codepoint_to_buffer(codepoint);
        } else {
          // At end - mark buffer as fully consumed
          m_current_buffer.clear();
          m_buffer_index = 0;
        }
      }
    };

  } // namespace detail

  // Lazy range view that encodes Unicode code points to platform (runtime) encoding
  template<std::ranges::view UnicodeRange>
  class unicode_to_runtime_encoding_view
    : public std::ranges::view_interface<unicode_to_runtime_encoding_view<UnicodeRange>>
  {
  public:
    unicode_to_runtime_encoding_view() = default;

    explicit unicode_to_runtime_encoding_view(UnicodeRange unicode_range)
      : m_unicode_range(std::move(unicode_range))
    {}

    auto begin() {
      // TODO: Design the ability to produce platform (runtime) encoding.
      //       See RuntimeEncoding::detected_encoding()
      //       For now, hard coded to UTF-8 (works on macOS but probably not Windows not Linux)
      return detail::unicode_to_utf8_iterator<UnicodeRange>(
        std::ranges::begin(m_unicode_range),
        std::ranges::end(m_unicode_range)
      );
    }

    auto end() {
      // TODO: Design the ability to produce platform (runtime) encoding.
      //       See RuntimeEncoding::detected_encoding()
      //       For now, hard coded to UTF-8 (works on macOS but probably not Windows not Linux)
      return detail::unicode_to_utf8_iterator<UnicodeRange>(
        std::ranges::end(m_unicode_range),
        std::ranges::end(m_unicode_range)
      );
    }

  private:
    UnicodeRange m_unicode_range;
  };

  // Range adaptor for piping syntax
  namespace adaptor {
    struct unicode_to_runtime_encoding_adaptor {
      template<std::ranges::viewable_range UnicodeRange>
      auto operator()(UnicodeRange&& unicode_range) const {
        return unicode_to_runtime_encoding_view<std::views::all_t<UnicodeRange>>(
          std::views::all(std::forward<UnicodeRange>(unicode_range))
        );
      }
    };

    // Piping support: unicode_range | unicode_to_runtime_encoding()
    template<std::ranges::viewable_range UnicodeRange>
    auto operator|(UnicodeRange&& unicode_range, unicode_to_runtime_encoding_adaptor adaptor) {
      return adaptor(std::forward<UnicodeRange>(unicode_range));
    }

    // Factory function for the adaptor
    inline unicode_to_runtime_encoding_adaptor unicode_to_runtime_encoding() {
      return {};
    }
  }

  // Convenience function to create view directly
  template<std::ranges::viewable_range UnicodeRange>
  auto unicode_to_runtime_encoding(UnicodeRange&& unicode_range) {
    return unicode_to_runtime_encoding_view<std::views::all_t<UnicodeRange>>(
      std::views::all(std::forward<UnicodeRange>(unicode_range))
    );
  }

} // namespace text::encoding::views
