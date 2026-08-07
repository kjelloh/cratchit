#include "subscriptions.hpp"

namespace app {

  tea::Subs subscriptions(app::Model const&) {
    tea::Subs result{};
    result.push_back(tea::MetronomeEventDescriptor{500}); // hard coded (TODO: map from model)
    result.push_back(tea::TestEventDescriptor{}); // POC of subscription to event with payload
    return result;
  } // subscriptions

} // app
