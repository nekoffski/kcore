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

## Result pipelines

`<kcore/Pipeline.hh>` provides an eager, synchronous chain over `kc::Result`.
Pass the first callable to `startPipeline`; it executes immediately. Later
success steps run in order until one fails. Callables are invoked directly
and are not stored or type-erased.

```cpp
#include <kcore/Pipeline.hh>

const auto load = []() -> kc::Result<int> { return 7; };
const auto validate = [](int n) -> kc::Result<void> {
    if (n < 0) {
        return kc::Error::unexpected(kc::ErrorCode::invalidArgument, "Negative value");
    }
    return {};
};
const auto toText = [](int n) { return std::to_string(n); };

auto result = kc::startPipeline(load)
    .check(validate)
    .transform(toText)
    .result(); // Result<std::string>
```

| Method | Callable returns | Effect |
|---|---|---|
| `then` | `Result<U>` | Replace the value with U, or propagate failure |
| `transform` | Ordinary U | Replace the value with U |
| `check` | `Result<void>` | Borrow the value as `const T&`; retain it on success |
| `inspect` | `void` | Observe `const T&` and retain the value |
| `mapError` | `Error` | Transform an error; skip on success |
| `recover` | `Result<T>` | Handle an error; skip on success |
| `result` | No callable | Return the owned result |

For `Result<void>`, success callables take no arguments. `then` returning
`Result<void>`, or `transform` returning `void`, discards the previous value;
use `check` to retain it instead.
Recovery applies to every preceding error that reaches that stage. A failed
recovery continues propagating its error; successful recovery resumes the chain.

`then`, `transform`, `mapError`, and `recover` forward values/errors with the
same lvalue, const, and rvalue categories as `std::expected`. Named lambdas can
be passed without copying, including move-only lambdas. A temporary pipeline
can transfer ownership to a callback; a named pipeline is not implicitly
moved from. `check` and `inspect` copy the result from lvalue pipelines and
move it from temporaries. `result()` likewise copies from lvalues and moves
from temporaries, returning a value rather than a potentially dangling reference.

The pipeline itself does not dynamically allocate. Retaining stages and
extraction can move payloads; contained types and callbacks may allocate or
throw. Exceptions propagate without being converted to `Error`.

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
