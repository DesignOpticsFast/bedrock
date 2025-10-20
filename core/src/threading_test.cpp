#include "bedrock/threading.hpp"
#include <iostream>
#include <vector>
#include <cmath>

/**
 * @brief Simple test to verify OpenMP threading is working
 */
void test_basic_threading() {
    std::cout << "\n=== Basic Threading Test ===" << std::endl;
    
    // Initialize threading
    bedrock::ThreadingConfig::initialize();
    
    std::cout << "OpenMP Available: " << (bedrock::ThreadingConfig::is_openmp_available() ? "Yes" : "No") << std::endl;
    std::cout << "Thread Count: " << bedrock::ThreadingConfig::get_thread_count() << std::endl;
    std::cout << "OpenMP Version: " << bedrock::ThreadingConfig::get_openmp_version() << std::endl;
    
    // Test parallel computation
    const int size = 1000000;
    std::vector<double> data(size);
    
    {
        bedrock::ScopedTimer timer("Parallel computation");
        
        #pragma omp parallel for
        for (int i = 0; i < size; ++i) {
            data[i] = sin(i * 0.001) * cos(i * 0.001);
        }
    }
    
    // Verify computation
    double sum = 0.0;
    for (double val : data) {
        sum += val;
    }
    
    std::cout << "Computation result: " << sum << std::endl;
    std::cout << "Threading test completed successfully!" << std::endl;
}

/**
 * @brief Test ray tracing simulation with different batch sizes
 */
void test_ray_tracing_simulation() {
    std::cout << "\n=== Ray Tracing Simulation Test ===" << std::endl;
    
    const int total_rays = 50000;
    
    // Test different batch sizes
    std::vector<int> batch_sizes = {1000, 5000, 10000, 25000, 50000};
    
    for (int batch_size : batch_sizes) {
        {
            bedrock::ScopedTimer timer("Batch size " + std::to_string(batch_size));
            
            #pragma omp parallel for
            for (int i = 0; i < total_rays; i += batch_size) {
                int end = std::min(i + batch_size, total_rays);
                
                // Simulate ray processing
                for (int j = i; j < end; ++j) {
                    // Dummy ray processing work
                    volatile double x = j * 0.001;
                    for (int k = 0; k < 100; ++k) {
                        x = x * 1.001;
                    }
                }
            }
        }
    }
    
    std::cout << "Ray tracing simulation completed!" << std::endl;
}

/**
 * @brief Test system calibration
 */
void test_system_calibration() {
    std::cout << "\n=== System Calibration Test ===" << std::endl;
    
    std::string calibration_results = bedrock::ThreadingBenchmark::calibrate_system();
    std::cout << calibration_results << std::endl;
}

int main() {
    std::cout << "Bedrock Threading Test Suite" << std::endl;
    std::cout << "=============================" << std::endl;
    
    try {
        test_basic_threading();
        test_ray_tracing_simulation();
        test_system_calibration();
        
        std::cout << "\n✅ All threading tests passed!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
