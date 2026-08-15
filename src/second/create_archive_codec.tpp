#pragma once

#include "ArchiveCodec.tpp"

template <typename T>
auto create_archive_codec(auto&&... args) {
  return ArchiveCodec<T>{std::forward<decltype(args)>(args)...};
} // create_archive_codec