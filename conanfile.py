import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout

class AdaptiveAutosarConan(ConanFile):
    name = "adaptive-autosar"
    version = "0.2.0"

    settings = "os", "compiler", "build_type", "arch"

    options = {
        "fPIC": [True, False],
        "build_apps": [True, False],
        "build_tests": [True, False],
    }

    default_options = {
        "fPIC": True,
        "build_apps": False,
        "build_tests": False,
    }

    def requirements(self):
        self.requires("boost/1.83.0")
        self.requires("fmt/10.1.1")
        if self.options.build_tests:
            self.test_requires("gtest/1.14.0")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)
        self.folders.build = str(self.settings.build_type)
        self.folders.generators = os.path.join(self.folders.build, "generators")

    def generate(self):
        toolchain = CMakeToolchain(self)
        toolchain.variables["OPEN_AA_BUILD_APPS"] = self.options.build_apps
        toolchain.variables["OPEN_AA_BUILD_TESTS"] = self.options.build_tests
        toolchain.variables["OPEN_AA_VERSION"] = self.version
        toolchain.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
