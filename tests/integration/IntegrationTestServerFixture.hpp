#pragma once

#include <QObject>
#include <QString>
#include <memory>

#ifdef BEDROCK_WITH_TRANSPORT_DEPS
#include "palantir/PalantirServer.hpp"
#include <QCoreApplication>
#include <QTimer>
#endif

/**
 * Integration test server fixture.
 * 
 * Manages an in-process PalantirServer for integration testing.
 * Starts server on a temporary socket and stops it on teardown.
 */
class IntegrationTestServerFixture {
public:
    IntegrationTestServerFixture();
    ~IntegrationTestServerFixture();

    /**
     * Start the server on a temporary socket.
     * @return true on success, false on failure
     */
    bool startServer();

    /**
     * Stop the server.
     */
    void stopServer();

    /**
     * Get the socket path the server is listening on.
     * @return Socket path, or empty string if server not started
     */
    QString socketPath() const { return socketPath_; }

    /**
     * Check if server is running.
     * @return true if server is running
     */
    bool isRunning() const;

private:
#ifdef BEDROCK_WITH_TRANSPORT_DEPS
    std::unique_ptr<QCoreApplication> app_;
    std::unique_ptr<PalantirServer> server_;
    QString socketPath_;
    static int instanceCount_;
#endif
};

