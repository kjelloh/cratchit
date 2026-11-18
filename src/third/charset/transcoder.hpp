#pragma once

#include "charset/cp437.hpp"
#include "charset/encoding.hpp"
#include "charset/unicode.hpp"
#include "charset/utf16.hpp"
#include "charset/utf8.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace charset {

// The one to four bytes of one encoded character, whatever the encoding
struct Bytes {
  std::array<std::uint8_t, 4> bytes{};
  std::size_t size = 0;

  std::uint8_t const* begin() const { return bytes.data(); }
  std::uint8_t const* end() const { return bytes.data() + size; }
};

// Pairs a decoder with an encoder. Takes bytes in one encoding one at a time
// and gives back the bytes of each character in the other encoding as it
// becomes available. A character the encoder cannot represent gives nothing.
// failure_count() and unmapped_count() tell the two kinds of trouble apart:
// input the decoder could not parse, and characters the encoder has no
// mapping for.
template <class Decoder, class Encoder>
class BasicTranscoder {
public:
  explicit BasicTranscoder(Decoder decoder = Decoder(), Encoder encoder = Encoder())
      : decoder_(std::move(decoder)), encoder_(std::move(encoder)) {}

  // The bytes of a character once the bytes pushed so far make one, else nothing
  std::optional<Bytes> push(std::uint8_t byte) {
    if (auto code_point = decoder_.push(byte)) return encode(*code_point);
    return std::nullopt;
  }

  // Ends the input. Gives the bytes of what the decoder had left, if anything.
  std::optional<Bytes> finish() {
    if constexpr (requires { decoder_.finish(); }) {
      if (auto code_point = decoder_.finish()) return encode(*code_point);
    }
    return std::nullopt;
  }

  // Number of failed input sequences so far, from the decoder
  std::size_t failure_count() const { return decoder_.failure_count(); }

  // Number of characters so far that the encoder has no mapping for
  std::size_t unmapped_count() const { return encoder_.unmapped_count(); }

private:
  // Turns the encoder's answer into Bytes: either an optional single byte
  // (as cp437) or a range of bytes (as utf8 and utf16)
  std::optional<Bytes> encode(unicode::CodePoint code_point) {
    auto encoded = encoder_.push(code_point);
    Bytes result;
    if constexpr (std::is_same_v<decltype(encoded), std::optional<std::uint8_t>>) {
      if (!encoded) return std::nullopt;
      result.bytes[result.size++] = *encoded;
    } else {
      for (auto byte : encoded) result.bytes[result.size++] = byte;
    }
    if (result.size == 0) return std::nullopt;
    return result;
  }

  Decoder decoder_;
  Encoder encoder_;
};

// Same as BasicTranscoder, but the decoder and encoder are chosen at runtime
class Transcoder {
public:
  template <class Decoder, class Encoder>
  explicit Transcoder(BasicTranscoder<Decoder, Encoder> transcoder)
      : impl_(std::make_unique<Model<Decoder, Encoder>>(std::move(transcoder))) {}

  std::optional<Bytes> push(std::uint8_t byte) { return impl_->push(byte); }
  std::optional<Bytes> finish() { return impl_->finish(); }
  std::size_t failure_count() const { return impl_->failure_count(); }
  std::size_t unmapped_count() const { return impl_->unmapped_count(); }

private:
  struct Interface {
    virtual ~Interface() = default;
    virtual std::optional<Bytes> push(std::uint8_t byte) = 0;
    virtual std::optional<Bytes> finish() = 0;
    virtual std::size_t failure_count() const = 0;
    virtual std::size_t unmapped_count() const = 0;
  };

  template <class Decoder, class Encoder>
  struct Model : Interface {
    explicit Model(BasicTranscoder<Decoder, Encoder> transcoder) : transcoder_(std::move(transcoder)) {}
    std::optional<Bytes> push(std::uint8_t byte) override { return transcoder_.push(byte); }
    std::optional<Bytes> finish() override { return transcoder_.finish(); }
    std::size_t failure_count() const override { return transcoder_.failure_count(); }
    std::size_t unmapped_count() const override { return transcoder_.unmapped_count(); }
    BasicTranscoder<Decoder, Encoder> transcoder_;
  };

  std::unique_ptr<Interface> impl_;
};

// A transcoder from one encoding to another
Transcoder make_transcoder(Encoding from, Encoding to);

} // namespace charset
