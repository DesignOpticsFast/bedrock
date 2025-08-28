cmake_minimum_required(VERSION 3.20)

project(Bedrock
  VERSION 0.0.1
  DESCRIPTION "AI-native optical design core"
  LANGUAGES CXX)

# Use modern C++
set(CMAKE_CXX_STANDARD 17)
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
