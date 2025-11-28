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
  # Palantir tests (only when transport deps enabled)
  $<$<BOOL:${BEDROCK_WITH_TRANSPORT_DEPS}>:palantir/CapabilitiesService_test.cpp>
  $<$<BOOL:${BEDROCK_WITH_TRANSPORT_DEPS}>:palantir/EnvelopeHelpers_test.cpp>
  $<$<BOOL:${BEDROCK_WITH_TRANSPORT_DEPS}>:palantir/ErrorResponse_test.cpp>
)

target_link_libraries(bedrock_tests
  PRIVATE
    bedrock_core
    GTest::gtest_main
)

# Link Palantir libraries when transport deps are enabled
if(BEDROCK_WITH_TRANSPORT_DEPS)
  target_link_libraries(bedrock_tests PRIVATE
    bedrock_capabilities_service
    bedrock_palantir_proto
    bedrock_palantir_server
  )
  target_compile_definitions(bedrock_tests PRIVATE BEDROCK_WITH_TRANSPORT_DEPS)
  target_include_directories(bedrock_tests PRIVATE
    ${CMAKE_BINARY_DIR}/generated/palantir
    ${CMAKE_SOURCE_DIR}/src
  )
endif()

include(GoogleTest)
gtest_discover_tests(bedrock_tests)

# Integration tests (only when transport deps enabled)
if(BEDROCK_WITH_TRANSPORT_DEPS)
    add_subdirectory(integration)
endif()
