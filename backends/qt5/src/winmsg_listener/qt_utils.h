#pragma once

#include <QJsonArray>
#include <QJsonValue>
#include <QRect>
#include <QString>
#include <QVariant>

class QObject;

namespace QtBackend {

// Utility functions shared by Qt server actions.
void dbg(const QString& message);
int readEnvInt(const wchar_t* name, int defaultValue);
bool waitForGuiLoop(int totalMs, int pollMs);

QJsonArray rectToArray(const QRect& rect);
QJsonValue variantToJson(const QVariant& value);
QVariant jsonToVariant(const QJsonValue& value);
bool setQObjectTextOrValue(QObject* object, const QString& text);

} // namespace QtBackend
