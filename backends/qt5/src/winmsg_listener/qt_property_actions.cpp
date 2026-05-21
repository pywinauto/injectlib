#include "qt_property_actions.h"

#include "qt_element_summary.h"
#include "qt_replies.h"
#include "qt_utils.h"

#include <QAbstractButton>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAbstractSlider>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsTextItem>
#include <QItemSelectionModel>
#include <QMetaObject>
#include <QMetaProperty>
#include <QProgressBar>
#include <QSpinBox>
#include <QTabBar>
#include <QTabWidget>
#include <QVariant>
#include <QWidget>

namespace QtBackend {

QJsonObject handleGetProperty(QtObjectStore& store, int id, const QString& name) {
    QObject* object = store.objectForId(id);
    if (!object)
        return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));

    if (auto combo = qobject_cast<QComboBox*>(object)) {
        if (name == QStringLiteral("count")) return replyOk("value", combo->count());
        if (name == QStringLiteral("currentIndex")) return replyOk("value", combo->currentIndex());
        if (name == QStringLiteral("currentText")) return replyOk("value", combo->currentText());
        if (name == QStringLiteral("editable")) return replyOk("value", combo->isEditable());
    }
    if (auto tab = qobject_cast<QTabWidget*>(object)) {
        if (name == QStringLiteral("count")) return replyOk("value", tab->count());
        if (name == QStringLiteral("currentIndex")) return replyOk("value", tab->currentIndex());
    }
    if (auto tabBar = qobject_cast<QTabBar*>(object)) {
        if (name == QStringLiteral("count")) return replyOk("value", tabBar->count());
        if (name == QStringLiteral("currentIndex")) return replyOk("value", tabBar->currentIndex());
    }
    if (auto button = qobject_cast<QAbstractButton*>(object)) {
        if (name == QStringLiteral("checked") || name == QStringLiteral("selected"))
            return replyOk("value", button->isChecked());
    }
    if (auto slider = qobject_cast<QAbstractSlider*>(object)) {
        if (name == QStringLiteral("minimum")) return replyOk("value", slider->minimum());
        if (name == QStringLiteral("maximum")) return replyOk("value", slider->maximum());
        if (name == QStringLiteral("value")) return replyOk("value", slider->value());
    }
    if (auto spin = qobject_cast<QSpinBox*>(object)) {
        if (name == QStringLiteral("minimum")) return replyOk("value", spin->minimum());
        if (name == QStringLiteral("maximum")) return replyOk("value", spin->maximum());
        if (name == QStringLiteral("value")) return replyOk("value", spin->value());
    }
    if (auto doubleSpin = qobject_cast<QDoubleSpinBox*>(object)) {
        if (name == QStringLiteral("minimum")) return replyOk("value", doubleSpin->minimum());
        if (name == QStringLiteral("maximum")) return replyOk("value", doubleSpin->maximum());
        if (name == QStringLiteral("value")) return replyOk("value", doubleSpin->value());
    }
    if (auto progress = qobject_cast<QProgressBar*>(object)) {
        if (name == QStringLiteral("minimum")) return replyOk("value", progress->minimum());
        if (name == QStringLiteral("maximum")) return replyOk("value", progress->maximum());
        if (name == QStringLiteral("value")) return replyOk("value", progress->value());
    }
    if (auto view = qobject_cast<QAbstractItemView*>(object)) {
        if (QAbstractItemModel* model = view->model()) {
            if (name == QStringLiteral("count") || name == QStringLiteral("rowCount"))
                return replyOk("value", model->rowCount());
            if (name == QStringLiteral("columnCount"))
                return replyOk("value", model->columnCount());
        }
        if (name == QStringLiteral("selected"))
            return replyOk("value", view->selectionModel() && view->selectionModel()->hasSelection());
    }
    if (auto widget = qobject_cast<QWidget*>(object)) {
        if (name == QStringLiteral("hasFocus")) return replyOk("value", widget->hasFocus());
        if (name == QStringLiteral("focusPolicy")) return replyOk("value", static_cast<int>(widget->focusPolicy()));
    }

    const QVariant value = object->property(name.toUtf8().constData());
    if (value.isValid())
        return replyOk("value", variantToJson(value));

    return replyError(NOT_FOUND, QStringLiteral("Property not found: %1").arg(name));
}

QJsonObject handleSetProperty(QtObjectStore& store, int id, const QString& name, const QJsonValue& value) {
    QObject* object = store.objectForId(id);
    if (!object)
        return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));

    const QVariant variant = jsonToVariant(value);

    if (auto button = qobject_cast<QAbstractButton*>(object)) {
        if (name == QStringLiteral("checked") || name == QStringLiteral("selected")) {
            button->setChecked(variant.toBool());
            return replyOk("value", QJsonObject{{"ok", true}});
        }
    }
    if (auto combo = qobject_cast<QComboBox*>(object)) {
        if (name == QStringLiteral("currentIndex")) {
            combo->setCurrentIndex(variant.toInt());
            return replyOk("value", QJsonObject{{"ok", true}});
        }
    }
    if (auto tab = qobject_cast<QTabWidget*>(object)) {
        if (name == QStringLiteral("currentIndex")) {
            tab->setCurrentIndex(variant.toInt());
            return replyOk("value", QJsonObject{{"ok", true}});
        }
    }
    if (auto tabBar = qobject_cast<QTabBar*>(object)) {
        if (name == QStringLiteral("currentIndex")) {
            tabBar->setCurrentIndex(variant.toInt());
            return replyOk("value", QJsonObject{{"ok", true}});
        }
    }

    QVariant converted = variant;
    const int propIndex = object->metaObject()->indexOfProperty(name.toUtf8().constData());
    if (propIndex >= 0) {
        QMetaProperty prop = object->metaObject()->property(propIndex);
        if (converted.canConvert(prop.userType()))
            converted.convert(prop.userType());
        if (prop.isWritable() && prop.write(object, converted))
            return replyOk("value", QJsonObject{{"ok", true}});
    }

    if (object->setProperty(name.toUtf8().constData(), converted))
        return replyOk("value", QJsonObject{{"ok", true}});

    return replyError(UNSUPPORTED_ACTION, QStringLiteral("Property is not writable: %1").arg(name));
}

QJsonObject handleInvokeMethod(QtObjectStore& store, int id, const QString& name) {
    QObject* object = store.objectForId(id);
    if (!object)
        return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));

    const QByteArray methodName = name.toUtf8();
    if (QMetaObject::invokeMethod(object, methodName.constData(), Qt::DirectConnection))
        return replyOk("value", QJsonObject{{"ok", true}});

    return replyError(UNSUPPORTED_ACTION, QStringLiteral("Method not found or unsupported: %1").arg(name));
}

QJsonObject handleGetValue(QtObjectStore& store, int id) {
    if (QGraphicsItem* item = store.gitemForId(id)) {
        if (auto textItem = dynamic_cast<QGraphicsTextItem*>(item))
            return replyOk("value", textItem->toPlainText());
        if (auto simpleTextItem = dynamic_cast<QGraphicsSimpleTextItem*>(item))
            return replyOk("value", simpleTextItem->text());
        return replyOk("value", QString());
    }

    if (QObject* object = store.objectForId(id))
        return replyOk("value", objectValueText(object));

    return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));
}

QJsonObject handleSetValue(QtObjectStore& store, int id, const QJsonValue& value) {
    const QVariant variant = jsonToVariant(value);

    if (QObject* object = store.objectForId(id)) {
        if (auto slider = qobject_cast<QAbstractSlider*>(object)) {
            slider->setValue(variant.toInt());
            return replyOk("value", QJsonObject{{"ok", true}});
        }
        if (auto spin = qobject_cast<QSpinBox*>(object)) {
            spin->setValue(variant.toInt());
            return replyOk("value", QJsonObject{{"ok", true}});
        }
        if (auto doubleSpin = qobject_cast<QDoubleSpinBox*>(object)) {
            doubleSpin->setValue(variant.toDouble());
            return replyOk("value", QJsonObject{{"ok", true}});
        }
        if (auto button = qobject_cast<QAbstractButton*>(object)) {
            button->setChecked(variant.toBool());
            return replyOk("value", QJsonObject{{"ok", true}});
        }
        if (setQObjectTextOrValue(object, variant.toString()))
            return replyOk("value", QJsonObject{{"ok", true}});
        return replyError(UNSUPPORTED_ACTION, QStringLiteral("Unsupported object type or read-only"));
    }

    if (QGraphicsItem* item = store.gitemForId(id)) {
        if (auto textItem = dynamic_cast<QGraphicsTextItem*>(item)) {
            textItem->setPlainText(variant.toString());
            return replyOk("value", QJsonObject{{"ok", true}});
        }
        if (auto simpleTextItem = dynamic_cast<QGraphicsSimpleTextItem*>(item)) {
            simpleTextItem->setText(variant.toString());
            return replyOk("value", QJsonObject{{"ok", true}});
        }
    }

    return replyError(NOT_FOUND, QStringLiteral("Invalid params: unknown id"));
}

} // namespace QtBackend
