#pragma once

#include "qt_object_store.h"

#include <QJsonObject>
#include <QString>

namespace QtBackend {

QJsonObject handleElementSetFocus(QtObjectStore& store, int id);
QJsonObject handleGetItems(QtObjectStore& store, int id);
QJsonObject handleSelect(QtObjectStore& store, const QJsonObject& request);
QJsonObject handleToggle(QtObjectStore& store, int id);
QJsonObject handleExpand(QtObjectStore& store, const QJsonObject& request);
QJsonObject handleCollapse(QtObjectStore& store, const QJsonObject& request);
QJsonObject handleIsExpanded(QtObjectStore& store, const QJsonObject& request);
QJsonObject handleGetItemText(QtObjectStore& store, const QJsonObject& request);
QJsonObject handleGetSelection(QtObjectStore& store, int id);
QJsonObject handleElementClick(QtObjectStore& store, int id);
QJsonObject handleElementSetText(QtObjectStore& store, int id, const QString& text);
QJsonObject handleGetCellInfo(QtObjectStore& store, int id, int row, int column);
QJsonObject handleSelectCell(QtObjectStore& store, int id, int row, int column);
QJsonObject handleClickCell(QtObjectStore& store, int id, int row, int column);
QJsonObject handleSetCellValue(QtObjectStore& store, int id, int row, int column, const QJsonValue& value);

} // namespace QtBackend
