#include "Msg.hpp"

namespace tea {

    UnicodeMsg::UnicodeMsg(int cp) : code_point{static_cast<char32_t>(cp)} {}

} // tea