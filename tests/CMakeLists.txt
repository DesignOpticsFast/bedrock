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
