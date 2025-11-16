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

  // Credit: https://andreasfertig.com/blog/2023/07/visiting-a-stdvariant-safely/
  // Creates a struct type 'overload' that implements all operator() provided by provided types.
  // It also implements an extra operator() for any type that does not math any of the provided.
  // I understand it like this.
  // 1) <class... Ts> matches any list of types provided
  // 2) 'struct overload : Ts...' expands to 'struct overload : T1,T2,T3,...' (Ti a provided type)
  //    That is, the struct inherits from all provided types.
  // 3) 'using Ts::operator()...;' expands to 'using T1::operator();using T2::operator();using T3::operator()...;'
  //    As I understands it the expansion is made with ';' separators - because it is a declaration expansion?
  //    Also, the 'using' brings the inherited operator() into 'scope'. Without it the C++ name lookup
  //    will not see all inherited operator() (virtual or not)? It has something to do with
  //    the behaviour of multipple inheritance name lookup?
  template<class... Ts>
  struct overload : Ts...
  {
    using Ts::operator()...;

    // Prevent implicit type conversions
    consteval void operator()(auto) const
    {
      static_assert(false, "Unsupported type");
    }
  };

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

} // namespace std_overload
