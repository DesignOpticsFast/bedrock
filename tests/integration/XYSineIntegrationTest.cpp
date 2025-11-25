#include "IntegrationTestServerFixture.hpp"
#include "IntegrationTestClient.hpp"

#ifdef BEDROCK_WITH_TRANSPORT_DEPS
#include <gtest/gtest.h>
#include "palantir/xysine.pb.h"
#include <QCoreApplication>
#include <QThread>
#include <QElapsedTimer>
#include <QDebug>
#include <cmath>

class XYSineIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure QCoreApplication exists
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char* argv[] = { const_cast<char*>("integration_tests"), nullptr };
            app_ = std::make_unique<QCoreApplication>(argc, argv);
        }
        
        qDebug() << "[TEST] SetUp: Starting server fixture...";
        // Start server
        ASSERT_TRUE(fixture_.startServer()) << "Failed to start test server";
        
        qDebug() << "[TEST] SetUp: Server started, processing events...";
        // Give server a moment to be ready and process any pending events
        QCoreApplication::processEvents();
        QThread::msleep(100);  // Small delay to ensure server is fully ready
        QCoreApplication::processEvents();
        qDebug() << "[TEST] SetUp: Server ready";
    }
    
    void TearDown() override {
        fixture_.stopServer();
        QCoreApplication::processEvents();
    }
    
    IntegrationTestServerFixture fixture_;
    std::unique_ptr<QCoreApplication> app_;
};

TEST_F(XYSineIntegrationTest, XYSineRequestResponse) {
    qDebug() << "[TEST] Starting XYSineRequestResponse test";
    
    // Create client and connect
    IntegrationTestClient client;
    qDebug() << "[TEST] Connecting to socket:" << fixture_.socketPath();
    ASSERT_TRUE(client.connect(fixture_.socketPath())) 
        << "Failed to connect to test server";
    
    qDebug() << "[TEST] Client connected, processing events to allow server to register client...";
    // Process events to allow connection to complete and server to register client
    QCoreApplication::processEvents();
    QThread::msleep(100);  // Give server time to process newConnection signal
    QCoreApplication::processEvents();
    QThread::msleep(50);
    QCoreApplication::processEvents();
    qDebug() << "[TEST] Events processed, ready to send request";
    
    // Build XYSineRequest
    palantir::XYSineRequest request;
    request.set_frequency(1.0);
    request.set_samples(10);
    request.set_amplitude(2.0);
    request.set_phase(0.0);
    
    qDebug() << "[TEST] Sending XYSineRequest: freq=" << request.frequency() 
             << ", samples=" << request.samples() 
             << ", amplitude=" << request.amplitude();
    
    // Send request and receive response
    palantir::XYSineResponse response;
    QString error;
    
    // Use explicit timeout with event processing
    bool success = false;
    const int maxAttempts = 20;
    const int timeoutMs = 5000;
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < maxAttempts && !success && timer.elapsed() < timeoutMs; ++i) {
        qDebug() << "[TEST] Attempt" << (i + 1) << "of" << maxAttempts << ", elapsed=" << timer.elapsed() << "ms";
        QCoreApplication::processEvents();
        success = client.sendXYSineRequest(request, response, error);
        if (!success && i < maxAttempts - 1) {
            QThread::msleep(50);
        }
    }
    
    qDebug() << "[TEST] sendXYSineRequest result: success=" << success << ", error=" << error;
    ASSERT_TRUE(success) 
        << "sendXYSineRequest failed: " << error.toStdString() << " (after " << timer.elapsed() << "ms)";
    
    // Verify response is valid
    ASSERT_TRUE(response.IsInitialized());
    
    // Validate response has correct number of samples
    ASSERT_EQ(response.x_size(), request.samples()) 
        << "Response x size should match request samples";
    ASSERT_EQ(response.y_size(), request.samples()) 
        << "Response y size should match request samples";
    
    // Validate numeric results
    // Server algorithm: t = i / (samples - 1), y = amplitude * sin(2π * frequency * t + phase)
    // For i in [0, samples-1], t goes from 0 to 1
    for (int i = 0; i < request.samples(); ++i) {
        double t = static_cast<double>(i) / (request.samples() - 1.0);  // 0 to 1
        double expected = request.amplitude() * std::sin(2.0 * M_PI * request.frequency() * t + request.phase());
        
        EXPECT_NEAR(response.y(i), expected, 1e-9) 
            << "Y value at index " << i << " should match expected sine wave";
    }
    
    // Validate X values (should be t * 2π, from 0 to 2π)
    for (int i = 0; i < request.samples(); ++i) {
        double t = static_cast<double>(i) / (request.samples() - 1.0);  // 0 to 1
        double expectedX = t * 2.0 * M_PI;  // 0 to 2π
        
        EXPECT_NEAR(response.x(i), expectedX, 1e-9) 
            << "X value at index " << i << " should match expected domain";
    }
    
    // Validate status (should be "OK" for successful computation)
    // Note: In proto3, status is always present (defaults to empty string)
    if (!response.status().empty()) {
        EXPECT_EQ(response.status(), "OK") 
            << "Response status should be OK";
    }
    
    qDebug() << "[TEST] XYSineRequestResponse test completed successfully";
}

#else
// Stub when transport deps disabled
#include <gtest/gtest.h>
TEST(XYSineIntegrationTest, DISABLED_RequiresTransportDeps) {
    GTEST_SKIP() << "Integration tests require BEDROCK_WITH_TRANSPORT_DEPS=ON";
}
#endif

