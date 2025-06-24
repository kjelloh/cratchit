#pragma once

#include <exception> // std::terminate
#include <ostream>
#include <istream>
#include <array>
#include <concepts>
#include <coroutine>

// Namespace that overloads on std types ()
namespace std_overload {
  // Note: We could have placed these overloads in either the global namespace or the std namespace.

  //       But placing them in the std namespace is considered bad practice as we would potentially pollute std with declarations with unknown conflicts as a result.
  //       And placing these in the global namespace renders these overloads to NOT be considered by the compiler is used in another (not global) namespace.

  //       Explanation: The compiler will apply C++ argument dependant lookup (ADL), i.e., look for overloads in the std namespace for arguments of std namespace types
  //       This means it will fail to consider this overload as it is not in the std namespace (and it will not look in teh global namespace)

  //       E.g., any 'std::istream >> array-type-variable' will fail to find this overload (fails to compile)

  //       Solution 1: Have the client use the call syntax ::operator>>(stream,array-type-variable)
  //       Solution 2: Place 'using std_overload::operator>>' in the namespace (class or function scope) where we want to use it
  //       Solution 3: Pre-create here the namespaces where we want to use this overload and inject them with 'using ::operator>>' so that the using code don't have to

  // None of the alternatives is fully satisfactory but in Cratchit we have chose alternative 2 for now.

  // Helper to  write an std::array of predefined size to a stream
  // Client namespace needs an 'using std_overload::operator<<' to 'see it'
  template <std::size_t N>
  std::ostream& operator<<(std::ostream& os, std::array<unsigned char, N> const& arr){
    for (int b : arr) os << " " << std::hex << b << std::dec; // Note: We need b to be an int to apply the appropriate integer output formatting
    return os;
  }

  // Helper to read from a stream into an std::array of predefined size
  // Client namespace needs an 'using std_overload::operator>>' to 'see it'
  template<std::size_t N>
  std::istream& operator>>(std::istream& is, std::array<unsigned char, N>& arr) {
      is.read(reinterpret_cast<char*>(arr.data()), arr.size());
      // if (is) {
      //   std::cout << "\noperator>> read array:" << arr;
      // }
      return is;
  }

    // std::ranges replacement (overload) until used with a tool-chain that supports what cratchit requires
    namespace ranges {

        // Helper traits to check if a type is a container
        template<typename T, typename = void>
        struct is_container : std::false_type {};

        template<typename T>
        struct is_container<T, std::void_t<typename T::value_type,
                                           decltype(std::begin(std::declval<T>())),
                                           decltype(std::end(std::declval<T>()))>>
            : std::true_type {};

        template<typename T>
        inline constexpr bool is_container_v = is_container<T>::value;

        // A concept to check if the type is a range
        template<typename T>
        concept range = requires(T t) {
            std::ranges::begin(t);
            std::ranges::end(t);
        };

        // Custom implementation of std::ranges::to
        template<typename Container, range R>
        requires is_container_v<Container>
        Container to(R&& r) {
            using std::ranges::begin, std::ranges::end;
            return Container(begin(r), end(r));
        }

    } // namespace ranges

  namespace coroutine {

    // Until C++ library of comiler supports std::generator
    template<typename T>
    class Generator {
    public:
      struct promise_type {
        T current_value;
        
        Generator get_return_object() {
          return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(T value) {
          current_value = value;
          return {};
        }
        
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
      };
      
      using handle_type = std::coroutine_handle<promise_type>;
      
      explicit Generator(handle_type coroutine) : coroutine_(coroutine) {}
      Generator(const Generator&) = delete;
      Generator(Generator&& other) noexcept : coroutine_(other.coroutine_) {
        other.coroutine_ = nullptr;
      }
      ~Generator() {
        if (coroutine_) {
          coroutine_.destroy();
        }
      }
      
      struct Iterator {
        handle_type coroutine_;
        
        Iterator(handle_type coroutine) : coroutine_(coroutine) {
          if (coroutine_) coroutine_.resume();
        }
        
        Iterator& operator++() {
          coroutine_.resume();
          if (coroutine_.done()) coroutine_ = nullptr;
          return *this;
        }
        
        const T& operator*() const { return coroutine_.promise().current_value; }
        const T* operator->() const { return &coroutine_.promise().current_value; }
        
        bool operator==(const Iterator& other) const { return coroutine_ == other.coroutine_; }
        bool operator!=(const Iterator& other) const { return !(*this == other); }
      };
      
      Iterator begin() {
        return coroutine_ ? Iterator{coroutine_} : end();
      }
      
      Iterator end() {
        return Iterator{nullptr};
      }
      
    private:
      handle_type coroutine_;
    };

  } // namespace coroutine

  template <typename T>
  using generator = coroutine::Generator<T>;

} // namespace std_overload
