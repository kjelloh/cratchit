#include "cratchit.h"
#include <vector>
#include <string>
#include "zeroth/main.cpp" // zeroth variant while refactoring into 'this' variant

int main(int argc, char *argv[]) {

    // shoe horned in conan package manager test code (just to keep it for now)
    if (true) {
        cratchit();
        std::vector<std::string> vec;
        vec.push_back("test_package");
        cratchit_print_vector(vec);
    }

    // Use zeroth source code until refactored into 'this' refactored version
    return zeroth::main(argc, argv);
}
