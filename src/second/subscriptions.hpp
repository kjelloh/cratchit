/**
 * This is The Elm Architecture (TEA) client provided Sub type and subscriptions() function
 * The name Msg is used to honor Elm tutorial on the Elm architecture
 * See https://guide.elm-lang.org/architecture/
 */

#pragma once

#include "Model.hpp"
#include "subscribables.hpp"

// #TEA::events: Event descriptor alias
using SubDescriptor = Subscribable;

// #TEA::events: List of descriptors of events to listen to
using Sub = std::vector<SubDescriptor>;

// #TEA::events: Client looks into model and returns list of descriptors of events to listen to
// See code tagging with '#TEA::events' for relevant components
Sub subscriptions(tea::Model const& model);
