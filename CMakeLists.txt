# Bedrock Engine - CMake Build Configuration
# See cmake/Options.cmake, cmake/Dependencies.cmake, cmake/Targets.cmake

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

# Include modular CMake files
include(cmake/Options.cmake)
include(cmake/Dependencies.cmake)
include(cmake/Targets.cmake)
