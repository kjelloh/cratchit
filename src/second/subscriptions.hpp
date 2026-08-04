/**
 * This is The Elm Architecture (TEA) client provided Sub type and subscriptions() function
 * The name Msg is used to honor Elm tutorial on the Elm architecture
 * See https://guide.elm-lang.org/architecture/
 */

#pragma once

#include "Model.hpp"
#include "subscribables.hpp"

/**
 * Overload on event descriptor to define event message to send on event
 * Enables TEA runtime to 'know' how to return the message ascociated with ascociated event
 */
tea::Msg to_event_msg(MetronomeEventDescriptor const&, MetronomeEventDescriptor::payload_type const&);

// #TEA::events: Free factory function creates message as required by descriptor and message payload
tea::Msg to_event_msg(TestEventDescriptor const&, TestEventDescriptor::payload_type const&);

// #TEA::events: Event descriptor alias
using SubDescriptor = Subscribable;

// #TEA::events: List of descriptors of events to listen to
using Sub = std::vector<SubDescriptor>;

// #TEA::events: Client looks into model and returns list of descriptors of events to listen to
// See code tagging with '#TEA::events' for relevant components
Sub subscriptions(tea::Model const& model);
