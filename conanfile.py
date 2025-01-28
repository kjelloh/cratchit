from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class cratchitRecipe(ConanFile):
    name = "cratchit"
    version = "0.6"
    package_type = "application"

    # Optional metadata
    license = "CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)"
    author = "Kjell-Olov Högdahl kjell-olov.hogdahl@itfied.se"
    url = "https://github.com/kjelloh/cratchit.git"
    description = "C++ console app for Swedish book keeping"
    topics = ("SIE", "BAS", "SKV","Finance", "Book keeping")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "src/*"

    def requirements(self):
        self.requires("sol2/3.3.1")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    

    
