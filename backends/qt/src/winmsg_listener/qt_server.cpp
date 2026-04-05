#include "qt_server.h"
#include "pipe_server.h"

#include <QJsonArray>
#include <QCoreApplication>
#include <QAbstractEventDispatcher>
#include <QMetaObject>
#include <QMetaType>
#include <QTimer>
#include <QThread>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include <QWidget>
#include <QWindow>
#include <QAbstractButton>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QLabel>
#include <QTabWidget>
#include <QTabBar>
#include <QListView>
#include <QTreeView>
#include <QTableView>
#include <QFrame>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QPointer>
#include <QAction>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QAbstractSpinBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateTimeEdit>

#include <windows.h>

#include <chrono>
#include <thread>

namespace {

// Status codes
constexpr int OK = 0;
constexpr int PARSE_ERROR = 1;
constexpr int UNSUPPORTED_ACTION = 2;
constexpr int MISSING_PARAM = 3;
constexpr int RUNTIME_ERROR = 4;
constexpr int NOT_FOUND = 5;
constexpr int INVALID_VALUE = 7;

// --------------- Response handlers----------------
QJsonObject reply_ok() {
    QJsonObject reply;
    reply["status_code"] = OK;
    return reply;
}

QJsonObject reply_ok(const char* key, const QJsonValue& value) {
    QJsonObject reply = reply_ok();
    reply[key] = value;
    return reply;
}

QJsonObject reply_error(int status_code, const QString& message) {
    QJsonObject reply;
    reply["status_code"] = status_code;
    reply["message"] = message;
    return reply;
}
// ---------------------------------------------------------------

// --------------- Utility functions ---------------
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

// map Qt class to pywinauto-friendly control type string
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
        if (!cb->isEditable()) return false;
        QMetaObject::invokeMethod(cb, [cb, text]() { cb->setEditText(text); }, Qt::QueuedConnection);
        return true;
    }

    if (auto sp = qobject_cast<QSpinBox*>(obj)) {
        bool ok = false;
        int v = text.toInt(&ok);
        if (!ok) return false;
        QMetaObject::invokeMethod(sp, [sp, v]() { sp->setValue(v); }, Qt::QueuedConnection);
        return true;
    }

    if (auto dsp = qobject_cast<QDoubleSpinBox*>(obj)) {
        bool ok = false;
        double v = text.toDouble(&ok);
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

    if (auto tme = qobject_cast<QTimeEdit*>(obj)) {
        QTime tv = QTime::fromString(text, Qt::ISODate);
        if (!tv.isValid()) tv = QTime::fromString(text, tme->displayFormat());
        if (!tv.isValid()) return false;
        QMetaObject::invokeMethod(tme, [tme, tv]() { tme->setTime(tv); }, Qt::QueuedConnection);
        return true;
    }

    return QMetaObject::invokeMethod(obj, "setText", Qt::QueuedConnection, Q_ARG(QString, text));
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
    : QObject(parent)
{
    // Ensure that QJsonObject can be used in queued connections
    qRegisterMetaType<QJsonObject>("QJsonObject");
}

bool QtHelloServer::isRunning() const noexcept {
    return m_pipeServer && m_pipeServer->isRunning();
}

void QtHelloServer::bootstrap() {
    const int totalMs = read_env_int(L"QT_INJECTED_WAIT_MS", 30000); // 30s default
    const int pollMs  = read_env_int(L"QT_INJECTED_POLL_MS", 100);   // 100ms default

    if (!wait_for_gui_loop(totalMs, pollMs)) {
        dbg(QString::fromLatin1("[injectlib] GUI event loop not ready — not starting server (waited %1 ms)\n")
                .arg(totalMs));
        return;
    }

    QCoreApplication* app = QCoreApplication::instance();
    QtHelloServer* srv = QtHelloServer::instance();

    if (srv->thread() != app->thread())
        srv->moveToThread(app->thread());

    dbg(QString::fromLatin1("[injectlib] GUI loop ready. Queueing pipe server start...\n"));
    QTimer::singleShot(0, srv, SLOT(start()));
}

void QtHelloServer::shutdown() {
    // Clean stop on the GUI thread if we're running.
    QtHelloServer* srv = QtHelloServer::instance();
    if (srv->isRunning()) {
        QMetaObject::invokeMethod(srv, "stop", Qt::QueuedConnection);
    }
}

void QtHelloServer::start() {
    if (isRunning()) {
        dbg(QString::fromLatin1("[injectlib] Qt pipe server already running.\n"));
        return;
    }

    const QString pipeName = QStringLiteral("process_%1").arg(QCoreApplication::applicationPid());
    m_pipeServer = std::make_unique<PipeServer>(pipeName, [this](const QJsonObject& request) {
        return processRequestThreadSafe(request);
    });
    m_pipeServer->start();
    dbg(QString::fromLatin1("[injectlib] Qt pipe server started: %1\n").arg(pipeName));
    emit started();
}

void QtHelloServer::stop() {
    if (!m_pipeServer)
        return;

    m_pipeServer->stop();
    m_pipeServer.reset();
    dbg(QString::fromLatin1("[injectlib] Qt pipe server stopped.\n"));
    emit stopped();
}

QJsonObject QtHelloServer::processRequestThreadSafe(const QJsonObject& request) {
    if (QThread::currentThread() == thread())
        return processRequest(request);

    QJsonObject reply;
    // run on Qt thread and block until it returns
    bool invoked = QMetaObject::invokeMethod(this,
                                             "processRequest",
                                             Qt::BlockingQueuedConnection,
                                             Q_RETURN_ARG(QJsonObject, reply),
                                             Q_ARG(QJsonObject, request));
    if (!invoked)
        return reply_error(RUNTIME_ERROR, QStringLiteral("Could not dispatch request to Qt thread"));
    return reply;
}

QJsonObject QtHelloServer::processRequest(const QJsonObject& request) {
    const QString action = request.value("action").toString();

    if (action == QStringLiteral("Ping"))
        return handlePing();
    if (action == QStringLiteral("GetAppInfo"))
        return handleAppInfo();
    if (action == QStringLiteral("GetRoots"))
        return handleElementsRoots();
    if (action == QStringLiteral("GetChildren"))
        return handleElementsChildren(request.value("element_id").toInt(0));
    if (action == QStringLiteral("GetElementInfo"))
        return handleElementInfo(request.value("element_id").toInt(0));
    if (action == QStringLiteral("Click"))
        return handleElementClick(request.value("element_id").toInt(0));
    if (action == QStringLiteral("SetText")) {
        if (!request.contains("text"))
            return reply_error(MISSING_PARAM, QStringLiteral("Missing text"));
        return handleElementSetText(request.value("element_id").toInt(0), request.value("text").toString());
    }

    if (action.isEmpty())
        return reply_error(PARSE_ERROR, QStringLiteral("Missing action"));
    return reply_error(UNSUPPORTED_ACTION, QStringLiteral("Unsupported action: %1").arg(action));
}

QJsonObject QtHelloServer::handlePing() {
    QJsonObject result;
    result["ok"] = true;
    result["pid"] = QCoreApplication::applicationPid();
    result["qt_version"] = QString::fromLatin1(qVersion());
    return reply_ok("value", result);
}

QJsonObject QtHelloServer::handleAppInfo() {
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
    return reply_ok("value", result);
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

QJsonObject QtHelloServer::handleElementsRoots() {
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

    return reply_ok("value", arr);
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
        j["auto_id"]      = w->objectName();
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
    j["auto_id"]      = obj->objectName();
    j["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
    return j;
}

QJsonObject QtHelloServer::handleElementsChildren(int parentId) {
    QJsonArray out;

    // 1) QGraphicsItem
    if (QGraphicsItem* parentGI = gitemForId(parentId)) {
        const auto kids = parentGI->childItems();
        for (QGraphicsItem* ch : kids) {
            if (!ch) continue;
            out.push_back(summarizeGraphicsItem(ch));
        }
        return reply_ok("value", out);
    }

    // 2) QObject
    if (QObject* parent = objectForId(parentId)) {
        QSet<QObject*> seen; // avoid duplicates when an object appears through multiple paths

        // 2.a) QWidget
        if (QWidget* pw = qobject_cast<QWidget*>(parent)) {
            const auto kids = pw->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
            for (QWidget* w : kids) {
                if (!w) continue;
                out.push_back(summarizeObject(w));
                seen.insert(w);
            }

            // QWidget might be a QGraphicsView
            if (QGraphicsView* view = qobject_cast<QGraphicsView*>(pw)) {
                if (QGraphicsScene* sc = view->scene()) {
                    // Only top-level items (no parentItem)
                    const auto items = sc->items(Qt::SortOrder::AscendingOrder);
                    for (QGraphicsItem* gi : items) {
                        if (!gi || gi->parentItem()) continue;
                        out.push_back(summarizeGraphicsItem(gi));
                    }
                }
            }
        }

        // 2.b) QWindow
        if (QWindow* pwin = qobject_cast<QWindow*>(parent)) {
            const QObjectList kids = pwin->children();
            for (QObject* ch : kids) {
                if (QWindow* cw = qobject_cast<QWindow*>(ch)) {
                    out.push_back(summarizeObject(cw));
                    seen.insert(cw);
                }
            }
        }

        return reply_ok("value", out);
    }

    return reply_error(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));
}

QJsonObject QtHelloServer::handleElementInfo(int id) {
    // 1) QGraphicsView
    if (QGraphicsItem* gi = gitemForId(id))
        return reply_ok("value", summarizeGraphicsItem(gi));

    // 2) QObject
    if (QObject* obj = objectForId(id))
        return reply_ok("value", summarizeObject(obj));

    return reply_error(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));
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
    j["auto_id"] = QString();
    j["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
    return j;
}

QJsonObject QtHelloServer::handleElementClick(int id) {
    // 1) QGraphicsItem
    if (QGraphicsItem* gi = gitemForId(id)) {
        // Only QGraphicsObject supports signals/slots; plain QGraphicsItem has no semantic click
        if (QGraphicsObject* go = gi->toGraphicsObject()) {
            // Try common names
            bool invoked =
                QMetaObject::invokeMethod(go, "click", Qt::QueuedConnection) ||
                QMetaObject::invokeMethod(go, "trigger", Qt::QueuedConnection) ||
                QMetaObject::invokeMethod(go, "clicked", Qt::QueuedConnection);
            if (invoked) return reply_ok("value", QJsonObject{{"ok", true}});
            return reply_error(UNSUPPORTED_ACTION, QStringLiteral("GraphicsObject has no invokable click/trigger/clicked"));
        }
        return reply_error(UNSUPPORTED_ACTION, QStringLiteral("GraphicsItem is not a QObject; semantic click unsupported"));
    }

    // 2) QObject
    if (QObject* obj = objectForId(id)) {
        // QAction: trigger
        if (QAction* act = qobject_cast<QAction*>(obj)) {
            QTimer::singleShot(0, act, &QAction::trigger);
            return reply_ok("value", QJsonObject{{"ok", true}});
        }
        // QAbstractButton: click
        if (QAbstractButton* btn = qobject_cast<QAbstractButton*>(obj)) {
            QTimer::singleShot(0, btn, &QAbstractButton::click);
            return reply_ok("value", QJsonObject{{"ok", true}});
        }
        // QComboBox: open popup as semantic "click"
        if (QComboBox* cb = qobject_cast<QComboBox*>(obj)) {
            QTimer::singleShot(0, cb, &QComboBox::showPopup);
            return reply_ok("value", QJsonObject{{"ok", true}});
        }
        // QTabBar: switch to current tab
        if (QTabBar* tb = qobject_cast<QTabBar*>(obj)) {
            int idx = tb->currentIndex();
            if (idx >= 0) {
                QTimer::singleShot(0, [tb, idx]() { tb->setCurrentIndex(idx); emit tb->tabBarClicked(idx); });
                return reply_ok("value", QJsonObject{{"ok", true}});
            }
            return reply_error(INVALID_VALUE, QStringLiteral("TabBar has no current index"));
        }

        // Try common names
        bool invoked =
            QMetaObject::invokeMethod(obj, "click",   Qt::QueuedConnection) ||
            QMetaObject::invokeMethod(obj, "trigger", Qt::QueuedConnection) ||
            QMetaObject::invokeMethod(obj, "clicked", Qt::QueuedConnection);
        if (invoked) return reply_ok("value", QJsonObject{{"ok", true}});

        return reply_error(UNSUPPORTED_ACTION, QStringLiteral("Unsupported object type for semantic click"));
    }

    // 3) Unknown id
    return reply_error(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));
}

QJsonObject QtHelloServer::handleElementSetText(int id, const QString& text) {
    // 1) QGraphicsItem
    if (QGraphicsItem* gi = gitemForId(id)) {
        // Prefer QGraphicsObject path
        if (QGraphicsObject* go = gi->toGraphicsObject()) {
            if (setQObjectTextOrValue(go, text)) return reply_ok("value", QJsonObject{{"ok", true}});
            return reply_error(UNSUPPORTED_ACTION, QStringLiteral("GraphicsObject has no writable text/value"));
        }
        // Non-QObject items with text APIs
        if (auto gti = dynamic_cast<QGraphicsTextItem*>(gi)) {
            QTimer::singleShot(0, [gti, text]() { gti->setPlainText(text); });
            return reply_ok("value", QJsonObject{{"ok", true}});
        }
        if (auto sti = dynamic_cast<QGraphicsSimpleTextItem*>(gi)) {
            QTimer::singleShot(0, [sti, text]() { sti->setText(text); });
            return reply_ok("value", QJsonObject{{"ok", true}});
        }
        return reply_error(UNSUPPORTED_ACTION, QStringLiteral("GraphicsItem not text-editable"));
    }

    // 2) QObject
    if (QObject* obj = objectForId(id)) {
        if (setQObjectTextOrValue(obj, text)) return reply_ok("value", QJsonObject{{"ok", true}});
        return reply_error(UNSUPPORTED_ACTION, QStringLiteral("Unsupported object type or read-only"));
    }

    // 3) Unknown id
    return reply_error(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));
}
