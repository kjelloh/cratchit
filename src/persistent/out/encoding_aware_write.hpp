
#include "raw_text_write.hpp" // 
#include <ostream>

namespace persistent {
  namespace out {

    namespace UTF8 {

      struct ostream {
        std::ostream& os;
      };

      persistent::out::UTF8::ostream& operator<<(persistent::out::UTF8::ostream& os,char32_t cp);

    } // UTF8

  } // in
} // persistent
