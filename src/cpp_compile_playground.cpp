#include <string>

// Trying to reproduce to_string compileation error
// See docs/thinking/thinking.md 'How can I make all free function to_string overloads to be found by the compiler?'

// 1. A global namespace to_string + namespace specific arg types
// 2. A first::to_string blocks global namespace to_string to bee seen from within namespace first
// 3. Preferred: Put fucntions and processed arg types in same namespace to make ADL work

namespace first {

  struct Date {};
  struct Amount {};

  // [3] Preferred: put functions in same namespace as processed types
  std::string to_string(Date) { return "second::date"; }
  std::string to_string(Amount) { return "second::amount"; }

} // first

namespace second{
  struct Date {};
  struct Amount {};
} // second 

// Use first
using Date = first::Date;
using Amount = first::Amount;

/*
// [1] global to_string will NOT be found by ADL (Date and Amount is in own namespace, not global one)
//     Do NOT do this!
std::string to_string(Date) { return "second::date"; }
std::string to_string(Amount) { return "second::amount"; }
*/

namespace first {

  // [2] ADL WILL find correct to_string in the namespace of Date and Amount
  //     This, this to_string WILL make any global to_string NOT SEEN
  struct DateRange {}; 
  std::string to_string(DateRange) { return "range"; } // Newly added

  void test(Date d, Amount a) {
      to_string(d);  // ERROR: unqualified lookup finds first::to_string(DateRange), no viable function
      to_string(a);  // ERROR: -"-
  }

}


// Trying to reproduce to_string compileation error
// See docs/thinking/thinking.md 'How can I make all free function to_string overloads to be found by the compiler?'

// 20260119 - Why does code below compile while AccountStatementState.cpp requires 
//            'using ::to_string' in namespace first to compile?

namespace N1 {
  struct some_type {};
  std::string to_string(some_type const& x) {
    return "";
  }
} // N1

namespace N2 {
  struct some_type { int x;};
  std::string to_string(some_type const& x) {
    return "";
  }
} // N2

void global_f() {
  auto s1 = to_string(N1::some_type{});
  auto s2 = to_string(N2::some_type{});
}

struct some_global_type{};
std::string to_string(some_global_type const& x) {
  return "";
}

using some_n1_type_alias = N1::some_type;
std::string to_string(some_n1_type_alias const& x) {
  return "";
}

namespace N1 {

  struct some_new_type{};

  std::string to_string(some_new_type const& x) {
    return "";
  }

  void local_f() {
    auto s = to_string(N2::some_type{});
  }
}

namespace N2 {
  void local_f() {
    auto s1 = to_string(N1::some_type{});
    auto s2 = to_string(some_n1_type_alias{});
  }
}
