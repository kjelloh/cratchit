#include "subscriptions.hpp"


Sub subscriptions(tea::Model const&) {
  Sub result{};
  result.push_back(MetronomeSub{500}); // hard coded (TODO: map from model)
  return result;
}