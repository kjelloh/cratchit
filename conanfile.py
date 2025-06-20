from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.errors import ConanInvalidConfiguration

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

    # Binary configuration (defile the keys we want to access)
    settings = "os", "compiler", "build_type", "arch"

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "src/*"
    
    def configure(self):
        # Override C++ std if not already set to 23
        cppstd = getattr(self.settings.compiler, "cppstd", None)
        if cppstd != "23":
            self.output.info(f"Overriding compiler.cppstd from {cppstd} to 23")
            self.settings.compiler.cppstd = "23"
    
    def validate(self):
      if self.settings.compiler.cppstd != "23":
        raise ConanInvalidConfiguration("cratchit requires C++23.")

    def requirements(self):
        self.requires("sol2/3.3.1")
        self.requires("ncurses/6.5")
        self.requires("pugixml/1.14")
        self.requires("icu/76.1")
        self.requires("immer/0.8.1")
        self.requires("spdlog/1.15.0")
        self.requires("gtest/1.15.0")
        
    def layout(self):
        cmake_layout(self)

    def generate(self):
        cppstd = self.settings.get_safe("compiler.cppstd")
        self.output.info(f"cratchit (conanfile.py) Compiler C++ standard (from profile/settings): {cppstd}")        
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.cache_variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = "ON"        
        tc.generate()
        
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    

    
