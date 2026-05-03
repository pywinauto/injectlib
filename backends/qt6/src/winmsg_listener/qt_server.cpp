#include "qt_server.h"

#include "pipe_server.h"
#include "qt_discovery_actions.h"
#include "qt_property_actions.h"
#include "qt_replies.h"
#include "qt_utils.h"
#include "qt_widget_actions.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QMetaType>
#include <QThread>
#include <QTimer>

using namespace QtBackend;

namespace {

QtHelloServer* g_instance = nullptr;

} // namespace

QtHelloServer* QtHelloServer::instance() {
    if (!g_instance) {
        g_instance = new QtHelloServer();
    }
    return g_instance;
}

QtHelloServer::QtHelloServer(QObject* parent)
    : QObject(parent)
{
    // Ensure that QJsonObject can be used in queued connections.
    qRegisterMetaType<QJsonObject>("QJsonObject");
}

bool QtHelloServer::isRunning() const noexcept {
    return m_pipeServer && m_pipeServer->isRunning();
}

void QtHelloServer::bootstrap() {
    const int totalMs = readEnvInt(L"QT_INJECTED_WAIT_MS", 30000); // 30s default
    const int pollMs = readEnvInt(L"QT_INJECTED_POLL_MS", 100);    // 100ms default

    if (!waitForGuiLoop(totalMs, pollMs)) {
        dbg(QString::fromLatin1("[injectlib] GUI event loop not ready - not starting server (waited %1 ms)\n")
                .arg(totalMs));
        return;
    }

    QCoreApplication* app = QCoreApplication::instance();
    QtHelloServer* server = QtHelloServer::instance();

    if (server->thread() != app->thread())
        server->moveToThread(app->thread());

    dbg(QString::fromLatin1("[injectlib] GUI loop ready. Queueing pipe server start...\n"));
    QTimer::singleShot(0, server, SLOT(start()));
}

void QtHelloServer::shutdown() {
    // Clean stop on the GUI thread if we're running.
    QtHelloServer* server = QtHelloServer::instance();
    if (server->isRunning()) {
        QMetaObject::invokeMethod(server, "stop", Qt::QueuedConnection);
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
    // Run on the Qt thread and block until it returns.
    const bool invoked = QMetaObject::invokeMethod(this,
                                                   "processRequest",
                                                   Qt::BlockingQueuedConnection,
                                                   Q_RETURN_ARG(QJsonObject, reply),
                                                   Q_ARG(QJsonObject, request));
    if (!invoked)
        return replyError(RUNTIME_ERROR, QStringLiteral("Could not dispatch request to Qt thread"));
    return reply;
}

QJsonObject QtHelloServer::processRequest(const QJsonObject& request) {
    const QString action = request.value("action").toString();

    if (action == QStringLiteral("Ping"))
        return handlePing();
    if (action == QStringLiteral("GetAppInfo"))
        return handleAppInfo();
    if (action == QStringLiteral("GetRoots"))
        return handleElementsRoots(m_objectStore);
    if (action == QStringLiteral("GetChildren"))
        return handleElementsChildren(m_objectStore, request.value("element_id").toInt(0));
    if (action == QStringLiteral("GetElementInfo"))
        return handleElementInfo(m_objectStore, request.value("element_id").toInt(0));
    if (action == QStringLiteral("GetParent"))
        return handleElementParent(m_objectStore, request.value("element_id").toInt(0));
    if (action == QStringLiteral("GetFocusedElement"))
        return handleFocusedElement(m_objectStore);
    if (action == QStringLiteral("SetFocus"))
        return handleElementSetFocus(m_objectStore, request.value("element_id").toInt(0));
    if (action == QStringLiteral("GetProperty")) {
        if (!request.contains("name"))
            return replyError(MISSING_PARAM, QStringLiteral("Missing name"));
        return handleGetProperty(m_objectStore, request.value("element_id").toInt(0), request.value("name").toString());
    }
    if (action == QStringLiteral("SetProperty")) {
        if (!request.contains("name") || !request.contains("value"))
            return replyError(MISSING_PARAM, QStringLiteral("Missing name or value"));
        return handleSetProperty(m_objectStore,
                                 request.value("element_id").toInt(0),
                                 request.value("name").toString(),
                                 request.value("value"));
    }
    if (action == QStringLiteral("InvokeMethod")) {
        if (!request.contains("name"))
            return replyError(MISSING_PARAM, QStringLiteral("Missing name"));
        return handleInvokeMethod(m_objectStore, request.value("element_id").toInt(0), request.value("name").toString());
    }
    if (action == QStringLiteral("GetValue"))
        return handleGetValue(m_objectStore, request.value("element_id").toInt(0));
    if (action == QStringLiteral("SetValue")) {
        if (!request.contains("value"))
            return replyError(MISSING_PARAM, QStringLiteral("Missing value"));
        return handleSetValue(m_objectStore, request.value("element_id").toInt(0), request.value("value"));
    }
    if (action == QStringLiteral("GetItems"))
        return handleGetItems(m_objectStore, request.value("element_id").toInt(0));
    if (action == QStringLiteral("Select"))
        return handleSelect(m_objectStore, request);
    if (action == QStringLiteral("Toggle"))
        return handleToggle(m_objectStore, request.value("element_id").toInt(0));
    if (action == QStringLiteral("Expand"))
        return handleExpand(m_objectStore, request);
    if (action == QStringLiteral("Collapse"))
        return handleCollapse(m_objectStore, request);
    if (action == QStringLiteral("IsExpanded"))
        return handleIsExpanded(m_objectStore, request);
    if (action == QStringLiteral("GetItemText"))
        return handleGetItemText(m_objectStore, request);
    if (action == QStringLiteral("GetSelection"))
        return handleGetSelection(m_objectStore, request.value("element_id").toInt(0));
    if (action == QStringLiteral("Click"))
        return handleElementClick(m_objectStore, request.value("element_id").toInt(0));
    if (action == QStringLiteral("SetText")) {
        if (!request.contains("text"))
            return replyError(MISSING_PARAM, QStringLiteral("Missing text"));
        return handleElementSetText(m_objectStore, request.value("element_id").toInt(0), request.value("text").toString());
    }
    if (action == QStringLiteral("GetCellInfo")) {
        if (!request.contains("row") || !request.contains("column"))
            return replyError(MISSING_PARAM, QStringLiteral("Missing row or column"));
        return handleGetCellInfo(m_objectStore,
                                 request.value("element_id").toInt(0),
                                 request.value("row").toInt(-1),
                                 request.value("column").toInt(-1));
    }
    if (action == QStringLiteral("SelectCell")) {
        if (!request.contains("row") || !request.contains("column"))
            return replyError(MISSING_PARAM, QStringLiteral("Missing row or column"));
        return handleSelectCell(m_objectStore,
                                request.value("element_id").toInt(0),
                                request.value("row").toInt(-1),
                                request.value("column").toInt(-1));
    }
    if (action == QStringLiteral("ClickCell")) {
        if (!request.contains("row") || !request.contains("column"))
            return replyError(MISSING_PARAM, QStringLiteral("Missing row or column"));
        return handleClickCell(m_objectStore,
                               request.value("element_id").toInt(0),
                               request.value("row").toInt(-1),
                               request.value("column").toInt(-1));
    }
    if (action == QStringLiteral("SetCellValue")) {
        if (!request.contains("row") || !request.contains("column") || !request.contains("value"))
            return replyError(MISSING_PARAM, QStringLiteral("Missing row, column, or value"));
        return handleSetCellValue(m_objectStore,
                                  request.value("element_id").toInt(0),
                                  request.value("row").toInt(-1),
                                  request.value("column").toInt(-1),
                                  request.value("value"));
    }

    if (action.isEmpty())
        return replyError(PARSE_ERROR, QStringLiteral("Missing action"));
    return replyError(UNSUPPORTED_ACTION, QStringLiteral("Unsupported action: %1").arg(action));
}
