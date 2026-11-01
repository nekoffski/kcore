# kcore

A C++23 utility library for macOS and Linux. Public types and functions use
the `kc` namespace; consumers link one target, `kcore::kcore`.

## Use as a submodule

```sh
git submodule add https://github.com/nekoffski/kcore.git external/kcore
```

Provide the dependencies listed in `conanfile.py` through your project's Conan
setup, then add the library before the targets that use it:

```cmake
add_subdirectory(external/kcore)
target_link_libraries(your_target PRIVATE kcore::kcore)
```

```cpp
#include <kcore/Core.hh>
#include <kcore/containers/FlatMap.hh>
#include <kcore/io/FileSystem.hh>
```

## Build or package

Requires Conan 2, CMake, Make, and a C++23 compiler. If you don't have a default
Conan profile, create one with `conan profile detect`, then run:

```sh
make build
make test
make clean
```

The Makefile installs dependencies and builds the library, tests, and sandbox
in Debug mode. Use `BUILD_TYPE=Release` for Release builds,
`CONAN_PROFILE=path/to/profile` for another Conan profile, or `JOBS=4` to set
build parallelism.

Enable profiling with `-DKCORE_ENABLE_PROFILING=ON` when configuring CMake.

Create a package in your local Conan cache with:

```sh
conan create . --build=missing -s compiler.cppstd=23
```

A package consumer requires `kcore/0.1.0` in Conan, then uses
`find_package(kcore CONFIG REQUIRED)` and links `kcore::kcore`. With a CMake
installation, the same interface is available after `cmake --install` and
setting `CMAKE_PREFIX_PATH` to the installation prefix.

## Sandbox

Build and run with:

```sh
make sandbox
make sandbox BUILD_TYPE=Release
```
