#include "PalantirServer.hpp"

#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QDebug>
#include <QCoreApplication>
#include <QThread>
#include <QDateTime>
#include <QElapsedTimer>
#include <cmath>
#include <algorithm>

PalantirServer::PalantirServer(QObject *parent)
    : QObject(parent)
    , server_(std::make_unique<QLocalServer>(this))
    , running_(false)
    , maxConcurrency_(QThread::idealThreadCount())
    , supportedFeatures_({"xy_sine", "heat_diffusion"})
    , protocolVersion_("1.0")
    , logFile_(nullptr)
    , logStream_(nullptr)
{
    // Initialize file-based logging
    QString logPath = QProcessEnvironment::systemEnvironment().value("BEDROCK_LOG_FILE", "/tmp/bedrock_startjob.log");
    logFile_ = new QFile(logPath);
    if (logFile_->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        logStream_ = new QTextStream(logFile_);
        logToFile(QString("=== PalantirServer initialized at %1 ===").arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
    } else {
        qWarning() << "Failed to open log file:" << logPath;
    }
    // Connect server signals
    connect(server_.get(), &QLocalServer::newConnection, this, &PalantirServer::onNewConnection);
    
    // Setup heartbeat timer
    heartbeatTimer_.setInterval(2000); // 2 seconds
    connect(&heartbeatTimer_, &QTimer::timeout, this, &PalantirServer::onHeartbeatTimer);
}

PalantirServer::~PalantirServer()
{
    stopServer();
    
    // Cleanup logging
    if (logStream_) {
        delete logStream_;
        logStream_ = nullptr;
    }
    if (logFile_) {
        logFile_->close();
        delete logFile_;
        logFile_ = nullptr;
    }
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
    // Send pong to all connected clients
    for (auto& [client, buffer] : clientBuffers_) {
        palantir::Pong pong;
        pong.set_timestamp_ms(QDateTime::currentMSecsSinceEpoch());
        sendMessage(client, pong);
    }
}

void PalantirServer::handleMessage(QLocalSocket* client, const QByteArray& message)
{
    logToFile(QString("handleMessage: Received message, size=%1").arg(message.size()));
    
    // Parse message type and dispatch
    // For now, handle basic message types
    
    // Try to parse as StartJob
    palantir::StartJob startJob;
    if (startJob.ParseFromArray(message.data(), message.size())) {
        QString jobId = QString::fromStdString(startJob.job_id().id());
        QString featureId = QString::fromStdString(startJob.spec().feature_id());
        logToFile(QString("handleMessage: Parsed StartJob, job_id=%1, feature_id=%2").arg(jobId, featureId));
        handleStartJob(client, startJob);
        return;
    }
    
    // Try to parse as Cancel
    palantir::Cancel cancel;
    if (cancel.ParseFromArray(message.data(), message.size())) {
        QString jobId = QString::fromStdString(cancel.job_id().id());
        logToFile(QString("handleMessage: Parsed Cancel, job_id=%1").arg(jobId));
        handleCancel(client, cancel);
        return;
    }
    
    // Try to parse as CapabilitiesRequest
    palantir::CapabilitiesRequest request;
    if (request.ParseFromArray(message.data(), message.size())) {
        logToFile("handleMessage: Parsed CapabilitiesRequest");
        handleCapabilitiesRequest(client);
        return;
    }
    
    // Try to parse as Ping
    palantir::Ping ping;
    if (ping.ParseFromArray(message.data(), message.size())) {
        logToFile("handleMessage: Parsed Ping");
        handlePing(client);
        return;
    }
    
    logToFile(QString("handleMessage: Unknown message type, size=%1, first_bytes=%2")
              .arg(message.size())
              .arg(message.left(16).toHex().toStdString().c_str()));
}

void PalantirServer::handleStartJob(QLocalSocket* client, const palantir::StartJob& startJob)
{
    QString jobId = QString::fromStdString(startJob.job_id().id());
    palantir::ComputeSpec spec = startJob.spec();
    QString featureId = QString::fromStdString(spec.feature_id());
    
    logToFile(QString("handleStartJob: ENTERED, job_id=%1, feature_id=%2").arg(jobId, featureId));
    
    // Check if we can handle this job
    if (!supportedFeatures_.contains(featureId)) {
        logToFile(QString("handleStartJob: Feature not supported: %1").arg(featureId));
        palantir::StartReply reply;
        reply.mutable_job_id()->set_id(jobId.toStdString());
        reply.set_status("UNIMPLEMENTED");
        reply.set_error_message("Feature not supported: " + featureId.toStdString());
        sendMessage(client, reply);
        logToFile(QString("handleStartJob: Sent UNIMPLEMENTED reply for job_id=%1").arg(jobId));
        return;
    }
    
    // Validate parameters for xy_sine feature (explicit INVALID_ARGUMENT errors)
    if (featureId == "xy_sine") {
        logToFile(QString("handleStartJob: Validating xy_sine parameters for job_id=%1").arg(jobId));
        QString validationError = validateXYSineParameters(spec);
        if (!validationError.isEmpty()) {
            logToFile(QString("handleStartJob: Validation failed: %1").arg(validationError));
            palantir::StartReply reply;
            reply.mutable_job_id()->set_id(jobId.toStdString());
            reply.set_status("INVALID_ARGUMENT");
            reply.set_error_message(validationError.toStdString());
            sendMessage(client, reply);
            logToFile(QString("handleStartJob: Sent INVALID_ARGUMENT reply for job_id=%1").arg(jobId));
            return;
        }
        logToFile(QString("handleStartJob: Validation passed for job_id=%1").arg(jobId));
    }
    
    // Check concurrency limit
    if (activeJobs_.size() >= maxConcurrency_) {
        logToFile(QString("handleStartJob: Server at capacity, active_jobs=%1").arg(activeJobs_.size()));
        palantir::StartReply reply;
        reply.mutable_job_id()->set_id(jobId.toStdString());
        reply.set_status("RESOURCE_EXHAUSTED");
        reply.set_error_message("Server at capacity");
        sendMessage(client, reply);
        logToFile(QString("handleStartJob: Sent RESOURCE_EXHAUSTED reply for job_id=%1").arg(jobId));
        return;
    }
    
    // Start job
    activeJobs_[jobId] = spec;
    jobClients_[jobId] = client;
    jobCancelled_[jobId] = false;
    logToFile(QString("handleStartJob: Job registered, active_jobs=%1").arg(activeJobs_.size()));
    
    // Send start reply
    palantir::StartReply reply;
    reply.mutable_job_id()->set_id(jobId.toStdString());
    reply.set_status("OK");
    logToFile(QString("handleStartJob: Sending StartReply OK for job_id=%1").arg(jobId));
    sendMessage(client, reply);
    logToFile(QString("handleStartJob: StartReply sent for job_id=%1").arg(jobId));
    
    // Start job processing in separate thread
    std::thread jobThread([this, jobId, spec]() {
        processJob(jobId, spec);
    });
    
    jobThreads_[jobId] = std::move(jobThread);
    
    logToFile(QString("handleStartJob: Job thread started for job_id=%1").arg(jobId));
}

void PalantirServer::handleCancel(QLocalSocket* client, const palantir::Cancel& cancel)
{
    QString jobId = QString::fromStdString(cancel.job_id().id());
    
    if (activeJobs_.contains(jobId)) {
        jobCancelled_[jobId] = true;
        qDebug() << "Cancelled job:" << jobId;
    }
}

void PalantirServer::handleCapabilitiesRequest(QLocalSocket* client)
{
    palantir::Capabilities caps;
    caps.set_max_concurrency(maxConcurrency_);
    caps.set_protocol_version(protocolVersion_.toStdString());
    
    for (const QString& feature : supportedFeatures_) {
        caps.add_supported_features(feature.toStdString());
    }
    
    sendMessage(client, caps);
}

void PalantirServer::handlePing(QLocalSocket* client)
{
    // Respond with pong
    palantir::Pong pong;
    pong.set_timestamp_ms(QDateTime::currentMSecsSinceEpoch());
    sendMessage(client, pong);
}

void PalantirServer::processJob(const QString& jobId, const palantir::ComputeSpec& spec)
{
    QString featureId = QString::fromStdString(spec.feature_id());
    
    logToFile(QString("processJob: STARTED, job_id=%1, feature_id=%2").arg(jobId, featureId));
    
    if (featureId == "xy_sine") {
        // Send initial progress
        sendProgress(jobId, 0.0, "Starting computation...");
        
        // Compute XY Sine with progress and cancellation support
        std::vector<double> xValues, yValues;
        logToFile(QString("processJob: Calling computeXYSineWithProgress for job_id=%1").arg(jobId));
        computeXYSineWithProgress(jobId, spec, xValues, yValues);
        logToFile(QString("processJob: computeXYSineWithProgress completed for job_id=%1, result_size=%2").arg(jobId).arg(xValues.size()));
        
        // Check if cancelled (computeXYSineWithProgress may have returned early)
        if (jobCancelled_[jobId]) {
            sendProgress(jobId, 100.0, "CANCELLED");
            
            // Send cancelled result meta
            palantir::ResultMeta meta;
            meta.mutable_job_id()->set_id(jobId.toStdString());
            meta.set_status("CANCELLED");
            sendResult(jobId, meta);
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
            int end = std::min(static_cast<int>(start + chunkSize), static_cast<int>(data.size()));
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

void PalantirServer::sendResult(const QString& jobId, const palantir::ResultMeta& meta)
{
    if (!jobClients_.contains(jobId)) {
        return;
    }
    
    QLocalSocket* client = jobClients_[jobId];
    sendMessage(client, meta);
}

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

PalantirServer::ParsedXYSineParams PalantirServer::parseXYSineParameters(const palantir::ComputeSpec& spec, bool trackPresence)
{
    ParsedXYSineParams params;
    bool explicitSamplesSet = false;
    
    for (const auto& [key, value] : spec.params()) {
        QString paramKey = QString::fromStdString(key);
        QString paramValue = QString::fromStdString(value);
        
        if (paramKey == "frequency") {
            bool ok;
            double val = paramValue.toDouble(&ok);
            if (ok) {
                params.frequency = val;
                if (trackPresence) {
                    params.hasFrequency = true;
                }
            }
        } else if (paramKey == "amplitude") {
            bool ok;
            double val = paramValue.toDouble(&ok);
            if (ok) {
                params.amplitude = val;
                if (trackPresence) {
                    params.hasAmplitude = true;
                }
            }
        } else if (paramKey == "phase") {
            bool ok;
            double val = paramValue.toDouble(&ok);
            if (ok) {
                params.phase = val;
                if (trackPresence) {
                    params.hasPhase = true;
                }
            }
        } else if (paramKey == "samples") {
            // Canonical parameter name (Phoenix standard)
            bool ok;
            int val = paramValue.toInt(&ok);
            if (ok) {
                params.samples = val;
                explicitSamplesSet = true;
                if (trackPresence) {
                    params.hasSamples = true;
                }
            }
        } else if (paramKey == "n_samples") {
            // Backwards-compatible alias (only used if "samples" not set)
            if (!explicitSamplesSet) {
                bool ok;
                int val = paramValue.toInt(&ok);
                if (ok) {
                    params.samples = val;
                    if (trackPresence) {
                        params.hasSamples = true;
                    }
                }
            }
        }
        // Note: "cycles" parameter is ignored (Phoenix doesn't use it)
    }
    
    return params;
}

QString PalantirServer::validateXYSineParameters(const palantir::ComputeSpec& spec)
{
    // Parse parameters with presence tracking
    ParsedXYSineParams params = parseXYSineParameters(spec, true);
    
    // Validate ranges (explicit INVALID_ARGUMENT errors, no silent clamping)
    if (params.hasFrequency && (params.frequency < 0.1 || params.frequency > 100.0)) {
        return QString("Parameter 'frequency' out of range: %1 (valid: 0.1-100.0)").arg(params.frequency);
    }
    
    if (params.hasAmplitude && (params.amplitude < 0.0 || params.amplitude > 10.0)) {
        return QString("Parameter 'amplitude' out of range: %1 (valid: 0.0-10.0)").arg(params.amplitude);
    }
    
    const double twoPi = 2.0 * M_PI;
    if (params.hasPhase && (params.phase < -twoPi || params.phase > twoPi)) {
        return QString("Parameter 'phase' out of range: %1 (valid: %2 to %3)")
            .arg(params.phase)
            .arg(-twoPi, 0, 'f', 2)
            .arg(twoPi, 0, 'f', 2);
    }
    
    if (params.hasSamples && (params.samples < 10 || params.samples > 100000)) {
        return QString("Parameter 'samples' out of range: %1 (valid: 10-100000)").arg(params.samples);
    }
    
    // Minimum samples validation (even if not explicitly set)
    if (params.samples < 10) {
        return QString("Parameter 'samples' too small: %1 (minimum: 10)").arg(params.samples);
    }
    
    return QString();  // Valid
}

void PalantirServer::computeXYSine(const palantir::ComputeSpec& spec, std::vector<double>& xValues, std::vector<double>& yValues)
{
    // Parse parameters using shared helper
    // Note: Parameters are already validated in handleStartJob(), so we can assume valid ranges
    ParsedXYSineParams params = parseXYSineParameters(spec, false);
    
    // Extract values for computation
    double frequency = params.frequency;
    double amplitude = params.amplitude;
    double phase = params.phase;
    int samples = params.samples;
    
    // Compute sine wave using Phoenix's algorithm
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

void PalantirServer::computeXYSineWithProgress(const QString& jobId, const palantir::ComputeSpec& spec, std::vector<double>& xValues, std::vector<double>& yValues)
{
    // Parse parameters using shared helper
    ParsedXYSineParams params = parseXYSineParameters(spec, false);
    
    // Extract values for computation
    double frequency = params.frequency;
    double amplitude = params.amplitude;
    double phase = params.phase;
    int samples = params.samples;
    
    // Compute sine wave with progress and cancellation support
    xValues.clear();
    yValues.clear();
    xValues.reserve(samples);
    yValues.reserve(samples);
    
    // Progress throttling: track last progress send time (≤5 Hz = every 200ms)
    QElapsedTimer progressTimer;
    progressTimer.start();
    const qint64 progressIntervalMs = 200;  // 5 Hz max
    
    for (int i = 0; i < samples; ++i) {
        // Check for cancellation every 100 samples (as per decision #5)
        if (i % 100 == 0 && jobCancelled_[jobId]) {
            return;  // Exit early if cancelled
        }
        
        // Send progress throttled to ≤5 Hz
        if (progressTimer.elapsed() >= progressIntervalMs) {
            double progress = (static_cast<double>(i) / samples) * 100.0;
            sendProgress(jobId, progress, "Computing...");
            progressTimer.restart();
        }
        
        double t = static_cast<double>(i) / (samples - 1.0);  // 0 to 1
        double x = t * 2.0 * M_PI;  // Scale to 0..2π domain
        double y = amplitude * std::sin(2.0 * M_PI * frequency * t + phase);
        
        xValues.push_back(x);
        yValues.push_back(y);
    }
    
    // Send final progress
    sendProgress(jobId, 100.0, "Complete");
}

void PalantirServer::sendMessage(QLocalSocket* client, const google::protobuf::Message& message)
{
    if (!client || client->state() != QLocalSocket::ConnectedState) {
        logToFile(QString("sendMessage: SKIPPED - client=%1, state=%2")
                  .arg(client ? "valid" : "null")
                  .arg(client ? QString::number(client->state()) : "N/A"));
        return;
    }
    
    // Serialize message
    std::string serialized;
    if (!message.SerializeToString(&serialized)) {
        logToFile("sendMessage: Failed to serialize message");
        return;
    }
    
    // Create length-prefixed message
    QByteArray data;
    uint32_t length = static_cast<uint32_t>(serialized.size());
    
    // Write length (little-endian)
    data.append(reinterpret_cast<const char*>(&length), 4);
    data.append(serialized.data(), serialized.size());
    
    // Try to identify message type for logging
    QString msgType = "Unknown";
    if (message.GetTypeName() == "palantir.StartReply") {
        const palantir::StartReply* reply = dynamic_cast<const palantir::StartReply*>(&message);
        if (reply) {
            msgType = QString("StartReply(status=%1, job_id=%2)")
                      .arg(QString::fromStdString(reply->status()))
                      .arg(QString::fromStdString(reply->job_id().id()));
        }
    } else {
        msgType = QString::fromStdString(message.GetTypeName());
    }
    
    logToFile(QString("sendMessage: Sending %1, size=%2").arg(msgType).arg(data.size()));
    
    // Send data
    qint64 written = client->write(data);
    if (written != data.size()) {
        logToFile(QString("sendMessage: Failed to send complete message, written=%1, expected=%2")
                  .arg(written).arg(data.size()));
        return;
    }
    
    // Flush data to ensure it's sent immediately
    if (!client->flush()) {
        logToFile("sendMessage: Failed to flush message");
    } else {
        logToFile(QString("sendMessage: Successfully sent and flushed %1").arg(msgType));
    }
}

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

void PalantirServer::logToFile(const QString& message)
{
    if (logStream_) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        *logStream_ << "[" << timestamp << "] " << message << Qt::endl;
        logStream_->flush();
    }
    // Also log to qDebug for immediate visibility
    qDebug() << message;
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





