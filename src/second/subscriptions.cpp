#include "subscriptions.hpp"

Subs subscriptions(tea::Model const&) {
  Subs result{};
  result.push_back(MetronomeEventDescriptor{500}); // hard coded (TODO: map from model)
  result.push_back(TestEventDescriptor{}); // POC of subscription to event with payload
  return result;
}