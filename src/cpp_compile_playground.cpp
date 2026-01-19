#include <string>

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
