#include "sub_to_string.hpp"

#include <format>

namespace tea {

  namespace detail {

    // Concept that there exists a concrete_sub_to_string for the concrete sub type
    template<typename ConcreteSub>
    concept StringifiableSub = requires(ConcreteSub concrete_sub) {
      { concrete_sub_to_string(concrete_sub) } -> std::same_as<std::string>;
    };

    // Dispatch concrete_sub_to_string if it exists for the concrete sub type
    // ,otherwise return a fallback string with general type info
    template<typename ConcreteSub>
    std::string concrete_sub_to_string_dispatch(ConcreteSub const& concrete_sub) {
      if constexpr (StringifiableSub<ConcreteSub>) {
        return concrete_sub_to_string(concrete_sub);
      }
      else {
        const std::type_info& ti = typeid(concrete_sub);
        return std::format(
          "'{}'::{}"
          ,ti.name()
          ,ti.hash_code()
        );
      }
    } // concrete_sub_to_string_dispatch

  } // detail

  // Dispacth Sub (variant) to string
  std::string sub_to_string(Sub const& sub) {
    return std::visit(
      [](auto const& concrete_sub) -> std::string {
        return detail::concrete_sub_to_string_dispatch(concrete_sub);
      }
      ,sub
    );
  }

  // Concrete (actual sub) to string
  std::string concrete_sub_to_string(TestEventDescriptor const& s) {
    return std::format("{}:{:X}","TestEventDescriptor",s.arg);
  }

} // tea

