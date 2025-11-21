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

# Palantir XY Sine integration tests (requires Qt6)
find_package(Qt6 QUIET COMPONENTS Core Network)
if(Qt6_FOUND AND Qt6Core_FOUND AND Qt6Network_FOUND)
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
else()
  message(STATUS "Skipping test_palantir_xysine - Qt6 not found")
endif()

# ---------------------------------------
# Echo RPC test (requires BEDROCK_WITH_TRANSPORT_DEPS)
# ---------------------------------------
if(BEDROCK_WITH_TRANSPORT_DEPS)
  # Check that generated files are available (set in parent CMakeLists.txt)
  if(NOT DEFINED ECHO_GENERATED_DIR)
    message(FATAL_ERROR "ECHO_GENERATED_DIR not set - transport deps may not be configured correctly")
  endif()
  
  # Echo RPC sanity test (in-process server + client)
  add_executable(bedrock_echo_sanity
    bedrock_echo_sanity.cpp
    ../src/echo/echo_service_impl.cpp
    ${ECHO_GENERATED_FILES}
  )
  
  # Ensure proto files are generated before building
  add_dependencies(bedrock_echo_sanity bedrock_echo_proto_gen)
  
  target_include_directories(bedrock_echo_sanity PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/echo
    ${ECHO_GENERATED_DIR}
  )
  
  target_link_libraries(bedrock_echo_sanity PRIVATE
    protobuf::libprotobuf
    gRPC::grpc++
    Threads::Threads
  )
  
  # Add CTest entry
  add_test(NAME bedrock_echo_rpc COMMAND bedrock_echo_sanity)
  set_tests_properties(bedrock_echo_rpc PROPERTIES
    TIMEOUT 10
  )
  
  message(STATUS "Echo RPC test configured: bedrock_echo_sanity")
endif()
