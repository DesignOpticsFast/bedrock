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
    // Parse message type and dispatch
    // WP1: Only handle CapabilitiesRequest
    // Future: Add StartJob, Cancel, Ping/Pong when those proto messages are defined
    
#ifdef BEDROCK_WITH_TRANSPORT_DEPS
    // Try to parse as CapabilitiesRequest
    palantir::CapabilitiesRequest request;
    if (request.ParseFromArray(message.data(), message.size())) {
        handleCapabilitiesRequest(client);
        return;
    }
#endif
    
    qDebug() << "Unknown message type received";
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
    sendMessage(client, response);
#else
    qWarning() << "Capabilities requested but transport deps disabled";
#endif
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
void PalantirServer::sendMessage(QLocalSocket* client, const google::protobuf::Message& message)
{
    if (!client || client->state() != QLocalSocket::ConnectedState) {
        return;
    }
    
    // Serialize message
    std::string serialized;
    if (!message.SerializeToString(&serialized)) {
        qDebug() << "Failed to serialize message";
        return;
    }
    
    // Create length-prefixed message
    QByteArray data;
    uint32_t length = static_cast<uint32_t>(serialized.size());
    
    // Write length (little-endian)
    data.append(reinterpret_cast<const char*>(&length), 4);
    data.append(serialized.data(), serialized.size());
    
    // Send data
    qint64 written = client->write(data);
    if (written != data.size()) {
        qDebug() << "Failed to send complete message";
    }
}
#endif // BEDROCK_WITH_TRANSPORT_DEPS

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
    memcpy(&length, buffer.data(), 4);
    
    if (buffer.size() < 4 + length) {
        return QByteArray();
    }
    
    QByteArray message = buffer.mid(4, length);
    buffer.remove(0, 4 + length);
    
    return message;
}

void PalantirServer::parseIncomingData(QLocalSocket* client)
{
    if (!client) {
        return;
    }
    
    QByteArray& buffer = clientBuffers_[client];
    buffer += client->readAll();
    
    while (buffer.size() >= 4) {
        QByteArray message = readMessage(client);
        if (message.isEmpty()) {
            break;
        }
        
        handleMessage(client, message);
    }
}

#include "PalantirServer.moc"





