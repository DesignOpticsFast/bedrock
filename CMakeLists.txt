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
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------
# Options
# ---------------------------------------
option(BUILD_TESTING "Build tests" ON)

# ---------------------------------------
# OpenMP for multithreading
# ---------------------------------------
# NOTE: Disabled for Phase 0 minimal build
# find_package(OpenMP REQUIRED)
# if(OpenMP_CXX_FOUND)
#     message(STATUS "OpenMP found: ${OpenMP_CXX_VERSION}")
#     message(STATUS "OpenMP flags: ${OpenMP_CXX_FLAGS}")
# else()
#     message(FATAL_ERROR "OpenMP not found - required for multithreading support")
# endif()

# ---------------------------------------
# Core library (existing project layout)
# ---------------------------------------
# NOTE: Disabled for Phase 0 minimal build - keeping code on disk but not building
# add_subdirectory(core)

# if(BUILD_TESTING)
#   include(CTest)
#   enable_testing()
#   add_subdirectory(tests)
# endif()

# ---------------------------------------
# Sprint 1: SOM header-only target
# ---------------------------------------
# NOTE: Disabled for Phase 0 minimal build
# add_library(bedrock_som INTERFACE)
# target_include_directories(bedrock_som
#   INTERFACE
#     ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ---------------------------------------
# Sprint 1: OpenCascade + geom target
# ---------------------------------------
# NOTE: Disabled for Phase 0 minimal build
# option(BEDROCK_WITH_OCCT "Build Bedrock OCCT components" ON)
# if(BEDROCK_WITH_OCCT)
#   find_package(OpenCASCADE REQUIRED)
#   set(BUILD_WITH_OCCT ON)
# else()
#   message(STATUS "Building Bedrock without OCCT support")
#   set(BUILD_WITH_OCCT OFF)
# endif()
# if(BUILD_WITH_OCCT)
#   add_library(bedrock_geom
#     include/bedrock/geom/step_export.hpp
#     src/geom/step_export.cpp)
#   target_include_directories(bedrock_geom
#     PUBLIC
#       ${CMAKE_CURRENT_SOURCE_DIR}/include
#     PRIVATE
#       ${OpenCASCADE_INCLUDE_DIR})
#   target_link_libraries(bedrock_geom
#     PUBLIC
#       bedrock_som
#     PRIVATE
#       ${OpenCASCADE_LIBRARIES})
# else()
#   message(STATUS "Skipping bedrock_geom library - OpenCASCADE not available")
# endif()

# ---------------------------------------
# Old bedrock_engine target (disabled for Phase 0)
# ---------------------------------------
# NOTE: Old engine target kept on disk but disabled - replaced by minimal bedrock_engine below
# add_library(bedrock_engine
#   include/bedrock/engine.hpp
#   src/engine/engine.cpp)
# target_include_directories(bedrock_engine PUBLIC
#   ${CMAKE_CURRENT_SOURCE_DIR}/include)
# target_link_libraries(bedrock_engine PUBLIC
#   bedrock_geom)  # brings in OCCT + bedrock_som

# ---------------------------------------
# Plugin interface for external consumers (like Phoenix)
# ---------------------------------------
# NOTE: Disabled for Phase 0 minimal build
# add_library(bedrock_plugin
#   include/bedrock/plugin_interface.hpp
#   src/plugin/plugin_interface.cpp)
# target_include_directories(bedrock_plugin PUBLIC
#   ${CMAKE_CURRENT_SOURCE_DIR}/include)
# target_link_libraries(bedrock_plugin PRIVATE
#   bedrock_engine)

# ---------------------------------------
# Clean SDK interface for external consumers (like Phoenix)
# ---------------------------------------
# NOTE: Disabled for Phase 0 minimal build
# add_library(bedrock_sdk INTERFACE)
# target_include_directories(bedrock_sdk INTERFACE
#   ${CMAKE_CURRENT_SOURCE_DIR}/include)
# target_link_libraries(bedrock_sdk INTERFACE
#   bedrock_som)

# --- Sprint 1: smoke (manual run)
# NOTE: Disabled for Phase 0 minimal build
# add_executable(bedrock_smoke_step tests/smoke_step_main.cpp)
# target_link_libraries(bedrock_smoke_step PRIVATE bedrock_engine)

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
