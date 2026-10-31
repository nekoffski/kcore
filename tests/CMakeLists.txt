find_package(GTest CONFIG REQUIRED)
add_executable(kcore-tests
    CoreTest.cpp EnumTest.cpp ErrorTest.cpp FunctionalTest.cpp HandleTest.cpp
    IdAllocatorTest.cpp LogTest.cpp ProfilerTest.cpp RandomTest.cpp RefTest.cpp
    SingletonTest.cpp StringTest.cpp TimeTest.cpp UnwrapTest.cpp
    containers/ContainersTest.cpp
    io/EnvTest.cpp io/FileSystemTest.cpp
    runtime/ThreadSafeQueueTest.cpp runtime/ThreadTest.cpp runtime/SignalHandlerTest.cpp runtime/SyncTest.cpp
    serialization/JsonTest.cpp serialization/YamlTest.cpp
)
target_include_directories(kcore-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kcore-tests PRIVATE kcore::kcore GTest::gmock_main)
include(GoogleTest)
gtest_discover_tests(kcore-tests PROPERTIES LABELS unit TIMEOUT 10)
