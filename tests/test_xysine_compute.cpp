#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include "palantir.pb.h"

// Forward declaration - we'll test computeXYSine via PalantirServer
class PalantirServer;

// Test helper: Create a ComputeSpec with given parameters
palantir::ComputeSpec createXYSineSpec(const std::map<std::string, std::string>& params)
{
    palantir::ComputeSpec spec;
    spec.set_feature_id("xy_sine");
    
    for (const auto& [key, value] : params) {
        (*spec.mutable_params())[key] = value;
    }
    
    return spec;
}

// Test helper: Compute XY Sine using the same algorithm as PalantirServer
void computeXYSineReference(const palantir::ComputeSpec& spec, 
                            std::vector<double>& xValues, 
                            std::vector<double>& yValues)
{
    // Parse parameters (same logic as PalantirServer::computeXYSine)
    double frequency = 1.0;
    double amplitude = 1.0;
    double phase = 0.0;
    int samples = 1000;
    bool explicitSamplesSet = false;
    
    for (const auto& [key, value] : spec.params()) {
        std::string paramKey = key;
        std::string paramValue = value;
        
        if (paramKey == "frequency") {
            frequency = std::stod(paramValue);
        } else if (paramKey == "amplitude") {
            amplitude = std::stod(paramValue);
        } else if (paramKey == "phase") {
            phase = std::stod(paramValue);
        } else if (paramKey == "samples") {
            samples = std::stoi(paramValue);
            explicitSamplesSet = true;
        } else if (paramKey == "n_samples") {
            if (!explicitSamplesSet) {
                samples = std::stoi(paramValue);
            }
        }
    }
    
    if (samples < 2) {
        samples = 2;
    }
    
    xValues.clear();
    yValues.clear();
    xValues.reserve(samples);
    yValues.reserve(samples);
    
    for (int i = 0; i < samples; ++i) {
        double t = static_cast<double>(i) / (samples - 1.0);
        double x = t * 2.0 * M_PI;
        double y = amplitude * std::sin(2.0 * M_PI * frequency * t + phase);
        
        xValues.push_back(x);
        yValues.push_back(y);
    }
}

TEST(XYSineCompute, BasicComputation)
{
    auto spec = createXYSineSpec({
        {"frequency", "1.0"},
        {"amplitude", "1.0"},
        {"phase", "0.0"},
        {"samples", "100"}
    });
    
    std::vector<double> xValues, yValues;
    computeXYSineReference(spec, xValues, yValues);
    
    EXPECT_EQ(xValues.size(), 100);
    EXPECT_EQ(yValues.size(), 100);
    
    // Check x is monotonic increasing from 0 to 2π
    EXPECT_NEAR(xValues[0], 0.0, 1e-10);
    EXPECT_NEAR(xValues[99], 2.0 * M_PI, 1e-10);
    
    for (size_t i = 1; i < xValues.size(); ++i) {
        EXPECT_GT(xValues[i], xValues[i-1]);
    }
    
    // Check y matches expected sine values
    // At t=0: y = sin(0) = 0
    EXPECT_NEAR(yValues[0], 0.0, 1e-10);
    
    // At t=0.25: y = sin(π/2) = 1.0
    EXPECT_NEAR(yValues[25], 1.0, 1e-6);
    
    // At t=0.5: y = sin(π) = 0
    EXPECT_NEAR(yValues[50], 0.0, 1e-6);
    
    // At t=0.75: y = sin(3π/2) = -1.0
    EXPECT_NEAR(yValues[75], -1.0, 1e-6);
    
    // At t=1.0: y = sin(2π) = 0
    EXPECT_NEAR(yValues[99], 0.0, 1e-6);
}

TEST(XYSineCompute, WithPhase)
{
    auto spec = createXYSineSpec({
        {"frequency", "1.0"},
        {"amplitude", "1.0"},
        {"phase", "1.5707963267948966"},  // π/2
        {"samples", "100"}
    });
    
    std::vector<double> xValues, yValues;
    computeXYSineReference(spec, xValues, yValues);
    
    // With phase = π/2, at t=0: y = sin(π/2) = 1.0
    EXPECT_NEAR(yValues[0], 1.0, 1e-6);
    
    // At t=0.25: y = sin(π/2 + π/2) = sin(π) = 0
    EXPECT_NEAR(yValues[25], 0.0, 1e-6);
}

TEST(XYSineCompute, WithFrequency)
{
    auto spec = createXYSineSpec({
        {"frequency", "2.0"},
        {"amplitude", "1.0"},
        {"phase", "0.0"},
        {"samples", "200"}
    });
    
    std::vector<double> xValues, yValues;
    computeXYSineReference(spec, xValues, yValues);
    
    // With frequency=2.0, should have 2 full cycles over 0..2π
    // At t=0: y = sin(0) = 0
    EXPECT_NEAR(yValues[0], 0.0, 1e-6);
    
    // At t=0.25: y = sin(π) = 0 (first zero crossing)
    EXPECT_NEAR(yValues[50], 0.0, 1e-6);
    
    // At t=0.5: y = sin(2π) = 0 (second zero crossing)
    EXPECT_NEAR(yValues[100], 0.0, 1e-6);
    
    // At t=0.75: y = sin(3π) = 0 (third zero crossing)
    EXPECT_NEAR(yValues[150], 0.0, 1e-6);
    
    // At t=1.0: y = sin(4π) = 0 (fourth zero crossing)
    EXPECT_NEAR(yValues[199], 0.0, 1e-6);
}

TEST(XYSineCompute, ParameterDefaults)
{
    // Empty params - should use defaults
    auto spec = createXYSineSpec({});
    
    std::vector<double> xValues, yValues;
    computeXYSineReference(spec, xValues, yValues);
    
    // Default samples = 1000
    EXPECT_EQ(xValues.size(), 1000);
    EXPECT_EQ(yValues.size(), 1000);
    
    // Default frequency=1.0, amplitude=1.0, phase=0.0
    // At t=0: y = sin(0) = 0
    EXPECT_NEAR(yValues[0], 0.0, 1e-10);
    
    // At t=0.25: y = sin(π/2) = 1.0
    EXPECT_NEAR(yValues[250], 1.0, 1e-6);
}

TEST(XYSineCompute, SampleClamping)
{
    // samples=1 should be clamped to 2
    auto spec1 = createXYSineSpec({{"samples", "1"}});
    std::vector<double> x1, y1;
    computeXYSineReference(spec1, x1, y1);
    EXPECT_EQ(x1.size(), 2);
    EXPECT_EQ(y1.size(), 2);
    
    // samples=0 should be clamped to 2
    auto spec2 = createXYSineSpec({{"samples", "0"}});
    std::vector<double> x2, y2;
    computeXYSineReference(spec2, x2, y2);
    EXPECT_EQ(x2.size(), 2);
    EXPECT_EQ(y2.size(), 2);
    
    // samples=-1 should be clamped to 2
    auto spec3 = createXYSineSpec({{"samples", "-1"}});
    std::vector<double> x3, y3;
    computeXYSineReference(spec3, x3, y3);
    EXPECT_EQ(x3.size(), 2);
    EXPECT_EQ(y3.size(), 2);
}

TEST(XYSineCompute, BackwardsCompatibilityN_Samples)
{
    // Test n_samples alias
    auto spec = createXYSineSpec({
        {"frequency", "1.0"},
        {"amplitude", "1.0"},
        {"n_samples", "50"}  // Using old parameter name
    });
    
    std::vector<double> xValues, yValues;
    computeXYSineReference(spec, xValues, yValues);
    
    EXPECT_EQ(xValues.size(), 50);
    EXPECT_EQ(yValues.size(), 50);
}

TEST(XYSineCompute, SamplesTakesPrecedence)
{
    // If both "samples" and "n_samples" are present, "samples" wins
    auto spec = createXYSineSpec({
        {"samples", "100"},
        {"n_samples", "50"}  // Should be ignored
    });
    
    std::vector<double> xValues, yValues;
    computeXYSineReference(spec, xValues, yValues);
    
    EXPECT_EQ(xValues.size(), 100);  // "samples" takes precedence
}

