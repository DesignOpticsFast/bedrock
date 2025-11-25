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
    // Message handling (legacy handleMessage removed - envelope-based transport only)
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
    bool extractMessage(QByteArray& buffer, palantir::MessageType& outType, QByteArray& outPayload, QString* outError = nullptr);
    // Legacy readMessage() removed - envelope-based transport only
#endif
    void parseIncomingData(QLocalSocket* client);
    
    // Constants
    static constexpr uint32_t MAX_MESSAGE_SIZE = 10 * 1024 * 1024; // 10MB
    
    // Server state
    std::unique_ptr<QLocalServer> server_;
    QTimer heartbeatTimer_;
    std::atomic<bool> running_;
    
    // Client management (thread-safe access required)
    std::map<QLocalSocket*, QByteArray> clientBuffers_;
    std::mutex clientBuffersMutex_;  // Protects clientBuffers_ access
    
    std::map<QString, QLocalSocket*> jobClients_;
    
    // Job tracking (disabled - proto messages not yet defined)
    // std::map<QString, palantir::ComputeSpec> activeJobs_;
    std::map<QString, std::atomic<bool>> jobCancelled_;
    
    // Threading
    std::map<QString, std::thread> jobThreads_;
    std::mutex jobMutex_;  // Protects jobClients_, jobCancelled_, jobThreads_
    
    // Capabilities
    int maxConcurrency_;
    QStringList supportedFeatures_;
    QString protocolVersion_;
};





