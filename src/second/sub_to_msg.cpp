#include "sub_to_msg.hpp"

tea::Msg sub_to_msg(MetronomeEventDescriptor const&, MetronomeEventDescriptor::payload_type const&) {
  return tea::CursorBlinkMsg{};
}
tea::Msg sub_to_msg(TestEventDescriptor const&, TestEventDescriptor::payload_type const& payload) {
  return tea::TestEventMsg(payload);
}
