#include "qt_replies.h"

namespace QtBackend {

QJsonObject replyOk() {
    QJsonObject reply;
    reply["status_code"] = OK;
    return reply;
}

QJsonObject replyOk(const char* key, const QJsonValue& value) {
    QJsonObject reply = replyOk();
    reply[key] = value;
    return reply;
}

QJsonObject replyError(int statusCode, const QString& message) {
    QJsonObject reply;
    reply["status_code"] = statusCode;
    reply["message"] = message;
    return reply;
}

} // namespace QtBackend
