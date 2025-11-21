cmake_minimum_required(VERSION 3.20)

project(Bedrock
  VERSION 0.0.1
  DESCRIPTION "AI-native optical design core"
  LANGUAGES CXX)

# ---------------------------------------
# Toolchain / language
# ---------------------------------------
# Silence CMP0135 (FetchContent timestamp) on all platforms
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------
# Options
# ---------------------------------------
option(BUILD_TESTING "Build tests" ON)
option(BEDROCK_WITH_TRANSPORT_DEPS "Enable gRPC/protobuf transport dependencies (Echo RPC)" OFF)

# ---------------------------------------
# OpenMP for multithreading
# ---------------------------------------
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found: ${OpenMP_CXX_VERSION}")
    message(STATUS "OpenMP flags: ${OpenMP_CXX_FLAGS}")
else()
    message(FATAL_ERROR "OpenMP not found - required for multithreading support")
endif()

# ---------------------------------------
# Transport dependencies (gRPC/protobuf) - Echo RPC
# ---------------------------------------
# Configure transport deps BEFORE tests subdirectory so variables are available
if(BEDROCK_WITH_TRANSPORT_DEPS)
  # Find required packages
  find_package(Protobuf REQUIRED)
  find_package(gRPC REQUIRED)
  
  # Locate tools
  find_program(PROTOC protoc REQUIRED)
  find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)
  
  message(STATUS "Transport deps enabled: protobuf=${Protobuf_VERSION}, gRPC=${gRPC_VERSION}")
  
  # Set generated directory
  set(ECHO_GENERATED_DIR ${CMAKE_BINARY_DIR}/generated/echo)
  file(MAKE_DIRECTORY ${ECHO_GENERATED_DIR})
  
  # Generate protobuf message files
  set(ECHO_PB_H ${ECHO_GENERATED_DIR}/bedrock_echo.pb.h)
  set(ECHO_PB_CC ${ECHO_GENERATED_DIR}/bedrock_echo.pb.cc)
  set(ECHO_GRPC_PB_H ${ECHO_GENERATED_DIR}/bedrock_echo.grpc.pb.h)
  set(ECHO_GRPC_PB_CC ${ECHO_GENERATED_DIR}/bedrock_echo.grpc.pb.cc)
  
  # Generate protobuf files
  add_custom_command(
    OUTPUT ${ECHO_PB_H} ${ECHO_PB_CC}
    COMMAND ${PROTOC}
    ARGS --cpp_out=${ECHO_GENERATED_DIR}
         --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
         ${CMAKE_CURRENT_SOURCE_DIR}/proto/bedrock_echo.proto
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/proto/bedrock_echo.proto
    COMMENT "Generating bedrock_echo protobuf files"
    VERBATIM
  )
  
  # Generate gRPC service stub files
  add_custom_command(
    OUTPUT ${ECHO_GRPC_PB_H} ${ECHO_GRPC_PB_CC}
    COMMAND ${PROTOC}
    ARGS --grpc_out=${ECHO_GENERATED_DIR}
         --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
         --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
         ${CMAKE_CURRENT_SOURCE_DIR}/proto/bedrock_echo.proto
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/proto/bedrock_echo.proto
            ${ECHO_PB_H}
    COMMENT "Generating bedrock_echo gRPC files"
    VERBATIM
  )
  
  # Create custom target to ensure proto generation happens
  add_custom_target(bedrock_echo_proto_gen
    DEPENDS ${ECHO_PB_H} ${ECHO_PB_CC} ${ECHO_GRPC_PB_H} ${ECHO_GRPC_PB_CC}
  )
  
  # Make generated files available to subdirectories
  set(ECHO_GENERATED_FILES ${ECHO_PB_H} ${ECHO_PB_CC} ${ECHO_GRPC_PB_H} ${ECHO_GRPC_PB_CC})
  
  # Optional: libsodium (not required for basic echo RPC)
  find_package(sodium QUIET)
  if(sodium_FOUND)
    message(STATUS "libsodium found (optional)")
  endif()
else()
  message(STATUS "Transport deps disabled (BEDROCK_WITH_TRANSPORT_DEPS=OFF)")
endif()

# ---------------------------------------
# Core library (existing project layout)
# ---------------------------------------
# Keep your existing core sources/tests structure intact.
# If 'core' or 'tests' don't exist yet, these can remain or be removed.
add_subdirectory(core)

if(BUILD_TESTING)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

# ---------------------------------------
# Sprint 1: SOM header-only target
# ---------------------------------------
add_library(bedrock_som INTERFACE)
target_include_directories(bedrock_som
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ---------------------------------------
# Sprint 1: OpenCascade + geom target
# ---------------------------------------
# Homebrew installs OCCT at /opt/homebrew/opt/opencascade
# Ensure your environment has:
#   export CMAKE_PREFIX_PATH="/opt/homebrew/opt/opencascade:${CMAKE_PREFIX_PATH}"
#   export OpenCASCADE_DIR="/opt/homebrew/opt/opencascade/lib/cmake/opencascade-7.9.1"
option(BEDROCK_WITH_OCCT "Build Bedrock OCCT components" ON)

if(BEDROCK_WITH_OCCT)
  find_package(OpenCASCADE REQUIRED)
  set(BUILD_WITH_OCCT ON)
else()
  message(STATUS "Building Bedrock without OCCT support")
  set(BUILD_WITH_OCCT OFF)
endif()

if(BUILD_WITH_OCCT)
  add_library(bedrock_geom
    include/bedrock/geom/step_export.hpp
    src/geom/step_export.cpp)

  target_include_directories(bedrock_geom
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
      ${OpenCASCADE_INCLUDE_DIR})

  target_link_libraries(bedrock_geom
    PUBLIC
      bedrock_som
    PRIVATE
      ${OpenCASCADE_LIBRARIES})
else()
  message(STATUS "Skipping bedrock_geom library - OpenCASCADE not available")
endif()

# ---------------------------------------
# (Next step will add bedrock_engine target)
# ---------------------------------------
# --- Sprint 1: engine target
add_library(bedrock_engine
  include/bedrock/engine.hpp
  src/engine/engine.cpp)

target_include_directories(bedrock_engine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(bedrock_engine PUBLIC
  bedrock_geom)  # brings in OCCT + bedrock_som

# ---------------------------------------
# Plugin interface for external consumers (like Phoenix)
# ---------------------------------------
# This provides a clean interface without exposing OCCT dependencies
add_library(bedrock_plugin
  include/bedrock/plugin_interface.hpp
  src/plugin/plugin_interface.cpp)

target_include_directories(bedrock_plugin PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Plugin depends on engine (which has OCCT) but doesn't expose it
target_link_libraries(bedrock_plugin PRIVATE
  bedrock_engine)

# ---------------------------------------
# Clean SDK interface for external consumers (like Phoenix)
# ---------------------------------------
# This provides a clean interface without exposing OCCT dependencies
add_library(bedrock_sdk INTERFACE)

# Only expose public headers, not OCCT internals
target_include_directories(bedrock_sdk INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link to bedrock_som (header-only) but NOT bedrock_geom (which has OCCT)
target_link_libraries(bedrock_sdk INTERFACE
  bedrock_som)

# --- Sprint 1: smoke (manual run)
add_executable(bedrock_smoke_step tests/smoke_step_main.cpp)
target_link_libraries(bedrock_smoke_step PRIVATE bedrock_engine)

# --- Palantir server executable (requires Qt6)
find_package(Qt6 QUIET COMPONENTS Core Network)
if(Qt6_FOUND AND Qt6Core_FOUND AND Qt6Network_FOUND)
  # Find protobuf for palantir messages
  find_package(Protobuf QUIET)
  if(Protobuf_FOUND)
    add_executable(bedrock_server
      src/palantir/bedrock_server.cpp
      src/palantir/PalantirServer.cpp
      src/palantir/PalantirServer.hpp
    )
    set_target_properties(bedrock_server PROPERTIES
      AUTOMOC ON
    )
    target_link_libraries(bedrock_server PRIVATE
      Qt6::Core
      Qt6::Network
      protobuf::libprotobuf
    )
    # Find palantir.proto location (may be in Phoenix or contracts submodule)
    find_path(PALANTIR_PROTO_DIR palantir.proto
      PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/proto
        ${CMAKE_CURRENT_SOURCE_DIR}/../phoenix/proto
        ${CMAKE_CURRENT_SOURCE_DIR}/contracts/proto
      NO_DEFAULT_PATH
    )
    if(PALANTIR_PROTO_DIR)
      message(STATUS "Found palantir.proto at: ${PALANTIR_PROTO_DIR}")
      # Generate palantir.pb.h/cc if not already generated
      set(PALANTIR_PB_H ${CMAKE_BINARY_DIR}/generated/palantir.pb.h)
      set(PALANTIR_PB_CC ${CMAKE_BINARY_DIR}/generated/palantir.pb.cc)
      add_custom_command(
        OUTPUT ${PALANTIR_PB_H} ${PALANTIR_PB_CC}
        COMMAND ${PROTOC}
        ARGS --cpp_out=${CMAKE_BINARY_DIR}/generated
             --proto_path=${PALANTIR_PROTO_DIR}
             ${PALANTIR_PROTO_DIR}/palantir.proto
        DEPENDS ${PALANTIR_PROTO_DIR}/palantir.proto
        COMMENT "Generating palantir protobuf files"
        VERBATIM
      )
      add_custom_target(palantir_proto_gen DEPENDS ${PALANTIR_PB_H} ${PALANTIR_PB_CC})
      add_dependencies(bedrock_server palantir_proto_gen)
      # Add generated .pb.cc to executable sources
      target_sources(bedrock_server PRIVATE ${PALANTIR_PB_CC})
      target_include_directories(bedrock_server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/generated
      )
    else()
      # Fallback: assume palantir.pb.h exists somewhere
      target_include_directories(bedrock_server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/proto
        ${CMAKE_CURRENT_SOURCE_DIR}/../phoenix/proto
        ${CMAKE_CURRENT_SOURCE_DIR}/contracts/proto
      )
      message(WARNING "palantir.proto not found - assuming palantir.pb.h exists")
    endif()
    # Link generated protobuf files if transport deps enabled
    if(BEDROCK_WITH_TRANSPORT_DEPS)
      add_dependencies(bedrock_server bedrock_echo_proto_gen)
      target_include_directories(bedrock_server PRIVATE ${ECHO_GENERATED_DIR})
    endif()
    message(STATUS "bedrock_server executable configured")
  else()
    message(STATUS "bedrock_server skipped: Protobuf not found")
  endif()
else()
  message(STATUS "bedrock_server skipped: Qt6 not found")
endif()
