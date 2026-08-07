#include "sub_to_msg.hpp"

app::Msg sub_to_msg(MetronomeEventDescriptor const&, MetronomeEventDescriptor::payload_type const&) {
  return app::CursorBlinkMsg{};
}
app::Msg sub_to_msg(TestEventDescriptor const&, TestEventDescriptor::payload_type const& payload) {
  return app::TestEventMsg(payload);
}
