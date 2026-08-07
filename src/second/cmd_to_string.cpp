#include "cmd_to_string.hpp"

#include <format>


namespace tea {

  namespace detail {

    // Concept that there exists a concrete_cmd_to_string for the concrete cmd type
    template<typename ConcreteCmd>
    concept StringifiableCmd = requires(ConcreteCmd concrete_cmd) {
      { concrete_cmd_to_string(concrete_cmd) } -> std::same_as<std::string>;
    };

    // Dispatch concrete_cmd_to_string if it exists for the concrete cmd type
    // ,otherwise return a fallback string with general type info
    template<typename ConcreteCmd>
    std::string concrete_cmd_to_string_dispatch(ConcreteCmd const& concrete_cmd) {
      if constexpr (StringifiableCmd<ConcreteCmd>) {
        return concrete_cmd_to_string(concrete_cmd);
      }
      else {
        const std::type_info& ti = typeid(concrete_cmd);
        return std::format(
          "'{}'::{}"
          ,ti.name()
          ,ti.hash_code()
        );
      }
    } // concrete_cmd_to_string_dispatch

  } // detail

  // Dispacth Cmd (variant) to string
  std::string cmd_to_string(Cmd const& cmd) {
    return std::visit(
      [](auto const& concrete_cmd) -> std::string {
        return detail::concrete_cmd_to_string_dispatch(concrete_cmd);
      }
      ,cmd
    );
  }

  // Concrete (actual message) to string
  std::string concrete_cmd_to_string(TestCmdDescriptor const& c) {
    return std::format("{}:{:X}","TestCmdDescriptor",static_cast<uint32_t>(c.arg));
  }

} // tea