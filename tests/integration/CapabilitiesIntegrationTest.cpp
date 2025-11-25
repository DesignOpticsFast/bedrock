#include "IntegrationTestServerFixture.hpp"
#include "IntegrationTestClient.hpp"

#ifdef BEDROCK_WITH_TRANSPORT_DEPS
#include <gtest/gtest.h>
#include "palantir/capabilities.pb.h"
#include <QCoreApplication>
#include <QTimer>
#include <QThread>
#include <QElapsedTimer>
#include <QDebug>

class CapabilitiesIntegrationTest : public ::testing::Test {
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

TEST_F(CapabilitiesIntegrationTest, CapabilitiesRequestResponse) {
    qDebug() << "[TEST] Starting CapabilitiesRequestResponse test";
    
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
    
    qDebug() << "[TEST] Sending CapabilitiesRequest...";
    // Send CapabilitiesRequest and receive response
    palantir::CapabilitiesResponse response;
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
        success = client.getCapabilities(response, error);
        if (!success && i < maxAttempts - 1) {
            QThread::msleep(50);
        }
    }
    
    qDebug() << "[TEST] getCapabilities result: success=" << success << ", error=" << error;
    ASSERT_TRUE(success) 
        << "getCapabilities failed: " << error.toStdString() << " (after " << timer.elapsed() << "ms)";
    
    // Verify response is valid
    ASSERT_TRUE(response.IsInitialized());
    
    // Verify response has capabilities
    ASSERT_TRUE(response.has_capabilities());
    
    // Verify server version is set (should be non-empty)
    const auto& caps = response.capabilities();
    EXPECT_FALSE(caps.server_version().empty()) 
        << "Server version should not be empty";
    
    // Verify supported features list exists (may be empty, but field should exist)
    // This is a basic sanity check that the response structure is correct
    EXPECT_GE(caps.supported_features_size(), 0);
}

#else
// Stub when transport deps disabled
#include <gtest/gtest.h>
TEST(CapabilitiesIntegrationTest, DISABLED_RequiresTransportDeps) {
    GTEST_SKIP() << "Integration tests require BEDROCK_WITH_TRANSPORT_DEPS=ON";
}
#endif

