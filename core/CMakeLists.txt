add_library(bedrock_core
  src/bedrock_math.cpp
  src/threading.cpp
)

target_include_directories(bedrock_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(bedrock_core PUBLIC cxx_std_17)

# Link OpenMP for multithreading support (if available)
if(OpenMP_CXX_FOUND)
    message(STATUS "Linking OpenMP to bedrock_core")
    target_link_libraries(bedrock_core
      PUBLIC
        OpenMP::OpenMP_CXX
    )
    target_compile_definitions(bedrock_core PUBLIC BEDROCK_HAS_OPENMP=1)
else()
    message(STATUS "Building bedrock_core without OpenMP")
endif()

# Add threading test executable
add_executable(bedrock_threading_test
  src/threading_test.cpp
)

target_link_libraries(bedrock_threading_test
  PRIVATE
    bedrock_core
)

# Link OpenMP to test executable if available
if(OpenMP_CXX_FOUND)
    target_link_libraries(bedrock_threading_test
      PRIVATE
        OpenMP::OpenMP_CXX
    )
endif()
