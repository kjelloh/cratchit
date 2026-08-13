#include "view_to_string.hpp"

namespace detail {

  // Concept that there exists a concrete_view_to_string for the concrete cmd type
  template<typename ConcreteView>
  concept StringifiableView = requires(ConcreteView concrete_view) {
    { concrete_view_to_string(concrete_view) } -> std::same_as<std::string>;
  };

  // Dispatch concrete_view_to_string if it exists for the concrete cmd type
  // ,otherwise return a fallback string with general type info
  template<typename ConcreteView>
  std::string concrete_view_to_string_dispatch(ConcreteView const& concrete_view) {
    if constexpr (StringifiableView<ConcreteView>) {
      return concrete_view_to_string(concrete_view);
    }
    else {
      const std::type_info& ti = typeid(concrete_view);
      return std::format(
        "'{}'::{}"
        ,ti.name()
        ,ti.hash_code()
      );
    }
  } // concrete_view_to_string_dispatch

} // detail

std::string view_to_string(ViewState const& view) {
  return std::visit(
    [](auto const& concrete_view) {
      return detail::concrete_view_to_string_dispatch(concrete_view);
    }
    ,view
  );
}

std::string concrete_view_to_string(RuntimeView const&) {
  return std::format("{}","RuntimeView");
}
