#pragma once

#include <QObject>
#include <QHostAddress>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHash>
#include <QPointer>
#include <QAccessible>
#include <QSet>
#include <QGraphicsItem>
// #include <QQuickItem>

// Forward declarations to keep the header lightweight.
class QTcpServer;

class QtHelloServer : public QObject {
    Q_OBJECT
public:
    /// Returns the singleton instance (created on first use on whichever thread calls it).
    /// Do not use directly from non-Qt threads except via bootstrap()/shutdown().
    static QtHelloServer* instance();

    /// Bootstrap entry: blocks briefly on a worker thread until QCoreApplication appears,
    /// then queues start() on the Qt thread. No-op if no QCoreApplication within timeout.
    static void bootstrap();

    /// Queues stop() on the Qt thread (if running). Safe to call multiple times.
    static void shutdown();

    /// Whether the server is currently listening.
    bool isRunning() const noexcept;

    /// The bound port (0 if not running).
    quint16 port() const noexcept;

    /// The address we bind to (defaults to QHostAddress::LocalHost).
    QHostAddress bindAddress() const noexcept;

public slots:
    /// Create and start the QTcpServer on the Qt thread.
    /// Binds to bindAddress():ephemeral_port (port 0). Emits started(port) on success.
    void start();

    /// Stop listening and close all client sockets. Emits stopped() when done.
    void stop();

signals:
    void started(quint16 port);
    void stopped();

private:
    explicit QtHelloServer(QObject* parent = nullptr);
    Q_DISABLE_COPY(QtHelloServer)

    
    // handlers for requests
    void handlePing(QTcpSocket* sock, int requestId);
    void handleAppInfo(QTcpSocket* sock, int requestId);
    void handleElementsRoots(QTcpSocket* sock, int requestId);
    void handleElementsChildren(QTcpSocket* sock, int requestId, int parentId);
    void handleElementInfo(QTcpSocket* sock, int requestId, int id);
    void handleElementClick(QTcpSocket* sock, int requestId, int id);
    void handleElementSetText(QTcpSocket* sock, int requestId, int id, const QString& text);

    // helpers
    void sendJson(QTcpSocket* sock, const QJsonObject& obj);
    static bool waitForQCoreApp(int totalMs = 30000, int pollMs = 100);
    // bool startImpl(const QHostAddress& addr, quint16 port0);

    void onNewConnection();

    QTcpServer*  m_server{nullptr};
    QHostAddress m_bindAddr{QHostAddress::LocalHost};
    quint16      m_port{0};

    // id map
    QHash<int, QPointer<QObject>> m_id2obj;
    QHash<QObject*, int>          m_obj2id;
    int m_nextId = 1;

    int ensureIdFor(QObject* obj);
    void dropIdFor(QObject* obj);
    QObject* objectForId(int id) const;


    // ---------- ID space for QGraphicsItem* (not QObject) ----------
    struct GItemMaps {
        QHash<QGraphicsItem*, int> gitem2id;
        QHash<int, QGraphicsItem*> id2gitem;
        int nextId = 1000000; // will be offset below so it doesn't collide with QObject IDs
    };
    GItemMaps gmaps;

    int ensureIdForGItem(QGraphicsItem* it);
    QGraphicsItem* gitemForId(int id);

    // summarizers
    QJsonObject summarizeTopLevel(QObject* obj);
    QJsonObject summarizeObject(QObject* obj);
    QJsonObject summarizeGraphicsItem(QGraphicsItem* gi);
    // QJsonObject summarizeQuickItem(QQuickItem* qi);

};

