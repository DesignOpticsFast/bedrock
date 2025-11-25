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
    
    // Wait for job threads to finish
    for (auto& [jobId, thread] : jobThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
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
        return;
    }
    
    // Connect client signals
    connect(client, &QLocalSocket::disconnected, this, &PalantirServer::onClientDisconnected);
    connect(client, &QLocalSocket::readyRead, this, &PalantirServer::onClientReadyRead);
    
    // Initialize client buffer
    clientBuffers_[client] = QByteArray();
    
    emit clientConnected();
    qDebug() << "Client connected";
}

void PalantirServer::onClientDisconnected()
{
    QLocalSocket* client = qobject_cast<QLocalSocket*>(sender());
    if (!client) {
        return;
    }
    
    // Remove client from tracking
    clientBuffers_.erase(client);
    
    // Cancel jobs for this client
    for (auto it = jobClients_.begin(); it != jobClients_.end();) {
        if (it->second == client) {
            QString jobId = it->first;
            jobCancelled_[jobId] = true;
            jobClients_.erase(it++);
        } else {
            ++it;
        }
    }
    
    client->deleteLater();
    emit clientDisconnected();
    qDebug() << "Client disconnected";
}

void PalantirServer::onClientReadyRead()
{
    QLocalSocket* client = qobject_cast<QLocalSocket*>(sender());
    if (!client) {
        return;
    }
    
    parseIncomingData(client);
}

void PalantirServer::onHeartbeatTimer()
{
    // WP1: Heartbeat/Pong not yet implemented (requires Pong proto message)
    // Future: Send pong to all connected clients when Pong message is defined
    // For now, heartbeat timer is disabled or no-op
}

void PalantirServer::handleMessage(QLocalSocket* client, const QByteArray& message)
{
#ifdef BEDROCK_WITH_TRANSPORT_DEPS
    // Sprint 4.5: Try new format first (with MessageType)
    palantir::MessageType messageType;
    QByteArray payload;
    
    if (readMessageWithType(client, messageType, payload)) {
        // New format: dispatch by MessageType
        switch (messageType) {
            case palantir::MessageType::CAPABILITIES_REQUEST: {
                palantir::CapabilitiesRequest request;
                if (request.ParseFromArray(payload.data(), payload.size())) {
                    handleCapabilitiesRequest(client);
                } else {
                    sendErrorResponse(client, palantir::ErrorCode::PROTOBUF_PARSE_ERROR,
                                     "Failed to parse CapabilitiesRequest");
                }
                return;
            }
            case palantir::MessageType::XY_SINE_REQUEST: {
                palantir::XYSineRequest request;
                if (request.ParseFromArray(payload.data(), payload.size())) {
                    handleXYSineRequest(client, request);
                } else {
                    sendErrorResponse(client, palantir::ErrorCode::PROTOBUF_PARSE_ERROR,
                                     "Failed to parse XYSineRequest");
                }
                return;
            }
            case palantir::MessageType::ERROR_RESPONSE:
                // Server should not receive ErrorResponse
                qDebug() << "Server received ErrorResponse (unexpected)";
                return;
            default:
                sendErrorResponse(client, palantir::ErrorCode::UNKNOWN_MESSAGE_TYPE,
                                 QString("Unknown message type: %1").arg(static_cast<int>(messageType)));
                return;
        }
    }
    
    // Backward compatibility: try old format (no MessageType)
    // Log deprecation warning
    static bool deprecationWarningLogged = false;
    if (!deprecationWarningLogged) {
        qWarning() << "DEPRECATED: Received message in old format (no MessageType). "
                   << "This format will be removed in Sprint 4.6. Please upgrade to new format.";
        deprecationWarningLogged = true;
    }
    
    // Try to parse as XYSineRequest (check before CapabilitiesRequest for specificity)
    palantir::XYSineRequest xySineRequest;
    if (xySineRequest.ParseFromArray(message.data(), message.size())) {
        handleXYSineRequest(client, xySineRequest);
        return;
    }
    
    // Try to parse as CapabilitiesRequest
    palantir::CapabilitiesRequest request;
    if (request.ParseFromArray(message.data(), message.size())) {
        handleCapabilitiesRequest(client);
        return;
    }
    
    // Unknown message format
    sendErrorResponse(client, palantir::ErrorCode::UNKNOWN_MESSAGE_TYPE,
                     "Failed to parse message (unknown format)");
#else
    qDebug() << "Message received but transport deps disabled";
#endif
}

// WP1: StartJob handler disabled (proto message not yet defined)
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
    
    qDebug() << "Started job:" << jobId;
}
*/

void PalantirServer::handleCapabilitiesRequest(QLocalSocket* client)
{
#ifdef BEDROCK_WITH_TRANSPORT_DEPS
    bedrock::palantir::CapabilitiesService service;
    palantir::CapabilitiesResponse response = service.getCapabilities();
    sendMessage(client, palantir::MessageType::CAPABILITIES_RESPONSE, response);
#else
    qWarning() << "Capabilities requested but transport deps disabled";
#endif
}

void PalantirServer::handleXYSineRequest(QLocalSocket* client, const palantir::XYSineRequest& request)
{
#ifdef BEDROCK_WITH_TRANSPORT_DEPS
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

// WP1: Ping/Pong handler disabled (proto message not yet defined)
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
    if (!client || client->state() != QLocalSocket::ConnectedState) {
        return;
    }
    
    // Serialize message
    std::string serialized;
    if (!message.SerializeToString(&serialized)) {
        qDebug() << "Failed to serialize message";
        sendErrorResponse(client, palantir::ErrorCode::INTERNAL_ERROR, 
                         "Failed to serialize message");
        return;
    }
    
    // Check size limit
    if (serialized.size() > MAX_MESSAGE_SIZE) {
        qDebug() << "Message too large:" << serialized.size();
        sendErrorResponse(client, palantir::ErrorCode::MESSAGE_TOO_LARGE,
                         QString("Message size %1 exceeds limit %2")
                         .arg(serialized.size()).arg(MAX_MESSAGE_SIZE));
        return;
    }
    
    // Create length-prefixed message with MessageType
    QByteArray data;
    uint32_t totalLength = static_cast<uint32_t>(serialized.size() + 1); // +1 for MessageType byte
    uint8_t typeByte = static_cast<uint8_t>(type);
    
    // Write length (little-endian, 4 bytes)
    data.append(reinterpret_cast<const char*>(&totalLength), 4);
    // Write MessageType (1 byte)
    data.append(reinterpret_cast<const char*>(&typeByte), 1);
    // Write serialized message
    data.append(serialized.data(), serialized.size());
    
    // Send data
    qint64 written = client->write(data);
    if (written != data.size()) {
        qDebug() << "Failed to send complete message";
    }
}

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

bool PalantirServer::readMessageWithType(QLocalSocket* client, palantir::MessageType& outType, QByteArray& outPayload)
{
    if (!client) {
        return false;
    }
    
    QByteArray& buffer = clientBuffers_[client];
    
    // Need at least 5 bytes: 4-byte length + 1-byte MessageType
    if (buffer.size() < 5) {
        return false;
    }
    
    // Read length (little-endian)
    uint32_t totalLength;
    std::memcpy(&totalLength, buffer.data(), 4);
    
    // Check size limit
    if (totalLength > MAX_MESSAGE_SIZE + 1) { // +1 for MessageType byte
        qDebug() << "Message length exceeds limit:" << totalLength;
        sendErrorResponse(client, palantir::ErrorCode::MESSAGE_TOO_LARGE,
                         QString("Message length %1 exceeds limit %2")
                         .arg(totalLength).arg(MAX_MESSAGE_SIZE + 1));
        buffer.clear(); // Clear buffer to prevent further parsing
        return false;
    }
    
    // Check if we have the complete message
    if (buffer.size() < 4 + totalLength) {
        return false;
    }
    
    // Read MessageType (1 byte after length)
    uint8_t typeByte = static_cast<uint8_t>(buffer[4]);
    outType = static_cast<palantir::MessageType>(typeByte);
    
    // Read payload (everything after MessageType)
    outPayload = buffer.mid(5, totalLength - 1);
    buffer.remove(0, 4 + totalLength);
    
    return true;
}

// Backward compatibility: read old format (no MessageType, just length + payload)
// This function is kept for backward compatibility but is deprecated
// It reads from the buffer directly (used by parseIncomingData for old format)
QByteArray PalantirServer::readMessage(QLocalSocket* client)
{
    if (!client) {
        return QByteArray();
    }
    
    QByteArray& buffer = clientBuffers_[client];
    
    if (buffer.size() < 4) {
        return QByteArray();
    }
    
    uint32_t length;
    std::memcpy(&length, buffer.data(), 4);
    
    // Check size limit
    if (length > MAX_MESSAGE_SIZE) {
        qDebug() << "Old-format message length exceeds limit:" << length;
        buffer.clear();
        return QByteArray();
    }
    
    if (buffer.size() < 4 + length) {
        return QByteArray();
    }
    
    QByteArray message = buffer.mid(4, length);
    buffer.remove(0, 4 + length);
    
    return message;
}
#endif // BEDROCK_WITH_TRANSPORT_DEPS

void PalantirServer::parseIncomingData(QLocalSocket* client)
{
    if (!client) {
        return;
    }
    
    QByteArray& buffer = clientBuffers_[client];
    buffer += client->readAll();
    
#ifdef BEDROCK_WITH_TRANSPORT_DEPS
    // Try new format first (5 bytes minimum: 4-byte length + 1-byte MessageType)
    while (buffer.size() >= 5) {
        palantir::MessageType messageType;
        QByteArray payload;
        if (readMessageWithType(client, messageType, payload)) {
            // Dispatch by MessageType
            switch (messageType) {
                case palantir::MessageType::CAPABILITIES_REQUEST: {
                    palantir::CapabilitiesRequest request;
                    if (request.ParseFromArray(payload.data(), payload.size())) {
                        handleCapabilitiesRequest(client);
                    } else {
                        sendErrorResponse(client, palantir::ErrorCode::PROTOBUF_PARSE_ERROR,
                                         "Failed to parse CapabilitiesRequest");
                    }
                    continue;
                }
                case palantir::MessageType::XY_SINE_REQUEST: {
                    palantir::XYSineRequest request;
                    if (request.ParseFromArray(payload.data(), payload.size())) {
                        handleXYSineRequest(client, request);
                    } else {
                        sendErrorResponse(client, palantir::ErrorCode::PROTOBUF_PARSE_ERROR,
                                         "Failed to parse XYSineRequest");
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
        break; // Not enough data yet
    }
    
    // Fallback: try old format (backward compatibility)
    while (buffer.size() >= 4) {
        QByteArray message = readMessage(client);
        if (message.isEmpty()) {
            break;
        }
        handleMessage(client, message);
    }
#else
    // Old format only (when transport deps disabled)
    while (buffer.size() >= 4) {
        QByteArray message = readMessage(client);
        if (message.isEmpty()) {
            break;
        }
        handleMessage(client, message);
    }
#endif
}

#include "PalantirServer.moc"





