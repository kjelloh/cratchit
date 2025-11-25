#pragma once

#include <string>

namespace cratchit {
  namespace functional {
  
    template<typename T>
    struct AnnotatedOptional {
      using Message = std::string;
      std::optional<T> m_value;
      std::vector<Message> m_messages;

      AnnotatedOptional() = default;
      template <typename U>
      AnnotatedOptional(U&& u_value) : m_value{u_value} {} 

      operator bool() const {return m_value.has_value();}
      T const& value() const& {return m_value.value();} // lvalue contxt
      T&& value() && {return std::move(m_value).value();} // rvalue context

      AnnotatedOptional& operator=(T value) {
        m_value = std::move(value);
        return *this;
      }

    private:
      // Shared helper: merge messages from this and next
      template<typename Next>
      static auto merge_messages(auto&& self, Next&& next) {
        using result_type = std::remove_cvref_t<Next>;
        result_type merged{};
        merged.m_value = std::move(next.m_value);
        merged.m_messages = std::forward<decltype(self)>(self).m_messages;
        merged.m_messages.insert(
            merged.m_messages.end(),
            next.m_messages.begin(),
            next.m_messages.end()
        );
        return merged;
      }

      // Shared helper: propagate failure with messages
      template<typename ResultType>
      static ResultType propagate_failure(auto&& self) {
        ResultType result{};
        result.m_messages = std::forward<decltype(self)>(self).m_messages;
        return result;
      }

    public:
      // rvalue context
      // Handle move-only-T (e.g. owning std::unique_ptr)
      // move optional<T>: T&& -> f -> next -> merged
      template <class F>
      auto and_then(F&& f) && {
        using result_type = std::invoke_result_t<F, T&&>;

        if (!*this) {
          return propagate_failure<result_type>(std::move(*this));
        }

        auto next = std::invoke(std::forward<F>(f), std::move(*this).value());
        return merge_messages(std::move(*this), std::move(next));
      }

      // lvalue context (called on named value, e.g., first in monadic chain)
      // pass non owning const&: T const& -> f -> next -> merged
      // NOTE: Can NOT be used on move-only T (e.g. std::unique_ptr)
      //       I.e., do std::move(m).and_then(...) to move
      template <class F>
      auto and_then(F&& f) const & {
        using result_type = std::invoke_result_t<F, T const&>;

        if (!*this) {
          return propagate_failure<result_type>(*this);
        }

        auto next = std::invoke(std::forward<F>(f), this->value());
        return merge_messages(*this, std::move(next));
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
        m_messages.push_back(std::move(message));
        return *this;
      }

      // Generate single-line caption from messages
      std::string to_caption() const {
        if (m_messages.empty()) {
          return *this ? "[ok]" : "[empty]";
        }

        std::string result;
        for (size_t i = 0; i < m_messages.size(); ++i) {
          if (i > 0) result += " | ";
          result += m_messages[i];
        }
        return result;
      }

    };

    // Free function: convert optional<T> + message → AnnotatedOptional<T>
    // Enables template argument deduction on return type
    template<typename T>
    AnnotatedOptional<T> annotated_from(std::optional<T> maybe, std::string message_on_nullopt) {
      AnnotatedOptional<T> result{};
      result.m_value = std::move(maybe);
      if (!result) result.push_message(std::move(message_on_nullopt));
      return result;
    }

    // Lift: f: T -> optional<T> to f: T -> AnnotatedOptional<T>
    template<typename F>
    auto to_annotated_nullopt(F&& f, std::string message_on_nullopt) {
      return [f = std::forward<F>(f), message_on_nullopt = std::move(message_on_nullopt)](auto&&... args) {
        using result_t = std::invoke_result_t<F, decltype(args)...>;
        using value_t = typename result_t::value_type;
        return annotated_from(
           std::invoke(f, std::forward<decltype(args)>(args)...),
           message_on_nullopt
        );
      };
    }

  } // functional
} // cratchit

template<typename T>
using AnnotatedMaybe = cratchit::functional::AnnotatedOptional<T>;
