#pragma once

#include <mutex>

template <typename T>
class ThreadSafeT {
public:
  ThreadSafeT(T const& t = T{}) : m_t{t} {}

  template <typename F>
  decltype(auto) apply(F f) {
      using result_t = decltype(f(std::declval<T&>()));

      // Protect against f that returns access to thread safe m_t.
      // We cant allow this (or refactor to allow const& and const* ?)
      static_assert(!std::is_reference_v<result_t>, "ThreadSafeT::apply() cannot return a reference");
      static_assert(!std::is_pointer_v<result_t>, "ThreadSafeT::apply() cannot return a pointer");

      std::lock_guard<std::mutex> guard(m_access_mutex);
      return f(m_t);
  }

private:
  std::mutex m_access_mutex{};
  T m_t;
};