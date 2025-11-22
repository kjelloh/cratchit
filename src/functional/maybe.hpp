#pragma once

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


    };
    
  } // functional
} // cratchit

template<typename T>
using AnnotatedMaybe = cratchit::functional::AnnotatedOptional<T>;
