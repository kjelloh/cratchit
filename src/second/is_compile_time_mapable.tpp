/**
 * First try that is able to trigger compile time error for 
 * what is 'wrong' about a variant alternative to work as a key in std::map.
 * This code triggers compiler error like ''operator<=>' is a private member of 'tea::NoCmd''
 * if we forget to make it public.
 * It seems to be the 'constexpr' directive that puts all this code into 'compile time evaluation'?
 * And this in turn causes the compiler to generate constexpr version of 'a < b'
 * Note: 'static_cast<void>(a < b);' does NOT trigger a 'unused value' warning
 *       But 'static_cast<bool>(a < b);' DOES!
 *       So the semantically correct '[[maybe_unused]] auto x = static_cast<bool>(a < b)' works.
 * 
 *       Oh don't we love all the quirks of C++ ...
 * 
 */ 
#pragma once

template <typename T>
constexpr void require_less_than() {
    constexpr T a{};
    constexpr T b{};
    // static_cast<void>(a < b); // A void value does NOT trigger 'unused value' warning
    // static_cast<bool>(a < b); // Error: Unused value
    // [[maybe_unused]] auto x = static_cast<bool>(a < b); // Works but compiler error reads less 'clear'
    [[maybe_unused]] constexpr auto 
      is_less_comparable = a < b;     // Type is not less-comparable
}      

template <typename... Ts>
constexpr void is_mapable(std::variant<Ts...> const*) {
    (require_less_than<Ts>(), ...);
}

template <typename T>
constexpr void is_compile_time_mapable() {
  is_mapable(static_cast<T const*>(nullptr)); 
}
