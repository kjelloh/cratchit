
#include "text/encoding.hpp"
#include <ostream>

namespace persistent {
  namespace out {

    namespace text {

      using BOM = ::text::encoding::BOM;

      // TODO: Implement for BOM aware text output (when required)
      //       Declared here for symetry with raw_text_read architecture
      class bom_ostream {
      public:
          std::ostream& raw_out;
          std::optional<BOM> bom{};
          bom_ostream(std::ostream& out);
          operator bool();
      private:
      };
    } // text
  } // out
} // persistent
