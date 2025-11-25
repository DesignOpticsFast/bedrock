#include "IntegrationTestServerFixture.hpp"

#ifdef BEDROCK_WITH_TRANSPORT_DEPS
#include <QCoreApplication>
#include <QDebug>
#include <QLocalServer>
#include <QUuid>
#include <QThread>

int IntegrationTestServerFixture::instanceCount_ = 0;

IntegrationTestServerFixture::IntegrationTestServerFixture()
{
    // Create QCoreApplication if it doesn't exist
    if (!QCoreApplication::instance()) {
        // Create a minimal argc/argv for QCoreApplication
        static int argc = 1;
        static char* argv[] = { const_cast<char*>("integration_tests"), nullptr };
        app_ = std::make_unique<QCoreApplication>(argc, argv);
    }
    
    server_ = std::make_unique<PalantirServer>();
    
    // Generate unique socket path for this test instance
    QString uniqueId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    socketPath_ = QString("palantir_test_%1").arg(uniqueId);
}

IntegrationTestServerFixture::~IntegrationTestServerFixture()
{
    stopServer();
}

bool IntegrationTestServerFixture::startServer()
{
    if (isRunning()) {
        return true;
    }
    
    qDebug() << "[FIXTURE] Starting server on socket:" << socketPath_;
    if (!server_->startServer(socketPath_)) {
        qDebug() << "[FIXTURE] ERROR - Failed to start integration test server";
        return false;
    }
    
    qDebug() << "[FIXTURE] Server started, processing events...";
    // Process events to allow server to start and be ready for connections
    if (QCoreApplication::instance()) {
        QCoreApplication::processEvents();
        QThread::msleep(50);  // Small delay to ensure server is fully ready
        QCoreApplication::processEvents();
    }
    
    qDebug() << "[FIXTURE] Server ready";
    return true;
}

void IntegrationTestServerFixture::stopServer()
{
    if (server_ && server_->isRunning()) {
        server_->stopServer();
        if (QCoreApplication::instance()) {
            QCoreApplication::processEvents();
        }
    }
    
    // Clean up socket file
    if (!socketPath_.isEmpty()) {
        QLocalServer::removeServer(socketPath_);
    }
}

bool IntegrationTestServerFixture::isRunning() const
{
    return server_ && server_->isRunning();
}

#else
// Stub implementation when transport deps disabled
IntegrationTestServerFixture::IntegrationTestServerFixture() {}
IntegrationTestServerFixture::~IntegrationTestServerFixture() {}
bool IntegrationTestServerFixture::startServer() { return false; }
void IntegrationTestServerFixture::stopServer() {}
bool IntegrationTestServerFixture::isRunning() const { return false; }
#endif

