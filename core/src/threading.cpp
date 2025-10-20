#include "bedrock/threading.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <functional>
#include <cmath>

namespace bedrock {

// Static member definitions
bool ThreadingConfig::s_initialized = false;
int ThreadingConfig::s_optimal_threads = 0;
int ThreadingConfig::s_max_threads = 0;

void ThreadingConfig::initialize(int max_threads) {
    if (s_initialized) {
        return;
    }
    
    s_max_threads = max_threads;
    
    if (is_openmp_available()) {
        // Get system capabilities
        int num_procs = omp_get_num_procs();
        int max_threads_available = omp_get_max_threads();
        
        // Determine optimal thread count
        if (max_threads > 0) {
            s_optimal_threads = std::min(max_threads, max_threads_available);
        } else {
            // Use all available processors by default
            s_optimal_threads = num_procs;
        }
        
        // Set OpenMP thread count
        omp_set_num_threads(s_optimal_threads);
        
        std::cout << "ThreadingConfig initialized: " << s_optimal_threads 
                  << " threads (max available: " << max_threads_available << ")" << std::endl;
    } else {
        s_optimal_threads = 1;
        std::cout << "OpenMP not available, using single-threaded mode" << std::endl;
    }
    
    s_initialized = true;
}

int ThreadingConfig::get_optimal_thread_count() {
    if (!s_initialized) {
        initialize();
    }
    return s_optimal_threads;
}

void ThreadingConfig::set_thread_count(int num_threads) {
    if (is_openmp_available()) {
        if (num_threads <= 0) {
            num_threads = get_optimal_thread_count();
        }
        omp_set_num_threads(num_threads);
        s_optimal_threads = num_threads;
    }
}

int ThreadingConfig::get_thread_count() {
    if (is_openmp_available()) {
        return omp_get_max_threads();
    }
    return 1;
}

bool ThreadingConfig::is_openmp_available() {
#ifdef _OPENMP
    return true;
#else
    return false;
#endif
}

std::string ThreadingConfig::get_openmp_version() {
    if (is_openmp_available()) {
        return std::to_string(_OPENMP);
    }
    return "OpenMP not available";
}

std::string ThreadingConfig::get_system_info() {
    std::string info;
    
    if (is_openmp_available()) {
        info += "OpenMP Version: " + get_openmp_version() + "\n";
        info += "Available Processors: " + std::to_string(omp_get_num_procs()) + "\n";
        info += "Max Threads: " + std::to_string(omp_get_max_threads()) + "\n";
        info += "Current Threads: " + std::to_string(omp_get_num_threads()) + "\n";
    } else {
        info += "OpenMP not available - single-threaded mode\n";
    }
    
    return info;
}

// ThreadingBenchmark implementation
template<typename Func>
std::vector<std::pair<int, double>> ThreadingBenchmark::benchmark_thread_scaling(
    Func&& func, 
    int iterations
) {
    std::vector<std::pair<int, double>> results;
    
    if (!ThreadingConfig::is_openmp_available()) {
        results.push_back({1, measure_execution_time(func, iterations)});
        return results;
    }
    
    int max_threads = omp_get_num_procs();
    
    for (int threads = 1; threads <= max_threads; ++threads) {
        omp_set_num_threads(threads);
        
        double time_ms = measure_execution_time(func, iterations);
        results.push_back({threads, time_ms});
        
        std::cout << "Threads: " << threads << ", Time: " << time_ms << " ms" << std::endl;
    }
    
    return results;
}

int ThreadingBenchmark::find_optimal_batch_size(
    int total_rays, 
    int min_batch_size, 
    int max_batch_size
) {
    if (!ThreadingConfig::is_openmp_available()) {
        return total_rays; // Single-threaded, process all at once
    }
    
    int optimal_batch_size = min_batch_size;
    double best_time = std::numeric_limits<double>::max();
    
    // Test different batch sizes
    for (int batch_size = min_batch_size; batch_size <= max_batch_size; batch_size *= 2) {
        if (batch_size > total_rays) break;
        
        // Create a test function that processes rays in batches
        auto test_func = [total_rays, batch_size]() {
            #pragma omp parallel for
            for (int i = 0; i < total_rays; i += batch_size) {
                int end = std::min(i + batch_size, total_rays);
                // Simulate ray processing work
                for (int j = i; j < end; ++j) {
                    // Dummy work - replace with actual ray processing
                    volatile double x = j * 0.001;
                    for (int k = 0; k < 100; ++k) {
                        x = x * 1.001;
                    }
                }
            }
        };
        
        double time_ms = measure_execution_time(test_func, 5);
        
        if (time_ms < best_time) {
            best_time = time_ms;
            optimal_batch_size = batch_size;
        }
        
        std::cout << "Batch size: " << batch_size << ", Time: " << time_ms << " ms" << std::endl;
    }
    
    return optimal_batch_size;
}

std::string ThreadingBenchmark::calibrate_system() {
    std::string results = "=== Bedrock Threading Calibration ===\n\n";
    
    results += ThreadingConfig::get_system_info() + "\n";
    
    if (!ThreadingConfig::is_openmp_available()) {
        results += "OpenMP not available - using single-threaded mode\n";
        return results;
    }
    
    // Test with a compute-intensive workload
    auto test_workload = []() {
        const int size = 1000000;
        std::vector<double> data(size);
        
        #pragma omp parallel for
        for (int i = 0; i < size; ++i) {
            data[i] = sin(i * 0.001) * cos(i * 0.001);
        }
        
        // Prevent optimization
        double sum = 0;
        for (double val : data) {
            sum += val;
        }
    };
    
    results += "Benchmarking thread scaling...\n";
    auto scaling_results = benchmark_thread_scaling(test_workload, 5);
    
    // Find optimal thread count
    auto best_result = std::min_element(scaling_results.begin(), scaling_results.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    results += "Optimal thread count: " + std::to_string(best_result->first) + "\n";
    results += "Best performance: " + std::to_string(best_result->second) + " ms\n\n";
    
    // Test ray tracing batch size
    results += "Testing ray tracing batch sizes...\n";
    int optimal_batch = find_optimal_batch_size(50000, 1000, 50000);
    results += "Optimal batch size for 50,000 rays: " + std::to_string(optimal_batch) + "\n";
    
    return results;
}

double ThreadingBenchmark::measure_execution_time(std::function<void()> func, int iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    return duration.count() / 1000.0 / iterations; // Convert to milliseconds
}

// PerformanceCounter implementation
PerformanceCounter::PerformanceCounter(const std::string& name) 
    : m_name(name), m_running(false) {
}

PerformanceCounter::~PerformanceCounter() {
    // Destructor implementation
}

void PerformanceCounter::start() {
    m_start_time = std::chrono::high_resolution_clock::now();
    m_running = true;
}

void PerformanceCounter::stop() {
    if (m_running) {
        m_running = false;
    }
}

double PerformanceCounter::get_elapsed_ms() const {
    if (m_running) {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - m_start_time);
        return duration.count() / 1000.0;
    }
    return 0.0;
}

double PerformanceCounter::get_elapsed_seconds() const {
    return get_elapsed_ms() / 1000.0;
}

// ScopedTimer implementation
ScopedTimer::ScopedTimer(const std::string& name) 
    : m_counter(std::make_unique<PerformanceCounter>(name)) {
    m_counter->start();
}

ScopedTimer::~ScopedTimer() {
    m_counter->stop();
    std::cout << "Timer: " << m_counter->get_elapsed_ms() << " ms" << std::endl;
}

} // namespace bedrock
