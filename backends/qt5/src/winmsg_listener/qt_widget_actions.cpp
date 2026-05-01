#include "qt_widget_actions.h"

#include "qt_replies.h"
#include "qt_utils.h"

#include <QAbstractButton>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QGraphicsObject>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsTextItem>
#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QJsonArray>
#include <QJsonValue>
#include <QMetaObject>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QTreeView>
#include <QWidget>
#include <QWindow>

namespace QtBackend {

namespace {

QAbstractItemView* tableViewForId(QtObjectStore& store, int id, QAbstractItemModel** model, QModelIndex* index, int row, int column) {
    QObject* object = store.objectForId(id);
    auto* view = qobject_cast<QAbstractItemView*>(object);
    if (!view)
        return nullptr;

    QAbstractItemModel* viewModel = view->model();
    if (!viewModel)
        return nullptr;

    QModelIndex modelIndex = viewModel->index(row, column);
    if (!modelIndex.isValid())
        return nullptr;

    if (model)
        *model = viewModel;
    if (index)
        *index = modelIndex;
    return view;
}

QJsonObject cellInfoForIndex(QAbstractItemView* view, const QModelIndex& index) {
    const QRect localRect = view->visualRect(index);
    const QPoint topLeft = view->viewport()->mapToGlobal(localRect.topLeft());
    const QRect globalRect(topLeft, localRect.size());
    const Qt::ItemFlags flags = index.flags();
    const QItemSelectionModel* selection = view->selectionModel();

    QJsonObject result;
    result["row"] = index.row();
    result["column"] = index.column();
    result["text"] = index.data(Qt::DisplayRole).toString();
    result["value"] = variantToJson(index.data(Qt::EditRole));
    result["rect"] = rectToArray(globalRect);
    result["enabled"] = bool(flags & Qt::ItemIsEnabled);
    result["editable"] = bool(flags & Qt::ItemIsEditable);
    result["selectable"] = bool(flags & Qt::ItemIsSelectable);
    result["selected"] = selection && selection->isSelected(index);
    return result;
}

void selectModelIndex(QAbstractItemView* view, const QModelIndex& index) {
    view->scrollTo(index);
    view->setCurrentIndex(index);
    if (QItemSelectionModel* selection = view->selectionModel())
        selection->select(index, QItemSelectionModel::ClearAndSelect);
}

QJsonObject resolveTreePath(QTreeView* tree, const QJsonArray& path, QModelIndex* index) {
    if (!tree)
        return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));

    QAbstractItemModel* model = tree->model();
    if (!model)
        return replyError(UNSUPPORTED_ACTION, QStringLiteral("Tree has no model"));
    if (path.isEmpty())
        return replyError(INVALID_VALUE, QStringLiteral("Tree item path must not be empty"));

    QModelIndex parent;
    QModelIndex current;
    for (const QJsonValue& segment : path) {
        if (segment.isDouble()) {
            const int row = segment.toInt(-1);
            if (row < 0 || row >= model->rowCount(parent))
                return replyError(INVALID_VALUE, QStringLiteral("Tree item row not found"));
            current = model->index(row, 0, parent);
        } else if (segment.isString()) {
            const QString text = segment.toString();
            current = QModelIndex();
            for (int row = 0; row < model->rowCount(parent); ++row) {
                const QModelIndex candidate = model->index(row, 0, parent);
                if (candidate.data(Qt::DisplayRole).toString() == text) {
                    current = candidate;
                    break;
                }
            }
            if (!current.isValid())
                return replyError(INVALID_VALUE, QStringLiteral("Tree item text not found"));
        } else {
            return replyError(INVALID_VALUE, QStringLiteral("Tree item path segments must be integers or strings"));
        }

        if (!current.isValid())
            return replyError(INVALID_VALUE, QStringLiteral("Tree item path not found"));
        parent = current;
    }

    *index = current;
    return QJsonObject();
}

} // namespace

QJsonObject handleElementSetFocus(QtObjectStore& store, int id) {
    if (QObject* object = store.objectForId(id)) {
        if (QWidget* widget = qobject_cast<QWidget*>(object)) {
            widget->setFocus(Qt::OtherFocusReason);
            if (widget->window())
                widget->window()->activateWindow();
            return replyOk("value", QJsonObject{{"ok", true}});
        }
        if (QWindow* window = qobject_cast<QWindow*>(object)) {
            window->requestActivate();
            return replyOk("value", QJsonObject{{"ok", true}});
        }
        return replyError(UNSUPPORTED_ACTION, QStringLiteral("Object cannot receive focus"));
    }

    return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));
}

QJsonObject handleGetItems(QtObjectStore& store, int id) {
    QObject* object = store.objectForId(id);
    if (!object)
        return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));

    QJsonArray items;
    if (auto combo = qobject_cast<QComboBox*>(object)) {
        for (int i = 0; i < combo->count(); ++i)
            items.push_back(combo->itemText(i));
        return replyOk("value", items);
    }
    if (auto tab = qobject_cast<QTabWidget*>(object)) {
        for (int i = 0; i < tab->count(); ++i)
            items.push_back(tab->tabText(i));
        return replyOk("value", items);
    }
    if (auto tabBar = qobject_cast<QTabBar*>(object)) {
        for (int i = 0; i < tabBar->count(); ++i)
            items.push_back(tabBar->tabText(i));
        return replyOk("value", items);
    }
    if (auto view = qobject_cast<QAbstractItemView*>(object)) {
        QAbstractItemModel* model = view->model();
        if (!model)
            return replyOk("value", items);
        for (int row = 0; row < model->rowCount(); ++row)
            items.push_back(model->index(row, 0).data(Qt::DisplayRole).toString());
        return replyOk("value", items);
    }

    return replyError(UNSUPPORTED_ACTION, QStringLiteral("Object has no item list"));
}

QJsonObject handleSelect(QtObjectStore& store, const QJsonObject& request) {
    const int id = request.value("element_id").toInt(0);
    QObject* object = store.objectForId(id);
    if (!object)
        return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));

    const bool hasIndex = request.contains("index");
    int index = request.value("index").toInt(-1);
    const QString text = request.value("text").toString();

    if (auto button = qobject_cast<QAbstractButton*>(object)) {
        button->setChecked(true);
        return replyOk("value", QJsonObject{{"ok", true}});
    }
    if (auto combo = qobject_cast<QComboBox*>(object)) {
        if (!hasIndex)
            index = combo->findText(text);
        if (index < 0 || index >= combo->count())
            return replyError(INVALID_VALUE, QStringLiteral("Combobox item not found"));
        combo->setCurrentIndex(index);
        return replyOk("value", QJsonObject{{"ok", true}});
    }
    if (auto tab = qobject_cast<QTabWidget*>(object)) {
        if (!hasIndex) {
            for (int i = 0; i < tab->count(); ++i) {
                if (tab->tabText(i) == text) {
                    index = i;
                    break;
                }
            }
        }
        if (index < 0 || index >= tab->count())
            return replyError(INVALID_VALUE, QStringLiteral("Tab item not found"));
        tab->setCurrentIndex(index);
        return replyOk("value", QJsonObject{{"ok", true}});
    }
    if (auto tabBar = qobject_cast<QTabBar*>(object)) {
        if (!hasIndex) {
            for (int i = 0; i < tabBar->count(); ++i) {
                if (tabBar->tabText(i) == text) {
                    index = i;
                    break;
                }
            }
        }
        if (index < 0 || index >= tabBar->count())
            return replyError(INVALID_VALUE, QStringLiteral("Tab item not found"));
        tabBar->setCurrentIndex(index);
        return replyOk("value", QJsonObject{{"ok", true}});
    }
    if (auto view = qobject_cast<QAbstractItemView*>(object)) {
        QAbstractItemModel* model = view->model();
        if (!model)
            return replyError(UNSUPPORTED_ACTION, QStringLiteral("View has no model"));
        if (!hasIndex) {
            for (int row = 0; row < model->rowCount(); ++row) {
                if (model->index(row, 0).data(Qt::DisplayRole).toString() == text) {
                    index = row;
                    break;
                }
            }
        }
        if (index < 0 || index >= model->rowCount())
            return replyError(INVALID_VALUE, QStringLiteral("View item not found"));
        const QModelIndex modelIndex = model->index(index, 0);
        view->setCurrentIndex(modelIndex);
        if (view->selectionModel())
            view->selectionModel()->select(modelIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        return replyOk("value", QJsonObject{{"ok", true}});
    }

    return replyError(UNSUPPORTED_ACTION, QStringLiteral("Object does not support selection"));
}

QJsonObject handleToggle(QtObjectStore& store, int id) {
    QObject* object = store.objectForId(id);
    if (!object)
        return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));

    if (auto button = qobject_cast<QAbstractButton*>(object)) {
        button->toggle();
        return replyOk("value", QJsonObject{{"ok", true}});
    }

    return replyError(UNSUPPORTED_ACTION, QStringLiteral("Object does not support toggle"));
}

QJsonObject handleExpand(QtObjectStore& store, const QJsonObject& request) {
    const int id = request.value("element_id").toInt(0);
    QObject* object = store.objectForId(id);
    if (!object)
        return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));

    if (auto combo = qobject_cast<QComboBox*>(object)) {
        combo->showPopup();
        return replyOk("value", QJsonObject{{"ok", true}});
    }
    if (auto tree = qobject_cast<QTreeView*>(object)) {
        if (request.contains("path")) {
            QModelIndex index;
            const QJsonObject error = resolveTreePath(tree, request.value("path").toArray(), &index);
            if (!error.isEmpty())
                return error;
            tree->expand(index);
            return replyOk("value", QJsonObject{{"ok", true}});
        }
        tree->expandToDepth(0);
        return replyOk("value", QJsonObject{{"ok", true}});
    }

    return replyError(UNSUPPORTED_ACTION, QStringLiteral("Object does not support expand"));
}

QJsonObject handleCollapse(QtObjectStore& store, const QJsonObject& request) {
    const int id = request.value("element_id").toInt(0);
    QObject* object = store.objectForId(id);
    if (!object)
        return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));

    if (auto combo = qobject_cast<QComboBox*>(object)) {
        combo->hidePopup();
        return replyOk("value", QJsonObject{{"ok", true}});
    }
    if (auto tree = qobject_cast<QTreeView*>(object)) {
        if (request.contains("path")) {
            QModelIndex index;
            const QJsonObject error = resolveTreePath(tree, request.value("path").toArray(), &index);
            if (!error.isEmpty())
                return error;
            tree->collapse(index);
            return replyOk("value", QJsonObject{{"ok", true}});
        }
        if (QAbstractItemModel* model = tree->model()) {
            for (int row = 0; row < model->rowCount(); ++row)
                tree->collapse(model->index(row, 0));
        }

        return replyOk("value", QJsonObject{{"ok", true}});
    }

    return replyError(UNSUPPORTED_ACTION, QStringLiteral("Object does not support collapse"));
}

QJsonObject handleIsExpanded(QtObjectStore& store, const QJsonObject& request) {
    const int id = request.value("element_id").toInt(0);
    QObject* object = store.objectForId(id);
    if (!object)
        return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));

    auto tree = qobject_cast<QTreeView*>(object);
    if (!tree)
        return replyError(UNSUPPORTED_ACTION, QStringLiteral("Object does not support expanded state"));
    if (!request.contains("path"))
        return replyError(MISSING_PARAM, QStringLiteral("Missing path"));

    QModelIndex index;
    const QJsonObject error = resolveTreePath(tree, request.value("path").toArray(), &index);
    if (!error.isEmpty())
        return error;
    return replyOk("value", tree->isExpanded(index));
}

QJsonObject handleGetItemText(QtObjectStore& store, const QJsonObject& request) {
    const int id = request.value("element_id").toInt(0);
    QObject* object = store.objectForId(id);
    if (!object)
        return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));

    auto tree = qobject_cast<QTreeView*>(object);
    if (!tree)
        return replyError(UNSUPPORTED_ACTION, QStringLiteral("Object does not support item text"));
    if (!request.contains("path"))
        return replyError(MISSING_PARAM, QStringLiteral("Missing path"));

    QModelIndex index;
    const QJsonObject error = resolveTreePath(tree, request.value("path").toArray(), &index);
    if (!error.isEmpty())
        return error;
    return replyOk("value", index.data(Qt::DisplayRole).toString());
}

QJsonObject handleGetSelection(QtObjectStore& store, int id) {
    QObject* object = store.objectForId(id);
    if (!object)
        return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));

    QJsonArray selection;
    if (auto combo = qobject_cast<QComboBox*>(object)) {
        selection.push_back(combo->currentText());
        return replyOk("value", selection);
    }
    if (auto view = qobject_cast<QAbstractItemView*>(object)) {
        if (view->selectionModel()) {
            const QModelIndexList selected = view->selectionModel()->selectedIndexes();
            for (const QModelIndex& index : selected)
                selection.push_back(index.data(Qt::DisplayRole).toString());
        }
        return replyOk("value", selection);
    }

    return replyError(UNSUPPORTED_ACTION, QStringLiteral("Object has no selection"));
}

QJsonObject handleElementClick(QtObjectStore& store, int id) {
    // 1) QGraphicsItem.
    if (QGraphicsItem* item = store.gitemForId(id)) {
        // Only QGraphicsObject supports signals/slots; plain QGraphicsItem has no semantic click.
        if (QGraphicsObject* object = item->toGraphicsObject()) {
            // Try common names.
            bool invoked =
                QMetaObject::invokeMethod(object, "click", Qt::QueuedConnection) ||
                QMetaObject::invokeMethod(object, "trigger", Qt::QueuedConnection) ||
                QMetaObject::invokeMethod(object, "clicked", Qt::QueuedConnection);
            if (invoked) return replyOk("value", QJsonObject{{"ok", true}});
            return replyError(UNSUPPORTED_ACTION, QStringLiteral("GraphicsObject has no invokable click/trigger/clicked"));
        }
        return replyError(UNSUPPORTED_ACTION, QStringLiteral("GraphicsItem is not a QObject; semantic click unsupported"));
    }

    // 2) QObject.
    if (QObject* object = store.objectForId(id)) {
        // QAction: trigger.
        if (QAction* action = qobject_cast<QAction*>(object)) {
            QTimer::singleShot(0, action, &QAction::trigger);
            return replyOk("value", QJsonObject{{"ok", true}});
        }
        // QAbstractButton: click.
        if (QAbstractButton* button = qobject_cast<QAbstractButton*>(object)) {
            QTimer::singleShot(0, button, &QAbstractButton::click);
            return replyOk("value", QJsonObject{{"ok", true}});
        }
        // QComboBox: open popup as semantic "click".
        if (QComboBox* combo = qobject_cast<QComboBox*>(object)) {
            QTimer::singleShot(0, combo, &QComboBox::showPopup);
            return replyOk("value", QJsonObject{{"ok", true}});
        }
        // QTabBar: switch to current tab.
        if (QTabBar* tabBar = qobject_cast<QTabBar*>(object)) {
            int index = tabBar->currentIndex();
            if (index >= 0) {
                QTimer::singleShot(0, [tabBar, index]() { tabBar->setCurrentIndex(index); emit tabBar->tabBarClicked(index); });
                return replyOk("value", QJsonObject{{"ok", true}});
            }
            return replyError(INVALID_VALUE, QStringLiteral("TabBar has no current index"));
        }

        // Try common names.
        bool invoked =
            QMetaObject::invokeMethod(object, "click", Qt::QueuedConnection) ||
            QMetaObject::invokeMethod(object, "trigger", Qt::QueuedConnection) ||
            QMetaObject::invokeMethod(object, "clicked", Qt::QueuedConnection);
        if (invoked) return replyOk("value", QJsonObject{{"ok", true}});

        return replyError(UNSUPPORTED_ACTION, QStringLiteral("Unsupported object type for semantic click"));
    }

    // 3) Unknown id.
    return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));
}

QJsonObject handleElementSetText(QtObjectStore& store, int id, const QString& text) {
    // 1) QGraphicsItem.
    if (QGraphicsItem* item = store.gitemForId(id)) {
        // Prefer QGraphicsObject path.
        if (QGraphicsObject* object = item->toGraphicsObject()) {
            if (setQObjectTextOrValue(object, text)) return replyOk("value", QJsonObject{{"ok", true}});
            return replyError(UNSUPPORTED_ACTION, QStringLiteral("GraphicsObject has no writable text/value"));
        }
        // Non-QObject items with text APIs.
        if (auto textItem = dynamic_cast<QGraphicsTextItem*>(item)) {
            QTimer::singleShot(0, [textItem, text]() { textItem->setPlainText(text); });
            return replyOk("value", QJsonObject{{"ok", true}});
        }
        if (auto simpleTextItem = dynamic_cast<QGraphicsSimpleTextItem*>(item)) {
            QTimer::singleShot(0, [simpleTextItem, text]() { simpleTextItem->setText(text); });
            return replyOk("value", QJsonObject{{"ok", true}});
        }
        return replyError(UNSUPPORTED_ACTION, QStringLiteral("GraphicsItem not text-editable"));
    }

    // 2) QObject.
    if (QObject* object = store.objectForId(id)) {
        if (setQObjectTextOrValue(object, text)) return replyOk("value", QJsonObject{{"ok", true}});
        return replyError(UNSUPPORTED_ACTION, QStringLiteral("Unsupported object type or read-only"));
    }

    // 3) Unknown id.
    return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));
}

QJsonObject handleGetCellInfo(QtObjectStore& store, int id, int row, int column) {
    QModelIndex index;
    QAbstractItemView* view = tableViewForId(store, id, nullptr, &index, row, column);
    if (!view)
        return replyError(NOT_FOUND, QStringLiteral("Invalid table cell: row %1, column %2").arg(row).arg(column));

    return replyOk("value", cellInfoForIndex(view, index));
}

QJsonObject handleSelectCell(QtObjectStore& store, int id, int row, int column) {
    QModelIndex index;
    QAbstractItemView* view = tableViewForId(store, id, nullptr, &index, row, column);
    if (!view)
        return replyError(NOT_FOUND, QStringLiteral("Invalid table cell: row %1, column %2").arg(row).arg(column));

    selectModelIndex(view, index);
    return replyOk("value", QJsonObject{{"ok", true}});
}

QJsonObject handleClickCell(QtObjectStore& store, int id, int row, int column) {
    QModelIndex index;
    QAbstractItemView* view = tableViewForId(store, id, nullptr, &index, row, column);
    if (!view)
        return replyError(NOT_FOUND, QStringLiteral("Invalid table cell: row %1, column %2").arg(row).arg(column));

    selectModelIndex(view, index);
    QMetaObject::invokeMethod(view, "clicked", Qt::QueuedConnection, Q_ARG(QModelIndex, index));
    return replyOk("value", QJsonObject{{"ok", true}});
}

QJsonObject handleSetCellValue(QtObjectStore& store, int id, int row, int column, const QJsonValue& value) {
    QAbstractItemModel* model = nullptr;
    QModelIndex index;
    QAbstractItemView* view = tableViewForId(store, id, &model, &index, row, column);
    if (!view || !model)
        return replyError(NOT_FOUND, QStringLiteral("Invalid table cell: row %1, column %2").arg(row).arg(column));

    if (!(index.flags() & Qt::ItemIsEditable))
        return replyError(UNSUPPORTED_ACTION, QStringLiteral("Table cell is not editable"));

    const QVariant variant = jsonToVariant(value);
    if (!model->setData(index, variant, Qt::EditRole))
        return replyError(UNSUPPORTED_ACTION, QStringLiteral("Table model rejected cell value"));

    return replyOk("value", cellInfoForIndex(view, index));
}

} // namespace QtBackend
