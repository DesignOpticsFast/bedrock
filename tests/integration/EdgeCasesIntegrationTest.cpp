#include "IntegrationTestServerFixture.hpp"
#include "IntegrationTestClient.hpp"

#ifdef BEDROCK_WITH_TRANSPORT_DEPS
#include <gtest/gtest.h>
#include "palantir/capabilities.pb.h"
#include "palantir/xysine.pb.h"
#include <QCoreApplication>
#include <QThread>
#include <QElapsedTimer>
#include <QDebug>
#include <vector>
#include <memory>

class EdgeCasesIntegrationTest : public ::testing::Test {
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

TEST_F(EdgeCasesIntegrationTest, MultipleConcurrentClients) {
    qDebug() << "[TEST] Starting MultipleConcurrentClients test";
    
    const int numClients = 5;
    std::vector<std::unique_ptr<IntegrationTestClient>> clients;
    
    // Create and connect all clients
    for (int i = 0; i < numClients; ++i) {
        clients.push_back(std::make_unique<IntegrationTestClient>());
        ASSERT_TRUE(clients[i]->connect(fixture_.socketPath())) 
            << "Failed to connect client " << i;
    }
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Send requests sequentially but rapidly (simulating concurrent load)
    // Each client sends XY Sine with different params
    std::vector<bool> results(numClients, false);
    std::vector<palantir::XYSineResponse> responses(numClients);
    
    for (int i = 0; i < numClients; ++i) {
        palantir::XYSineRequest request;
        request.set_frequency(1.0 + i * 0.5);  // Different frequency for each client
        request.set_samples(10 + i * 5);       // Different sample count
        request.set_amplitude(1.0);
        request.set_phase(0.0);
        
        QString error;
        
        // Send request (rapidly, one after another)
        bool success = false;
        const int maxAttempts = 10;
        QElapsedTimer timer;
        timer.start();
        
        for (int j = 0; j < maxAttempts && !success && timer.elapsed() < 5000; ++j) {
            QCoreApplication::processEvents();
            success = clients[i]->sendXYSineRequest(request, responses[i], error);
            if (!success && j < maxAttempts - 1) {
                QThread::msleep(50);
            }
        }
        
        results[i] = success;
        if (!success) {
            qDebug() << "[TEST] Client" << i << "failed:" << error;
        } else {
            // Verify response matches request
            if (responses[i].x_size() != request.samples() || responses[i].y_size() != request.samples()) {
                qDebug() << "[TEST] Client" << i << "response size mismatch";
                results[i] = false;
            }
        }
    }
    
    // Verify all clients succeeded
    bool allSucceeded = true;
    for (int i = 0; i < numClients; ++i) {
        EXPECT_TRUE(results[i]) << "Client " << i << " request failed";
        if (!results[i]) {
            allSucceeded = false;
        }
    }
    
    ASSERT_TRUE(allSucceeded) << "Not all concurrent clients succeeded";
    
    // Verify no cross-talk: each client got its own result
    // Check that responses have different sizes (different sample counts)
    for (int i = 0; i < numClients; ++i) {
        int expectedSamples = 10 + i * 5;
        EXPECT_EQ(responses[i].x_size(), expectedSamples) 
            << "Client " << i << " should have " << expectedSamples << " samples";
        EXPECT_EQ(responses[i].y_size(), expectedSamples) 
            << "Client " << i << " should have " << expectedSamples << " samples";
    }
    
    qDebug() << "[TEST] MultipleConcurrentClients test completed successfully";
}

TEST_F(EdgeCasesIntegrationTest, RapidConnectDisconnect) {
    qDebug() << "[TEST] Starting RapidConnectDisconnect test";
    
    const int iterations = 20;
    
    // Rapid connect/disconnect loop
    for (int i = 0; i < iterations; ++i) {
        IntegrationTestClient client;
        if (client.connect(fixture_.socketPath())) {
            QCoreApplication::processEvents();
            QThread::msleep(10);  // Brief connection
            client.disconnect();
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
    }
    
    // Final client should still work
    IntegrationTestClient finalClient;
    ASSERT_TRUE(finalClient.connect(fixture_.socketPath())) 
        << "Failed to connect final client after rapid connect/disconnect";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Send a valid Capabilities request
    palantir::CapabilitiesResponse response;
    QString error;
    
    bool success = false;
    const int maxAttempts = 10;
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < maxAttempts && !success && timer.elapsed() < 5000; ++i) {
        QCoreApplication::processEvents();
        success = finalClient.getCapabilities(response, error);
        if (!success && i < maxAttempts - 1) {
            QThread::msleep(50);
        }
    }
    
    ASSERT_TRUE(success) 
        << "Final client request failed after rapid connect/disconnect: " << error.toStdString();
    
    // Verify response is valid
    ASSERT_TRUE(response.IsInitialized());
    ASSERT_TRUE(response.has_capabilities());
    
    qDebug() << "[TEST] RapidConnectDisconnect test completed successfully";
}

TEST_F(EdgeCasesIntegrationTest, LargeValidXYSineRequest) {
    qDebug() << "[TEST] Starting LargeValidXYSineRequest test";
    
    // Create a large but valid XY Sine request
    // MAX_MESSAGE_SIZE = 10MB, but we need to account for envelope overhead
    // A single sample in XYSineResponse is roughly: 8 bytes (x) + 8 bytes (y) = 16 bytes
    // Plus envelope overhead (~50-100 bytes), we can safely use ~500k samples
    // But to be conservative and ensure we stay well under 10MB, use 100k samples
    const int largeSamples = 100000;  // 100k samples = ~1.6MB payload (well under 10MB)
    
    IntegrationTestClient client;
    ASSERT_TRUE(client.connect(fixture_.socketPath())) 
        << "Failed to connect to test server";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Build large XY Sine request
    palantir::XYSineRequest request;
    request.set_frequency(1.0);
    request.set_samples(largeSamples);
    request.set_amplitude(1.0);
    request.set_phase(0.0);
    
    qDebug() << "[TEST] Sending large XY Sine request with" << largeSamples << "samples";
    
    // Send request and receive response
    palantir::XYSineResponse response;
    QString error;
    
    // Use explicit timeout with event processing
    bool success = false;
    const int maxAttempts = 30;  // More attempts for large request
    const int timeoutMs = 30000;  // 30 second timeout for large computation
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < maxAttempts && !success && timer.elapsed() < timeoutMs; ++i) {
        qDebug() << "[TEST] Attempt" << (i + 1) << "of" << maxAttempts << ", elapsed=" << timer.elapsed() << "ms";
        QCoreApplication::processEvents();
        success = client.sendXYSineRequest(request, response, error);
        if (!success && i < maxAttempts - 1) {
            QThread::msleep(100);
        }
    }
    
    qDebug() << "[TEST] sendXYSineRequest result: success=" << success << ", error=" << error;
    ASSERT_TRUE(success) 
        << "Large XY Sine request failed: " << error.toStdString() << " (after " << timer.elapsed() << "ms)";
    
    // Verify response is valid
    ASSERT_TRUE(response.IsInitialized());
    
    // Verify response has correct number of samples
    ASSERT_EQ(response.x_size(), largeSamples) 
        << "Response x size should match request samples";
    ASSERT_EQ(response.y_size(), largeSamples) 
        << "Response y size should match request samples";
    
    // Verify a few sample values (spot check)
    EXPECT_NEAR(response.x(0), 0.0, 1e-9) << "First x value should be 0";
    EXPECT_NEAR(response.y(0), 0.0, 1e-9) << "First y value should be 0 (sin(0) = 0)";
    
    // Verify no MESSAGE_TOO_LARGE error occurred
    EXPECT_FALSE(error.contains("too large")) << "Should not receive MESSAGE_TOO_LARGE error";
    
    qDebug() << "[TEST] LargeValidXYSineRequest test completed successfully";
}

TEST_F(EdgeCasesIntegrationTest, MinimalXYSineRequest) {
    qDebug() << "[TEST] Starting MinimalXYSineRequest test";
    
    // Create minimal XY Sine request (samples = 2, the lower bound)
    IntegrationTestClient client;
    ASSERT_TRUE(client.connect(fixture_.socketPath())) 
        << "Failed to connect to test server";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Build minimal XY Sine request
    palantir::XYSineRequest request;
    request.set_frequency(1.0);
    request.set_samples(2);  // Minimum valid samples
    request.set_amplitude(1.0);
    request.set_phase(0.0);
    
    qDebug() << "[TEST] Sending minimal XY Sine request with" << request.samples() << "samples";
    
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
        QCoreApplication::processEvents();
        success = client.sendXYSineRequest(request, response, error);
        if (!success && i < maxAttempts - 1) {
            QThread::msleep(50);
        }
    }
    
    qDebug() << "[TEST] sendXYSineRequest result: success=" << success << ", error=" << error;
    ASSERT_TRUE(success) 
        << "Minimal XY Sine request failed: " << error.toStdString() << " (after " << timer.elapsed() << "ms)";
    
    // Verify response is valid
    ASSERT_TRUE(response.IsInitialized());
    
    // Verify response has exactly 2 samples
    ASSERT_EQ(response.x_size(), 2) 
        << "Response x size should be 2";
    ASSERT_EQ(response.y_size(), 2) 
        << "Response y size should be 2";
    
    // Verify no validation error occurred
    EXPECT_FALSE(error.contains("Invalid parameter")) << "Should not receive INVALID_PARAMETER_VALUE error";
    EXPECT_FALSE(error.contains("samples")) << "Should not receive samples validation error";
    
    // Verify boundary correctness: samples = 2 should be accepted (not rejected like samples = 1)
    EXPECT_TRUE(response.x_size() == 2 && response.y_size() == 2) 
        << "Boundary case (samples=2) should succeed";
    
    qDebug() << "[TEST] MinimalXYSineRequest test completed successfully";
}

TEST_F(EdgeCasesIntegrationTest, MixedModeSequence) {
    qDebug() << "[TEST] Starting MixedModeSequence test";
    
    IntegrationTestClient client;
    ASSERT_TRUE(client.connect(fixture_.socketPath())) 
        << "Failed to connect to test server";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Step 1: Valid Capabilities request
    qDebug() << "[TEST] Step 1: Sending Capabilities request";
    palantir::CapabilitiesResponse capsResponse;
    QString error;
    
    bool success = false;
    const int maxAttempts = 10;
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < maxAttempts && !success && timer.elapsed() < 5000; ++i) {
        QCoreApplication::processEvents();
        success = client.getCapabilities(capsResponse, error);
        if (!success && i < maxAttempts - 1) {
            QThread::msleep(50);
        }
    }
    
    ASSERT_TRUE(success) << "Capabilities request failed: " << error.toStdString();
    ASSERT_TRUE(capsResponse.IsInitialized());
    qDebug() << "[TEST] Step 1: Capabilities request succeeded";
    
    // Step 2: Valid XY Sine request
    qDebug() << "[TEST] Step 2: Sending valid XY Sine request";
    palantir::XYSineRequest validRequest;
    validRequest.set_frequency(1.0);
    validRequest.set_samples(10);
    validRequest.set_amplitude(1.0);
    validRequest.set_phase(0.0);
    
    palantir::XYSineResponse validResponse;
    error.clear();
    success = false;
    timer.restart();
    
    for (int i = 0; i < maxAttempts && !success && timer.elapsed() < 5000; ++i) {
        QCoreApplication::processEvents();
        success = client.sendXYSineRequest(validRequest, validResponse, error);
        if (!success && i < maxAttempts - 1) {
            QThread::msleep(50);
        }
    }
    
    ASSERT_TRUE(success) << "Valid XY Sine request failed: " << error.toStdString();
    ASSERT_TRUE(validResponse.IsInitialized());
    ASSERT_EQ(validResponse.x_size(), 10);
    qDebug() << "[TEST] Step 2: Valid XY Sine request succeeded";
    
    // Step 3: Invalid XY Sine request (should return error but not poison connection)
    qDebug() << "[TEST] Step 3: Sending invalid XY Sine request (samples=1)";
    palantir::XYSineRequest invalidRequest;
    invalidRequest.set_frequency(1.0);
    invalidRequest.set_samples(1);  // Invalid: must be >= 2
    invalidRequest.set_amplitude(1.0);
    invalidRequest.set_phase(0.0);
    
    palantir::XYSineResponse invalidResponse;
    error.clear();
    success = client.sendXYSineRequest(invalidRequest, invalidResponse, error);
    
    // This should fail with an error
    EXPECT_FALSE(success) << "Invalid XY Sine request should fail";
    EXPECT_FALSE(error.isEmpty()) << "Should receive error message";
    EXPECT_TRUE(error.contains("Invalid parameter") || error.contains("samples")) 
        << "Error should mention invalid parameter or samples";
    qDebug() << "[TEST] Step 3: Invalid XY Sine request correctly rejected with error:" << error;
    
    // Step 4: Another valid XY Sine request (should still work after error)
    qDebug() << "[TEST] Step 4: Sending another valid XY Sine request (after error)";
    palantir::XYSineRequest finalRequest;
    finalRequest.set_frequency(2.0);
    finalRequest.set_samples(20);
    finalRequest.set_amplitude(1.5);
    finalRequest.set_phase(0.5);
    
    palantir::XYSineResponse finalResponse;
    error.clear();
    success = false;
    timer.restart();
    
    for (int i = 0; i < maxAttempts && !success && timer.elapsed() < 5000; ++i) {
        QCoreApplication::processEvents();
        success = client.sendXYSineRequest(finalRequest, finalResponse, error);
        if (!success && i < maxAttempts - 1) {
            QThread::msleep(50);
        }
    }
    
    ASSERT_TRUE(success) << "Final valid XY Sine request failed after error: " << error.toStdString();
    ASSERT_TRUE(finalResponse.IsInitialized());
    ASSERT_EQ(finalResponse.x_size(), 20);
    ASSERT_EQ(finalResponse.y_size(), 20);
    qDebug() << "[TEST] Step 4: Final valid XY Sine request succeeded (connection not poisoned)";
    
    qDebug() << "[TEST] MixedModeSequence test completed successfully";
}

#else
// Stub when transport deps disabled
#include <gtest/gtest.h>
TEST(EdgeCasesIntegrationTest, DISABLED_RequiresTransportDeps) {
    GTEST_SKIP() << "Integration tests require BEDROCK_WITH_TRANSPORT_DEPS=ON";
}
#endif

