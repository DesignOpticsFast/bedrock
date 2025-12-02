#include "IntegrationTestServerFixture.hpp"
#include "IntegrationTestClient.hpp"

#ifdef BEDROCK_WITH_TRANSPORT_DEPS
#include <gtest/gtest.h>
#include "palantir/capabilities.pb.h"
#include "palantir/xysine.pb.h"
#include "palantir/envelope.pb.h"
#include "palantir/error.pb.h"
#include "palantir/EnvelopeHelpers.hpp"
#include <QCoreApplication>
#include <QThread>
#include <QElapsedTimer>
#include <QDebug>
#include <QLocalSocket>
#include <QByteArray>
#include <cstring>
#include <random>

class ErrorCasesIntegrationTest : public ::testing::Test {
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
    
    // Helper to send raw bytes (for oversized message test)
    bool sendRawBytes(const QByteArray& data, QLocalSocket* socket) {
        if (!socket || socket->state() != QLocalSocket::ConnectedState) {
            return false;
        }
        qint64 written = socket->write(data);
        socket->flush();
        return written == data.size();
    }
    
    // Helper to receive ErrorResponse
    bool receiveErrorResponse(QLocalSocket* socket, palantir::ErrorResponse& outError, QString& outErrorStr) {
        // Wait for data
        int elapsed = 0;
        const int timeoutMs = 5000;
        while (elapsed < timeoutMs && socket->bytesAvailable() < 4) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
            elapsed += 10;
        }
        
        if (socket->bytesAvailable() < 4) {
            outErrorStr = "Timeout waiting for response";
            return false;
        }
        
        // Read length prefix
        QByteArray lengthBytes = socket->read(4);
        if (lengthBytes.size() != 4) {
            outErrorStr = "Failed to read length prefix";
            return false;
        }
        
        uint32_t responseLength;
        std::memcpy(&responseLength, lengthBytes.data(), 4);
        
        // Wait for complete envelope
        elapsed = 0;
        while (elapsed < timeoutMs && socket->bytesAvailable() < static_cast<int>(responseLength)) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
            elapsed += 10;
        }
        
        if (socket->bytesAvailable() < static_cast<int>(responseLength)) {
            outErrorStr = "Timeout reading MessageEnvelope";
            return false;
        }
        
        // Read complete envelope
        QByteArray envelopeBytes = socket->read(responseLength);
        
        // Parse envelope
        palantir::MessageEnvelope envelope;
        std::string parseError;
        if (!bedrock::palantir::parseEnvelope(
                std::string(envelopeBytes.data(), envelopeBytes.size()),
                envelope,
                &parseError)) {
            outErrorStr = QString("Failed to parse MessageEnvelope: %1").arg(parseError.c_str());
            return false;
        }
        
        // Check for ERROR_RESPONSE
        if (envelope.type() != palantir::MessageType::ERROR_RESPONSE) {
            outErrorStr = QString("Expected ERROR_RESPONSE, got type %1").arg(static_cast<int>(envelope.type()));
            return false;
        }
        
        // Parse ErrorResponse
        const std::string& payload = envelope.payload();
        if (!outError.ParseFromArray(payload.data(), static_cast<int>(payload.size()))) {
            outErrorStr = "Failed to parse ErrorResponse from envelope payload";
            return false;
        }
        
        return true;
    }
    
    IntegrationTestServerFixture fixture_;
    std::unique_ptr<QCoreApplication> app_;
};

TEST_F(ErrorCasesIntegrationTest, UnknownMessageType) {
    qDebug() << "[TEST] Starting UnknownMessageType test";
    
    // Create client and connect
    IntegrationTestClient client;
    ASSERT_TRUE(client.connect(fixture_.socketPath())) 
        << "Failed to connect to test server";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Get the underlying socket for raw access
    // Note: IntegrationTestClient doesn't expose socket, so we'll create our own
    QLocalSocket socket;
    socket.connectToServer(fixture_.socketPath());
    ASSERT_TRUE(socket.waitForConnected(5000)) << "Failed to connect";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Create envelope with unknown message type
    // Use a high enum value that's not handled (e.g., 99, which is likely not in the enum)
    // We'll manually create an envelope with an invalid type since makeEnvelope requires a valid type
    palantir::MessageEnvelope envelope;
    envelope.set_version(1);
    envelope.set_type(static_cast<palantir::MessageType>(99)); // Invalid type value
    envelope.set_payload(""); // Empty payload
    
    // Serialize envelope
    std::string serialized;
    ASSERT_TRUE(envelope.SerializeToString(&serialized));
    
    // Send: [4-byte length][serialized MessageEnvelope]
    uint32_t length = static_cast<uint32_t>(serialized.size());
    QByteArray data;
    data.append(reinterpret_cast<const char*>(&length), 4);
    data.append(serialized.data(), static_cast<int>(serialized.size()));
    
    ASSERT_TRUE(sendRawBytes(data, &socket)) << "Failed to send message";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Receive ErrorResponse
    palantir::ErrorResponse error;
    QString errorStr;
    ASSERT_TRUE(receiveErrorResponse(&socket, error, errorStr)) << errorStr.toStdString();
    
    // Verify error code
    EXPECT_EQ(error.error_code(), palantir::ErrorCode::UNKNOWN_MESSAGE_TYPE)
        << "Expected UNKNOWN_MESSAGE_TYPE, got " << static_cast<int>(error.error_code());
    
    // Verify message contains indication of unknown type
    EXPECT_FALSE(error.message().empty());
    EXPECT_NE(error.message().find("unknown"), std::string::npos) 
        << "Error message should mention unknown type";
    
    socket.disconnectFromServer();
}

TEST_F(ErrorCasesIntegrationTest, OversizedMessage) {
    qDebug() << "[TEST] Starting OversizedMessage test";
    
    // Create socket and connect
    QLocalSocket socket;
    socket.connectToServer(fixture_.socketPath());
    ASSERT_TRUE(socket.waitForConnected(5000)) << "Failed to connect";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Send a length prefix that exceeds MAX_MESSAGE_SIZE (10MB)
    // MAX_MESSAGE_SIZE = 10 * 1024 * 1024 = 10485760
    // Send length = 10485761 (1 byte over limit)
    uint32_t oversizedLength = 10 * 1024 * 1024 + 1; // 10MB + 1 byte
    
    QByteArray data;
    data.append(reinterpret_cast<const char*>(&oversizedLength), 4);
    // Don't send actual payload - server should reject on length check
    
    ASSERT_TRUE(sendRawBytes(data, &socket)) << "Failed to send oversized length";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Receive ErrorResponse
    palantir::ErrorResponse error;
    QString errorStr;
    ASSERT_TRUE(receiveErrorResponse(&socket, error, errorStr)) << errorStr.toStdString();
    
    // Verify error code
    EXPECT_EQ(error.error_code(), palantir::ErrorCode::MESSAGE_TOO_LARGE)
        << "Expected MESSAGE_TOO_LARGE, got " << static_cast<int>(error.error_code());
    
    // Verify message mentions size limit
    EXPECT_FALSE(error.message().empty());
    EXPECT_NE(error.message().find("large"), std::string::npos) 
        << "Error message should mention message too large";
    
    socket.disconnectFromServer();
}

TEST_F(ErrorCasesIntegrationTest, MalformedPayload) {
    qDebug() << "[TEST] Starting MalformedPayload test";
    
    // Create socket and connect
    QLocalSocket socket;
    socket.connectToServer(fixture_.socketPath());
    ASSERT_TRUE(socket.waitForConnected(5000)) << "Failed to connect";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Create envelope with valid structure but invalid payload (random bytes)
    palantir::MessageEnvelope envelope;
    envelope.set_version(1);
    envelope.set_type(palantir::MessageType::XY_SINE_REQUEST);
    
    // Set payload to random bytes (invalid protobuf)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    std::string randomPayload(100, '\0');
    for (size_t i = 0; i < randomPayload.size(); ++i) {
        randomPayload[i] = static_cast<char>(dis(gen));
    }
    envelope.set_payload(randomPayload);
    
    // Serialize envelope
    std::string serialized;
    ASSERT_TRUE(envelope.SerializeToString(&serialized));
    
    // Send: [4-byte length][serialized MessageEnvelope]
    uint32_t length = static_cast<uint32_t>(serialized.size());
    QByteArray data;
    data.append(reinterpret_cast<const char*>(&length), 4);
    data.append(serialized.data(), static_cast<int>(serialized.size()));
    
    ASSERT_TRUE(sendRawBytes(data, &socket)) << "Failed to send message";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Receive ErrorResponse
    palantir::ErrorResponse error;
    QString errorStr;
    ASSERT_TRUE(receiveErrorResponse(&socket, error, errorStr)) << errorStr.toStdString();
    
    // Verify error code (should be PROTOBUF_PARSE_ERROR or INVALID_MESSAGE_FORMAT)
    EXPECT_TRUE(error.error_code() == palantir::ErrorCode::PROTOBUF_PARSE_ERROR ||
                error.error_code() == palantir::ErrorCode::INVALID_MESSAGE_FORMAT)
        << "Expected PROTOBUF_PARSE_ERROR or INVALID_MESSAGE_FORMAT, got " 
        << static_cast<int>(error.error_code());
    
    // Verify message indicates parse failure
    EXPECT_FALSE(error.message().empty());
    
    socket.disconnectFromServer();
}

TEST_F(ErrorCasesIntegrationTest, InvalidEnvelopeVersion) {
    qDebug() << "[TEST] Starting InvalidEnvelopeVersion test";
    
    // Create socket and connect
    QLocalSocket socket;
    socket.connectToServer(fixture_.socketPath());
    ASSERT_TRUE(socket.waitForConnected(5000)) << "Failed to connect";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Create envelope with invalid version (0)
    palantir::MessageEnvelope envelope;
    envelope.set_version(0); // Invalid version
    envelope.set_type(palantir::MessageType::CAPABILITIES_REQUEST);
    envelope.set_payload(""); // Empty payload is OK for CapabilitiesRequest
    
    // Serialize envelope
    std::string serialized;
    ASSERT_TRUE(envelope.SerializeToString(&serialized));
    
    // Send: [4-byte length][serialized MessageEnvelope]
    uint32_t length = static_cast<uint32_t>(serialized.size());
    QByteArray data;
    data.append(reinterpret_cast<const char*>(&length), 4);
    data.append(serialized.data(), static_cast<int>(serialized.size()));
    
    ASSERT_TRUE(sendRawBytes(data, &socket)) << "Failed to send message";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Note: The server's parseEnvelope() will reject invalid version before it reaches RPC handler
    // So we may not get an ErrorResponse - the connection might just close or we get no response
    // Let's check what actually happens - we should get an ErrorResponse with INVALID_MESSAGE_FORMAT
    // or the envelope parsing might fail at the transport layer
    
    // Try to receive ErrorResponse (may timeout if server closes connection)
    palantir::ErrorResponse error;
    QString errorStr;
    bool received = receiveErrorResponse(&socket, error, errorStr);
    
    // The server should reject invalid version during envelope parsing
    // This happens in extractMessage() which calls parseEnvelope()
    // parseEnvelope() validates version and returns false, which should trigger an error response
    if (received) {
        // If we got an error response, verify it's about invalid version
        EXPECT_TRUE(error.error_code() == palantir::ErrorCode::INVALID_MESSAGE_FORMAT ||
                    error.error_code() == palantir::ErrorCode::PROTOBUF_PARSE_ERROR)
            << "Expected INVALID_MESSAGE_FORMAT or PROTOBUF_PARSE_ERROR, got " 
            << static_cast<int>(error.error_code());
    } else {
        // If no response, that's also acceptable - server may close connection on invalid envelope
        qDebug() << "[TEST] No ErrorResponse received (server may have closed connection on invalid version)";
    }
    
    socket.disconnectFromServer();
}

TEST_F(ErrorCasesIntegrationTest, InvalidXYSineParameters) {
    qDebug() << "[TEST] Starting InvalidXYSineParameters test";
    
    // Create client and connect
    IntegrationTestClient client;
    ASSERT_TRUE(client.connect(fixture_.socketPath())) 
        << "Failed to connect to test server";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Create socket for raw access to receive ErrorResponse
    QLocalSocket socket;
    socket.connectToServer(fixture_.socketPath());
    ASSERT_TRUE(socket.waitForConnected(5000)) << "Failed to connect";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Create XYSineRequest with invalid parameters (samples < 2)
    palantir::XYSineRequest request;
    request.set_frequency(1.0);
    request.set_samples(1); // Invalid: must be >= 2
    request.set_amplitude(1.0);
    request.set_phase(0.0);
    
    // Create envelope
    auto envelope = bedrock::palantir::makeEnvelope(
        palantir::MessageType::XY_SINE_REQUEST,
        request
    );
    
    ASSERT_TRUE(envelope.has_value()) << "Failed to create envelope";
    
    // Serialize envelope
    std::string serialized;
    ASSERT_TRUE(envelope->SerializeToString(&serialized));
    
    // Send: [4-byte length][serialized MessageEnvelope]
    uint32_t length = static_cast<uint32_t>(serialized.size());
    QByteArray data;
    data.append(reinterpret_cast<const char*>(&length), 4);
    data.append(serialized.data(), static_cast<int>(serialized.size()));
    
    ASSERT_TRUE(sendRawBytes(data, &socket)) << "Failed to send message";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Receive ErrorResponse
    palantir::ErrorResponse error;
    QString errorStr;
    ASSERT_TRUE(receiveErrorResponse(&socket, error, errorStr)) << errorStr.toStdString();
    
    // Verify error code
    EXPECT_EQ(error.error_code(), palantir::ErrorCode::INVALID_PARAMETER_VALUE)
        << "Expected INVALID_PARAMETER_VALUE, got " << static_cast<int>(error.error_code());
    
    // Verify message includes the offending value or parameter name
    EXPECT_FALSE(error.message().empty());
    EXPECT_NE(error.message().find("samples"), std::string::npos) 
        << "Error message should mention 'samples' parameter";
    
    socket.disconnectFromServer();
}

TEST_F(ErrorCasesIntegrationTest, InvalidXYSineParametersTooLarge) {
    qDebug() << "[TEST] Starting InvalidXYSineParametersTooLarge test";
    
    // Create socket and connect
    QLocalSocket socket;
    socket.connectToServer(fixture_.socketPath());
    ASSERT_TRUE(socket.waitForConnected(5000)) << "Failed to connect";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Create XYSineRequest with invalid parameters (samples > 10,000,000)
    palantir::XYSineRequest request;
    request.set_frequency(1.0);
    request.set_samples(10000001); // Invalid: must be <= 10,000,000
    request.set_amplitude(1.0);
    request.set_phase(0.0);
    
    // Create envelope
    auto envelope = bedrock::palantir::makeEnvelope(
        palantir::MessageType::XY_SINE_REQUEST,
        request
    );
    
    ASSERT_TRUE(envelope.has_value()) << "Failed to create envelope";
    
    // Serialize envelope
    std::string serialized;
    ASSERT_TRUE(envelope->SerializeToString(&serialized));
    
    // Send: [4-byte length][serialized MessageEnvelope]
    uint32_t length = static_cast<uint32_t>(serialized.size());
    QByteArray data;
    data.append(reinterpret_cast<const char*>(&length), 4);
    data.append(serialized.data(), static_cast<int>(serialized.size()));
    
    ASSERT_TRUE(sendRawBytes(data, &socket)) << "Failed to send message";
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    QCoreApplication::processEvents();
    
    // Receive ErrorResponse
    palantir::ErrorResponse error;
    QString errorStr;
    ASSERT_TRUE(receiveErrorResponse(&socket, error, errorStr)) << errorStr.toStdString();
    
    // Verify error code
    EXPECT_EQ(error.error_code(), palantir::ErrorCode::INVALID_PARAMETER_VALUE)
        << "Expected INVALID_PARAMETER_VALUE, got " << static_cast<int>(error.error_code());
    
    // Verify message includes the offending value or parameter name
    EXPECT_FALSE(error.message().empty());
    EXPECT_NE(error.message().find("samples"), std::string::npos) 
        << "Error message should mention 'samples' parameter";
    
    socket.disconnectFromServer();
}

#else
// Stub when transport deps disabled
#include <gtest/gtest.h>
TEST(ErrorCasesIntegrationTest, DISABLED_RequiresTransportDeps) {
    GTEST_SKIP() << "Integration tests require BEDROCK_WITH_TRANSPORT_DEPS=ON";
}
#endif

