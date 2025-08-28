add_library(bedrock_core
  src/bedrock_math.cpp
)

target_include_directories(bedrock_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(bedrock_core PUBLIC cxx_std_17)
