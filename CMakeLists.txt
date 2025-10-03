cmake_minimum_required(VERSION 3.20)

project(Bedrock
  VERSION 0.0.1
  DESCRIPTION "AI-native optical design core"
  LANGUAGES CXX)

# ---------------------------------------
# Toolchain / language
# ---------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------
# Options
# ---------------------------------------
option(BUILD_TESTING "Build tests" ON)

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
find_package(OpenCASCADE REQUIRED)

add_library(bedrock_geom
  include/bedrock/geom/step_export.hpp
  src/geom/step_export.cpp)

target_include_directories(bedrock_geom
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCASCADE_INCLUDE_DIR})

target_link_libraries(bedrock_geom
  PUBLIC
    bedrock_som
    ${OpenCASCADE_LIBRARIES})

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
