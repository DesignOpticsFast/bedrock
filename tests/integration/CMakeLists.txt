# Integration tests for Palantir transport layer
# Tests Phoenix ↔ Bedrock communication end-to-end using envelope-based transport

if(BEDROCK_WITH_TRANSPORT_DEPS)
    # Find Qt6 (required for QCoreApplication, QLocalSocket, QTest, etc.)
    find_package(Qt6 REQUIRED COMPONENTS Core Network Test)
    
    add_executable(integration_tests
        integration_main.cpp
        IntegrationTestServerFixture.cpp
        IntegrationTestClient.cpp
        CapabilitiesIntegrationTest.cpp
        XYSineIntegrationTest.cpp
        ErrorCasesIntegrationTest.cpp
        EdgeCasesIntegrationTest.cpp
    )
    
    target_link_libraries(integration_tests
        PRIVATE
            bedrock_palantir_server
            bedrock_palantir_proto
            bedrock_capabilities_service
            GTest::gtest
            Qt6::Core
            Qt6::Network
            Qt6::Test
    )
    
    target_compile_definitions(integration_tests PRIVATE BEDROCK_WITH_TRANSPORT_DEPS)
    
    target_include_directories(integration_tests PRIVATE
        ${CMAKE_BINARY_DIR}/generated/palantir
        ${CMAKE_SOURCE_DIR}/src
    )
    
    # Enable Qt MOC for QObject-derived classes
    set_target_properties(integration_tests PROPERTIES
        AUTOMOC ON
    )
    
    include(GoogleTest)
    gtest_discover_tests(integration_tests)
else()
    message(STATUS "Skipping integration tests - BEDROCK_WITH_TRANSPORT_DEPS not enabled")
endif()

