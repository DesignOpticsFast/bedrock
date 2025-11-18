# cmake/Targets.cmake
# Target definitions for Bedrock

# ---------------------------------------
# Phase 0: Minimal bedrock_engine (C++23)
# ---------------------------------------
add_library(bedrock_engine STATIC
  src/bedrock_engine.cpp)

target_include_directories(bedrock_engine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ---------------------------------------
# Phase 0: Sanity executable (built by default)
# ---------------------------------------
add_executable(bedrock_sanity
  tests/bedrock_sanity.cpp)

target_link_libraries(bedrock_sanity PRIVATE
  bedrock_engine)

# ---------------------------------------
# Phase 0: CTest integration
# ---------------------------------------
enable_testing()
add_test(NAME bedrock_sanity COMMAND bedrock_sanity)

# ---------------------------------------
# Phase 0: Transport dependencies sanity target
# ---------------------------------------
if(BEDROCK_WITH_TRANSPORT_DEPS)
    if(HAVE_PROTOBUF AND HAVE_GRPC)
        add_executable(bedrock_transport_deps_sanity
            tests/bedrock_transport_deps_sanity.cpp)

        target_include_directories(bedrock_transport_deps_sanity PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

        # Link against protobuf (handle both CONFIG and MODULE modes)
        if(TARGET protobuf::libprotobuf)
            target_link_libraries(bedrock_transport_deps_sanity PRIVATE protobuf::libprotobuf)
        elseif(TARGET protobuf::protobuf)
            target_link_libraries(bedrock_transport_deps_sanity PRIVATE protobuf::protobuf)
        elseif(Protobuf_LIBRARIES)
            target_link_libraries(bedrock_transport_deps_sanity PRIVATE ${Protobuf_LIBRARIES})
            target_include_directories(bedrock_transport_deps_sanity PRIVATE ${Protobuf_INCLUDE_DIRS})
        else()
            target_link_libraries(bedrock_transport_deps_sanity PRIVATE protobuf)
        endif()

        # Link against gRPC (handle both CONFIG and MODULE modes)
        if(TARGET gRPC::grpc++)
            target_link_libraries(bedrock_transport_deps_sanity PRIVATE gRPC::grpc++)
        elseif(TARGET gRPC::grpcpp)
            target_link_libraries(bedrock_transport_deps_sanity PRIVATE gRPC::grpcpp)
        elseif(gRPC_LIBRARIES)
            target_link_libraries(bedrock_transport_deps_sanity PRIVATE ${gRPC_LIBRARIES})
            target_include_directories(bedrock_transport_deps_sanity PRIVATE ${gRPC_INCLUDE_DIRS})
        else()
            target_link_libraries(bedrock_transport_deps_sanity PRIVATE grpc++)
        endif()

        # Conditionally link libsodium if found
        if(LIBSODIUM_FOUND)
            if(TARGET sodium::sodium)
                target_link_libraries(bedrock_transport_deps_sanity PRIVATE sodium::sodium)
                target_compile_definitions(bedrock_transport_deps_sanity PRIVATE HAVE_LIBSODIUM)
            elseif(TARGET libsodium::libsodium)
                target_link_libraries(bedrock_transport_deps_sanity PRIVATE libsodium::libsodium)
                target_compile_definitions(bedrock_transport_deps_sanity PRIVATE HAVE_LIBSODIUM)
            elseif(sodium_LIBRARIES)
                target_link_libraries(bedrock_transport_deps_sanity PRIVATE ${sodium_LIBRARIES})
                target_include_directories(bedrock_transport_deps_sanity PRIVATE ${sodium_INCLUDE_DIRS})
                target_compile_definitions(bedrock_transport_deps_sanity PRIVATE HAVE_LIBSODIUM)
            elseif(libsodium_LIBRARIES)
                target_link_libraries(bedrock_transport_deps_sanity PRIVATE ${libsodium_LIBRARIES})
                target_include_directories(bedrock_transport_deps_sanity PRIVATE ${libsodium_INCLUDE_DIRS})
                target_compile_definitions(bedrock_transport_deps_sanity PRIVATE HAVE_LIBSODIUM)
            endif()
        endif()

        # Add CTest entry if target exists
        if(TARGET bedrock_transport_deps_sanity)
            add_test(NAME bedrock_transport_deps_sanity COMMAND bedrock_transport_deps_sanity)
        endif()
    endif()

    # ---------------------------------------
    # Phase 0: Echo RPC proto generation
    # ---------------------------------------
    if(HAVE_PROTOBUF AND HAVE_GRPC)
        # Set up generated code directory
        set(ECHO_GENERATED_DIR ${CMAKE_BINARY_DIR}/generated/echo)
        file(MAKE_DIRECTORY ${ECHO_GENERATED_DIR})

        # Proto file path
        set(ECHO_PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/proto/bedrock_echo.proto)

        # Generated file paths
        set(ECHO_PB_H ${ECHO_GENERATED_DIR}/bedrock_echo.pb.h)
        set(ECHO_PB_CC ${ECHO_GENERATED_DIR}/bedrock_echo.pb.cc)
        set(ECHO_GRPC_PB_H ${ECHO_GENERATED_DIR}/bedrock_echo.grpc.pb.h)
        set(ECHO_GRPC_PB_CC ${ECHO_GENERATED_DIR}/bedrock_echo.grpc.pb.cc)

        if(PROTOC AND GRPC_CPP_PLUGIN)
            # Generate protobuf files (.pb.cc and .pb.h)
            add_custom_command(
                OUTPUT ${ECHO_PB_H} ${ECHO_PB_CC}
                COMMAND ${PROTOC}
                ARGS --cpp_out=${ECHO_GENERATED_DIR}
                     --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
                     ${ECHO_PROTO_FILE}
                DEPENDS ${ECHO_PROTO_FILE}
                COMMENT "Generating protobuf C++ code from bedrock_echo.proto"
            )

            # Generate gRPC files (.grpc.pb.cc and .grpc.pb.h)
            add_custom_command(
                OUTPUT ${ECHO_GRPC_PB_H} ${ECHO_GRPC_PB_CC}
                COMMAND ${PROTOC}
                ARGS --grpc_out=${ECHO_GENERATED_DIR}
                     --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
                     --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
                     ${ECHO_PROTO_FILE}
                DEPENDS ${ECHO_PROTO_FILE} ${ECHO_PB_H}
                COMMENT "Generating gRPC C++ code from bedrock_echo.proto"
            )

            # Collect generated sources
            set(ECHO_GENERATED_SOURCES
                ${ECHO_PB_CC}
                ${ECHO_GRPC_PB_CC}
            )

            set(ECHO_GENERATED_HEADERS
                ${ECHO_PB_H}
                ${ECHO_GRPC_PB_H}
            )

            message(STATUS "Echo RPC proto generation configured")

            # ---------------------------------------
            # Phase 0: Echo RPC sanity executable
            # ---------------------------------------
            add_executable(bedrock_echo_sanity
                tests/bedrock_echo_sanity.cpp
                src/echo/echo_service_impl.cpp
                ${ECHO_GENERATED_SOURCES}
            )

            target_include_directories(bedrock_echo_sanity PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/include
                ${CMAKE_CURRENT_SOURCE_DIR}/src/echo
                ${ECHO_GENERATED_DIR}
            )

            # Link against protobuf (handle both CONFIG and MODULE modes)
            if(TARGET protobuf::libprotobuf)
                target_link_libraries(bedrock_echo_sanity PRIVATE protobuf::libprotobuf)
            elseif(TARGET protobuf::protobuf)
                target_link_libraries(bedrock_echo_sanity PRIVATE protobuf::protobuf)
            elseif(Protobuf_LIBRARIES)
                target_link_libraries(bedrock_echo_sanity PRIVATE ${Protobuf_LIBRARIES})
                target_include_directories(bedrock_echo_sanity PRIVATE ${Protobuf_INCLUDE_DIRS})
            else()
                target_link_libraries(bedrock_echo_sanity PRIVATE protobuf)
            endif()

            # Link against gRPC (handle both CONFIG and MODULE modes)
            if(TARGET gRPC::grpc++)
                target_link_libraries(bedrock_echo_sanity PRIVATE gRPC::grpc++)
            elseif(TARGET gRPC::grpcpp)
                target_link_libraries(bedrock_echo_sanity PRIVATE gRPC::grpcpp)
            elseif(gRPC_LIBRARIES)
                target_link_libraries(bedrock_echo_sanity PRIVATE ${gRPC_LIBRARIES})
                target_include_directories(bedrock_echo_sanity PRIVATE ${gRPC_INCLUDE_DIRS})
            else()
                target_link_libraries(bedrock_echo_sanity PRIVATE grpc++)
            endif()

            # Add CTest entry
            if(TARGET bedrock_echo_sanity)
                add_test(NAME bedrock_echo_sanity COMMAND bedrock_echo_sanity)
            endif()
        else()
            if(NOT PROTOC)
                message(WARNING "protoc not found - Echo RPC will not be built")
            endif()
            if(NOT GRPC_CPP_PLUGIN)
                message(WARNING "grpc_cpp_plugin not found - Echo RPC will not be built")
            endif()
        endif()
    endif()
endif()

