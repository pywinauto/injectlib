#pragma once

#include "qt_object_store.h"

#include <QJsonObject>

namespace QtBackend {

QJsonObject handlePing();
QJsonObject handleAppInfo();
QJsonObject handleElementsRoots(QtObjectStore& store);
QJsonObject handleElementsChildren(QtObjectStore& store, int parentId);
QJsonObject handleElementInfo(QtObjectStore& store, int id);
QJsonObject handleElementParent(QtObjectStore& store, int id);
QJsonObject handleFocusedElement(QtObjectStore& store);

} // namespace QtBackend
