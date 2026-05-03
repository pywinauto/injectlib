#pragma once

#include "qt_object_store.h"

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace QtBackend {

QJsonObject handleGetProperty(QtObjectStore& store, int id, const QString& name);
QJsonObject handleSetProperty(QtObjectStore& store, int id, const QString& name, const QJsonValue& value);
QJsonObject handleInvokeMethod(QtObjectStore& store, int id, const QString& name);
QJsonObject handleGetValue(QtObjectStore& store, int id);
QJsonObject handleSetValue(QtObjectStore& store, int id, const QJsonValue& value);

} // namespace QtBackend
