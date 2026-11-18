from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.errors import ConanInvalidConfiguration

class cratchitRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    name = "cratchit"
    version = "0.6"
    package_type = "application"

    # Optional metadata
    license = "CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)"
    author = "Kjell-Olov Högdahl kjell-olov.hogdahl@itfied.se"
    url = "https://github.com/kjelloh/cratchit.git"
    description = "C++ console app for Swedish book keeping"
    topics = ("SIE", "BAS", "SKV","Finance", "Book keeping")

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "src/*"

    # with_legacy: compile in the variants zeroth, first and second, which build on
    # macOS only (CMake option CRATCHIT_WITH_LEGACY). Without them cratchit is
    # variant third only (src/third), which builds on Windows, Linux and macOS.
    # Default: True on macOS, False elsewhere. Turn off with -o "&:with_legacy=False".
    options = {"with_legacy": [True, False]}
    default_options = {"with_legacy": True}

    def config_options(self):
        if self.settings.os != "Macos":
            self.options.with_legacy = False

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
        # Variant third (src/third)
        self.requires("zlib/1.3.1")  # walk_pdf: compressed PDF streams
        self.requires("gtest/1.15.0")  # third's tests, and the legacy variants' --test

        # The legacy variants zeroth, first and second
        if self.options.with_legacy:
            self.requires("cli11/2.6.2")
            self.requires("sol2/3.3.1")
            self.requires("ncurses/6.5")
            self.requires("pugixml/1.14")
            self.requires("icu/76.1")
            self.requires("immer/0.8.1")
            self.requires("spdlog/1.15.0")
            self.requires("raylib/6.0")
        
    def layout(self):
        cmake_layout(self)

    def generate(self):        
        cppstd = self.settings.get_safe("compiler.cppstd")
        self.output.info(f"cratchit (conanfile.py) Compiler C++ standard (from profile/settings): {cppstd}")        
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.cache_variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = "ON"        
        tc.cache_variables["CRATCHIT_WITH_LEGACY"] = bool(self.options.with_legacy)
        tc.generate()
        
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    

    
