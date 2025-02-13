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

    int result{};
    while (true) {
        // Toggle between zeroth (older) and first (this variant) of cratching
        if (result = zeroth::main(argc, argv);result > 0) break;
        // std::cout << "\nCentral main sais Hello :)" << std::flush;
        if (result = first::main(argc,argv);result == 0) break;
    }
    // std::cout << "\nCentral main sais Bye :)" << std::flush;
    return result;
}
