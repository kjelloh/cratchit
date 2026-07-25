#include "Msg.hpp"

namespace tea {

    UnicodeKeyMsg::UnicodeKeyMsg(int cp) : code_point{static_cast<char32_t>(cp)} {}

} // tea