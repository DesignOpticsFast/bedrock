#pragma once

#include <QString>
#include <QByteArray>
#include <optional>

#ifdef BEDROCK_WITH_TRANSPORT_DEPS
#include "palantir/envelope.pb.h"
#include "palantir/capabilities.pb.h"
#include "palantir/xysine.pb.h"
#include "palantir/EnvelopeHelpers.hpp"
#include <QLocalSocket>
#include <memory>
#endif

/**
 * Minimal C++ client for integration testing.
 * 
 * Connects to PalantirServer and sends/receives envelope-encoded messages
 * using the live transport layer.
 */
class IntegrationTestClient {
public:
    IntegrationTestClient();
    ~IntegrationTestClient();

    /**
     * Connect to server.
     * @param socketPath Unix domain socket path
     * @return true on success, false on failure
     */
    bool connect(const QString& socketPath);

    /**
     * Disconnect from server.
     */
    void disconnect();

    /**
     * Check if connected.
     * @return true if connected
     */
    bool isConnected() const;

    /**
     * Send CapabilitiesRequest and receive CapabilitiesResponse.
     * @param outResponse Output response (populated on success)
     * @param outError Output error message (populated on failure)
     * @return true on success, false on failure
     */
    bool getCapabilities(palantir::CapabilitiesResponse& outResponse, QString& outError);
    
    /**
     * Send XYSineRequest and receive XYSineResponse.
     * @param request Input request
     * @param outResponse Output response (populated on success)
     * @param outError Output error message (populated on failure)
     * @return true on success, false on failure
     */
    bool sendXYSineRequest(const palantir::XYSineRequest& request, palantir::XYSineResponse& outResponse, QString& outError);

private:
#ifdef BEDROCK_WITH_TRANSPORT_DEPS
    std::unique_ptr<QLocalSocket> socket_;
    
    /**
     * Send envelope-encoded message.
     * @param type Message type
     * @param message Inner message to wrap
     * @param outError Output error message
     * @return true on success, false on failure
     */
    bool sendEnvelope(palantir::MessageType type, const google::protobuf::Message& message, QString& outError);
    
    /**
     * Receive and parse envelope-encoded message.
     * @param outEnvelope Output envelope (populated on success)
     * @param outError Output error message (populated on failure)
     * @return true on success, false on failure
     */
    bool receiveEnvelope(palantir::MessageEnvelope& outEnvelope, QString& outError);
#endif
};

