#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>

#include <atomic>
#include <functional>
#include <thread>

#include <windows.h>

class PipeServer {
public:
    using RequestHandler = std::function<QJsonObject(const QJsonObject&)>;

    PipeServer(QString pipeName, RequestHandler handler);
    ~PipeServer();

    bool start();
    void stop();
    bool isRunning() const noexcept;

private:
    void run();
    bool readMessage(QByteArray& data);
    bool writeMessage(const QByteArray& data);
    void closeCurrentPipe();

    QString m_pipeName;
    // callback to handle incoming requests and generate responses
    RequestHandler m_handler;
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
    std::atomic<bool> m_running{false};
    HANDLE m_pipe{INVALID_HANDLE_VALUE};
};
