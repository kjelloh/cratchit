#include "subscriptions.hpp"

tea::Msg to_event_msg(MetronomeEventDescriptor const&, MetronomeEventDescriptor::payload_type const&) {
  return tea::CursorBlinkMsg{};
}
tea::Msg to_event_msg(TestEventDescriptor const&, TestEventDescriptor::payload_type const& payload) {
  return tea::TestEventMsg(payload);
}

Sub subscriptions(tea::Model const&) {
  Sub result{};
  result.push_back(MetronomeEventDescriptor{500}); // hard coded (TODO: map from model)
  result.push_back(TestEventDescriptor{}); // POC of subscription to event with payload
  return result;
}