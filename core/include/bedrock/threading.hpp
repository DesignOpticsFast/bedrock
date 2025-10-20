#pragma once

#include <omp.h>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <functional>
#include <cmath>
#include <limits>

namespace bedrock {

/**
 * @brief Threading configuration and performance utilities for Bedrock
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
    
    /**
     * @brief Set the number of threads for OpenMP
     * @param num_threads Number of threads (0 = auto-detect)
     */
    static void set_thread_count(int num_threads);
    
    /**
     * @brief Get current thread count
     * @return Current number of threads
     */
    static int get_thread_count();
    
    /**
     * @brief Check if OpenMP is available
     * @return True if OpenMP is available
     */
    static bool is_openmp_available();
    
    /**
     * @brief Get OpenMP version string
     * @return OpenMP version information
     */
    static std::string get_openmp_version();
    
    /**
     * @brief Get system information for threading optimization
     * @return System information string
     */
    static std::string get_system_info();

private:
    static bool s_initialized;
    static int s_optimal_threads;
    static int s_max_threads;
};

/**
 * @brief Performance measurement utilities for threading optimization
 */
class ThreadingBenchmark {
public:
    /**
     * @brief Benchmark a function with different thread counts
     * @tparam Func Function type
     * @param func Function to benchmark
     * @param iterations Number of iterations to run
     * @return Vector of (thread_count, time_ms) pairs
     */
    template<typename Func>
    static std::vector<std::pair<int, double>> benchmark_thread_scaling(
        Func&& func, 
        int iterations = 10
    );
    
    /**
     * @brief Measure optimal batch size for ray tracing
     * @param total_rays Total number of rays to process
     * @param min_batch_size Minimum batch size to test
     * @param max_batch_size Maximum batch size to test
     * @return Optimal batch size
     */
    static int find_optimal_batch_size(
        int total_rays, 
        int min_batch_size = 1000, 
        int max_batch_size = 100000
    );
    
    /**
     * @brief Calibrate threading for the current system
     * @return Calibration results
     */
    static std::string calibrate_system();

private:
    static double measure_execution_time(std::function<void()> func, int iterations);
};

/**
 * @brief Thread-safe performance counter
 */
class PerformanceCounter {
public:
    PerformanceCounter(const std::string& name);
    ~PerformanceCounter();
    
    void start();
    void stop();
    double get_elapsed_ms() const;
    double get_elapsed_seconds() const;
    
private:
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start_time;
    bool m_running;
};

/**
 * @brief RAII performance timer
 */
class ScopedTimer {
public:
    ScopedTimer(const std::string& name);
    ~ScopedTimer();
    
private:
    std::unique_ptr<PerformanceCounter> m_counter;
};

// Convenience macros for performance measurement
#define BEDROCK_TIMER(name) bedrock::ScopedTimer _timer(name)
#define BEDROCK_PARALLEL_FOR(iterations) _Pragma("omp parallel for") for(int i = 0; i < (iterations); ++i)

} // namespace bedrock
