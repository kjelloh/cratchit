#pragma once

#include <stdexcept> // std::runtime_error

namespace cratchit {
  namespace functional {
    namespace memory {

      template <typename T>
      struct OwningMaybeRef {
        using value_type = std::unique_ptr<T>;
        std::unique_ptr<T> m_p{};
        operator bool() const & {return m_p != nullptr;}
        T& value() const & {
          if (*this) {
            return *m_p;
          }
          throw std::runtime_error("Access to 'null' OwningMaybeRef<T>");
        }

        template <class F>
        auto and_then(F&& f) const & {
            using result_type = std::invoke_result_t<F, T&>;
            static_assert(
                std::is_default_constructible_v<result_type>,
                "f must return an optional-like type T which is default-constructible"
            );

            if (*this)
                return std::invoke(std::forward<F>(f), value());
            else
                return result_type{}; // empty
        }
      };

      template <typename T>
      struct MaybeRef {
        using value_type = T*;
        T* m_p{}; // non owning 'plain' as-ref wrapper

        operator bool() const & {return m_p != nullptr;}

        T& value() const & {
          if (*this) {
            return *m_p;
          }
          throw std::runtime_error("Access to 'null' MaybeRef<T>");
        }

        template <class F>
        auto and_then(F&& f) const & {
            using result_type = std::invoke_result_t<F, T&>;
            static_assert(
                std::is_default_constructible_v<result_type>,
                "f must return an optional-like type T which is default-constructible"
            );

            if (*this)
                return std::invoke(std::forward<F>(f), value());
            else
                return result_type{}; // empty
        }

        static MaybeRef from(T& t) {return {&t};}

      };


    } // memory
  } // functional
} // cratchit