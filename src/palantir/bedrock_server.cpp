#include "PalantirServer.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QCommandLineParser>
#include <QTimer>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    
    QCommandLineParser parser;
    parser.setApplicationDescription("Bedrock Palantir Server");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption socketOption("socket", "Socket name", "socket", "palantir_bedrock");
    parser.addOption(socketOption);
    
    parser.process(app);
    
    QString socketName = parser.value(socketOption);
    
    // Create server
    PalantirServer server;
    
    // Connect signals
    QObject::connect(&server, &PalantirServer::clientConnected, []() {
        qDebug() << "Client connected";
    });
    
    QObject::connect(&server, &PalantirServer::clientDisconnected, []() {
        qDebug() << "Client disconnected";
    });
    
    QObject::connect(&server, &PalantirServer::errorOccurred, [](const QString& error) {
        qDebug() << "Server error:" << error;
    });
    
    // Start server
    if (!server.startServer(socketName)) {
        qDebug() << "Failed to start server";
        return 1;
    }
    
    qDebug() << "Bedrock server running on socket:" << socketName;
    qDebug() << "Max concurrency:" << server.maxConcurrency();
    qDebug() << "Supported features:" << server.supportedFeatures();
    
    return app.exec();
}





