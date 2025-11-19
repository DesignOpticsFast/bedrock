include(FetchContent)

# Fetch GoogleTest (no system install required)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# Workaround for old CMake policies in CI
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(bedrock_tests
  test_math.cpp
)

target_link_libraries(bedrock_tests
  PRIVATE
    bedrock_core
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(bedrock_tests)

# XY Sine compute tests
add_executable(test_xysine_compute
  test_xysine_compute.cpp
)

target_include_directories(test_xysine_compute
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(test_xysine_compute
  PRIVATE
    bedrock_core
    GTest::gtest_main
)

# Link protobuf if available
find_package(Protobuf QUIET)
if(Protobuf_FOUND)
  target_link_libraries(test_xysine_compute PRIVATE protobuf::libprotobuf)
endif()

gtest_discover_tests(test_xysine_compute)

# Palantir XY Sine integration tests
add_executable(test_palantir_xysine
  test_palantir_xysine.cpp
  ../src/palantir/PalantirServer.cpp
)

target_include_directories(test_palantir_xysine
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(test_palantir_xysine
  PRIVATE
    bedrock_core
    Qt6::Core
    Qt6::Network
    GTest::gtest_main
)

# Link protobuf if available
find_package(Protobuf QUIET)
if(Protobuf_FOUND)
  target_link_libraries(test_palantir_xysine PRIVATE protobuf::libprotobuf)
endif()

gtest_discover_tests(test_palantir_xysine)
