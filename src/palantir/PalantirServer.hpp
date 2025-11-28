#pragma once

#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QObject>
#include <QByteArray>
#include <QString>
#include <memory>
#include <map>
#include <atomic>
#include <thread>
#include <mutex>

#ifdef BEDROCK_WITH_TRANSPORT_DEPS
#include "palantir/capabilities.pb.h"
#include "palantir/xysine.pb.h"
#include "palantir/envelope.pb.h"
#include "palantir/error.pb.h"
#include "CapabilitiesService.hpp"
#endif

// PalantirServer: Qt-based IPC server for Palantir protocol
// Threading: All operations run on Qt's event loop thread (main thread)
// - Socket I/O, message parsing, and request handling execute synchronously on event loop thread
// - Worker thread infrastructure exists but is currently disabled (commented out)
// - Mutexes protect shared data structures for future multi-threading support
// See docs/THREADING.md for detailed threading model documentation
class PalantirServer : public QObject
{
    Q_OBJECT

public:
    explicit PalantirServer(QObject *parent = nullptr);
    ~PalantirServer() override;

    bool startServer(const QString& socketName);
    void stopServer();
    bool isRunning() const;

    // Server capabilities
    int maxConcurrency() const;
    QStringList supportedFeatures() const;

signals:
    void clientConnected();
    void clientDisconnected();
    void errorOccurred(const QString& error);

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onClientReadyRead();
    void onHeartbeatTimer();

private:
    // Message handling (envelope-based protocol only)
#ifdef BEDROCK_WITH_TRANSPORT_DEPS
    void handleCapabilitiesRequest(QLocalSocket* client);
    void handleXYSineRequest(QLocalSocket* client, const palantir::XYSineRequest& request);
    void computeXYSine(const palantir::XYSineRequest& request, std::vector<double>& xValues, std::vector<double>& yValues);
#endif
    // Future: Add StartJob, Cancel, Ping handlers when proto messages are defined
    // void handleStartJob(QLocalSocket* client, const palantir::StartJob& startJob);
    // void handleCancel(QLocalSocket* client, const palantir::Cancel& cancel);
    // void handlePing(QLocalSocket* client);
    
    // Job processing (disabled - proto messages not yet defined)
    // Future: Re-enable when ComputeSpec, ResultMeta, etc. are defined
    // void processJob(const QString& jobId, const palantir::ComputeSpec& spec);
    // void sendProgress(const QString& jobId, double progress, const QString& status);
    // void sendResult(const QString& jobId, const palantir::ResultMeta& meta);
    // void sendDataChunk(const QString& jobId, const QByteArray& data, int chunkIndex, int totalChunks);
    
    // XY Sine computation (implemented via handleXYSineRequest)
    // void computeXYSine(const palantir::ComputeSpec& spec, std::vector<double>& xValues, std::vector<double>& yValues);
    
    // Protocol helpers
#ifdef BEDROCK_WITH_TRANSPORT_DEPS
    void sendMessage(QLocalSocket* client, palantir::MessageType type, const google::protobuf::Message& message);
    void sendErrorResponse(QLocalSocket* client, palantir::ErrorCode errorCode, const QString& message, const QString& details = QString());
    // extractMessage() implements envelope-based protocol only:
    // Wire format: [4-byte length][serialized MessageEnvelope]
    // No legacy [length][type][payload] format support
    bool extractMessage(QByteArray& buffer, palantir::MessageType& outType, QByteArray& outPayload, QString* outError = nullptr);
#endif
    void parseIncomingData(QLocalSocket* client);
    
    // Constants
    static constexpr uint32_t MAX_MESSAGE_SIZE = 10 * 1024 * 1024; // 10MB
    
    // Server state
    std::unique_ptr<QLocalServer> server_;
    QTimer heartbeatTimer_;
    std::atomic<bool> running_;
    
    // Client management (thread-safe access required)
    // clientBuffersMutex_ protects clientBuffers_ map
    // Invariant: When locked, clientBuffers_ map is in consistent state
    // Currently all accesses are from event loop thread, but mutex provides:
    // - Future-proofing for worker threads
    // - Defensive programming
    // - Clear documentation of shared data
    std::map<QLocalSocket*, QByteArray> clientBuffers_;
    std::mutex clientBuffersMutex_;  // Protects clientBuffers_ access
    
    std::map<QString, QLocalSocket*> jobClients_;
    
    // Job tracking (disabled - proto messages not yet defined)
    // std::map<QString, palantir::ComputeSpec> activeJobs_;
    std::map<QString, std::atomic<bool>> jobCancelled_;
    
    // Threading
    // jobMutex_ protects jobClients_, jobCancelled_, jobThreads_ maps
    // Invariant: When locked, job tracking maps are in consistent state
    // Currently all accesses are from event loop thread, but mutex provides:
    // - Future-proofing for worker threads (when enabled)
    // - Defensive programming
    // - Clear documentation of shared data
    std::map<QString, std::thread> jobThreads_;
    std::mutex jobMutex_;  // Protects jobClients_, jobCancelled_, jobThreads_
    
    // Capabilities
    int maxConcurrency_;
    QStringList supportedFeatures_;
    QString protocolVersion_;
};





