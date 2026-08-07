#include "sub_to_msg.hpp"

namespace app {

  app::Msg sub_to_msg(tea::MetronomeEventDescriptor const&, tea::MetronomeEventDescriptor::payload_type const&) {
    return app::CursorBlinkMsg{};
  }
  app::Msg sub_to_msg(tea::TestEventDescriptor const&, tea::TestEventDescriptor::payload_type const& payload) {
    return app::TestEventMsg(payload);
  }

} // app
