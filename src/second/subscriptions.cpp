#include "subscriptions.hpp"

Sub subscriptions(tea::Model const&) {
  Sub result{};
  result.push_back(MetronomeEventDescriptor{500}); // hard coded (TODO: map from model)
  result.push_back(TestEventDescriptor{}); // POC of subscription to event with payload
  return result;
}