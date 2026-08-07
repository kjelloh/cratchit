/**
 * This is The Elm Architecture (TEA) client provided Subs type and subscriptions() function
 * The name Msg is used to honor Elm tutorial on the Elm architecture
 * See https://guide.elm-lang.org/architecture/
 */

#pragma once

#include "Model.hpp"
#include "Sub.hpp"

namespace app {

  // #TEA::events: Client looks into model and returns list of descriptors of events to listen to
  // See code tagging with '#TEA::events' for relevant components
  tea::Subs subscriptions(app::Model const& model);

} // app

