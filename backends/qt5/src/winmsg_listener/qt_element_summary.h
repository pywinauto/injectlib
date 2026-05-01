#pragma once

#include "qt_object_store.h"

#include <QJsonObject>
#include <QString>

class QObject;
class QGraphicsItem;

namespace QtBackend {

// Map Qt classes to pywinauto-friendly control type strings.
QString controlTypeFor(QObject* object);
QString objectValueText(QObject* object);
QString objectNameText(QObject* object);

// Summarizers convert Qt runtime objects to the JSON shape consumed by QtElementInfo.
QJsonObject summarizeTopLevel(QtObjectStore& store, QObject* object);
QJsonObject summarizeObject(QtObjectStore& store, QObject* object);
QJsonObject summarizeGraphicsItem(QtObjectStore& store, QGraphicsItem* item);

} // namespace QtBackend
