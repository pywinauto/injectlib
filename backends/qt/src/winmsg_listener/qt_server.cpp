#include "qt_server.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QAbstractEventDispatcher>
#include <QMetaObject>
#include <QTimer>
#include <QThread>
#include <QByteArray>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include <QWidget>
#include <QWindow>
#include <windows.h>
#include <QAbstractButton>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QLabel>
#include <QTabWidget>
#include <QListView>
#include <QTreeView>
#include <QTableView>
#include <QFrame>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QPointer>
#include <QAction>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QAbstractSpinBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateTimeEdit>

#include <thread>
#include <chrono>

// helpers
namespace {

void dbg(const QString& s) {
    OutputDebugStringW(reinterpret_cast<const wchar_t*>(s.utf16()));
}

int read_env_int(const wchar_t* name, int def_val) {
    wchar_t buf[64];
    DWORD n = GetEnvironmentVariableW(name, buf, (DWORD)(sizeof(buf) / sizeof(buf[0])));
    if (n == 0 || n >= (DWORD)(sizeof(buf) / sizeof(buf[0])))
        return def_val;
    return _wtoi(buf);
}

int clamp_int(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void send_json(QTcpSocket* sock, const QJsonObject& obj) {
    QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray frame = QByteArray::number(payload.size()) + "\n" + payload;
    sock->write(frame);
    sock->flush();
}

bool wait_for_gui_loop(int total_ms, int poll_ms) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(total_ms);
    while (std::chrono::steady_clock::now() < deadline) {
        QCoreApplication* app = QCoreApplication::instance();
        if (app && !QCoreApplication::startingUp() &&
            QAbstractEventDispatcher::instance(app->thread()) != nullptr) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(poll_ms));
    }
    return false;
}

QJsonArray rect_to_array(const QRect& r) {
    return QJsonArray{ r.x(), r.y(), r.width(), r.height() };
}

QString control_type_for(QObject* obj) {
    if (!obj) return QStringLiteral("Object");

    if (auto w = qobject_cast<QWidget*>(obj)) {
        if (qobject_cast<QAbstractButton*>(w)) return QStringLiteral("Button");
        if (qobject_cast<QLineEdit*>(w)) return QStringLiteral("Edit");
        if (qobject_cast<QComboBox*>(w)) return QStringLiteral("ComboBox");
        if (qobject_cast<QCheckBox*>(w)) return QStringLiteral("CheckBox");
        if (qobject_cast<QRadioButton*>(w)) return QStringLiteral("RadioButton");
        if (qobject_cast<QTabWidget*>(w)) return QStringLiteral("TabControl");
        if (qobject_cast<QListView*>(w)) return QStringLiteral("List");
        if (qobject_cast<QTreeView*>(w)) return QStringLiteral("TreeView");
        if (qobject_cast<QTableView*>(w)) return QStringLiteral("Table");
        if (qobject_cast<QLabel*>(w)) return QStringLiteral("Text");
        return QStringLiteral("Pane");
    }
    if (qobject_cast<QWindow*>(obj))
        return QStringLiteral("Window");

    return QStringLiteral("Object");
}

bool setQObjectTextOrValue(QObject* obj, const QString& text) {
    if (!obj) return false;

    if (auto le = qobject_cast<QLineEdit*>(obj)) {
        if (le->isReadOnly()) return false;
        QMetaObject::invokeMethod(le, [le, text]() { le->setText(text); }, Qt::QueuedConnection);
        return true;
    }

    if (auto pe = qobject_cast<QPlainTextEdit*>(obj)) {
        if (pe->isReadOnly()) return false;
        QMetaObject::invokeMethod(pe, [pe, text]() { pe->setPlainText(text); }, Qt::QueuedConnection);
        return true;
    }

    if (auto te = qobject_cast<QTextEdit*>(obj)) {
        if (te->isReadOnly()) return false;
        QMetaObject::invokeMethod(te, [te, text]() { te->setPlainText(text); }, Qt::QueuedConnection);
        return true;
    }

    if (auto cb = qobject_cast<QComboBox*>(obj)) {
        if (cb->isEditable())
            QMetaObject::invokeMethod(cb, [cb, text]() { cb->setEditText(text); }, Qt::QueuedConnection);
            return true;
    }

    if (auto sp = qobject_cast<QSpinBox*>(obj)) {
        bool ok = false; int v = text.toInt(&ok);
        if (!ok) return false;
        QMetaObject::invokeMethod(sp, [sp, v]() { sp->setValue(v); }, Qt::QueuedConnection);
        return true;
    }
    if (auto dsp = qobject_cast<QDoubleSpinBox*>(obj)) {
        bool ok = false; double v = text.toDouble(&ok);
        if (!ok) return false;
        QMetaObject::invokeMethod(dsp, [dsp, v]() { dsp->setValue(v); }, Qt::QueuedConnection);
        return true;
    }

    if (auto dt = qobject_cast<QDateTimeEdit*>(obj)) {
        QDateTime dtv = QDateTime::fromString(text, Qt::ISODate);
        if (!dtv.isValid()) return false;
        QMetaObject::invokeMethod(dt, [dt, dtv]() { dt->setDateTime(dtv); }, Qt::QueuedConnection);
        return true;
    }
    if (auto de = qobject_cast<QDateEdit*>(obj)) {
        QDate dv = QDate::fromString(text, Qt::ISODate);
        if (!dv.isValid()) dv = QDate::fromString(text, de->displayFormat());
        if (!dv.isValid()) return false;
        QMetaObject::invokeMethod(de, [de, dv]() { de->setDate(dv); }, Qt::QueuedConnection);
        return true;
    }
    if (auto te2 = qobject_cast<QTimeEdit*>(obj)) {
        QTime tv = QTime::fromString(text, Qt::ISODate);
        if (!tv.isValid()) tv = QTime::fromString(text, te2->displayFormat());
        if (!tv.isValid()) return false;
        QMetaObject::invokeMethod(te2, [te2, tv]() { te2->setTime(tv); }, Qt::QueuedConnection);
        return true;
    }
    // Try common methods
    if (QMetaObject::invokeMethod(obj, "setText", Qt::QueuedConnection, Q_ARG(QString, text))) return true;

    return false;
}

} // namespace

// ----------------- QtHelloServer -----------------

QtHelloServer* g_instance = nullptr;

QtHelloServer* QtHelloServer::instance() {
    if (!g_instance) {
        g_instance = new QtHelloServer();
    }
    return g_instance;
}

QtHelloServer::QtHelloServer(QObject* parent)
    : QObject(parent),
      m_server(nullptr),
      m_bindAddr(QHostAddress::LocalHost),
      m_port(0)
{
    // QTcpServer is created in start() on the GUI thread.
}

bool QtHelloServer::isRunning() const noexcept {
    return (m_server && m_server->isListening());
}

quint16 QtHelloServer::port() const noexcept { return m_port; }
QHostAddress QtHelloServer::bindAddress() const noexcept { return m_bindAddr; }

bool QtHelloServer::waitForQCoreApp(int totalMs, int pollMs) {
    // Keep this for compatibility, but bootstrap() uses wait_for_gui_loop() below.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(totalMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (QCoreApplication::instance() != NULL) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(pollMs));
    }
    return false;
}

void QtHelloServer::bootstrap() {
    const int totalMs = read_env_int(L"QT_INJECTED_WAIT_MS", 30000); // 30s default
    const int pollMs  = read_env_int(L"QT_INJECTED_POLL_MS", 100);   // 100ms default

    if (!wait_for_gui_loop(totalMs, pollMs)) {
        dbg(QString::fromLatin1("[injectdll] GUI event loop not ready — not starting server (waited %1 ms)\n")
                .arg(totalMs));
        return;
    }

    QCoreApplication* app = QCoreApplication::instance();
    QtHelloServer* srv = QtHelloServer::instance();

    if (srv->thread() != app->thread())
        srv->moveToThread(app->thread());

    dbg(QString::fromLatin1("[injectdll] GUI loop ready. Queueing server start...\n"));

    // Qt5-safe: queue onto the receiver's thread
    QTimer::singleShot(0, srv, SLOT(start()));
}

void QtHelloServer::shutdown() {
    // Clean stop on the GUI thread if we're running.
    QtHelloServer* srv = QtHelloServer::instance();
    if (srv->isRunning()) {
        QMetaObject::invokeMethod(srv, "stop", Qt::QueuedConnection);
    }
}

// Create and start the TCP server on the GUI thread
void QtHelloServer::start() {
    dbg(QString::fromLatin1("[injectdll] start() entered on thread %1\n")
        .arg(reinterpret_cast<qulonglong>(QThread::currentThreadId())));

    if (isRunning()) {
        dbg(QString::fromLatin1("[injectdll] Already running on %1:%2\n")
                .arg(m_bindAddr.toString()).arg(m_port));
        return;
    }

    if (!m_server) {
        m_server = new QTcpServer(this);
        connect(m_server, &QTcpServer::newConnection,
                this, &QtHelloServer::onNewConnection);
    }

    int raw = read_env_int(L"QT_INJECTED_SERVER_PORT", 5555);
    quint16 requestedPort = static_cast<quint16>(clamp_int(raw, 1, 65535));

    if (!m_server->listen(m_bindAddr, requestedPort)) {
        const QString err = m_server->errorString();
        dbg(QString::fromLatin1("[injectdll] listen(%1:%2) FAILED: %3\n")
                .arg(m_bindAddr.toString()).arg(requestedPort).arg(err));
        m_port = 0;
        return;
    }

    m_port = m_server->serverPort();
    dbg(QString::fromLatin1("[injectdll] Server started on %1:%2 (thread %3)\n")
            .arg(m_bindAddr.toString()).arg(m_port)
            .arg(reinterpret_cast<qulonglong>(QThread::currentThreadId())));
    emit started(m_port);
}

void QtHelloServer::stop() {
    if (!m_server || !m_server->isListening())
        return;

    m_server->close();
    m_port = 0;
    dbg(QString::fromLatin1("[injectdll] Server stopped.\n"));
    emit stopped();
}

void QtHelloServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket* c = m_server->nextPendingConnection();

        dbg(QString::fromLatin1("[injectdll] New connection (sock=%1)\n")
                .arg(reinterpret_cast<qulonglong>(c)));

        connect(c, &QTcpSocket::disconnected, c, &QObject::deleteLater);

        // Reply when data arrives
        connect(c, &QTcpSocket::readyRead, this, [this, c]() {
            while (c->canReadLine()) {
                // Read length prefix line
                QByteArray lenLine = c->readLine().trimmed();
                bool ok = false;
                int length = lenLine.toInt(&ok);
                if (!ok || length <= 0) {
                    dbg(QString::fromLatin1("[injectdll] Invalid frame length: '%1'\n").arg(QString::fromLatin1(lenLine)));
                    c->disconnectFromHost();
                    return;
                }

                // Read payload of declared length
                QByteArray payload = c->read(length);

                // Parse JSON
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
                if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                    dbg(QString::fromLatin1("[injectdll] Invalid JSON: %1\n").arg(parseError.errorString()));
                    c->disconnectFromHost();
                    return;
                }

                QJsonObject req = doc.object();
                int reqId = req.value("id").toInt(-1);
                QString method = req.value("method").toString();

                dbg(QString::fromLatin1("[injectdll] Request: id=%1, method=%2\n").arg(reqId).arg(method));

                if (method == "ping") {
                    handlePing(c, reqId);
                } else if (method == "app.info") {
                    handleAppInfo(c, reqId);
                } else if (method == "elements.roots") {
                    handleElementsRoots(c, reqId);
                } else if (method == "elements.children") {
                    // Expect: { "id": X, "method": "elements.children", "params": { "id": <parentId> } }
                    const QJsonObject params = req.value("params").toObject();
                    const int parentId = params.value("id").toInt(0);
                    handleElementsChildren(c, reqId, parentId);
                } else if (method == "elements.info") {
                    // Expect: { "id": X, "method": "elements.info", "params": { "id": <int> } }
                    const QJsonObject params = req.value("params").toObject();
                    const int targetId = params.value("id").toInt(0);
                    handleElementInfo(c, reqId, targetId);
                } else if (method == "elements.click") {
                    // Expect: { "id": X, "method": "elements.click", "params": { "id": <int> } }
                    const QJsonObject params = req.value("params").toObject();
                    const int targetId = params.value("id").toInt(0);
                    handleElementClick(c, reqId, targetId);
                } else if (method == "elements.setText") {
                    // Expect: { "id": X, "method": "elements.setText", "params": { "id": <int>, "text": "string" } }
                    const QJsonObject params = req.value("params").toObject();
                    const int targetId = params.value("id").toInt(0);
                    const QString text  = params.value("text").toString();
                    handleElementSetText(c, reqId, targetId, text);
                } else {
                    QJsonObject error;
                    error["error"] = QStringLiteral("Unknown method");
                    QJsonObject resp;
                    resp["id"] = reqId;
                    resp["result"] = error;
                    send_json(c, resp);
                }
            }
        });
    }
}

void QtHelloServer::sendJson(QTcpSocket* sock, const QJsonObject& obj) {
    QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray frame = QByteArray::number(payload.size()) + "\n" + payload;
    sock->write(frame);
    sock->flush();
}

void QtHelloServer::handlePing(QTcpSocket* sock, int requestId) {
    QJsonObject result;
    result["ok"] = true;
    result["pid"] = QCoreApplication::applicationPid();
    result["qt_version"] = QString::fromLatin1(qVersion());

    QJsonObject response;
    response["id"] = requestId;
    response["result"] = result;

    send_json(sock, response);
}

void QtHelloServer::handleAppInfo(QTcpSocket* sock, int requestId) {
    QJsonObject result;

    // Basic app/process info
    result["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
    result["app_name"] = QCoreApplication::applicationName();
    result["app_path"] = QCoreApplication::applicationFilePath();
    result["org_name"] = QCoreApplication::organizationName();
    result["org_domain"] = QCoreApplication::organizationDomain();
    result["version"] = QCoreApplication::applicationVersion();
    result["qt_version"] = QString::fromLatin1(qVersion());

    // Screens
    QJsonArray screensArr;
    const auto screens = QGuiApplication::screens();
    for (auto* sc : screens) {
        if (!sc) continue;
        const QRect g = sc->geometry();
        QJsonObject js;
        js["name"]   = sc->name();
        js["geom"]   = QJsonArray{ g.x(), g.y(), g.width(), g.height() };
        js["dpr"]    = sc->devicePixelRatio();
        js["dpi_logical"]  = sc->logicalDotsPerInch();
        js["dpi_physical"] = sc->physicalDotsPerInch();
        screensArr.push_back(js);
    }
    result["screens"] = screensArr;
    if (auto* pri = QGuiApplication::primaryScreen()) {
        result["primary_screen"] = pri->name();
    }

    // Build response
    QJsonObject resp;
    resp["id"]     = requestId;
    resp["result"] = result;

    sendJson(sock, resp);
}

int QtHelloServer::ensureIdFor(QObject* obj) {
    if (!obj) return 0;
    if (m_obj2id.contains(obj)) return m_obj2id.value(obj);

    const int id = m_nextId++;
    m_obj2id.insert(obj, id);
    m_id2obj.insert(id, QPointer<QObject>(obj));

    // Auto-drop when the QObject is destroyed
    connect(obj, &QObject::destroyed, this, [this, obj]() { dropIdFor(obj); });
    return id;
}

void QtHelloServer::dropIdFor(QObject* obj) {
    if (!obj) return;
    auto it = m_obj2id.find(obj);
    if (it == m_obj2id.end()) return;
    const int id = it.value();
    m_obj2id.erase(it);
    m_id2obj.remove(id);
}

QJsonObject QtHelloServer::summarizeTopLevel(QObject* obj) {
    QJsonObject j;
    if (!obj) return j;

    if (QWidget* w = qobject_cast<QWidget*>(obj)) {
        const int id = ensureIdFor(w);
        const QRect fr = w->frameGeometry();
        j["id"]           = id;
        j["name"]         = w->windowTitle();
        j["class"]        = w->metaObject()->className();
        j["control_type"] = control_type_for(w);
        j["rect"]         = rect_to_array(fr);
        j["visible"]      = w->isVisible();
        j["enabled"]      = w->isEnabled();
        j["auto_id"] = w->objectName(); 
        j["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
        return j;
    }

    if (QWindow* win = qobject_cast<QWindow*>(obj)) {
        const int id = ensureIdFor(win);
        const QRect fr = win->frameGeometry();
        j["id"]           = id;
        j["name"]         = win->title();
        j["class"]        = win->metaObject()->className();
        j["control_type"] = control_type_for(win);
        j["rect"]         = rect_to_array(fr);
        j["visible"]      = win->isVisible();
        j["enabled"]      = true; // QWindow has no enabled; treat as always enabled
        j["auto_id"] = win->objectName();
        j["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
        return j;
    }

    // Fallback (unknown top-level type)
    const int id = ensureIdFor(obj);
    j["id"]           = id;
    j["name"]         = QString(); // unknown
    j["class"]        = obj->metaObject()->className();
    j["control_type"] = control_type_for(obj);
    j["rect"]         = rect_to_array(QRect());
    j["visible"]      = true;
    j["enabled"]      = true;
    j["auto_id"] = obj->objectName();
    j["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
    return j;
}

void QtHelloServer::handleElementsRoots(QTcpSocket* sock, int requestId) {
    QJsonArray arr;

    // QWidget top-levels
    const auto widgetRoots = QApplication::topLevelWidgets();
    for (QWidget* w : widgetRoots) {
        if (!w) continue;
        arr.push_back(summarizeTopLevel(w));
    }

    // QWindow top-levels
    const auto windowRoots = QGuiApplication::topLevelWindows();
    for (QWindow* win : windowRoots) {
        if (!win) continue;
        arr.push_back(summarizeTopLevel(win));
    }

    QJsonObject resp;
    resp["id"]     = requestId;
    resp["result"] = arr;
    sendJson(sock, resp);
}

QObject* QtHelloServer::objectForId(int id) const {
    if (id <= 0) return NULL;
    QPointer<QObject> ptr = m_id2obj.value(id);
    return ptr ? ptr.data() : NULL;
}

QJsonObject QtHelloServer::summarizeObject(QObject* obj) {
    QJsonObject j;
    if (!obj) return j;

    if (QWidget* w = qobject_cast<QWidget*>(obj)) {
        const int id = ensureIdFor(static_cast<QObject*>(w));

        // Global rectangle
        const QPoint topLeft = w->mapToGlobal(QPoint(0, 0));
        const QSize  sz      = w->size();
        const QRect  gr(topLeft, sz);

        // title/text
        QString name = w->windowTitle();
        if (name.isEmpty()) {
            // If don't have windowTitle, use accessibleName or objectName as a fallback
            name = w->accessibleName();
            if (name.isEmpty())
                name = w->objectName();
        }

        j["id"]           = id;
        j["name"]         = name;
        j["class"]        = w->metaObject()->className();
        j["control_type"] = control_type_for(w);
        j["rect"]         = rect_to_array(gr);
        j["visible"]      = w->isVisible();
        j["enabled"]      = w->isEnabled();
        j["auto_id"] = w->objectName();
        j["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
        return j;
    }

    if (QWindow* win = qobject_cast<QWindow*>(obj)) {
        const int id = ensureIdFor(static_cast<QObject*>(win));
        const QRect fr = win->frameGeometry();
        j["id"] = id;
        j["name"] = win->title();
        j["class"] = win->metaObject()->className();
        j["control_type"] = control_type_for(win); 
        j["rect"] = rect_to_array(fr);
        j["visible"] = win->isVisible();
        j["enabled"] = true;
        j["auto_id"] = win->objectName();
        j["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
        return j;
    }

    // Fallback for unknown object types
    const int id = ensureIdFor(obj);
    j["id"]           = id;
    j["name"]         = obj->objectName();
    j["class"]        = obj->metaObject()->className();
    j["control_type"] = control_type_for(obj);
    j["rect"]         = rect_to_array(QRect());
    j["visible"]      = true;
    j["enabled"]      = true;
    j["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
    return j;
}

void QtHelloServer::handleElementsChildren(QTcpSocket* sock, int requestId, int parentId) {
    QJsonArray out;

    // 1) QGraphicsItem
    if (QGraphicsItem* parentGI = gitemForId(parentId)) {
        dbg(QString::fromLatin1("[injectdll] handleElementsChildren parentId %1 - QGraphicsItem branch\n")
                .arg(parentId));
        const auto kids = parentGI->childItems();
        for (QGraphicsItem* ch : kids) {
            if (!ch) continue;
            dbg(QString::fromLatin1("[injectdll] handleElementsChildren parentId %1 - QGraphicsItem branch add child\n")
                .arg(parentId));
            out.push_back(summarizeGraphicsItem(ch));
        }
        QJsonObject resp; resp["id"] = requestId; resp["result"] = out; sendJson(sock, resp);
        return;
    }

    // 2) QObject
    if (QObject* parent = objectForId(parentId)) {
        dbg(QString::fromLatin1("[injectdll] handleElementsChildren parentId %1 - QObject branch\n")
                .arg(parentId));
        QSet<QObject*> seen; // avoid duplicates when an object appears through multiple paths

        // 2.a) QWidget
        if (QWidget* pw = qobject_cast<QWidget*>(parent)) {
            const auto kids = pw->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
            for (QWidget* w : kids) {
                dbg(QString::fromLatin1("[injectdll] handleElementsChildren parentId %1 - QGraphicsItem branch add QWidget child\n")
                    .arg(parentId));
                if (!w) continue;
                out.push_back(summarizeObject(w));
                seen.insert(w);
            }

            // QWidget might be a QGraphicsView
            if (QGraphicsView* view = qobject_cast<QGraphicsView*>(pw)) {
                dbg(QString::fromLatin1("[injectdll] handleElementsChildren parentId %1 - QGraphicsItem branch can cast to QGraphicsView\n")
                    .arg(parentId));
                if (QGraphicsScene* sc = view->scene()) {
                    dbg(QString::fromLatin1("[injectdll] handleElementsChildren parentId %1 - QGraphicsItem branch can cast to QGraphicsView has scene\n")
                    .arg(parentId));
                    // Only top-level items (no parentItem)
                    const auto items = sc->items(Qt::SortOrder::AscendingOrder);
                    for (QGraphicsItem* gi : items) {
                        dbg(QString::fromLatin1("[injectdll] handleElementsChildren parentId %1 - QGraphicsItem branch can cast to QGraphicsView has scene add child\n")
                    .arg(parentId));
                        if (!gi || gi->parentItem()) continue;
                        out.push_back(summarizeGraphicsItem(gi));
                    }
                }
            }
        }

        // 2.b) QWindow
        if (QWindow* pwin = qobject_cast<QWindow*>(parent)) {
            dbg(QString::fromLatin1("[injectdll] handleElementsChildren parentId %1 - QWindow branch\n")
                .arg(parentId));
            const QObjectList kids = pwin->children();
            for (QObject* ch : kids) {
                if (QWindow* cw = qobject_cast<QWindow*>(ch)) {
                    out.push_back(summarizeObject(cw));
                    seen.insert(cw);
                }
            }
        }

        QJsonObject resp; resp["id"] = requestId; resp["result"] = out; sendJson(sock, resp);
        return;
    }

    // Unknown id error
    QJsonObject resp; resp["id"] = requestId;
    QJsonObject err;  err["code"] = -32602; err["message"] = "Invalid params: unknown id";
    resp["error"] = err;
    sendJson(sock, resp);
}

void QtHelloServer::handleElementInfo(QTcpSocket* sock, int requestId, int id) {
    QJsonObject resp;
    resp["id"] = requestId;

    // 1) QGraphicsView
    if (QGraphicsItem* gi = gitemForId(id)) {
        dbg(QString::fromLatin1("[injectdll] handleElementInfo id %1 - summarizeGraphicsItem branch\n")
                .arg(id));
        resp["result"] = summarizeGraphicsItem(gi);
        sendJson(sock, resp);
        return;
    }

    // 2) QObject
    if (QObject* obj = objectForId(id)) {
        dbg(QString::fromLatin1("[injectdll] handleElementInfo id %1 - summarizeObject branch\n")
                .arg(id));
        resp["result"] = summarizeObject(obj);
        sendJson(sock, resp);
        return;
    }

    QJsonObject err;
    err["code"] = -32602;
    err["message"] = QStringLiteral("Invalid params: unknown id");
    resp["error"] = err;
    sendJson(sock, resp);
    return;
}

int QtHelloServer::ensureIdForGItem(QGraphicsItem* it) {
    if (!it) return 0;
    auto itId = gmaps.gitem2id.find(it);
    if (itId != gmaps.gitem2id.end())
        return *itId;
    // start graphics ids far from QObject ids
    const int id = gmaps.nextId++;
    gmaps.gitem2id.insert(it, id);
    gmaps.id2gitem.insert(id, it);
    return id;
}

QGraphicsItem* QtHelloServer::gitemForId(int id) {
    auto it = gmaps.id2gitem.find(id);
    return (it == gmaps.id2gitem.end()) ? nullptr : it.value();
}

QJsonObject QtHelloServer::summarizeGraphicsItem(QGraphicsItem* gi) {
    QJsonObject j;
    if (!gi) return j;

    const int id = ensureIdForGItem(gi);
    j["id"] = id;
    j["name"] = QString(); // QGraphicsItem has no name
    j["class"] = QStringLiteral("QGraphicsItem");
    j["control_type"] = QStringLiteral("Pane");

    // Compute a best-effort screen rect using the first view of the item's scene (if any)
    QRect rScreen;
    if (QGraphicsScene* sc = gi->scene()) {
        const QList<QGraphicsView*> views = sc->views();
        if (!views.isEmpty()) {
            QGraphicsView* v = views.first();
            const QRectF sceneRect = gi->sceneBoundingRect();
            const QPolygon viewPoly = v->mapFromScene(sceneRect);
            const QRect viewRect = viewPoly.boundingRect();
            const QPoint globalTL = v->viewport()->mapToGlobal(viewRect.topLeft());
            rScreen = QRect(globalTL, viewRect.size());
        }
    }
    j["rect"] = rect_to_array(rScreen);
    j["visible"] = gi->isVisible();
    j["enabled"] = true; // no enabled state for plain QGraphicsItem
    return j;
}

void QtHelloServer::handleElementClick(QTcpSocket* sock, int requestId, int id) {
    QJsonObject resp; resp["id"] = requestId;

    dbg(QString::fromLatin1("[injectdll] handleElementClick id %1\n")
                .arg(id));

    auto ok = [&]() {
        QJsonObject r; r["ok"] = true;
        resp["result"] = r;
        sendJson(sock, resp);
    };
    auto err = [&](int code, const QString& msg) {
        QJsonObject e; e["code"] = code; e["message"] = msg;
        resp["error"] = e;
        sendJson(sock, resp);
    };

    // 1) QGraphicsItem
    if (QGraphicsItem* gi = gitemForId(id)) {
        // Only QGraphicsObject supports signals/slots; plain QGraphicsItem has no semantic click
        if (QGraphicsObject* go = gi->toGraphicsObject()) {
            // Try common names
            bool invoked =
                QMetaObject::invokeMethod(go, "click", Qt::QueuedConnection) ||
                QMetaObject::invokeMethod(go, "trigger", Qt::QueuedConnection) ||
                QMetaObject::invokeMethod(go, "clicked", Qt::QueuedConnection);
            if (invoked) return ok();
            return err(-32001, "GraphicsObject has no invokable click/trigger/clicked");
        }
        return err(-32000, "GraphicsItem is not a QObject; semantic click unsupported");
    }

    // 2) QObject
    if (QObject* obj = objectForId(id)) {
        // QAction: trigger
        if (QAction* act = qobject_cast<QAction*>(obj)) {
            QTimer::singleShot(0, act, &QAction::trigger);
            return ok();
        }
        // QAbstractButton: click
        if (QAbstractButton* btn = qobject_cast<QAbstractButton*>(obj)) {
            QTimer::singleShot(0, btn, &QAbstractButton::click);
            return ok();
        }
        // QComboBox: open popup as semantic "click"
        if (QComboBox* cb = qobject_cast<QComboBox*>(obj)) {
            QTimer::singleShot(0, cb, &QComboBox::showPopup);
            return ok();
        }
        // QTabBar: switch to current tab
        if (QTabBar* tb = qobject_cast<QTabBar*>(obj)) {
            int idx = tb->currentIndex();
            if (idx >= 0) {
                QTimer::singleShot(0, [tb, idx]() { tb->setCurrentIndex(idx); emit tb->tabBarClicked(idx); });
                return ok();
            }
            return err(-32006, "TabBar has no current index");
        }

        // Try common names
        bool invoked =
            QMetaObject::invokeMethod(obj, "click",   Qt::QueuedConnection) ||
            QMetaObject::invokeMethod(obj, "trigger", Qt::QueuedConnection) ||
            QMetaObject::invokeMethod(obj, "clicked", Qt::QueuedConnection);
        if (invoked) return ok();

        return err(-32008, "Unsupported object type for semantic click");
    }

    // 3) Unknown id
    return err(-32602, "Invalid params: unknown id");
}

void QtHelloServer::handleElementSetText(QTcpSocket* sock, int requestId, int id, const QString& text) {
    QJsonObject resp; resp["id"] = requestId;

    auto ok = [&]() {
        QJsonObject r; r["ok"] = true;
        resp["result"] = r;
        sendJson(sock, resp);
    };
    auto err = [&](int code, const QString& msg) {
        QJsonObject e; e["code"] = code; e["message"] = msg;
        resp["error"] = e;
        sendJson(sock, resp);
    };

    // 1) QGraphicsItem
    if (QGraphicsItem* gi = gitemForId(id)) {
        // Prefer QGraphicsObject path
        if (QGraphicsObject* go = gi->toGraphicsObject()) {
            if (setQObjectTextOrValue(go, text)) return ok();
            return err(-32021, "GraphicsObject has no writable text/value");
        }
        // Non-QObject items with text APIs
        if (auto gti = dynamic_cast<QGraphicsTextItem*>(gi)) {
            QTimer::singleShot(0, [gti, text]() { gti->setPlainText(text); });
            return ok();
        }
        if (auto sti = dynamic_cast<QGraphicsSimpleTextItem*>(gi)) {
            QTimer::singleShot(0, [sti, text]() { sti->setText(text); });
            return ok();
        }
        return err(-32020, "GraphicsItem not text-editable");
    }

    // 2) QObject
    if (QObject* obj = objectForId(id)) {
        if (setQObjectTextOrValue(obj, text)) return ok();
        return err(-32010, "Unsupported object type or read-only");
    }

    // 3) Unknown id
    return err(-32602, "Invalid params: unknown id");
}
