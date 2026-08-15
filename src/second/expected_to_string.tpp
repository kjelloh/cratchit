#pragma once

#include <string>

namespace detail {

    // Concept that there exists a concrete_expected_value_to_string for the concrete value type
    template<typename ConcreteValue>
    concept StringifiableExpectedValue = requires(ConcreteValue concrete_expected_value) {
      { concrete_expected_value_to_string(concrete_expected_value) } -> std::same_as<std::string>;
    };

    template<typename ConcreteValue>
    std::string concrete_expected_value_to_string_dispatch(ConcreteValue const& concrete_value) {
      if constexpr (StringifiableExpectedValue<ConcreteValue>) {
        return concrete_expected_value_to_string(concrete_value);
      }
      else {
        const std::type_info& ti = typeid(concrete_value);
        return std::format(
          "'{}'::{}"
          ,ti.name()
          ,ti.hash_code()
        );
      }
    } // concrete_expected_value_to_string_dispatch

    // Concept that there exists a concrete_expected_value_to_string for the concrete value type
    template<typename ConcreteError>
    concept StringifiableError = requires(ConcreteError concrete_error) {
      { concrete_error_to_string(concrete_error) } -> std::same_as<std::string>;
    };

    template<typename ConcreteError>
    std::string concrete_error_to_string_dispatch(ConcreteError const& concrete_error) {
      if constexpr (StringifiableError<ConcreteError>) {
        return concrete_error_to_string(concrete_error);
      }
      else {
        const std::type_info& ti = typeid(concrete_error);
        return std::format(
          "'{}'::{}"
          ,ti.name()
          ,ti.hash_code()
        );
      }
    } // concrete_error_to_string_dispatch

} // detail

template <typename T>
std::string expected_to_string(T expected) {
  if (expected) {
    return std::format(
       "value={}"
      ,detail::concrete_expected_value_to_string_dispatch(expected.value())
    );
  }
  
  return std::format(
       "error={}"
      ,detail::concrete_error_to_string_dispatch(expected.error())
  );
}