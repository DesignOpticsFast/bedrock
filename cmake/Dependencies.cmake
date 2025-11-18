# cmake/Dependencies.cmake
# External dependencies for Bedrock

# ---------------------------------------
# Phase 0: Transport dependencies (protobuf, gRPC, libsodium)
# ---------------------------------------
if(BEDROCK_WITH_TRANSPORT_DEPS)
    message(STATUS "Transport dependencies requested (BEDROCK_WITH_TRANSPORT_DEPS=ON)")

    # Find Protobuf (try CONFIG first, then MODULE)
    find_package(Protobuf CONFIG QUIET)
    if(NOT Protobuf_FOUND)
        find_package(Protobuf QUIET)
    endif()
    # Check if Protobuf was found (either via CONFIG or MODULE)
    if(Protobuf_FOUND OR Protobuf_LIBRARIES OR Protobuf_VERSION)
        if(NOT Protobuf_FOUND)
            set(Protobuf_FOUND TRUE)
        endif()
        if(Protobuf_VERSION)
            message(STATUS "Protobuf found: ${Protobuf_VERSION}")
        else()
            message(STATUS "Protobuf found")
        endif()
    else()
        message(WARNING "Protobuf not found - transport deps sanity target will not be built")
    endif()

    # Find gRPC (try CONFIG first, then MODULE)
    find_package(gRPC CONFIG QUIET)
    if(NOT gRPC_FOUND)
        find_package(gRPC QUIET)
    endif()
    if(gRPC_FOUND)
        message(STATUS "gRPC found: ${gRPC_VERSION}")
    else()
        message(WARNING "gRPC not found - transport deps sanity target will not be built")
    endif()

    # Find libsodium (optional)
    find_package(sodium QUIET)
    if(NOT sodium_FOUND)
        find_package(libsodium QUIET)
    endif()
    if(sodium_FOUND OR libsodium_FOUND)
        message(STATUS "libsodium found")
        set(LIBSODIUM_FOUND TRUE)
    else()
        message(WARNING "libsodium not found - will be optional for transport deps sanity")
        set(LIBSODIUM_FOUND FALSE)
    endif()

    # Set flags for transport deps availability
    set(HAVE_PROTOBUF FALSE)
    set(HAVE_GRPC FALSE)
    
    if(Protobuf_FOUND OR Protobuf_LIBRARIES OR Protobuf_VERSION)
        set(HAVE_PROTOBUF TRUE)
    endif()
    if(gRPC_FOUND)
        set(HAVE_GRPC TRUE)
    endif()

    # Find protoc and grpc_cpp_plugin for proto generation
    if(HAVE_PROTOBUF AND HAVE_GRPC)
        find_program(PROTOC protoc)
        find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
    endif()
endif()

