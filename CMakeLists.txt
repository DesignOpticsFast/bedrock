cmake_minimum_required(VERSION 3.20)

project(Bedrock
  VERSION 0.3.0
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
# ---------------------------------------------
# macOS OpenMP fallback (AppleClang + Homebrew)
# ---------------------------------------------
if(APPLE AND NOT DEFINED OpenMP_CXX_FLAGS)
    if(EXISTS "/opt/homebrew/opt/libomp")
        # Apple Silicon (M1/M2/M3)
        set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include" CACHE STRING "OpenMP C++ compiler flags")
        set(OpenMP_CXX_LIB_NAMES "omp" CACHE STRING "OpenMP library name")
        set(OpenMP_omp_LIBRARY "/opt/homebrew/opt/libomp/lib/libomp.dylib" CACHE FILEPATH "Path to OpenMP library")
        message(STATUS "Bedrock: Auto-detected Homebrew libomp at /opt/homebrew")
    else()
        message(WARNING "Bedrock: libomp not found. Install with: brew install libomp")
    endif()
endif()

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
# OpenCASCADE is optional - set BEDROCK_WITH_OPENCASCADE=ON to enable
# Homebrew installs OCCT at /opt/homebrew/opt/opencascade
# Ensure your environment has:
#   export CMAKE_PREFIX_PATH="/opt/homebrew/opt/opencascade:${CMAKE_PREFIX_PATH}"
#   export OpenCASCADE_DIR="/opt/homebrew/opt/opencascade/lib/cmake/opencascade-7.9.1"
option(BEDROCK_WITH_OPENCASCADE "Enable OpenCASCADE geometry backend" OFF)
option(BEDROCK_WITH_OCCT "Build Bedrock OCCT components" OFF)  # Legacy alias, deprecated

# Support both option names for backward compatibility
if(BEDROCK_WITH_OPENCASCADE OR BEDROCK_WITH_OCCT)
  find_package(OpenCASCADE REQUIRED)
  set(BUILD_WITH_OCCT ON)
else()
  message(STATUS "Building Bedrock without OCCT support (use -DBEDROCK_WITH_OPENCASCADE=ON to enable)")
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
  bedrock_som
  $<$<BOOL:${BUILD_WITH_OCCT}>:bedrock_geom>)  # Conditionally link OCCT if available

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
  set(XYSINE_PROTO "${PALANTIR_PROTO_DIR}/palantir/xysine.proto")
  set(ENVELOPE_PROTO "${PALANTIR_PROTO_DIR}/palantir/envelope.proto")
  set(ERROR_PROTO "${PALANTIR_PROTO_DIR}/palantir/error.proto")

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
  endif()

  if(EXISTS "${XYSINE_PROTO}")
    add_custom_command(
      OUTPUT
        "${PALANTIR_PROTO_OUT_DIR}/palantir/xysine.pb.cc"
        "${PALANTIR_PROTO_OUT_DIR}/palantir/xysine.pb.h"
      COMMAND "${PROTOC_EXECUTABLE}"
        --proto_path="${PALANTIR_PROTO_DIR}"
        --cpp_out="${PALANTIR_PROTO_OUT_DIR}"
        "${XYSINE_PROTO}"
      DEPENDS "${XYSINE_PROTO}"
      COMMENT "Generating C++ from Palantir xysine.proto"
    )
  endif()

  if(EXISTS "${ENVELOPE_PROTO}")
    add_custom_command(
      OUTPUT
        "${PALANTIR_PROTO_OUT_DIR}/palantir/envelope.pb.cc"
        "${PALANTIR_PROTO_OUT_DIR}/palantir/envelope.pb.h"
      COMMAND "${PROTOC_EXECUTABLE}"
        --proto_path="${PALANTIR_PROTO_DIR}"
        --cpp_out="${PALANTIR_PROTO_OUT_DIR}"
        "${ENVELOPE_PROTO}"
      DEPENDS "${ENVELOPE_PROTO}"
      COMMENT "Generating C++ from Palantir envelope.proto"
    )
  endif()

  if(EXISTS "${ERROR_PROTO}")
    add_custom_command(
      OUTPUT
        "${PALANTIR_PROTO_OUT_DIR}/palantir/error.pb.cc"
        "${PALANTIR_PROTO_OUT_DIR}/palantir/error.pb.h"
      COMMAND "${PROTOC_EXECUTABLE}"
        --proto_path="${PALANTIR_PROTO_DIR}"
        --cpp_out="${PALANTIR_PROTO_OUT_DIR}"
        "${ERROR_PROTO}"
      DEPENDS "${ERROR_PROTO}"
      COMMENT "Generating C++ from Palantir error.proto"
    )
  endif()

  # Build proto library with all available proto files
  set(PROTO_SOURCES)
  if(EXISTS "${CAPABILITIES_PROTO}")
    list(APPEND PROTO_SOURCES "${PALANTIR_PROTO_OUT_DIR}/palantir/capabilities.pb.cc")
  endif()
  if(EXISTS "${XYSINE_PROTO}")
    list(APPEND PROTO_SOURCES "${PALANTIR_PROTO_OUT_DIR}/palantir/xysine.pb.cc")
  endif()
  if(EXISTS "${ENVELOPE_PROTO}")
    list(APPEND PROTO_SOURCES "${PALANTIR_PROTO_OUT_DIR}/palantir/envelope.pb.cc")
  endif()
  if(EXISTS "${ERROR_PROTO}")
    list(APPEND PROTO_SOURCES "${PALANTIR_PROTO_OUT_DIR}/palantir/error.pb.cc")
  endif()

  if(PROTO_SOURCES)
    add_library(bedrock_palantir_proto STATIC
      ${PROTO_SOURCES}
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
    find_library(ABSL_LOG_INTERNAL_NULLGUARD_LIB absl_log_internal_nullguard PATHS /opt/homebrew/opt/abseil/lib NO_DEFAULT_PATH)
    find_library(ABSL_HASH_LIB absl_hash PATHS /opt/homebrew/opt/abseil/lib NO_DEFAULT_PATH)
    find_library(ABSL_LOG_INTERNAL_FORMAT_LIB absl_log_internal_format PATHS /opt/homebrew/opt/abseil/lib NO_DEFAULT_PATH)
    
    set(ABSEIL_LIBS)
    if(ABSL_DIE_IF_NULL_LIB)
      list(APPEND ABSEIL_LIBS ${ABSL_DIE_IF_NULL_LIB})
    endif()
    if(ABSL_LOG_INITIALIZE_LIB)
      list(APPEND ABSEIL_LIBS ${ABSL_LOG_INITIALIZE_LIB})
    endif()
    if(ABSL_STATUSOR_LIB)
      list(APPEND ABSEIL_LIBS ${ABSL_STATUSOR_LIB})
    endif()
    if(ABSL_LOG_INTERNAL_CHECK_OP_LIB)
      list(APPEND ABSEIL_LIBS ${ABSL_LOG_INTERNAL_CHECK_OP_LIB})
    endif()
    if(ABSL_LOG_INTERNAL_CONDITIONS_LIB)
      list(APPEND ABSEIL_LIBS ${ABSL_LOG_INTERNAL_CONDITIONS_LIB})
    endif()
    if(ABSL_LOG_INTERNAL_MESSAGE_LIB)
      list(APPEND ABSEIL_LIBS ${ABSL_LOG_INTERNAL_MESSAGE_LIB})
    endif()
    if(ABSL_LOG_INTERNAL_NULLGUARD_LIB)
      list(APPEND ABSEIL_LIBS ${ABSL_LOG_INTERNAL_NULLGUARD_LIB})
    endif()
    if(ABSL_HASH_LIB)
      list(APPEND ABSEIL_LIBS ${ABSL_HASH_LIB})
    endif()
    if(ABSL_LOG_INTERNAL_FORMAT_LIB)
      list(APPEND ABSEIL_LIBS ${ABSL_LOG_INTERNAL_FORMAT_LIB})
    endif()
    
    if(ABSEIL_LIBS)
      target_link_libraries(bedrock_palantir_proto PUBLIC ${ABSEIL_LIBS})
    endif()

    message(STATUS "Palantir proto codegen enabled: Capabilities, XYSine, Envelope, Error -> bedrock_palantir_proto")
    
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
    
    # ---------------------------------------
    # PalantirServer library (Qt-based IPC server)
    # ---------------------------------------
    find_package(Qt6 REQUIRED COMPONENTS Core Network)
    
    add_library(bedrock_palantir_server STATIC
      src/palantir/PalantirServer.cpp
      src/palantir/EnvelopeHelpers.cpp
      src/palantir/EnvelopeHelpers.hpp
    )
    
    target_include_directories(bedrock_palantir_server PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    target_link_libraries(bedrock_palantir_server PUBLIC
      Qt6::Core
      Qt6::Network
      bedrock_palantir_proto
      bedrock_capabilities_service
    )
    
    # Link abseil libraries (required by protobuf 6.33+)
    if(ABSEIL_LIBS)
      target_link_libraries(bedrock_palantir_server PUBLIC ${ABSEIL_LIBS})
    endif()
    
    target_compile_definitions(bedrock_palantir_server PRIVATE BEDROCK_WITH_TRANSPORT_DEPS)
    
    # ---------------------------------------
    # bedrock_server executable
    # ---------------------------------------
    add_executable(bedrock_server
      src/palantir/bedrock_server.cpp
    )
    
    target_link_libraries(bedrock_server PRIVATE
      bedrock_palantir_server
    )
    
    # Enable Qt MOC for PalantirServer
    set_target_properties(bedrock_palantir_server PROPERTIES
      AUTOMOC ON
    )
    
    message(STATUS "bedrock_server executable configured")
  else()
    message(WARNING "Capabilities.proto not found at ${CAPABILITIES_PROTO}")
  endif()
endif()

# --- Sprint 1: smoke (manual run)
# smoke_step test requires OCCT
if(BUILD_WITH_OCCT)
  add_executable(bedrock_smoke_step tests/smoke_step_main.cpp)
  target_link_libraries(bedrock_smoke_step PRIVATE bedrock_engine)
  add_test(NAME bedrock_smoke_step COMMAND bedrock_smoke_step)
else()
  message(STATUS "Skipping bedrock_smoke_step test - requires OCCT (use -DBEDROCK_WITH_OPENCASCADE=ON to enable)")
endif()

# ---------------------------------------
# Side Quest 1: C++23 Probe Target
# ---------------------------------------
add_executable(cpp23_probe experiments/cpp23_probe.cpp)
target_compile_features(cpp23_probe PRIVATE cxx_std_23)

# Side Quest 1: Simple Job (C++23 vs C++20 comparison)
# Simple job links against engine, which might link against geom (OCCT).
# If OCCT is ON, simple_job.cpp's stub for WriteTSEasSTEP conflicts with the real one in geom.
# To avoid ODR violations, only build simple_job when OCCT is OFF.
if(NOT BUILD_WITH_OCCT)
  add_executable(bedrock_simple_job experiments/simple_job.cpp)
  target_link_libraries(bedrock_simple_job PRIVATE bedrock_engine)
  # We will inherit the global CMAKE_CXX_STANDARD for this one to allow toggling.
else()
  message(STATUS "Skipping bedrock_simple_job - incompatible with OCCT enabled (ODR check)")
endif()

# If the global standard is less than 23, this specific target attempts to override it.
# Note: Some compilers/CMake versions require the global standard to match or exceed if set globally.
# But `target_compile_features` usually handles the upgrade for the specific target.

