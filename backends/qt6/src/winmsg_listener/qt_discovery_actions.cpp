#include "qt_discovery_actions.h"

#include "qt_element_summary.h"
#include "qt_replies.h"
#include "qt_utils.h"

#include <QApplication>
#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGuiApplication>
#include <QJsonArray>
#include <QScreen>
#include <QSet>
#include <QWidget>
#include <QWindow>

namespace QtBackend {

QJsonObject handlePing() {
    QJsonObject result;
    result["ok"] = true;
    result["pid"] = QCoreApplication::applicationPid();
    result["qt_version"] = QString::fromLatin1(qVersion());
    return replyOk("value", result);
}

QJsonObject handleAppInfo() {
    QJsonObject result;

    // Basic app/process info.
    result["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
    result["app_name"] = QCoreApplication::applicationName();
    result["app_path"] = QCoreApplication::applicationFilePath();
    result["org_name"] = QCoreApplication::organizationName();
    result["org_domain"] = QCoreApplication::organizationDomain();
    result["version"] = QCoreApplication::applicationVersion();
    result["qt_version"] = QString::fromLatin1(qVersion());

    // Screens.
    QJsonArray screensArray;
    const auto screens = QGuiApplication::screens();
    for (auto* screen : screens) {
        if (!screen) continue;
        const QRect geometry = screen->geometry();
        QJsonObject js;
        js["name"] = screen->name();
        js["geom"] = rectToArray(geometry);
        js["dpr"] = screen->devicePixelRatio();
        js["dpi_logical"] = screen->logicalDotsPerInch();
        js["dpi_physical"] = screen->physicalDotsPerInch();
        screensArray.push_back(js);
    }
    result["screens"] = screensArray;
    if (auto* primary = QGuiApplication::primaryScreen())
        result["primary_screen"] = primary->name();

    return replyOk("value", result);
}

QJsonObject handleElementsRoots(QtObjectStore& store) {
    QJsonArray result;
    QSet<QObject*> seen;

    // QWidget top-levels.
    const auto widgetRoots = QApplication::topLevelWidgets();
    for (QWidget* widget : widgetRoots) {
        if (!widget) continue;
        if (widget->windowType() == Qt::Popup) continue;
        result.push_back(summarizeTopLevel(store, widget));
        seen.insert(widget);
        if (QWindow* window = widget->windowHandle())
            seen.insert(window);
    }

    // QWindow top-levels.
    const auto windowRoots = QGuiApplication::topLevelWindows();
    for (QWindow* window : windowRoots) {
        if (!window) continue;
        if (window->type() == Qt::Popup) continue;
        if (seen.contains(window)) continue;
        result.push_back(summarizeTopLevel(store, window));
        seen.insert(window);
    }

    return replyOk("value", result);
}

QJsonObject handleElementsChildren(QtObjectStore& store, int parentId) {
    QJsonArray result;

    // 1) QGraphicsItem.
    if (QGraphicsItem* parentItem = store.gitemForId(parentId)) {
        const auto children = parentItem->childItems();
        for (QGraphicsItem* child : children) {
            if (!child) continue;
            result.push_back(summarizeGraphicsItem(store, child));
        }
        return replyOk("value", result);
    }

    // 2) QObject.
    if (QObject* parent = store.objectForId(parentId)) {
        // Avoid duplicates when an object appears through multiple paths.
        QSet<QObject*> seen;

        // 2.a) QWidget.
        if (QWidget* parentWidget = qobject_cast<QWidget*>(parent)) {
            // TODO: expose more children with QModelIndex.
            const auto children = parentWidget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
            for (QWidget* widget : children) {
                if (!widget) continue;
                result.push_back(summarizeObject(store, widget));
                seen.insert(widget);
            }

            // QWidget might be a QGraphicsView.
            if (QGraphicsView* view = qobject_cast<QGraphicsView*>(parentWidget)) {
                if (QGraphicsScene* scene = view->scene()) {
                    // Only top-level items (no parentItem).
                    const auto items = scene->items(Qt::SortOrder::AscendingOrder);
                    for (QGraphicsItem* item : items) {
                        if (!item || item->parentItem()) continue;
                        result.push_back(summarizeGraphicsItem(store, item));
                    }
                }
            }
        }

        // 2.b) QWindow.
        if (QWindow* parentWindow = qobject_cast<QWindow*>(parent)) {
            const QObjectList children = parentWindow->children();
            for (QObject* child : children) {
                if (QWindow* childWindow = qobject_cast<QWindow*>(child)) {
                    result.push_back(summarizeObject(store, childWindow));
                    seen.insert(childWindow);
                }
            }
        }

        return replyOk("value", result);
    }

    return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));
}

QJsonObject handleElementInfo(QtObjectStore& store, int id) {
    if (QGraphicsItem* item = store.gitemForId(id))
        return replyOk("value", summarizeGraphicsItem(store, item));

    if (QObject* object = store.objectForId(id))
        return replyOk("value", summarizeObject(store, object));

    return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));
}

QJsonObject handleElementParent(QtObjectStore& store, int id) {
    if (QGraphicsItem* item = store.gitemForId(id)) {
        if (QGraphicsItem* parent = item->parentItem())
            return replyOk("value", summarizeGraphicsItem(store, parent));
        return replyOk("value", QJsonObject());
    }

    if (QObject* object = store.objectForId(id)) {
        if (QWidget* widget = qobject_cast<QWidget*>(object)) {
            if (QWidget* parentWidget = widget->parentWidget())
                return replyOk("value", summarizeObject(store, parentWidget));
        }
        if (QObject* parent = object->parent())
            return replyOk("value", summarizeObject(store, parent));
        return replyOk("value", QJsonObject());
    }

    return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));
}

QJsonObject handleFocusedElement(QtObjectStore& store) {
    if (QWidget* focused = QApplication::focusWidget())
        return replyOk("value", summarizeObject(store, focused));
    if (QWindow* focusedWindow = QGuiApplication::focusWindow())
        return replyOk("value", summarizeObject(store, focusedWindow));
    return replyOk("value", QJsonObject());
}

} // namespace QtBackend
