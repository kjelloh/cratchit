#pragma once
#include "Model.hpp"
#include "subscribeables.hpp"

tea::Msg to_event_msg(MetronomeEventDescriptor const&, MetronomeEventDescriptor::payload_type const&);
tea::Msg to_event_msg(TestEventDescriptor const&, TestEventDescriptor::payload_type const&);

using SubDescriptor = std::variant<MetronomeEventDescriptor, TestEventDescriptor>; // SubDescriptor

using Sub = std::vector<SubDescriptor>; // Sub lists requested subscriptions
Sub subscriptions(tea::Model const& model);
