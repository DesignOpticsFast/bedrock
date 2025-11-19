#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QThread>
#include <QTimer>
#include <QLocalSocket>
#include <QByteArray>
#include <memory>
#include <vector>
#include <map>
#include <cmath>
#include "palantir/PalantirServer.hpp"
#include "palantir.pb.h"

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

TEST(PalantirXYSine, DirectComputation)
{
    // Create QCoreApplication for Qt event loop (required for PalantirServer)
    int argc = 1;
    char* argv[] = {const_cast<char*>("test")};
    QCoreApplication app(argc, argv);
    
    PalantirServer server;
    
    // Test with standard parameters
    auto spec = createXYSineSpec({
        {"frequency", "1.0"},
        {"amplitude", "1.0"},
        {"phase", "0.0"},
        {"samples", "100"}
    });
    
    std::vector<double> xValues, yValues;
    server.computeXYSine(spec, xValues, yValues);
    
    EXPECT_EQ(xValues.size(), 100);
    EXPECT_EQ(yValues.size(), 100);
    
    // Verify x is monotonic increasing from 0 to 2π
    EXPECT_NEAR(xValues[0], 0.0, 1e-10);
    EXPECT_NEAR(xValues[99], 2.0 * M_PI, 1e-10);
    
    for (size_t i = 1; i < xValues.size(); ++i) {
        EXPECT_GT(xValues[i], xValues[i-1]);
    }
    
    // Verify y matches expected sine values
    EXPECT_NEAR(yValues[0], 0.0, 1e-10);      // sin(0) = 0
    EXPECT_NEAR(yValues[25], 1.0, 1e-6);     // sin(π/2) = 1
    EXPECT_NEAR(yValues[50], 0.0, 1e-6);      // sin(π) = 0
    EXPECT_NEAR(yValues[75], -1.0, 1e-6);    // sin(3π/2) = -1
    EXPECT_NEAR(yValues[99], 0.0, 1e-6);     // sin(2π) = 0
}

TEST(PalantirXYSine, ParameterParsing)
{
    int argc = 1;
    char* argv[] = {const_cast<char*>("test")};
    QCoreApplication app(argc, argv);
    
    PalantirServer server;
    
    // Test with custom parameters
    auto spec = createXYSineSpec({
        {"frequency", "2.0"},
        {"amplitude", "3.0"},
        {"phase", "1.5707963267948966"},  // π/2
        {"samples", "200"}
    });
    
    std::vector<double> xValues, yValues;
    server.computeXYSine(spec, xValues, yValues);
    
    EXPECT_EQ(xValues.size(), 200);
    EXPECT_EQ(yValues.size(), 200);
    
    // With phase = π/2, at t=0: y = amplitude * sin(π/2) = 3.0
    EXPECT_NEAR(yValues[0], 3.0, 1e-6);
}

TEST(PalantirXYSine, DefaultParameters)
{
    int argc = 1;
    char* argv[] = {const_cast<char*>("test")};
    QCoreApplication app(argc, argv);
    
    PalantirServer server;
    
    // Empty params - should use defaults
    auto spec = createXYSineSpec({});
    
    std::vector<double> xValues, yValues;
    server.computeXYSine(spec, xValues, yValues);
    
    // Default samples = 1000
    EXPECT_EQ(xValues.size(), 1000);
    EXPECT_EQ(yValues.size(), 1000);
    
    // Default frequency=1.0, amplitude=1.0, phase=0.0
    EXPECT_NEAR(yValues[0], 0.0, 1e-10);
    EXPECT_NEAR(yValues[250], 1.0, 1e-6);  // sin(π/2) = 1
}

TEST(PalantirXYSine, BackwardsCompatibility)
{
    int argc = 1;
    char* argv[] = {const_cast<char*>("test")};
    QCoreApplication app(argc, argv);
    
    PalantirServer server;
    
    // Test n_samples alias
    auto spec = createXYSineSpec({
        {"n_samples", "50"}  // Old parameter name
    });
    
    std::vector<double> xValues, yValues;
    server.computeXYSine(spec, xValues, yValues);
    
    EXPECT_EQ(xValues.size(), 50);
}

TEST(PalantirXYSine, SamplesPrecedence)
{
    int argc = 1;
    char* argv[] = {const_cast<char*>("test")};
    QCoreApplication app(argc, argv);
    
    PalantirServer server;
    
    // If both "samples" and "n_samples" are present, "samples" wins
    auto spec = createXYSineSpec({
        {"samples", "100"},
        {"n_samples", "50"}  // Should be ignored
    });
    
    std::vector<double> xValues, yValues;
    server.computeXYSine(spec, xValues, yValues);
    
    EXPECT_EQ(xValues.size(), 100);  // "samples" takes precedence
}

TEST(PalantirXYSine, SampleClamping)
{
    int argc = 1;
    char* argv[] = {const_cast<char*>("test")};
    QCoreApplication app(argc, argv);
    
    PalantirServer server;
    
    // samples=1 should be clamped to 2
    auto spec = createXYSineSpec({{"samples", "1"}});
    std::vector<double> xValues, yValues;
    server.computeXYSine(spec, xValues, yValues);
    
    EXPECT_EQ(xValues.size(), 2);
    EXPECT_EQ(yValues.size(), 2);
}

TEST(PalantirXYSine, SupportedFeature)
{
    int argc = 1;
    char* argv[] = {const_cast<char*>("test")};
    QCoreApplication app(argc, argv);
    
    PalantirServer server;
    
    // Verify xy_sine is in supported features
    QStringList features = server.supportedFeatures();
    EXPECT_TRUE(features.contains("xy_sine"));
}

