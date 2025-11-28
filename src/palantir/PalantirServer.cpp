#include "PalantirServer.hpp"

#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QDebug>
#include <QCoreApplication>
#include <QThread>
#include <QDateTime>
#include <cmath>
#include <algorithm>

#ifdef BEDROCK_WITH_TRANSPORT_DEPS
#include "palantir/xysine.pb.h"
#include "palantir/envelope.pb.h"
#include "palantir/error.pb.h"
#include "EnvelopeHelpers.hpp"
#endif

PalantirServer::PalantirServer(QObject *parent)
    : QObject(parent)
    , server_(std::make_unique<QLocalServer>(this))
    , running_(false)
    , maxConcurrency_(QThread::idealThreadCount())
    , supportedFeatures_({"xy_sine", "heat_diffusion"})
    , protocolVersion_("1.0")
{
    // Connect server signals
    connect(server_.get(), &QLocalServer::newConnection, this, &PalantirServer::onNewConnection);
    
    // Setup heartbeat timer
    heartbeatTimer_.setInterval(2000); // 2 seconds
    connect(&heartbeatTimer_, &QTimer::timeout, this, &PalantirServer::onHeartbeatTimer);
}

PalantirServer::~PalantirServer()
{
    stopServer();
}

bool PalantirServer::startServer(const QString& socketName)
{
    if (running_) {
        return true;
    }
    
    // Remove existing socket file
    QLocalServer::removeServer(socketName);
    
    // Start server
    if (!server_->listen(socketName)) {
        emit errorOccurred("Failed to start server: " + server_->errorString());
        return false;
    }
    
    running_ = true;
    heartbeatTimer_.start();
    
    qDebug() << "Palantir server started on socket:" << socketName;
    return true;
}

void PalantirServer::stopServer()
{
    if (!running_) {
        return;
    }
    
    // Cancel all active jobs
    {
        std::lock_guard<std::mutex> lock(jobMutex_);
        for (auto& [jobId, cancelled] : jobCancelled_) {
            cancelled = true;
        }
    }
    
    // Wait for job threads to finish (thread-safe)
    {
        std::lock_guard<std::mutex> lock(jobMutex_);
        for (auto& [jobId, thread] : jobThreads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        jobThreads_.clear();
        jobClients_.clear();
        jobCancelled_.clear();
    }
    
    // Clear client buffers (thread-safe)
    {
        std::lock_guard<std::mutex> lock(clientBuffersMutex_);
        clientBuffers_.clear();
    }
    
    // Stop server
    server_->close();
    running_ = false;
    heartbeatTimer_.stop();
    
    qDebug() << "Palantir server stopped";
}

bool PalantirServer::isRunning() const
{
    return running_;
}

int PalantirServer::maxConcurrency() const
{
    return maxConcurrency_;
}

QStringList PalantirServer::supportedFeatures() const
{
    return supportedFeatures_;
}

void PalantirServer::onNewConnection()
{
    QLocalSocket* client = server_->nextPendingConnection();
    if (!client) {
        qDebug() << "[SERVER] onNewConnection: ERROR - no pending connection";
        return;
    }
    
    qDebug() << "[SERVER] onNewConnection: new client" << client << ", state=" << client->state();
    
    // Connect client signals
    connect(client, &QLocalSocket::disconnected, this, &PalantirServer::onClientDisconnected);
    connect(client, &QLocalSocket::readyRead, this, &PalantirServer::onClientReadyRead);
    
    // Initialize client buffer (thread-safe)
    {
        std::lock_guard<std::mutex> lock(clientBuffersMutex_);
        clientBuffers_[client] = QByteArray();
        qDebug() << "[SERVER] onNewConnection: client added to buffers map, map size=" << clientBuffers_.size();
    }
    
    emit clientConnected();
    qDebug() << "[SERVER] onNewConnection: Client connected signal emitted";
}

void PalantirServer::onClientDisconnected()
{
    QLocalSocket* client = qobject_cast<QLocalSocket*>(sender());
    if (!client) {
        return;
    }
    
    // Remove client from tracking (thread-safe)
    {
        std::lock_guard<std::mutex> lock(clientBuffersMutex_);
        clientBuffers_.erase(client);
    }
    
    // Cancel jobs for this client (thread-safe)
    {
        std::lock_guard<std::mutex> lock(jobMutex_);
        for (auto it = jobClients_.begin(); it != jobClients_.end();) {
            if (it->second == client) {
                QString jobId = it->first;
                jobCancelled_[jobId] = true;
                jobClients_.erase(it++);
            } else {
                ++it;
            }
        }
    }
    
    // Qt will handle socket deletion via parent-child relationship
    // No need to call deleteLater() explicitly - client is child of server
    emit clientDisconnected();
    qDebug() << "Client disconnected";
}

void PalantirServer::onClientReadyRead()
{
    QLocalSocket* client = qobject_cast<QLocalSocket*>(sender());
    if (!client) {
        qDebug() << "[SERVER] onClientReadyRead: ERROR - no client";
        return;
    }
    
    qDebug() << "[SERVER] onClientReadyRead: bytes available=" << client->bytesAvailable();
    parseIncomingData(client);
}

void PalantirServer::onHeartbeatTimer()
{
    // Heartbeat/Pong not yet implemented (requires Pong proto message)
    // Future: Send pong to all connected clients when Pong message is defined
    // For now, heartbeat timer is disabled or no-op
}

// Message handling uses envelope-based protocol only
// All messages are wrapped in MessageEnvelope and handled via parseIncomingData() -> extractMessage()

// StartJob handler disabled (proto message not yet defined)
// Future: Re-enable when StartJob proto is added
/*
void PalantirServer::handleStartJob(QLocalSocket* client, const palantir::StartJob& startJob)
{
    QString jobId = QString::fromStdString(startJob.job_id().id());
    palantir::ComputeSpec spec = startJob.spec();
    
    // Check if we can handle this job
    QString featureId = QString::fromStdString(spec.feature_id());
    if (!supportedFeatures_.contains(featureId)) {
        palantir::StartReply reply;
        reply.mutable_job_id()->set_id(jobId.toStdString());
        reply.set_status("UNIMPLEMENTED");
        reply.set_error_message("Feature not supported: " + featureId.toStdString());
        sendMessage(client, reply);
        return;
    }
    
    // Check concurrency limit
    if (activeJobs_.size() >= maxConcurrency_) {
        palantir::StartReply reply;
        reply.mutable_job_id()->set_id(jobId.toStdString());
        reply.set_status("RESOURCE_EXHAUSTED");
        reply.set_error_message("Server at capacity");
        sendMessage(client, reply);
        return;
    }
    
    // Start job
    activeJobs_[jobId] = spec;
    jobClients_[jobId] = client;
    jobCancelled_[jobId] = false;
    
    // Send start reply
    palantir::StartReply reply;
    reply.mutable_job_id()->set_id(jobId.toStdString());
    reply.set_status("OK");
    sendMessage(client, reply);
    
    // Start job processing in separate thread
    std::thread jobThread([this, jobId, spec]() {
        processJob(jobId, spec);
    });
    
    jobThreads_[jobId] = std::move(jobThread);
    
    // Job start logic commented out (requires StartJob proto message)
}
*/

// handleCapabilitiesRequest: RPC handler for Capabilities query
// Validation: CapabilitiesRequest has no fields (empty message), so no parameter validation needed
// If protobuf parsing fails, error is returned in parseIncomingData() before this handler is called
void PalantirServer::handleCapabilitiesRequest(QLocalSocket* client)
{
#ifdef BEDROCK_WITH_TRANSPORT_DEPS
    qDebug() << "[SERVER] handleCapabilitiesRequest: starting";
    bedrock::palantir::CapabilitiesService service;
    palantir::CapabilitiesResponse response = service.getCapabilities();
    qDebug() << "[SERVER] handleCapabilitiesRequest: got capabilities, server_version=" << response.capabilities().server_version().c_str();
    qDebug() << "[SERVER] handleCapabilitiesRequest: calling sendMessage...";
    sendMessage(client, palantir::MessageType::CAPABILITIES_RESPONSE, response);
    qDebug() << "[SERVER] handleCapabilitiesRequest: sendMessage returned";
#else
    qWarning() << "Capabilities requested but transport deps disabled";
#endif
}

// handleXYSineRequest: RPC handler for XY Sine computation
// Validation rules (enforced at RPC boundary before compute logic):
//   - samples: Must be >= 2 and <= 10,000,000 (DoS prevention)
//   - frequency: Must be finite (no NaN/Inf), default 1.0 if 0.0
//   - amplitude: Must be finite (no NaN/Inf), default 1.0 if 0.0
//   - phase: Must be finite (no NaN/Inf), 0.0 is valid default
// Error codes:
//   - INVALID_PARAMETER_VALUE: Semantically invalid parameters (out of range, non-finite)
//   - PROTOBUF_PARSE_ERROR: Returned in parseIncomingData() if protobuf parsing fails
void PalantirServer::handleXYSineRequest(QLocalSocket* client, const palantir::XYSineRequest& request)
{
#ifdef BEDROCK_WITH_TRANSPORT_DEPS
    // Validate request parameters at RPC boundary
    // Note: proto3 provides default values (0.0 for double, 0 for int32)
    // We apply defaults and validate before calling compute logic
    
    // Validation: samples range (DoS prevention)
    int samples = request.samples() != 0 ? request.samples() : 1000;
    if (samples < 2) {
        sendErrorResponse(client, palantir::ErrorCode::INVALID_PARAMETER_VALUE,
                         QString("Samples must be between 2 and 10,000,000 (got %1)").arg(samples),
                         QString("Received samples=%1").arg(samples));
        return;
    }
    if (samples > 10000000) {  // 10M samples max (reasonable limit)
        sendErrorResponse(client, palantir::ErrorCode::INVALID_PARAMETER_VALUE,
                         QString("Samples must be between 2 and 10,000,000 (got %1)").arg(samples),
                         QString("Received samples=%1").arg(samples));
        return;
    }
    
    // Validation: frequency, amplitude, phase must be finite (not NaN or Inf)
    double frequency = request.frequency() != 0.0 ? request.frequency() : 1.0;
    double amplitude = request.amplitude() != 0.0 ? request.amplitude() : 1.0;
    double phase = request.phase();  // 0.0 is valid default
    if (!std::isfinite(frequency) || !std::isfinite(amplitude) || !std::isfinite(phase)) {
        sendErrorResponse(client, palantir::ErrorCode::INVALID_PARAMETER_VALUE,
                         "Frequency, amplitude, and phase must be finite numbers",
                         QString("frequency=%1, amplitude=%2, phase=%3")
                         .arg(frequency).arg(amplitude).arg(phase));
        return;
    }
    
    // Compute XY Sine
    std::vector<double> xValues, yValues;
    computeXYSine(request, xValues, yValues);
    
    // Build response
    palantir::XYSineResponse response;
    response.mutable_x()->Reserve(static_cast<int>(xValues.size()));
    response.mutable_y()->Reserve(static_cast<int>(yValues.size()));
    for (double x : xValues) {
        response.add_x(x);
    }
    for (double y : yValues) {
        response.add_y(y);
    }
    response.set_status("OK");
    
    // Send response
    sendMessage(client, palantir::MessageType::XY_SINE_RESPONSE, response);
#else
    qWarning() << "XY Sine requested but transport deps disabled";
#endif
}

void PalantirServer::computeXYSine(const palantir::XYSineRequest& request, std::vector<double>& xValues, std::vector<double>& yValues)
{
    // Parse parameters from request (proto3 provides default values: 0.0 for double, 0 for int32)
    // Use explicit defaults to match Phoenix behavior
    double frequency = request.frequency() != 0.0 ? request.frequency() : 1.0;
    double amplitude = request.amplitude() != 0.0 ? request.amplitude() : 1.0;
    double phase = request.phase();  // 0.0 is valid default
    int samples = request.samples() != 0 ? request.samples() : 1000;
    
    // Validate samples (minimum 2) - matches Phoenix behavior
    if (samples < 2) {
        samples = 2;
    }
    
    // Compute sine wave using EXACT Phoenix algorithm
    // t = i / (samples - 1) from 0 to 1
    // x = t * 2π (0..2π domain)
    // y = amplitude * sin(2π * frequency * t + phase)
    xValues.clear();
    yValues.clear();
    xValues.reserve(samples);
    yValues.reserve(samples);
    
    for (int i = 0; i < samples; ++i) {
        double t = static_cast<double>(i) / (samples - 1.0);  // 0 to 1
        double x = t * 2.0 * M_PI;  // Scale to 0..2π domain
        double y = amplitude * std::sin(2.0 * M_PI * frequency * t + phase);
        
        xValues.push_back(x);
        yValues.push_back(y);
    }
}

// Ping/Pong handler disabled (proto message not yet defined)
// Future: Re-enable when Pong proto is added
/*
void PalantirServer::handlePing(QLocalSocket* client)
{
    // Respond with pong
    palantir::Pong pong;
    pong.set_timestamp_ms(QDateTime::currentMSecsSinceEpoch());
    sendMessage(client, pong);
}
*/

// WP1: processJob disabled (proto message not yet defined)
/*
void PalantirServer::processJob(const QString& jobId, const palantir::ComputeSpec& spec)
{
    QString featureId = QString::fromStdString(spec.feature_id());
    
    if (featureId == "xy_sine") {
        // Compute XY Sine
        std::vector<double> xValues, yValues;
        computeXYSine(spec, xValues, yValues);
        
        // Check if cancelled
        if (jobCancelled_[jobId]) {
            sendProgress(jobId, 100.0, "CANCELLED");
            return;
        }
        
        // Send result
        palantir::ResultMeta meta;
        meta.mutable_job_id()->set_id(jobId.toStdString());
        meta.set_status("SUCCEEDED");
        meta.set_dtype("f64");
        meta.add_shape(xValues.size());
        meta.set_compute_elapsed_ms(0); // Instantaneous
        meta.set_bytes_total(xValues.size() * sizeof(double) * 2);
        
        sendResult(jobId, meta);
        
        // Send data chunks
        QByteArray data;
        data.append(reinterpret_cast<const char*>(xValues.data()), xValues.size() * sizeof(double));
        data.append(reinterpret_cast<const char*>(yValues.data()), yValues.size() * sizeof(double));
        
        int chunkSize = 64 * 1024; // 64KB chunks
        int totalChunks = (data.size() + chunkSize - 1) / chunkSize;
        
        for (int i = 0; i < totalChunks; ++i) {
            if (jobCancelled_[jobId]) {
                break;
            }
            
            int start = i * chunkSize;
            int end = std::min(start + chunkSize, data.size());
            QByteArray chunk = data.mid(start, end - start);
            
            sendDataChunk(jobId, chunk, i, totalChunks);
        }
    }
    
    // Cleanup
    {
        std::lock_guard<std::mutex> lock(jobMutex_);
        activeJobs_.erase(jobId);
        jobClients_.erase(jobId);
        jobCancelled_.erase(jobId);
        
        if (jobThreads_.contains(jobId)) {
            if (jobThreads_[jobId].joinable()) {
                jobThreads_[jobId].join();
            }
            jobThreads_.erase(jobId);
        }
    }
}
*/

// WP1: sendProgress disabled (proto message not yet defined)
/*
void PalantirServer::sendProgress(const QString& jobId, double progress, const QString& status)
{
    if (!jobClients_.contains(jobId)) {
        return;
    }
    
    QLocalSocket* client = jobClients_[jobId];
    
    palantir::Progress progressMsg;
    progressMsg.mutable_job_id()->set_id(jobId.toStdString());
    progressMsg.set_progress_pct(progress);
    progressMsg.set_status(status.toStdString());
    
    sendMessage(client, progressMsg);
}
*/

// WP1: sendResult disabled (proto message not yet defined)
/*
void PalantirServer::sendResult(const QString& jobId, const palantir::ResultMeta& meta)
{
    if (!jobClients_.contains(jobId)) {
        return;
    }
    
    QLocalSocket* client = jobClients_[jobId];
    sendMessage(client, meta);
}
*/

// WP1: sendDataChunk disabled (proto message not yet defined)
/*
void PalantirServer::sendDataChunk(const QString& jobId, const QByteArray& data, int chunkIndex, int totalChunks)
{
    if (!jobClients_.contains(jobId)) {
        return;
    }
    
    QLocalSocket* client = jobClients_[jobId];
    
    palantir::DataChunk chunk;
    chunk.mutable_job_id()->set_id(jobId.toStdString());
    chunk.set_chunk_index(chunkIndex);
    chunk.set_total_chunks(totalChunks);
    chunk.set_data(data.data(), data.size());
    
    sendMessage(client, chunk);
}
*/

// WP1: computeXYSine disabled (proto message not yet defined)
/*
void PalantirServer::computeXYSine(const palantir::ComputeSpec& spec, std::vector<double>& xValues, std::vector<double>& yValues)
{
    // Parse parameters
    double amplitude = 1.0;
    double frequency = 1.0;
    double phase = 0.0;
    int cycles = 1;
    int nSamples = 500;
    
    for (const auto& [key, value] : spec.params()) {
        QString paramKey = QString::fromStdString(key);
        QString paramValue = QString::fromStdString(value);
        
        if (paramKey == "amplitude") {
            amplitude = paramValue.toDouble();
        } else if (paramKey == "frequency") {
            frequency = paramValue.toDouble();
        } else if (paramKey == "phase") {
            phase = paramValue.toDouble();
        } else if (paramKey == "cycles") {
            cycles = paramValue.toInt();
        } else if (paramKey == "n_samples") {
            nSamples = paramValue.toInt();
        }
    }
    
    // Compute sine wave
    xValues.clear();
    yValues.clear();
    
    double period = 2.0 * M_PI / frequency;
    double totalLength = cycles * period;
    double step = totalLength / nSamples;
    
    for (int i = 0; i < nSamples; ++i) {
        double x = i * step;
        double y = amplitude * std::sin(frequency * x + phase);
        
        xValues.push_back(x);
        yValues.push_back(y);
    }
}
*/

#ifdef BEDROCK_WITH_TRANSPORT_DEPS
void PalantirServer::sendMessage(QLocalSocket* client, palantir::MessageType type, const google::protobuf::Message& message)
{
    if (!client) {
        return;
    }
    
    // Threading: This function runs on Qt event loop thread
    // QLocalSocket operations are safe because we're on the socket's owner thread
    // Note: QLocalSocket::state() is thread-safe for reading
    if (client->state() != QLocalSocket::ConnectedState) {
        qDebug() << "Attempted to send message to disconnected client";
        return;
    }
    
    // Note: Removed clientBuffers_ map check to avoid deadlock.
    // We rely on socket state check above. If client is connected,
    // it's safe to write to it.
    
    qDebug() << "[SERVER] sendMessage: type=" << static_cast<int>(type) << ", client=" << client;
    
    // Create envelope from message
    std::string envelopeError;
    auto envelope = bedrock::palantir::makeEnvelope(type, message, {}, &envelopeError);
    
    if (!envelope.has_value()) {
        qDebug() << "[SERVER] sendMessage: ERROR - failed to create envelope:" << envelopeError.c_str();
        sendErrorResponse(client, palantir::ErrorCode::INTERNAL_ERROR, 
                         QString("Failed to create envelope: %1").arg(envelopeError.c_str()));
        return;
    }
    
    qDebug() << "[SERVER] sendMessage: envelope created, version=" << envelope->version() << ", type=" << static_cast<int>(envelope->type());
    
    // Serialize envelope
    std::string serialized;
    if (!envelope->SerializeToString(&serialized)) {
        qDebug() << "[SERVER] sendMessage: ERROR - failed to serialize envelope";
        sendErrorResponse(client, palantir::ErrorCode::INTERNAL_ERROR, 
                         "Failed to serialize MessageEnvelope");
        return;
    }
    
    qDebug() << "[SERVER] sendMessage: serialized size=" << serialized.size();
    
    // Check size limit
    if (serialized.size() > MAX_MESSAGE_SIZE) {
        qDebug() << "[SERVER] sendMessage: ERROR - envelope too large:" << serialized.size();
        sendErrorResponse(client, palantir::ErrorCode::MESSAGE_TOO_LARGE,
                         QString("Envelope size %1 exceeds limit %2")
                         .arg(serialized.size()).arg(MAX_MESSAGE_SIZE));
        return;
    }
    
    // Create length-prefixed message: [4-byte length][serialized MessageEnvelope]
    QByteArray data;
    uint32_t totalLength = static_cast<uint32_t>(serialized.size());
    
    // Write length (little-endian, 4 bytes)
    data.append(reinterpret_cast<const char*>(&totalLength), 4);
    // Write serialized envelope
    data.append(serialized.data(), serialized.size());
    
    qDebug() << "[SERVER] sendMessage: writing" << data.size() << "bytes to client";
    // Send data
    qint64 written = client->write(data);
    qDebug() << "[SERVER] sendMessage: wrote" << written << "bytes";
    
    if (written != data.size()) {
        qDebug() << "[SERVER] sendMessage: ERROR - failed to send complete message (wrote" << written << "of" << data.size() << "bytes)";
    } else {
        qDebug() << "[SERVER] sendMessage: SUCCESS - message sent";
    }
}

// sendErrorResponse: Centralized error response helper
// Error codes used in Sprint 4.5:
//   - INTERNAL_ERROR: Server-side failures (envelope creation, serialization)
//   - MESSAGE_TOO_LARGE: Envelope size exceeds MAX_MESSAGE_SIZE (10MB)
//   - INVALID_MESSAGE_FORMAT: Malformed envelope or extraction errors
//   - PROTOBUF_PARSE_ERROR: Failed to parse request protobuf
//   - UNKNOWN_MESSAGE_TYPE: Message type not recognized
//   - INVALID_PARAMETER_VALUE: Request parameter validation failed (e.g., invalid samples)
void PalantirServer::sendErrorResponse(QLocalSocket* client, palantir::ErrorCode errorCode, 
                                       const QString& message, const QString& details)
{
    palantir::ErrorResponse error;
    error.set_error_code(errorCode);
    error.set_message(message.toStdString());
    if (!details.isEmpty()) {
        error.set_details(details.toStdString());
    }
    sendMessage(client, palantir::MessageType::ERROR_RESPONSE, error);
}

bool PalantirServer::extractMessage(QByteArray& buffer, palantir::MessageType& outType, QByteArray& outPayload, QString* outError)
{
    // Need at least 4 bytes for length prefix
    if (buffer.size() < 4) {
        return false; // Incomplete frame, need more data
    }
    
    // Read length (little-endian)
    uint32_t envelopeLength;
    std::memcpy(&envelopeLength, buffer.data(), 4);
    
    // Check size limit before reading payload (fail fast, prevent DoS)
    // MAX_MESSAGE_SIZE = 10MB - rejects oversize messages immediately
    if (envelopeLength > MAX_MESSAGE_SIZE) {
        if (outError) {
            *outError = QString("Envelope length %1 exceeds limit %2")
                       .arg(envelopeLength).arg(MAX_MESSAGE_SIZE);
        }
        buffer.clear(); // Clear buffer to prevent further parsing
        return false; // Hard error
    }
    
    // Check if we have the complete envelope
    if (buffer.size() < 4 + envelopeLength) {
        return false; // Incomplete frame, need more data
    }
    
    // Extract envelope bytes (after length prefix)
    QByteArray envelopeBytes = buffer.mid(4, envelopeLength);
    buffer.remove(0, 4 + envelopeLength);
    
    // Parse envelope
    ::palantir::MessageEnvelope envelope;
    std::string parseError;
    if (!bedrock::palantir::parseEnvelope(
            std::string(envelopeBytes.data(), envelopeBytes.size()),
            envelope,
            &parseError)) {
        if (outError) {
            *outError = QString("Malformed envelope: %1").arg(parseError.c_str());
        }
        return false; // Hard error
    }
    
    // Validate envelope version
    if (envelope.version() != 1) {
        if (outError) {
            *outError = QString("Invalid envelope version: %1").arg(envelope.version());
        }
        return false; // Hard error
    }
    
    // Extract type and payload
    outType = envelope.type();
    outPayload = QByteArray(envelope.payload().data(), envelope.payload().size());
    
    return true; // Success
}

// extractMessage() implements envelope-based protocol only:
// Wire format: [4-byte length][serialized MessageEnvelope]
// No legacy [length][type][payload] format support
#endif // BEDROCK_WITH_TRANSPORT_DEPS

void PalantirServer::parseIncomingData(QLocalSocket* client)
{
    if (!client) {
        qDebug() << "[SERVER] parseIncomingData: ERROR - no client";
        return;
    }
    
    // Read all available data first
    QByteArray newData = client->readAll();
    qDebug() << "[SERVER] parseIncomingData: read" << newData.size() << "bytes from client";
    
    if (newData.isEmpty()) {
        qDebug() << "[SERVER] parseIncomingData: no data available";
        return;
    }
    
#ifdef BEDROCK_WITH_TRANSPORT_DEPS
    // Parse envelope-based messages
    // Threading: This function runs on Qt event loop thread
    // Lock scope is narrowed to buffer manipulation only; dispatch happens outside lock
    // This prevents holding mutex during message dispatch (which may call sendMessage)
    while (true) {
        palantir::MessageType messageType;
        QByteArray payload;
        QString extractError;
        
        // === CRITICAL SECTION: buffer manipulation only ===
        // Lock protects clientBuffers_ map during append/extract operations
        // Released before dispatch to avoid holding lock during handler execution
        {
            std::lock_guard<std::mutex> lock(clientBuffersMutex_);
            auto it = clientBuffers_.find(client);
            if (it == clientBuffers_.end()) {
                // Client not found (may have disconnected)
                qDebug() << "[SERVER] parseIncomingData: ERROR - client not in buffers map";
                return;
            }
            QByteArray& buffer = it->second;
            
            // Append new data only once per call
            if (!newData.isEmpty()) {
                buffer += newData;
                newData.clear();
                qDebug() << "[SERVER] parseIncomingData: buffer size now=" << buffer.size();
            }
            
            // Extract message from buffer (no locking inside extractMessage)
            qDebug() << "[SERVER] parseIncomingData: attempting to extract message...";
            if (!extractMessage(buffer, messageType, payload, &extractError)) {
                // Check if it's a hard error or just incomplete data
                if (!extractError.isEmpty()) {
                    // Hard error - will handle outside lock
                    qDebug() << "[SERVER] parseIncomingData: extractMessage error:" << extractError;
                } else {
                    // Incomplete frame - need more data, wait for next readyRead
                    qDebug() << "[SERVER] parseIncomingData: incomplete message, waiting for more data";
                }
                break; // Exit critical section
            }
            
            qDebug() << "[SERVER] parseIncomingData: extracted message, type=" << static_cast<int>(messageType) << ", payload size=" << payload.size();
        }
        // === LOCK RELEASED HERE ===
        
        // Handle extraction errors outside lock
        if (!extractError.isEmpty()) {
            if (extractError.contains("exceeds limit")) {
                sendErrorResponse(client, palantir::ErrorCode::MESSAGE_TOO_LARGE, extractError);
            } else {
                sendErrorResponse(client, palantir::ErrorCode::INVALID_MESSAGE_FORMAT, extractError);
            }
            continue; // Try to extract next message if available
        }
        
        // === DISPATCH WITHOUT HOLDING MUTEX ===
        // RPC boundary: Parse protobuf and validate before calling handlers
        // Error codes:
        //   - PROTOBUF_PARSE_ERROR: Protobuf deserialization failed (malformed payload)
        //   - INVALID_PARAMETER_VALUE: Handler validates and rejects semantically invalid parameters
        switch (messageType) {
            case palantir::MessageType::CAPABILITIES_REQUEST: {
                qDebug() << "[SERVER] parseIncomingData: handling CAPABILITIES_REQUEST";
                palantir::CapabilitiesRequest request;
                if (request.ParseFromArray(payload.data(), payload.size())) {
                    qDebug() << "[SERVER] parseIncomingData: parsed CapabilitiesRequest, calling handleCapabilitiesRequest";
                    handleCapabilitiesRequest(client);
                    qDebug() << "[SERVER] parseIncomingData: handleCapabilitiesRequest returned";
                } else {
                    qDebug() << "[SERVER] parseIncomingData: ERROR - failed to parse CapabilitiesRequest";
                    sendErrorResponse(client, palantir::ErrorCode::PROTOBUF_PARSE_ERROR,
                                     "Failed to parse CapabilitiesRequest: malformed protobuf payload");
                }
                continue;
            }
            case palantir::MessageType::XY_SINE_REQUEST: {
                palantir::XYSineRequest request;
                if (request.ParseFromArray(payload.data(), payload.size())) {
                    // RPC boundary: Validation happens in handleXYSineRequest()
                    handleXYSineRequest(client, request);
                } else {
                    qDebug() << "[SERVER] parseIncomingData: ERROR - failed to parse XYSineRequest";
                    sendErrorResponse(client, palantir::ErrorCode::PROTOBUF_PARSE_ERROR,
                                     "Failed to parse XYSineRequest: malformed protobuf payload");
                }
                continue;
            }
            case palantir::MessageType::ERROR_RESPONSE:
                qDebug() << "Server received ErrorResponse (unexpected)";
                continue;
            default:
                sendErrorResponse(client, palantir::ErrorCode::UNKNOWN_MESSAGE_TYPE,
                                 QString("Unknown message type: %1").arg(static_cast<int>(messageType)));
                continue;
        }
    }
#else
    // Transport deps disabled - envelope-based transport not available
    qDebug() << "Transport deps disabled - cannot process envelope-based messages";
#endif
}

#include "PalantirServer.moc"





