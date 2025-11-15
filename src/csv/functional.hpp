#pragma once
#include "functional/maybe.hpp" // AnnotatedOptional,...

namespace CSV {
  namespace functional {

    template <typename T>
    using CSVProcessResult = cratchit::functional::AnnotatedOptional<T>;

  }
} // CSV