#include "pipe_server.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonParseError>

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#include <utility>

namespace {
// status codes:
// 0 OK
// 1 PARSE_ERROR
// 2 UNSUPPORTED_ACTION
// 3 MISSING_PARAM,
// 4 RUNTIME_ERROR
// 5 NOT_FOUND
// 6 UNSUPPORTED_TYPE
// 7 INVALID_VALUE.
QJsonObject errorReply(int statusCode, const QString& message) {
    QJsonObject reply;
    reply["status_code"] = statusCode;
    reply["message"] = message;
    return reply;
}

} // namespace

PipeServer::PipeServer(QString pipeName, RequestHandler handler)
    : m_pipeName(std::move(pipeName)),
      m_handler(std::move(handler))
{
}

PipeServer::~PipeServer() {
    stop();
}

bool PipeServer::start() {
    if (m_running)
        return true;

    m_stop = false;
    m_thread = std::thread([this]() { run(); });
    return true;
}

void PipeServer::stop() {
    m_stop = true;

#ifdef _WIN32
    closeCurrentPipe();

    const QString fullName = QStringLiteral("\\\\.\\pipe\\%1").arg(m_pipeName);
    HANDLE client = CreateFileW(reinterpret_cast<const wchar_t*>(fullName.utf16()),
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                nullptr,
                                OPEN_EXISTING,
                                0,
                                nullptr);
    if (client != INVALID_HANDLE_VALUE)
        CloseHandle(client);

    if (m_thread.joinable())
        m_thread.join();
#else
    closeCurrentPipe();

    if (!m_socketPath.empty()) {
        int client = socket(AF_UNIX, SOCK_STREAM, 0);
        if (client >= 0) {
            sockaddr_un addr;
            std::memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            std::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path) - 1);
            connect(client, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
            close(client);
        }
    }

    if (m_thread.joinable())
        m_thread.join();
#endif
}

bool PipeServer::isRunning() const noexcept {
    return m_running;
}

void PipeServer::run() {
    m_running = true;
#ifdef _WIN32
    const QString fullName = QStringLiteral("\\\\.\\pipe\\%1").arg(m_pipeName);

    SECURITY_DESCRIPTOR securityDescriptor;
    InitializeSecurityDescriptor(&securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    // Allow all access to the pipe for everyone
    SetSecurityDescriptorDacl(&securityDescriptor, TRUE, nullptr, FALSE);

    // Set up the security attributes for the pipe
    SECURITY_ATTRIBUTES securityAttributes;
    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.lpSecurityDescriptor = &securityDescriptor;
    // Do not allow the handle to be inherited by child processes
    securityAttributes.bInheritHandle = FALSE;

    while (!m_stop) {
        m_pipe = CreateNamedPipeW(reinterpret_cast<const wchar_t*>(fullName.utf16()),
                                  // Allow read and write
                                  PIPE_ACCESS_DUPLEX,
                                  // Message type pipe, blocking mode, and wait for the client to connect
                                  PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                  // Allow up to 1 instance of the pipe
                                  1,
                                  // Output and input buffer sizes
                                  64 * 1024,
                                  64 * 1024,
                                  // Default timeout 50 milliseconds
                                  0,
                                  &securityAttributes);
        if (m_pipe == INVALID_HANDLE_VALUE)
            break;

        BOOL connected = ConnectNamedPipe(m_pipe, nullptr)
            ? TRUE
            : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!connected) {
            closeCurrentPipe();
            continue;
        }

        while (!m_stop) {
            QByteArray data;
            if (!readMessage(data))
                break;

            if (data == "disconnect")
                break;
            if (data == "shutdown") {
                m_stop = true;
                break;
            }

            QJsonParseError parseError;
            QJsonDocument requestDoc = QJsonDocument::fromJson(data, &parseError);
            QJsonObject reply;
            if (parseError.error != QJsonParseError::NoError || !requestDoc.isObject()) {
                reply = errorReply(1, parseError.errorString());
            } else if (m_handler) {
                reply = m_handler(requestDoc.object());
            } else {
                reply = errorReply(4, QStringLiteral("Request handler callback was not provided"));
            }

            const QByteArray response = QJsonDocument(reply).toJson(QJsonDocument::Compact);
            if (!writeMessage(response))
                break;
        }

        // FlushFileBuffers function does not return until the client process has read all the data.
        FlushFileBuffers(m_pipe);
        // Close pipe for client
        DisconnectNamedPipe(m_pipe);
        // Finally close handle
        closeCurrentPipe();
    }

    closeCurrentPipe();
    m_running = false;
}
#else
    m_socketPath = QStringLiteral("/tmp/injectlib_qt_%1_%2.sock")
        .arg(static_cast<unsigned long long>(getuid()))
        .arg(m_pipeName.mid(QStringLiteral("process_").size()))
        .toStdString();

    unlink(m_socketPath.c_str());
    m_serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_serverFd < 0) {
        m_running = false;
        return;
    }

    sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(m_serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 ||
        listen(m_serverFd, 8) != 0) {
        closeCurrentPipe();
        unlink(m_socketPath.c_str());
        m_running = false;
        return;
    }
    chmod(m_socketPath.c_str(), 0600);

    while (!m_stop) {
        m_clientFd = accept(m_serverFd, nullptr, nullptr);
        if (m_clientFd < 0) {
            if (errno == EINTR)
                continue;
            break;
        }

        QByteArray data;
        if (readMessage(data) && !data.isEmpty()) {
            QJsonParseError parseError;
            QJsonDocument requestDoc = QJsonDocument::fromJson(data, &parseError);
            QJsonObject reply;
            if (parseError.error != QJsonParseError::NoError || !requestDoc.isObject()) {
                reply = errorReply(1, parseError.errorString());
            } else if (m_handler) {
                reply = m_handler(requestDoc.object());
            } else {
                reply = errorReply(4, QStringLiteral("Request handler callback was not provided"));
            }

            const QByteArray response = QJsonDocument(reply).toJson(QJsonDocument::Compact);
            writeMessage(response);
        }

        close(m_clientFd);
        m_clientFd = -1;
    }

    closeCurrentPipe();
    unlink(m_socketPath.c_str());
    m_running = false;
}
#endif

bool PipeServer::readMessage(QByteArray& data) {
    data.clear();
    char buffer[4096];

#ifdef _WIN32
    while (!m_stop) {
        DWORD bytesRead = 0;
        BOOL ok = ReadFile(m_pipe, buffer, sizeof(buffer), &bytesRead, nullptr);
        if (bytesRead > 0)
            data.append(buffer, static_cast<int>(bytesRead));

        if (ok)
            return true;

        const DWORD err = GetLastError();
        if (err == ERROR_MORE_DATA)
            continue;
        return false;
    }

    return false;
#else
    while (!m_stop) {
        const ssize_t bytesRead = recv(m_clientFd, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            data.append(buffer, static_cast<int>(bytesRead));
            continue;
        }
        if (bytesRead == 0)
            return true;
        if (errno == EINTR)
            continue;
        return false;
    }
    return false;
#endif
}

bool PipeServer::writeMessage(const QByteArray& data) {
#ifdef _WIN32
    DWORD bytesWritten = 0;
    const BOOL ok = WriteFile(m_pipe,
                              data.constData(),
                              static_cast<DWORD>(data.size()),
                              &bytesWritten,
                              nullptr);
    if (!ok || bytesWritten != static_cast<DWORD>(data.size()))
        return false;

    // Ensure the data is sent to the client immediately
    FlushFileBuffers(m_pipe);
    return true;
#else
    const char* cursor = data.constData();
    ssize_t remaining = data.size();
    while (remaining > 0) {
        const ssize_t written = send(m_clientFd, cursor, static_cast<size_t>(remaining), MSG_NOSIGNAL);
        if (written < 0) {
            if (errno == EINTR)
                continue;
            return false;
        }
        cursor += written;
        remaining -= written;
    }
    return true;
#endif
}

void PipeServer::closeCurrentPipe() {
#ifdef _WIN32
    HANDLE pipe = m_pipe;
    if (pipe != INVALID_HANDLE_VALUE) {
        m_pipe = INVALID_HANDLE_VALUE;
        CloseHandle(pipe);
    }
#else
    if (m_clientFd >= 0) {
        close(m_clientFd);
        m_clientFd = -1;
    }
    if (m_serverFd >= 0) {
        close(m_serverFd);
        m_serverFd = -1;
    }
#endif
}
