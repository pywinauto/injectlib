#pragma once

#include <QObject>
#include <QJsonObject>
#include <QHash>
#include <QPointer>
#include <QAccessible>
#include <QSet>
#include <QGraphicsItem>

#include <memory>

class PipeServer;

class QtHelloServer : public QObject {
    Q_OBJECT
public:
    // Returns the singleton instance
    static QtHelloServer* instance();

    // Bootstrap entry: blocks until QCoreApplication appears, then queues start() on the Qt thread
    static void bootstrap();

    // Queues stop() on the Qt thread (if running). Safe to call multiple times.
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

    QJsonObject handlePing();
    QJsonObject handleAppInfo();
    QJsonObject handleElementsRoots();
    QJsonObject handleElementsChildren(int parentId);
    QJsonObject handleElementInfo(int id);
    QJsonObject handleElementClick(int id);
    QJsonObject handleElementSetText(int id, const QString& text);

    std::unique_ptr<PipeServer> m_pipeServer;

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
};
