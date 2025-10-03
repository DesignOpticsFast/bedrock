cmake_minimum_required(VERSION 3.20)

project(Bedrock
  VERSION 0.0.1
  DESCRIPTION "AI-native optical design core"
  LANGUAGES CXX)

# Use modern C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTING "Build tests" ON)

# Core library
add_subdirectory(core)

# Tests
if(BUILD_TESTING)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

# --- Sprint 1: header-only SOM target
add_library(bedrock_som INTERFACE)
target_include_directories(bedrock_som INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# --- Sprint 1: OpenCascade + geom target
# Homebrew installs OCCT at /opt/homebrew/opt/opencascade; ensure CMAKE_PREFIX_PATH is set.
find_package(OpenCASCADE REQUIRED COMPONENTS
  TKernel TKSTEP TKBRep TKPrim TKTopAlgo TKGeomBase)

add_library(bedrock_geom
  include/bedrock/geom/step_export.hpp
  src/geom/step_export.cpp)

target_include_directories(bedrock_geom PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${OpenCASCADE_INCLUDE_DIR})

target_link_libraries(bedrock_geom PUBLIC
  bedrock_som
  ${OpenCASCADE_LIBRARIES})
