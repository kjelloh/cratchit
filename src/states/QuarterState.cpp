#include "Q1State.hpp"

namespace first {
  // ----------------------------------
  Q1State::Q1State(StateImpl::UX ux) : StateImpl{ux} {
    this->add_option('0',{"VAT Returns",VATReturns_factory});
  }
}