#pragma once

#include <cstdint>
#include <span>

// Character set detection: tests that tell whether bytes are likely in a
// given charset. One test per charset, added as the need arises. Each client
// decides which tests to run and in what order.
namespace charset::detect {

// True when the bytes are well-formed UTF-8 holding at least one multi-byte
// character. Pure ASCII gives false: it is valid in most charsets, so it tells
// nothing about which one was meant.
bool is_utf8(std::span<std::uint8_t const> bytes);

} // namespace charset::detect
