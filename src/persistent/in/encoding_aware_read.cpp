#include "encoding_aware_read.hpp"
#include "logger/log.hpp"

namespace persistent {
  namespace in {

    namespace ISO_8859_1 {

      istream::istream(std::istream& in) : bom_istream{in} {
        if (this->bom) {
          // We expect the input stream to be without BOM
          logger::cout_proxy << "\nSorry, Expected ISO8859-1 stream but found bom:" << *(this->bom);

          this->raw_in.setstate(std::ios_base::failbit); // disable reading this stream
        }
      }

    } // ISO_8859_1

    namespace UTF8 {

      istream::istream(std::istream& in) : bom_istream{in} {
        if (this->bom) {
          if (this->bom->value != text::BOM::UTF8_VALUE) {
            // We expect the input stream to be in UTF8 and thus any BOM must confirm this
            logger::cout_proxy << "\nSorry, Expected an UTF8 input stream but found contradicting BOM:" << *(this->bom);

            this->raw_in.setstate(std::ios_base::failbit); // disable reading this stream
          }
        }
      }

    } // namespace UTF8

    namespace CP437 {

      istream::istream(std::istream& in) : bom_istream{in} {
        if (this->bom) {
          // We expect the input stream to be without BOM
          logger::cout_proxy << "\nSorry, Expected CP437 (SIE-file) stream but found bom:" << *(this->bom);

          this->raw_in.setstate(std::ios_base::failbit); // disable reading this stream
        }
      }

    } // namespace CP437

  } // in
} // persistent
