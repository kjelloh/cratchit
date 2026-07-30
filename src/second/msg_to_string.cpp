#include "msg_to_string.hpp"

#include <format>

namespace detail {

  // Concept that there exists a concrete_msg_to_string for the concrete msg type
  template<typename ConcreteMsg>
  concept StringifiableMsg = requires(ConcreteMsg concrete_msg) {
    { concrete_msg_to_string(concrete_msg) } -> std::same_as<std::string>;
  };

  // Dispatch concrete_msg_to_string if it exists for the concrete msg type
  // ,otherwise return a fallback string with general type info
  template<typename ConcreteMessage>
  std::string concrete_msg_to_string_dispatch(ConcreteMessage const& concrete_msg) {
    if constexpr (StringifiableMsg<ConcreteMessage>) {
      return concrete_msg_to_string(concrete_msg);
    }
    else {
      const std::type_info& ti = typeid(concrete_msg);
      return std::format(
         "No concrete_msg_to_string for msg type :'{}' hash_code:{}"
        ,ti.name()
        ,ti.hash_code()
      );
    }
  } // concrete_msg_to_string_dispatch

} // detail

// Dispacth Msg (variant) to string
std::string msg_to_string(tea::Msg const& msg) {
  return std::visit(
    [](auto const& concrete_msg) -> std::string {
      return detail::concrete_msg_to_string_dispatch(concrete_msg);
    }
    ,msg
  );
}

// Concrete (actual message) to string
std::string concrete_msg_to_string(tea::UnicodeKeyMsg const& m) {
  return std::format("{}:{:X}","UnicodeKeyMsg",static_cast<uint32_t>(m.code_point));
}
