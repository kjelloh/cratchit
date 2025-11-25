#pragma once

#include <string>

namespace cratchit {
  namespace functional {
  
    template<typename T>
    struct AnnotatedOptional {
      using Message = std::string;
      std::optional<T> m_value;
      std::vector<Message> m_messages;

      operator bool() const {return m_value.has_value();}
      T const& value() const& {return m_value.value();} // lvalue contxt
      T&& value() && {return std::move(m_value).value();} // rvalue context

      AnnotatedOptional& operator=(T value) {
        m_value = std::move(value);
        return *this;
      }

      // rvalue context
      // Handle move-only-T (e.g. owning std::unique_ptr)
      // move optional<T>: T&& -> f -> next -> merged
      template <class F>
      auto and_then(F&& f) && {
        using result_type = std::invoke_result_t<F, T&&>;
        if (*this) {
          auto next = std::invoke(std::forward<F>(f), std::move(*this).value());

          result_type merged{};
          merged.m_value = std::move(next.m_value); // move to next
          merged.m_messages = this->m_messages;
          merged.m_messages.insert(
              merged.m_messages.end(),
              next.m_messages.begin(),
              next.m_messages.end()
          );
          return merged;
        }
        else {
          result_type result{};
          result.m_messages = this->m_messages;
          return result;
        }
      }

      // lvalue context (called on named value, e.g., first in monadic chain)
      // pass non owning const&: T const& -> f -> next -> merged
      // NOTE: Can NOT be used on move-only T (e.g. std::unique_ptr)
      //       I.e., do std::move(m).and_then(...) to move
      template <class F>
      auto and_then(F&& f) const & {
        using result_type = std::invoke_result_t<F, T const&>;
        static_assert(
            std::is_default_constructible_v<result_type>,
            "f must return an AnnotatedOptional<U>"
        );

        if (*this) {
          auto next = std::invoke(std::forward<F>(f), this->value());

          result_type merged{};
          merged.m_value = std::move(next.m_value);
          merged.m_messages = this->m_messages;
          merged.m_messages.insert(
              merged.m_messages.end(),
              next.m_messages.begin(),
              next.m_messages.end()
          );
          return merged;
        }
        else {
          result_type result{};
          result.m_messages = this->m_messages;
          return result;
        }
      }

      // tap combinator - inject side effects without breaking the chain
      // Allows observation of the value without consuming it
      // rvalue context (for move-only types like unique_ptr)
      template <class F>
      auto tap(F&& f) && {
        if (*this) {
          std::invoke(std::forward<F>(f), this->value());
        }
        return std::move(*this);
      }

      // tap combinator - lvalue context
      template <class F>
      auto tap(F&& f) const & {
        if (*this) {
          std::invoke(std::forward<F>(f), this->value());
        }
        return *this;
      }

      AnnotatedOptional& push_message(Message message) {
        m_messages.push_back(message);
        return *this;
      }

      std::string to_caption() const {
        std::string result{"?caption?"};
        return result;
      }

      // Factory: convert optional<T> + message → AnnotatedOptional<T>
      static AnnotatedOptional from(std::optional<T> maybe, std::string message_on_nullopt) {
        AnnotatedOptional result{};
        result.m_value = std::move(maybe);
        if (!result) result.push_message(std::move(message_on_nullopt));
        return result;
      }

    };

    // lift optional<T> to AnnotatedMaybe<T>
    template <typename T>
    AnnotatedOptional<T> lift_optional(std::optional<T> opt, std::string msg = "") {
        if (opt) {
            return AnnotatedOptional<T>{std::move(*opt)};
        } else {
            AnnotatedOptional<T> result{};
            if (!msg.empty()) result.m_messages.push_back(msg);
            return result;
        }
    }

    // Lift: f: T -> optional<T> to f: T -> AnnotatedOptional<T>
    template<typename F>
    auto to_annotated_nullopt(F&& f, std::string message_on_nullopt) {
      return [f = std::forward<F>(f), message_on_nullopt = std::move(message_on_nullopt)](auto&&... args) {
        using result_t = std::invoke_result_t<F, decltype(args)...>;
        using value_t = typename result_t::value_type;
        return AnnotatedOptional<value_t>::from(
           std::invoke(f, std::forward<decltype(args)>(args)...)
          ,std::move(message_on_nullopt)
        );
      };
    }

  } // functional
} // cratchit

template<typename T>
using AnnotatedMaybe = cratchit::functional::AnnotatedOptional<T>;
