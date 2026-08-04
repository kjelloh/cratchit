#pragma once

#include "Msg.hpp"
#include "subscribables.hpp"

/**
 * Overload on event descriptor to define event message to send on event
 * Enables TEA runtime to 'know' how to return the message ascociated with ascociated event
 */
tea::Msg sub_to_msg(MetronomeEventDescriptor const&, MetronomeEventDescriptor::payload_type const&);

// #TEA::events: Free factory function creates message as required by descriptor and message payload
tea::Msg sub_to_msg(TestEventDescriptor const&, TestEventDescriptor::payload_type const&);
