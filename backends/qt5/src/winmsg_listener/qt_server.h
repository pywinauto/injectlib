#pragma once

#include "qt_object_store.h"
#include "pipe_server.h"

#include <QObject>
#include <QJsonObject>

#include <memory>

class QtHelloServer : public QObject {
    Q_OBJECT
public:
    // Returns the singleton instance
    static QtHelloServer* instance();

    // Bootstrap entry: blocks until QCoreApplication appears, then queues start() on the Qt thread
    static void bootstrap();

    // Queues stop() on the GUI thread if the server is running. Safe to call multiple times.
    static void shutdown();

    bool isRunning() const noexcept;
    QJsonObject processRequestThreadSafe(const QJsonObject& request);

public slots:
    void start();
    void stop();
    QJsonObject processRequest(const QJsonObject& request);

signals:
    void started();
    void stopped();

private:
    explicit QtHelloServer(QObject* parent = nullptr);
    Q_DISABLE_COPY(QtHelloServer)

    std::unique_ptr<PipeServer> m_pipeServer;

    // Stable runtime IDs for QObject and QGraphicsItem instances exposed to Python.
    QtBackend::QtObjectStore m_objectStore;
};
