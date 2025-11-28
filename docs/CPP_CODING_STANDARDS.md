# C++ Coding Standards - Bedrock

**Version:** 1.0.0  
**Last Updated:** 2025-10-20  
**Language:** C++20  
**Framework:** OpenMP (Qt optional, see [VERSIONS.md](VERSIONS.md))

> **Note:** For current toolchain versions (C++ standard, CMake, Protobuf, Qt, etc.), see [VERSIONS.md](VERSIONS.md).

---

## Core Principles

### 1. Modern C++ First
Use C++20 features and modern idioms. Prefer standard library over custom implementations.

**Good:**
```cpp
#include <memory>
#include <vector>
#include <algorithm>

auto process_rays(const std::vector<Ray>& rays) -> std::vector<Result> {
    std::vector<Result> results;
    results.reserve(rays.size());
    
    #pragma omp parallel for
    for (size_t i = 0; i < rays.size(); ++i) {
        results[i] = compute_ray(rays[i]);
    }
    return results;
}
```

**Bad:**
```cpp
// Raw pointers, manual memory management
Ray* process_rays(Ray* rays, int count) {
    Ray* results = new Ray[count];
    // ... manual processing
    return results;
}
```

---

### 2. OpenMP Integration
Use OpenMP for parallel computation with automatic optimization.

**Good:**
```cpp
#include "bedrock/threading.hpp"

void compute_parallel() {
    bedrock::ThreadingConfig::initialize();
    
    #pragma omp parallel for
    for (int i = 0; i < 1000000; ++i) {
        // Compute-intensive work
    }
}
```

**Bad:**
```cpp
// Manual threading without OpenMP
std::vector<std::thread> threads;
// ... complex manual thread management
```

---

### 3. Performance Measurement
Always measure performance for compute-intensive operations.

**Good:**
```cpp
{
    bedrock::ScopedTimer timer("Ray tracing");
    #pragma omp parallel for
    for (int i = 0; i < ray_count; ++i) {
        // Ray processing
    }
}
```

**Bad:**
```cpp
// No performance measurement
for (int i = 0; i < ray_count; ++i) {
    // Ray processing
}
```

---

## Naming Conventions

### Classes and Types
- **PascalCase** for classes and types
- **Descriptive names** that explain purpose

```cpp
class ThreadingConfig { };
class PerformanceCounter { };
class ScopedTimer { };
```

### Functions and Variables
- **snake_case** for functions and variables
- **Verb-noun** pattern for functions

```cpp
void initialize_threading();
int get_optimal_thread_count();
double measure_execution_time();
```

### Constants
- **UPPER_SNAKE_CASE** for constants

```cpp
const int MAX_THREADS = 16;
const double PI = 3.14159265359;
```

---

## OpenMP Best Practices

### 1. Automatic Threading
Always use automatic threading configuration:

```cpp
// Initialize with system-optimal settings
bedrock::ThreadingConfig::initialize();

// Let OpenMP handle thread management
#pragma omp parallel for
for (int i = 0; i < size; ++i) {
    // Work
}
```

### 2. Batch Size Optimization
Use optimal batch sizes for ray tracing:

```cpp
// Get optimal batch size
int batch_size = bedrock::ThreadingBenchmark::find_optimal_batch_size(total_rays);

#pragma omp parallel for
for (int i = 0; i < total_rays; i += batch_size) {
    int end = std::min(i + batch_size, total_rays);
    for (int j = i; j < end; ++j) {
        // Process ray j
    }
}
```

### 3. Performance Profiling
Always profile parallel code:

```cpp
// Benchmark thread scaling
auto results = bedrock::ThreadingBenchmark::benchmark_thread_scaling(
    []() { /* computation */ }, 10
);
```

---

## Error Handling

### 1. Exceptions for Errors
Use exceptions for error conditions, not return codes.

**Good:**
```cpp
void initialize_threading() {
    if (!bedrock::ThreadingConfig::is_openmp_available()) {
        throw std::runtime_error("OpenMP not available");
    }
    // ... initialization
}
```

**Bad:**
```cpp
int initialize_threading() {
    if (!bedrock::ThreadingConfig::is_openmp_available()) {
        return -1; // Error code
    }
    return 0; // Success
}
```

### 2. RAII for Resources
Use RAII for automatic resource management.

```cpp
class ScopedTimer {
public:
    ScopedTimer(const std::string& name) : m_name(name) {
        m_start = std::chrono::high_resolution_clock::now();
    }
    
    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start);
        std::cout << m_name << ": " << duration.count() << " ms" << std::endl;
    }
    
private:
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start;
};
```

---

## Documentation

### 1. Header Documentation
Document all public APIs:

```cpp
/**
 * @brief Threading configuration and performance utilities
 * 
 * Provides OpenMP-based multithreading support with automatic optimization
 * and system capability measurement for optimal performance.
 */
class ThreadingConfig {
public:
    /**
     * @brief Initialize threading configuration
     * @param max_threads Maximum number of threads to use (0 = auto-detect)
     */
    static void initialize(int max_threads = 0);
    
    /**
     * @brief Get the optimal number of threads for the current system
     * @return Number of threads
     */
    static int get_optimal_thread_count();
};
```

### 2. Inline Comments
Explain complex algorithms and OpenMP directives:

```cpp
// Find optimal batch size for ray tracing
int optimal_batch = bedrock::ThreadingBenchmark::find_optimal_batch_size(
    50000,  // total rays
    1000,   // min batch size
    50000   // max batch size
);

// Parallel ray processing with optimal batching
#pragma omp parallel for
for (int i = 0; i < total_rays; i += optimal_batch) {
    int end = std::min(i + optimal_batch, total_rays);
    for (int j = i; j < end; ++j) {
        // Process ray j with optimal batch size
        process_ray(j);
    }
}
```

---

## Testing

### 1. Unit Tests
Write tests for all public APIs:

```cpp
#include <gtest/gtest.h>
#include "bedrock/threading.hpp"

TEST(ThreadingConfig, Initialize) {
    bedrock::ThreadingConfig::initialize();
    EXPECT_TRUE(bedrock::ThreadingConfig::is_openmp_available());
    EXPECT_GT(bedrock::ThreadingConfig::get_thread_count(), 0);
}

TEST(ThreadingBenchmark, Performance) {
    auto results = bedrock::ThreadingBenchmark::benchmark_thread_scaling(
        []() { /* test workload */ }, 5
    );
    EXPECT_FALSE(results.empty());
}
```

### 2. Performance Tests
Include performance benchmarks:

```cpp
TEST(ThreadingBenchmark, RayTracingPerformance) {
    const int total_rays = 50000;
    
    {
        bedrock::ScopedTimer timer("Ray tracing test");
        #pragma omp parallel for
        for (int i = 0; i < total_rays; ++i) {
            // Simulate ray processing
            volatile double x = i * 0.001;
            for (int j = 0; j < 100; ++j) {
                x = x * 1.001;
            }
        }
    }
}
```

---

## Build System

### 1. CMakeLists.txt
Include OpenMP in all targets:

```cmake
# Find OpenMP
find_package(OpenMP REQUIRED)

# Link to targets
target_link_libraries(bedrock_core
  PUBLIC
    OpenMP::OpenMP_CXX
)
```

### 2. Compiler Flags
Use appropriate compiler flags:

```cmake
# OpenMP flags are handled automatically by CMake
# No manual flag management needed
```

---

## Summary

- **Use modern C++20** features and idioms
- **Integrate OpenMP** for parallel computation
- **Measure performance** for all compute-intensive operations
- **Use RAII** for resource management
- **Document all APIs** with clear examples
- **Write comprehensive tests** including performance benchmarks
- **Follow naming conventions** consistently

These standards ensure high-performance, maintainable C++ code with optimal OpenMP integration for Bedrock's computational requirements.

