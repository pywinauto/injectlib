#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace QtBackend {

// Status codes used by injectlib's JSON pipe protocol.
enum ErrorCode {
    OK = 0,
    PARSE_ERROR = 1,
    UNSUPPORTED_ACTION = 2,
    MISSING_PARAM = 3,
    RUNTIME_ERROR = 4,
    NOT_FOUND = 5,
    INVALID_VALUE = 7,
};

// Response handlers.
QJsonObject replyOk();
QJsonObject replyOk(const char* key, const QJsonValue& value);
QJsonObject replyError(int statusCode, const QString& message);

} // namespace QtBackend
