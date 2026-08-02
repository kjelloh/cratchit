/**
 * This is The Elm Architecture (TEA) client provided Sub type and subscriptions() function
 * The name Msg is used to honor Elm tutorial on the Elm architecture
 * See https://guide.elm-lang.org/architecture/
 */

#pragma once

#include "Model.hpp"
#include "subscribeables.hpp"

/**
 * Overload on event descriptor to define event message to send on event
 * Enables TEA runtime to 'know' how to return the message ascociated with ascociated event
 */
tea::Msg to_event_msg(MetronomeEventDescriptor const&, MetronomeEventDescriptor::payload_type const&);
tea::Msg to_event_msg(TestEventDescriptor const&, TestEventDescriptor::payload_type const&);

using SubDescriptor = Subscribeable;

// Use the name Sub to honour the Elm tutorial on The Elm Architecture (TEA)
using Sub = std::vector<SubDescriptor>; // Sub lists requested subscriptions
Sub subscriptions(tea::Model const& model);
