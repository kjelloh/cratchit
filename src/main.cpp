#include "cratchit.h"
#include <vector>
#include <string>

int main() {
    cratchit();

    std::vector<std::string> vec;
    vec.push_back("test_package");

    cratchit_print_vector(vec);
}
