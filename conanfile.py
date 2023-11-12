import os

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, cmake_layout


class CxxPlaygroundRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("benchmark/1.8.3")
        self.requires("catch2/3.4.0")

    def generate(self):
        cmake_deps = CMakeDeps(self)
        cmake_deps.generate()
        cmake_toolchain = CMakeToolchain(self)
        cmake_toolchain.cache_variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = True
        cmake_toolchain.generate()

    def layout(self):
        cmake_layout(self)
