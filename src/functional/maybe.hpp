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
      T const& value() const {return m_value.value();}

      AnnotatedOptional& operator=(T value) {
        m_value = std::move(value);
        return *this;
      }

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

    // Lift: wrap optional<T>-returning function → AnnotatedOptional<T>-returning function
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
