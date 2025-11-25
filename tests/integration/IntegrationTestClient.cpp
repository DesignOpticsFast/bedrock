#include "IntegrationTestClient.hpp"

#ifdef BEDROCK_WITH_TRANSPORT_DEPS
#include <QLocalSocket>
#include <QDebug>
#include <QCoreApplication>
#include <QTest>
#include <cstring>
#include "palantir/xysine.pb.h"

IntegrationTestClient::IntegrationTestClient()
    : socket_(std::make_unique<QLocalSocket>())
{
}

IntegrationTestClient::~IntegrationTestClient()
{
    disconnect();
}

bool IntegrationTestClient::connect(const QString& socketPath)
{
    if (!socket_) {
        socket_ = std::make_unique<QLocalSocket>();
    }
    
    if (socket_->state() == QLocalSocket::ConnectedState) {
        return true;
    }
    
    socket_->connectToServer(socketPath);
    
    if (!socket_->waitForConnected(5000)) {
        return false;
    }
    
    return true;
}

void IntegrationTestClient::disconnect()
{
    if (socket_) {
        socket_->disconnectFromServer();
        if (socket_->state() != QLocalSocket::UnconnectedState) {
            socket_->waitForDisconnected(1000);
        }
    }
}

bool IntegrationTestClient::isConnected() const
{
    return socket_ && socket_->state() == QLocalSocket::ConnectedState;
}

bool IntegrationTestClient::sendEnvelope(palantir::MessageType type, const google::protobuf::Message& message, QString& outError)
{
    qDebug() << "[CLIENT] sendEnvelope: type=" << static_cast<int>(type) << ", connected=" << isConnected();
    
    if (!isConnected()) {
        outError = "Not connected";
        qDebug() << "[CLIENT] sendEnvelope: ERROR - not connected";
        return false;
    }
    
    // Create envelope using the live transport helper
    std::string envelopeError;
    auto envelope = bedrock::palantir::makeEnvelope(type, message, {}, &envelopeError);
    
    if (!envelope.has_value()) {
        outError = QString("Failed to create envelope: %1").arg(envelopeError.c_str());
        qDebug() << "[CLIENT] sendEnvelope: ERROR - failed to create envelope:" << envelopeError.c_str();
        return false;
    }
    
    qDebug() << "[CLIENT] sendEnvelope: envelope created, version=" << envelope->version() << ", type=" << static_cast<int>(envelope->type());
    
    // Validate envelope payload size (even empty messages should have some envelope overhead)
    qDebug() << "[CLIENT] sendEnvelope: envelope payload size=" << envelope->payload().size();
    if (envelope->payload().size() == 0) {
        qDebug() << "[CLIENT] sendEnvelope: WARNING - payload is empty (this is OK for empty messages like CapabilitiesRequest)";
    }
    
    // Serialize envelope
    std::string serialized;
    if (!envelope->SerializeToString(&serialized)) {
        outError = "Failed to serialize MessageEnvelope";
        qDebug() << "[CLIENT] sendEnvelope: ERROR - failed to serialize";
        return false;
    }
    
    qDebug() << "[CLIENT] sendEnvelope: serialized envelope size=" << serialized.size();
    
    // Send: [4-byte length][serialized MessageEnvelope]
    uint32_t length = static_cast<uint32_t>(serialized.size());
    QByteArray data;
    data.append(reinterpret_cast<const char*>(&length), 4);
    data.append(serialized.data(), static_cast<int>(serialized.size()));
    
    qDebug() << "[CLIENT] sendEnvelope: writing" << data.size() << "bytes to socket";
    qint64 written = socket_->write(data);
    qDebug() << "[CLIENT] sendEnvelope: wrote" << written << "bytes";
    
    if (written != data.size()) {
        outError = QString("Failed to send complete message (wrote %1 of %2 bytes)").arg(written).arg(data.size());
        qDebug() << "[CLIENT] sendEnvelope: ERROR -" << outError;
        return false;
    }
    
    // Flush to ensure data is sent (write() already accepted all bytes)
    socket_->flush();
    qDebug() << "[CLIENT] sendEnvelope: flushed, message queued for sending";
    
    qDebug() << "[CLIENT] sendEnvelope: SUCCESS - message sent";
    return true;
}

bool IntegrationTestClient::receiveEnvelope(palantir::MessageEnvelope& outEnvelope, QString& outError)
{
    qDebug() << "[CLIENT] receiveEnvelope: starting, connected=" << isConnected();
    
    if (!isConnected()) {
        outError = "Not connected";
        qDebug() << "[CLIENT] receiveEnvelope: ERROR - not connected";
        return false;
    }
    
    // Wait for data using Qt test pattern (spins event loop properly)
    qDebug() << "[CLIENT] receiveEnvelope: waiting for data using QTest::qWait...";
    int elapsed = 0;
    const int timeoutMs = 5000;
    while (elapsed < timeoutMs && socket_->bytesAvailable() < 4) {
        QTest::qWait(10);  // Spins Qt event loop for 10ms
        elapsed += 10;
    }
    
    if (socket_->bytesAvailable() < 4) {
        outError = QString("Timeout waiting for response (elapsed: %1ms)").arg(elapsed);
        qDebug() << "[CLIENT] receiveEnvelope: ERROR -" << outError;
        return false;
    }
    
    qDebug() << "[CLIENT] receiveEnvelope: data available, bytes=" << socket_->bytesAvailable();
    
    // Read length prefix (4 bytes)
    QByteArray lengthBytes = socket_->read(4);
    qDebug() << "[CLIENT] receiveEnvelope: read length prefix, size=" << lengthBytes.size();
    
    if (lengthBytes.size() != 4) {
        outError = QString("Failed to read length prefix (got %1 bytes)").arg(lengthBytes.size());
        qDebug() << "[CLIENT] receiveEnvelope: ERROR -" << outError;
        return false;
    }
    
    uint32_t responseLength;
    std::memcpy(&responseLength, lengthBytes.data(), 4);
    qDebug() << "[CLIENT] receiveEnvelope: response length=" << responseLength;
    
    // Wait for complete envelope using Qt test pattern
    elapsed = 0;
    while (elapsed < timeoutMs && socket_->bytesAvailable() < static_cast<int>(responseLength)) {
        QTest::qWait(10);  // Spins Qt event loop for 10ms
        elapsed += 10;
    }
    
    if (socket_->bytesAvailable() < static_cast<int>(responseLength)) {
        outError = QString("Timeout reading MessageEnvelope (got %1 of %2 bytes, elapsed: %3ms)")
                   .arg(static_cast<int>(socket_->bytesAvailable())).arg(static_cast<int>(responseLength)).arg(elapsed);
        qDebug() << "[CLIENT] receiveEnvelope: ERROR -" << outError;
        return false;
    }
    
    // Read complete envelope
    QByteArray envelopeBytes = socket_->read(responseLength);
    qDebug() << "[CLIENT] receiveEnvelope: read complete envelope, size=" << envelopeBytes.size();
    
    if (envelopeBytes.size() != static_cast<int>(responseLength)) {
        outError = QString("Failed to read complete MessageEnvelope (got %1 of %2 bytes)")
                   .arg(envelopeBytes.size()).arg(static_cast<int>(responseLength));
        qDebug() << "[CLIENT] receiveEnvelope: ERROR -" << outError;
        return false;
    }
    
    // Parse envelope using the live transport helper
    std::string parseError;
    if (!bedrock::palantir::parseEnvelope(
            std::string(envelopeBytes.data(), envelopeBytes.size()),
            outEnvelope,
            &parseError)) {
        outError = QString("Failed to parse MessageEnvelope: %1").arg(parseError.c_str());
        qDebug() << "[CLIENT] receiveEnvelope: ERROR -" << outError;
        return false;
    }
    
    qDebug() << "[CLIENT] receiveEnvelope: parsed envelope, version=" << outEnvelope.version() << ", type=" << static_cast<int>(outEnvelope.type());
    
    // Validate envelope version
    if (outEnvelope.version() != 1) {
        outError = QString("Invalid envelope version: %1").arg(outEnvelope.version());
        qDebug() << "[CLIENT] receiveEnvelope: ERROR -" << outError;
        return false;
    }
    
    qDebug() << "[CLIENT] receiveEnvelope: SUCCESS";
    return true;
}

bool IntegrationTestClient::getCapabilities(palantir::CapabilitiesResponse& outResponse, QString& outError)
{
    // Send CapabilitiesRequest
    // Note: CapabilitiesRequest is an empty message (no fields), but protobuf will still
    // serialize it as an empty payload. The envelope itself should still serialize correctly
    // with version and type fields.
    palantir::CapabilitiesRequest request;
    
    // Verify the request can be serialized (even if empty)
    std::string requestSerialized;
    if (!request.SerializeToString(&requestSerialized)) {
        outError = "Failed to serialize CapabilitiesRequest";
        qDebug() << "[CLIENT] getCapabilities: ERROR - failed to serialize request";
        return false;
    }
    qDebug() << "[CLIENT] getCapabilities: CapabilitiesRequest serialized size=" << requestSerialized.size();
    
    if (!sendEnvelope(palantir::MessageType::CAPABILITIES_REQUEST, request, outError)) {
        return false;
    }
    
    // Receive envelope
    palantir::MessageEnvelope envelope;
    if (!receiveEnvelope(envelope, outError)) {
        return false;
    }
    
    // Validate response type
    if (envelope.type() != palantir::MessageType::CAPABILITIES_RESPONSE) {
        outError = QString("Unexpected message type: %1").arg(static_cast<int>(envelope.type()));
        return false;
    }
    
    // Parse inner CapabilitiesResponse
    const std::string& payload = envelope.payload();
    if (!outResponse.ParseFromArray(payload.data(), static_cast<int>(payload.size()))) {
        outError = "Failed to parse CapabilitiesResponse from envelope payload";
        return false;
    }
    
    return true;
}

bool IntegrationTestClient::sendXYSineRequest(const palantir::XYSineRequest& request, palantir::XYSineResponse& outResponse, QString& outError)
{
    // Send XYSineRequest
    if (!sendEnvelope(palantir::MessageType::XY_SINE_REQUEST, request, outError)) {
        return false;
    }
    
    // Receive envelope
    palantir::MessageEnvelope envelope;
    if (!receiveEnvelope(envelope, outError)) {
        return false;
    }
    
    // Validate response type
    if (envelope.type() != palantir::MessageType::XY_SINE_RESPONSE) {
        outError = QString("Unexpected message type: %1").arg(static_cast<int>(envelope.type()));
        return false;
    }
    
    // Parse inner XYSineResponse
    const std::string& payload = envelope.payload();
    if (!outResponse.ParseFromArray(payload.data(), static_cast<int>(payload.size()))) {
        outError = "Failed to parse XYSineResponse from envelope payload";
        return false;
    }
    
    return true;
}

#else
// Stub implementation when transport deps disabled
IntegrationTestClient::IntegrationTestClient() {}
IntegrationTestClient::~IntegrationTestClient() {}
bool IntegrationTestClient::connect(const QString&) { return false; }
void IntegrationTestClient::disconnect() {}
bool IntegrationTestClient::isConnected() const { return false; }
bool IntegrationTestClient::getCapabilities(palantir::CapabilitiesResponse&, QString&) { return false; }
bool IntegrationTestClient::sendXYSineRequest(const palantir::XYSineRequest&, palantir::XYSineResponse&, QString&) { return false; }
#endif

