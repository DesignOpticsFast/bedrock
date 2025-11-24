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
option(BEDROCK_WITH_TRANSPORT_DEPS "Enable Palantir/transport deps" OFF)

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

# ---------------------------------------
# Palantir protobuf code generation (WP1)
# ---------------------------------------
if(BEDROCK_WITH_TRANSPORT_DEPS)
  find_program(PROTOC_EXECUTABLE protoc)
  if(NOT PROTOC_EXECUTABLE)
    message(FATAL_ERROR "protoc not found but BEDROCK_WITH_TRANSPORT_DEPS=ON. Install protobuf (e.g., brew install protobuf)")
  endif()

  set(PALANTIR_PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/docs/palantir/proto")
  set(PALANTIR_PROTO_OUT_DIR "${CMAKE_BINARY_DIR}/generated/palantir")
  file(MAKE_DIRECTORY "${PALANTIR_PROTO_OUT_DIR}")

  set(CAPABILITIES_PROTO "${PALANTIR_PROTO_DIR}/palantir/capabilities.proto")

  if(EXISTS "${CAPABILITIES_PROTO}")
    add_custom_command(
      OUTPUT
        "${PALANTIR_PROTO_OUT_DIR}/palantir/capabilities.pb.cc"
        "${PALANTIR_PROTO_OUT_DIR}/palantir/capabilities.pb.h"
      COMMAND "${PROTOC_EXECUTABLE}"
        --proto_path="${PALANTIR_PROTO_DIR}"
        --cpp_out="${PALANTIR_PROTO_OUT_DIR}"
        "${CAPABILITIES_PROTO}"
      DEPENDS "${CAPABILITIES_PROTO}"
      COMMENT "Generating C++ from Palantir Capabilities.proto"
    )

    add_library(bedrock_palantir_proto STATIC
      "${PALANTIR_PROTO_OUT_DIR}/palantir/capabilities.pb.cc"
    )

    target_include_directories(bedrock_palantir_proto PUBLIC
      "${PALANTIR_PROTO_OUT_DIR}"
    )

    find_package(Protobuf REQUIRED)
    target_link_libraries(bedrock_palantir_proto PUBLIC
      protobuf::libprotobuf
    )
    
    # Protobuf 6.33+ requires abseil libraries
    # Link abseil libraries that protobuf depends on
    find_library(ABSL_DIE_IF_NULL_LIB absl_die_if_null PATHS /opt/homebrew/opt/abseil/lib NO_DEFAULT_PATH)
    find_library(ABSL_LOG_INITIALIZE_LIB absl_log_initialize PATHS /opt/homebrew/opt/abseil/lib NO_DEFAULT_PATH)
    find_library(ABSL_STATUSOR_LIB absl_statusor PATHS /opt/homebrew/opt/abseil/lib NO_DEFAULT_PATH)
    find_library(ABSL_LOG_INTERNAL_CHECK_OP_LIB absl_log_internal_check_op PATHS /opt/homebrew/opt/abseil/lib NO_DEFAULT_PATH)
    find_library(ABSL_LOG_INTERNAL_CONDITIONS_LIB absl_log_internal_conditions PATHS /opt/homebrew/opt/abseil/lib NO_DEFAULT_PATH)
    find_library(ABSL_LOG_INTERNAL_MESSAGE_LIB absl_log_internal_message PATHS /opt/homebrew/opt/abseil/lib NO_DEFAULT_PATH)
    
    if(ABSL_DIE_IF_NULL_LIB AND ABSL_LOG_INITIALIZE_LIB AND ABSL_STATUSOR_LIB)
      target_link_libraries(bedrock_palantir_proto PUBLIC
        ${ABSL_DIE_IF_NULL_LIB}
        ${ABSL_LOG_INITIALIZE_LIB}
        ${ABSL_STATUSOR_LIB}
        ${ABSL_LOG_INTERNAL_CHECK_OP_LIB}
        ${ABSL_LOG_INTERNAL_CONDITIONS_LIB}
        ${ABSL_LOG_INTERNAL_MESSAGE_LIB}
      )
    endif()

    message(STATUS "Palantir proto codegen enabled: Capabilities.proto -> bedrock_palantir_proto")
    
    # ---------------------------------------
    # CapabilitiesService library
    # ---------------------------------------
    add_library(bedrock_capabilities_service STATIC
      src/palantir/CapabilitiesService.cpp
    )
    
    target_include_directories(bedrock_capabilities_service PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    target_link_libraries(bedrock_capabilities_service PUBLIC
      bedrock_palantir_proto
    )
    
    # Add compile definition so code can check for transport deps
    target_compile_definitions(bedrock_capabilities_service PRIVATE BEDROCK_WITH_TRANSPORT_DEPS)
  else()
    message(WARNING "Capabilities.proto not found at ${CAPABILITIES_PROTO}")
  endif()
endif()

# --- Sprint 1: smoke (manual run)
add_executable(bedrock_smoke_step tests/smoke_step_main.cpp)
target_link_libraries(bedrock_smoke_step PRIVATE bedrock_engine)
