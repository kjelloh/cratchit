#include "double_dispatch_accept.hpp"
#include "double_dispatch_accept.tpp"

ViewState double_dispatch_accept(ViewState const& target, ViewState const& source) {
  // 1. Dispatch to target accept
  return std::visit(
    [&source](auto const& concrete_target){
      return detail::accept(concrete_target,source);
    }
    ,target
  );
  return target;
} // double_dispatch_accept
