#pragma once

#include "qt_object_store.h"

#include <QJsonObject>
#include <QString>

namespace QtBackend {

QJsonObject handleElementSetFocus(QtObjectStore& store, int id);
QJsonObject handleGetItems(QtObjectStore& store, int id);
QJsonObject handleSelect(QtObjectStore& store, const QJsonObject& request);
QJsonObject handleToggle(QtObjectStore& store, int id);
QJsonObject handleExpand(QtObjectStore& store, int id);
QJsonObject handleCollapse(QtObjectStore& store, int id);
QJsonObject handleGetSelection(QtObjectStore& store, int id);
QJsonObject handleElementClick(QtObjectStore& store, int id);
QJsonObject handleElementSetText(QtObjectStore& store, int id, const QString& text);

} // namespace QtBackend
