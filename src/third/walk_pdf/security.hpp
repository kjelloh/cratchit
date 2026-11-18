#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace walk_pdf {

// What an encrypted file's encryption dictionary says, and whether walk_pdf
// can decrypt it (PDF 32000-1:2008, 7.6 Encryption). walk_pdf supports the
// Standard security handler with RC4 (V 1, 2, and 4 with /CFM /V2), opened
// with the empty user password, as viewers do when no password is asked for.
struct Security {
  std::string filter;                  // the security handler, e.g. "Standard"
  int version = 0;                     // /V
  int revision = 0;                    // /R
  std::size_t key_bytes = 5;           // /Length in bytes
  std::uint32_t permissions = 0;       // /P, see allowed()
  bool encrypt_metadata = true;        // /EncryptMetadata
  std::optional<std::uint64_t> dictionary_object; // the encryption dictionary's object (never encrypted)
  std::optional<std::string> key;      // the file key, when walk_pdf can decrypt
  std::string status;                  // e.g. "RC4 128-bit, no password", or why it can not decrypt

  // Is the permission with this bit (1-based, as in the PDF standard) granted?
  bool allowed(int bit) const { return ((permissions >> (bit - 1)) & 1U) != 0; }

  // The names of the permissions /P does not grant
  std::vector<std::string> denied() const;
};

// The file key, if the empty user password opens the file (Algorithm 2, and
// Algorithm 4 or 5 to check it against /U). owner and user are the bytes of /O and /U.
std::optional<std::string> key_for_empty_password(int revision, std::size_t key_bytes,
                                                  std::string_view owner, std::string_view user,
                                                  std::uint32_t permissions,
                                                  std::string_view first_id, bool encrypt_metadata);

// The RC4 key for the strings and streams of one object (Algorithm 1)
std::string object_key(std::string_view file_key, std::uint64_t number, std::uint32_t generation);

} // namespace walk_pdf
