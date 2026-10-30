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

Use a Conan profile configured for a C++23 compiler and run:

```sh
conan install . --build=missing -s compiler.cppstd=23 -s build_type=Debug
cmake --preset conan-debug
cmake --build --preset conan-debug
```

The generated preset name depends on your CMake generator. The Conan recipe
disables tests for package builds. To run the standalone tests, configure with
`cmake --preset conan-debug -DKCORE_BUILD_TESTS=ON`, build, then run
`ctest --test-dir build/Debug --output-on-failure`.

Create a package in your local Conan cache with:

```sh
conan create . --build=missing -s compiler.cppstd=23
```

A package consumer requires `kcore/0.1.0` in Conan, then uses
`find_package(kcore CONFIG REQUIRED)` and links `kcore::kcore`. With a CMake
installation, the same interface is available after `cmake --install` and
setting `CMAKE_PREFIX_PATH` to the installation prefix.

## Errors

`kc::Result<T>` is `std::expected<T, kc::Error>`. An error stores a
`std::error_code` and a detailed message. kcore errors use `kc::ErrorCode`;
projects can supply their own enums by defining `make_error_code` in the enum's
namespace and specializing `std::is_error_code_enum`.

```cpp
auto error = kc::Error{kc::ErrorCode::fileNotFound, "Missing file: {}", path};
auto text = fmt::format("{}", error); // code: kcore:9, details: 'Missing file: ...'
```

Error codes retain their category when passed between library and application
code. Custom categories must have a stable lifetime and one shared instance.

## Layout

- Root headers: common types, concepts, errors, strings, identity, logging, profiling.
- `containers/`: data structures.
- `runtime/`: threads, queues, signals, synchronization helpers.
- `io/`: filesystem and environment helpers.
- `serialization/`: JSON and YAML helpers.

Profiling can be enabled with `-DKCORE_ENABLE_PROFILING=ON`.

`ThreadPool` was extracted with unfinished queue/worker implementation; submitted
tasks are not executed yet. Use it only after that implementation is completed.
