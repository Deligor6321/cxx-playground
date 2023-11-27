import os

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.build import check_min_cppstd


class CxxPlaygroundRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    requires = "ms-gsl/4.0.0"
    test_requires = "benchmark/1.8.3", "catch2/3.4.0"

    def validate_build(self):
        check_min_cppstd(self, 23)

    def generate(self):
        cmake_deps = CMakeDeps(self)
        cmake_deps.generate()
        cmake_toolchain = CMakeToolchain(self)
        cmake_toolchain.cache_variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = True
        cmake_toolchain.generate()

    def layout(self):
        cmake_layout(self)
