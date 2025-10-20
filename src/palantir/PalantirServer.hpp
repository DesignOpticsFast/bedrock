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

#include "palantir.pb.h"

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
    // Message handling
    void handleMessage(QLocalSocket* client, const QByteArray& message);
    void handleStartJob(QLocalSocket* client, const palantir::StartJob& startJob);
    void handleCancel(QLocalSocket* client, const palantir::Cancel& cancel);
    void handleCapabilitiesRequest(QLocalSocket* client);
    void handlePing(QLocalSocket* client);
    
    // Job processing
    void processJob(const QString& jobId, const palantir::ComputeSpec& spec);
    void sendProgress(const QString& jobId, double progress, const QString& status);
    void sendResult(const QString& jobId, const palantir::ResultMeta& meta);
    void sendDataChunk(const QString& jobId, const QByteArray& data, int chunkIndex, int totalChunks);
    
    // XY Sine computation
    void computeXYSine(const palantir::ComputeSpec& spec, std::vector<double>& xValues, std::vector<double>& yValues);
    
    // Protocol helpers
    void sendMessage(QLocalSocket* client, const google::protobuf::Message& message);
    QByteArray readMessage(QLocalSocket* client);
    void parseIncomingData(QLocalSocket* client);
    
    // Server state
    std::unique_ptr<QLocalServer> server_;
    QTimer heartbeatTimer_;
    std::atomic<bool> running_;
    
    // Client management
    std::map<QLocalSocket*, QByteArray> clientBuffers_;
    std::map<QString, QLocalSocket*> jobClients_;
    
    // Job tracking
    std::map<QString, palantir::ComputeSpec> activeJobs_;
    std::map<QString, std::atomic<bool>> jobCancelled_;
    
    // Threading
    std::map<QString, std::thread> jobThreads_;
    std::mutex jobMutex_;
    
    // Capabilities
    int maxConcurrency_;
    QStringList supportedFeatures_;
    QString protocolVersion_;
};




