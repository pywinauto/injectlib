#include "qt_element_summary.h"

#include "qt_utils.h"

#include <QAbstractButton>
#include <QAbstractSlider>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QRadioButton>
#include <QCheckBox>
#include <QTabBar>
#include <QTabWidget>
#include <QTableView>
#include <QTextEdit>
#include <QTreeView>
#include <QListView>
#include <QWidget>
#include <QWindow>
#include <QSpinBox>
#include <QDoubleSpinBox>

namespace QtBackend {

QString controlTypeFor(QObject* object) {
    if (!object) return QStringLiteral("Object");

    if (auto widget = qobject_cast<QWidget*>(object)) {
        if (qobject_cast<QCheckBox*>(widget)) return QStringLiteral("CheckBox");
        if (qobject_cast<QRadioButton*>(widget)) return QStringLiteral("RadioButton");
        if (qobject_cast<QAbstractButton*>(widget)) return QStringLiteral("Button");
        if (qobject_cast<QLineEdit*>(widget)) return QStringLiteral("Edit");
        if (qobject_cast<QPlainTextEdit*>(widget)) return QStringLiteral("Edit");
        if (qobject_cast<QTextEdit*>(widget)) return QStringLiteral("Edit");
        if (qobject_cast<QComboBox*>(widget)) return QStringLiteral("ComboBox");
        if (qobject_cast<QTabWidget*>(widget)) return QStringLiteral("TabControl");
        if (qobject_cast<QTabBar*>(widget)) return QStringLiteral("TabControl");
        if (qobject_cast<QListView*>(widget)) return QStringLiteral("List");
        if (qobject_cast<QTreeView*>(widget)) return QStringLiteral("Tree");
        if (qobject_cast<QTableView*>(widget)) return QStringLiteral("Table");
        if (qobject_cast<QAbstractSlider*>(widget)) return QStringLiteral("Slider");
        if (qobject_cast<QAbstractSpinBox*>(widget)) return QStringLiteral("Spinner");
        if (qobject_cast<QProgressBar*>(widget)) return QStringLiteral("ProgressBar");
        if (qobject_cast<QLabel*>(widget)) return QStringLiteral("Text");
        if (qobject_cast<QGroupBox*>(widget)) return QStringLiteral("GroupBox");
        return QStringLiteral("Pane");
    }
    if (qobject_cast<QWindow*>(object))
        return QStringLiteral("Window");

    return QStringLiteral("Object");
}

QString objectValueText(QObject* object) {
    if (!object) return QString();

    if (auto lineEdit = qobject_cast<QLineEdit*>(object)) return lineEdit->text();
    if (auto plainEdit = qobject_cast<QPlainTextEdit*>(object)) return plainEdit->toPlainText();
    if (auto textEdit = qobject_cast<QTextEdit*>(object)) return textEdit->toPlainText();
    if (auto combo = qobject_cast<QComboBox*>(object)) return combo->currentText();
    if (auto label = qobject_cast<QLabel*>(object)) return label->text();
    if (auto button = qobject_cast<QAbstractButton*>(object)) return button->text();
    if (auto spin = qobject_cast<QSpinBox*>(object)) return QString::number(spin->value());
    if (auto doubleSpin = qobject_cast<QDoubleSpinBox*>(object)) return QString::number(doubleSpin->value());
    if (auto slider = qobject_cast<QAbstractSlider*>(object)) return QString::number(slider->value());
    if (auto progress = qobject_cast<QProgressBar*>(object)) return QString::number(progress->value());
    if (auto tab = qobject_cast<QTabWidget*>(object)) return tab->tabText(tab->currentIndex());
    if (auto tabBar = qobject_cast<QTabBar*>(object)) return tabBar->tabText(tabBar->currentIndex());
    if (auto widget = qobject_cast<QWidget*>(object)) {
        if (!widget->windowTitle().isEmpty()) return widget->windowTitle();
        if (!widget->accessibleName().isEmpty()) return widget->accessibleName();
    }
    if (auto window = qobject_cast<QWindow*>(object)) return window->title();
    return object->objectName();
}

QString objectNameText(QObject* object) {
    if (!object) return QString();

    const QString valueText = objectValueText(object);
    if (!valueText.isEmpty())
        return valueText;
    return object->objectName();
}

QJsonObject summarizeTopLevel(QtObjectStore& store, QObject* object) {
    QJsonObject result;
    if (!object) return result;

    if (QWidget* widget = qobject_cast<QWidget*>(object)) {
        const int id = store.ensureIdFor(widget);
        const QRect frame = widget->frameGeometry();
        result["id"] = id;
        result["name"] = objectNameText(widget);
        result["class"] = widget->metaObject()->className();
        result["control_type"] = QStringLiteral("Window");
        result["rect"] = rectToArray(frame);
        result["visible"] = widget->isVisible();
        result["enabled"] = widget->isEnabled();
        result["auto_id"] = widget->objectName();
        result["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
        result["value"] = objectValueText(widget);
        return result;
    }

    if (QWindow* window = qobject_cast<QWindow*>(object)) {
        const int id = store.ensureIdFor(window);
        const QRect frame = window->frameGeometry();
        result["id"] = id;
        result["name"] = objectNameText(window);
        result["class"] = window->metaObject()->className();
        result["control_type"] = controlTypeFor(window);
        result["rect"] = rectToArray(frame);
        result["visible"] = window->isVisible();
        result["enabled"] = true;
        result["auto_id"] = window->objectName();
        result["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
        result["value"] = objectValueText(window);
        return result;
    }

    // Fallback for unknown top-level types.
    const int id = store.ensureIdFor(object);
    result["id"] = id;
    result["name"] = QString();
    result["class"] = object->metaObject()->className();
    result["control_type"] = controlTypeFor(object);
    result["rect"] = rectToArray(QRect());
    result["visible"] = true;
    result["enabled"] = true;
    result["auto_id"] = object->objectName();
    result["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
    result["value"] = objectValueText(object);
    return result;
}

QJsonObject summarizeObject(QtObjectStore& store, QObject* object) {
    QJsonObject result;
    if (!object) return result;

    if (QWidget* widget = qobject_cast<QWidget*>(object)) {
        const int id = store.ensureIdFor(widget);
        // Global rectangle.
        const QPoint topLeft = widget->mapToGlobal(QPoint(0, 0));
        const QRect globalRect(topLeft, widget->size());

        result["id"] = id;
        result["name"] = objectNameText(widget);
        result["class"] = widget->metaObject()->className();
        result["control_type"] = controlTypeFor(widget);
        result["rect"] = rectToArray(globalRect);
        result["visible"] = widget->isVisible();
        result["enabled"] = widget->isEnabled();
        result["auto_id"] = widget->objectName();
        result["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
        result["value"] = objectValueText(widget);
        return result;
    }

    if (QWindow* window = qobject_cast<QWindow*>(object)) {
        const int id = store.ensureIdFor(window);
        const QRect frame = window->frameGeometry();
        result["id"] = id;
        result["name"] = objectNameText(window);
        result["class"] = window->metaObject()->className();
        result["control_type"] = controlTypeFor(window);
        result["rect"] = rectToArray(frame);
        result["visible"] = window->isVisible();
        result["enabled"] = true;
        result["auto_id"] = window->objectName();
        result["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
        result["value"] = objectValueText(window);
        return result;
    }

    // Fallback for unknown object types.
    const int id = store.ensureIdFor(object);
    result["id"] = id;
    result["name"] = objectNameText(object);
    result["class"] = object->metaObject()->className();
    result["control_type"] = controlTypeFor(object);
    result["rect"] = rectToArray(QRect());
    result["visible"] = true;
    result["enabled"] = true;
    result["auto_id"] = object->objectName();
    result["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
    result["value"] = objectValueText(object);
    return result;
}

QJsonObject summarizeGraphicsItem(QtObjectStore& store, QGraphicsItem* item) {
    QJsonObject result;
    if (!item) return result;

    const int id = store.ensureIdForGItem(item);
    result["id"] = id;
    // QGraphicsItem has no QObject name.
    result["name"] = QString();
    result["class"] = QStringLiteral("QGraphicsItem");
    result["control_type"] = QStringLiteral("Pane");

    QRect screenRect;
    // Compute a best-effort screen rect using the first view of the item's scene.
    if (QGraphicsScene* scene = item->scene()) {
        const QList<QGraphicsView*> views = scene->views();
        if (!views.isEmpty()) {
            QGraphicsView* view = views.first();
            const QRectF sceneRect = item->sceneBoundingRect();
            const QPolygon viewPoly = view->mapFromScene(sceneRect);
            const QRect viewRect = viewPoly.boundingRect();
            const QPoint globalTopLeft = view->viewport()->mapToGlobal(viewRect.topLeft());
            screenRect = QRect(globalTopLeft, viewRect.size());
        }
    }
    result["rect"] = rectToArray(screenRect);
    result["visible"] = item->isVisible();
    // Plain QGraphicsItem has no enabled state.
    result["enabled"] = true;
    result["auto_id"] = QString();
    result["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
    if (auto textItem = dynamic_cast<QGraphicsTextItem*>(item))
        result["value"] = textItem->toPlainText();
    else if (auto simpleTextItem = dynamic_cast<QGraphicsSimpleTextItem*>(item))
        result["value"] = simpleTextItem->text();
    else
        result["value"] = QString();
    return result;
}

} // namespace QtBackend
