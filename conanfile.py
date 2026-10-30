from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy
import os


class KcoreConan(ConanFile):
    name = "kcore"
    version = "0.1.0"
    package_type = "static-library"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "CMakeLists.txt", "cmake/*", "include/*", "src/*", "LICENSE"
    default_options = {"*:fPIC": True, "*:shared": False}

    def requirements(self):
        self.requires("fmt/10.2.1", transitive_headers=True)
        self.requires("spdlog/1.14.1", transitive_headers=True)
        self.requires("stduuid/1.2.3", transitive_headers=True)
        self.requires("nlohmann_json/3.12.0", transitive_headers=True)
        self.requires("yaml-cpp/0.8.0", transitive_headers=True)
        self.requires("p-ranav-glob/0.0.1")

    def layout(self):
        cmake_layout(self)

    def validate(self):
        check_min_cppstd(self, "23")
        if self.settings.os not in ("Macos", "Linux"):
            raise ConanInvalidConfiguration("kcore supports macOS and Linux.")

    def build_requirements(self):
        self.test_requires("gtest/1.15.0")

    def generate(self):
        CMakeDeps(self).generate()
        toolchain = CMakeToolchain(self)
        toolchain.variables["KCORE_BUILD_TESTS"] = False
        toolchain.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        CMake(self).install()
        copy(self, "LICENSE", self.source_folder,
             os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        self.cpp_info.libs = ["kcore"]
        self.cpp_info.set_property("cmake_file_name", "kcore")
        self.cpp_info.set_property("cmake_target_name", "kcore::kcore")
        if self.settings.os == "Linux":
            self.cpp_info.system_libs = ["pthread"]
