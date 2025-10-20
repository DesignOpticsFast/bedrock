add_library(bedrock_core
  src/bedrock_math.cpp
  src/threading.cpp
)

target_include_directories(bedrock_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(bedrock_core PUBLIC cxx_std_17)

# Link OpenMP for multithreading support
target_link_libraries(bedrock_core
  PUBLIC
    OpenMP::OpenMP_CXX
)

# Add threading test executable
add_executable(bedrock_threading_test
  src/threading_test.cpp
)

target_link_libraries(bedrock_threading_test
  PRIVATE
    bedrock_core
    OpenMP::OpenMP_CXX
)
