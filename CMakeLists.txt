cmake_minimum_required(VERSION 3.23)
project(kcore VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(KCORE_BUILD_TESTS "Build kcore tests" ${PROJECT_IS_TOP_LEVEL})
option(KCORE_ENABLE_PROFILING "Enable profiling instrumentation" OFF)

find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(stduuid CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(p-ranav-glob CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_library(kcore STATIC
    src/Core.cpp
    src/Error.cpp
    src/ErrorCode.cpp
    src/Handle.cpp
    src/Id.cpp
    src/Log.cpp
    src/Profiler.cpp
    src/Random.cpp
    src/String.cpp
    src/io/FileSystem.cpp
    src/runtime/SignalHandler.cpp
    src/runtime/Thread.cpp
    src/runtime/ThreadPool.cpp
    src/serialization/Json.cpp
    src/serialization/Yaml.cpp
)
add_library(kcore::kcore ALIAS kcore)
target_compile_features(kcore PUBLIC cxx_std_23)
set_target_properties(kcore PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
target_include_directories(kcore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(kcore PUBLIC
    fmt::fmt spdlog::spdlog stduuid::stduuid nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp Threads::Threads
    PRIVATE p-ranav-glob::p-ranav-glob
)
if(KCORE_ENABLE_PROFILING)
    target_compile_definitions(kcore PUBLIC KCORE_ENABLE_PROFILING=1)
endif()

if(KCORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS kcore EXPORT kcoreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/kcore DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT kcoreTargets NAMESPACE kcore:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kcore)
configure_package_config_file(cmake/kcoreConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/kcoreConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kcore
)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/kcoreConfigVersion.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kcoreConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kcoreConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kcore
)
