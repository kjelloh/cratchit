from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import cmake_layout


class CratchitConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("zlib/1.3.1")  # walk_pdf: compressed PDF streams
        self.test_requires("gtest/1.15.0")

    def validate(self):
        check_min_cppstd(self, "23")

    def layout(self):
        cmake_layout(self)
