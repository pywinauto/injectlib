#include "qt_utils.h"

#include <QAbstractEventDispatcher>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <QThread>
#include <QTimeEdit>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdio>
#endif

#include <chrono>
#include <thread>

namespace QtBackend {

void dbg(const QString& message) {
#ifdef _WIN32
    OutputDebugStringW(reinterpret_cast<const wchar_t*>(message.utf16()));
#else
    fprintf(stderr, "%s", message.toUtf8().constData());
#endif
}

int readEnvInt(const wchar_t* name, int defaultValue) {
#ifdef _WIN32
    wchar_t buf[64];
    DWORD n = GetEnvironmentVariableW(name, buf, (DWORD)(sizeof(buf) / sizeof(buf[0])));
    if (n == 0 || n >= (DWORD)(sizeof(buf) / sizeof(buf[0])))
        return defaultValue;
    return _wtoi(buf);
#else
    const QString key = QString::fromWCharArray(name);
    const QByteArray value = qgetenv(key.toLatin1().constData());
    if (value.isEmpty())
        return defaultValue;
    bool ok = false;
    const int parsed = QString::fromLatin1(value).toInt(&ok);
    return ok ? parsed : defaultValue;
#endif
}

bool waitForGuiLoop(int totalMs, int pollMs) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(totalMs);
    while (std::chrono::steady_clock::now() < deadline) {
        QCoreApplication* app = QCoreApplication::instance();
        if (app && !QCoreApplication::startingUp() &&
            QAbstractEventDispatcher::instance(app->thread()) != nullptr) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(pollMs));
    }
    return false;
}

QJsonArray rectToArray(const QRect& rect) {
    return QJsonArray{ rect.x(), rect.y(), rect.width(), rect.height() };
}

QJsonValue variantToJson(const QVariant& value) {
    if (!value.isValid())
        return QJsonValue();

    switch (value.type()) {
    case QVariant::Bool:
        return value.toBool();
    case QVariant::Int:
    case QVariant::UInt:
        return value.toInt();
    case QVariant::LongLong:
    case QVariant::ULongLong:
        return static_cast<double>(value.toLongLong());
    case QVariant::Double:
        return value.toDouble();
    case QVariant::String:
    case QVariant::ByteArray:
    case QVariant::Char:
        return value.toString();
    default:
        return QJsonValue::fromVariant(value);
    }
}

QVariant jsonToVariant(const QJsonValue& value) {
    if (value.isBool())
        return value.toBool();
    if (value.isDouble()) {
        const double number = value.toDouble();
        const int integer = static_cast<int>(number);
        if (number == integer)
            return integer;
        return number;
    }
    if (value.isString())
        return value.toString();
    return value.toVariant();
}

bool setQObjectTextOrValue(QObject* object, const QString& text) {
    if (!object) return false;

    if (auto lineEdit = qobject_cast<QLineEdit*>(object)) {
        if (lineEdit->isReadOnly()) return false;
        QMetaObject::invokeMethod(lineEdit, [lineEdit, text]() { lineEdit->setText(text); }, Qt::QueuedConnection);
        return true;
    }

    if (auto plainEdit = qobject_cast<QPlainTextEdit*>(object)) {
        if (plainEdit->isReadOnly()) return false;
        QMetaObject::invokeMethod(plainEdit, [plainEdit, text]() { plainEdit->setPlainText(text); }, Qt::QueuedConnection);
        return true;
    }

    if (auto textEdit = qobject_cast<QTextEdit*>(object)) {
        if (textEdit->isReadOnly()) return false;
        QMetaObject::invokeMethod(textEdit, [textEdit, text]() { textEdit->setPlainText(text); }, Qt::QueuedConnection);
        return true;
    }

    if (auto combo = qobject_cast<QComboBox*>(object)) {
        if (!combo->isEditable()) return false;
        QMetaObject::invokeMethod(combo, [combo, text]() { combo->setEditText(text); }, Qt::QueuedConnection);
        return true;
    }

    if (auto spin = qobject_cast<QSpinBox*>(object)) {
        bool ok = false;
        int value = text.toInt(&ok);
        if (!ok) return false;
        QMetaObject::invokeMethod(spin, [spin, value]() { spin->setValue(value); }, Qt::QueuedConnection);
        return true;
    }

    if (auto doubleSpin = qobject_cast<QDoubleSpinBox*>(object)) {
        bool ok = false;
        double value = text.toDouble(&ok);
        if (!ok) return false;
        QMetaObject::invokeMethod(doubleSpin, [doubleSpin, value]() { doubleSpin->setValue(value); }, Qt::QueuedConnection);
        return true;
    }

    if (auto dateTimeEdit = qobject_cast<QDateTimeEdit*>(object)) {
        QDateTime value = QDateTime::fromString(text, Qt::ISODate);
        if (!value.isValid()) return false;
        QMetaObject::invokeMethod(dateTimeEdit, [dateTimeEdit, value]() { dateTimeEdit->setDateTime(value); }, Qt::QueuedConnection);
        return true;
    }

    if (auto dateEdit = qobject_cast<QDateEdit*>(object)) {
        QDate value = QDate::fromString(text, Qt::ISODate);
        if (!value.isValid()) value = QDate::fromString(text, dateEdit->displayFormat());
        if (!value.isValid()) return false;
        QMetaObject::invokeMethod(dateEdit, [dateEdit, value]() { dateEdit->setDate(value); }, Qt::QueuedConnection);
        return true;
    }

    if (auto timeEdit = qobject_cast<QTimeEdit*>(object)) {
        QTime value = QTime::fromString(text, Qt::ISODate);
        if (!value.isValid()) value = QTime::fromString(text, timeEdit->displayFormat());
        if (!value.isValid()) return false;
        QMetaObject::invokeMethod(timeEdit, [timeEdit, value]() { timeEdit->setTime(value); }, Qt::QueuedConnection);
        return true;
    }

    return QMetaObject::invokeMethod(object, "setText", Qt::QueuedConnection, Q_ARG(QString, text));
}

} // namespace QtBackend
