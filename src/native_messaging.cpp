#include "native_messaging.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QCoreApplication>
#include <QProcess>
#include <QThread>
#include <QtEndian>

#include <array>
#include <iostream>

#include "control_socket.h"

namespace {
constexpr quint32 MaxNativeMessageBytes = 1024 * 1024;

bool readExact(std::istream &input, char *data, std::streamsize size)
{
    input.read(data, size);
    return input.gcount() == size;
}

void writeNativeMessage(const QByteArray &message)
{
    const quint32 length = qToLittleEndian(static_cast<quint32>(message.size()));
    std::cout.write(reinterpret_cast<const char *>(&length), sizeof(length));
    std::cout.write(message.constData(), message.size());
    std::cout.flush();
}

QByteArray readOneNativeMessage()
{
    quint32 encodedLength = 0;
    if (!readExact(std::cin, reinterpret_cast<char *>(&encodedLength), sizeof(encodedLength))) {
        return {};
    }
    const quint32 length = qFromLittleEndian(encodedLength);
    if (length == 0 || length > MaxNativeMessageBytes) {
        return {};
    }
    QByteArray payload(static_cast<qsizetype>(length), Qt::Uninitialized);
    if (!readExact(std::cin, payload.data(), static_cast<std::streamsize>(length))) {
        return {};
    }
    return payload;
}

QByteArray unavailableResponse(const QString &reason)
{
    return QJsonDocument(QJsonObject{{QStringLiteral("action"), QStringLiteral("fallback")},
                                     {QStringLiteral("reason"), reason}})
        .toJson(QJsonDocument::Compact);
}

QByteArray forwardToBirdtray(const QByteArray &request,
                             int connectTimeoutMs = 1200,
                             int responseTimeoutMs = 4500)
{
    QLocalSocket socket;
#if defined(Q_OS_LINUX)
    socket.setSocketOptions(QLocalSocket::AbstractNamespaceOption);
#endif
    socket.connectToServer(birdtrayControlSocketName(), QIODevice::ReadWrite);
    if (!socket.waitForConnected(connectTimeoutMs)) {
        return unavailableResponse(QStringLiteral("birdtray-not-running"));
    }
    if (socket.write(request + '\n') != request.size() + 1 || !socket.waitForBytesWritten(1200)) {
        return unavailableResponse(QStringLiteral("control-write-failed"));
    }
    if (!socket.canReadLine() && !socket.waitForReadyRead(responseTimeoutMs)) {
        return unavailableResponse(QStringLiteral("control-timeout"));
    }
    const QByteArray response = socket.readLine(64 * 1024).trimmed();
    if (response.isEmpty()) {
        return unavailableResponse(QStringLiteral("empty-control-response"));
    }
    return response;
}

QString actionFromResponse(const QByteArray &response)
{
    const QJsonDocument document = QJsonDocument::fromJson(response);
    return document.isObject()
        ? document.object().value(QStringLiteral("action")).toString()
        : QString();
}

bool isLegacyHelperResponse(const QByteArray &response)
{
    const QJsonDocument document = QJsonDocument::fromJson(response);
    return document.isObject()
        && document.object().value(QStringLiteral("action")).toString()
            == QStringLiteral("fallback")
        && !document.object().contains(QStringLiteral("reason"));
}

QByteArray runningResponse()
{
    return QJsonDocument(QJsonObject{{QStringLiteral("action"), QStringLiteral("running")}})
        .toJson(QJsonDocument::Compact);
}

bool isMissingHelperResponse(const QByteArray &response)
{
    const QJsonDocument document = QJsonDocument::fromJson(response);
    return document.isObject()
        && document.object().value(QStringLiteral("reason")).toString()
            == QStringLiteral("birdtray-not-running");
}

bool isSuccessfulResponse(const QByteArray &response, const QString &requestType)
{
    const QString action = actionFromResponse(response);
    if (requestType == QStringLiteral("ensure-running")) {
        return action == QStringLiteral("running");
    }
    return action == QStringLiteral("hidden") || action == QStringLiteral("allow-close");
}

QByteArray startOrForwardToBirdtray(const QByteArray &request, const QString &requestType)
{
    QByteArray response = forwardToBirdtray(request, 300, 1000);

    if (isSuccessfulResponse(response, requestType)) {
        return response;
    }
    // Older helpers do not recognize ensure-running, but their control server
    // response still proves that a single Birdtray instance is already active.
    if (requestType == QStringLiteral("ensure-running") && isLegacyHelperResponse(response)) {
        return runningResponse();
    }
    if (!isMissingHelperResponse(response)) {
        return response;
    }

    if (!QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList())) {
        return unavailableResponse(QStringLiteral("birdtray-start-failed"));
    }

    QElapsedTimer startupTimer;
    startupTimer.start();
    while (startupTimer.elapsed() < 10000) {
        QThread::msleep(100);
        response = forwardToBirdtray(request, 300, 1000);
        if (isSuccessfulResponse(response, requestType)) {
            return response;
        }
        if (requestType == QStringLiteral("ensure-running") && isLegacyHelperResponse(response)) {
            return runningResponse();
        }
        if (!isMissingHelperResponse(response))
            return response;
    }
    return unavailableResponse(QStringLiteral("birdtray-start-timeout"));
}
}

int runNativeMessagingHost()
{
    const QByteArray request = readOneNativeMessage();
    if (request.isEmpty()) {
        writeNativeMessage(unavailableResponse(QStringLiteral("invalid-native-message")));
        return 1;
    }
    const QJsonDocument requestDocument = QJsonDocument::fromJson(request);
    if (!requestDocument.isObject()) {
        writeNativeMessage(unavailableResponse(QStringLiteral("invalid-native-message")));
        return 1;
    }

    const QString requestType = requestDocument.object().value(QStringLiteral("type")).toString();
    if (requestType == QStringLiteral("ensure-running")) {
        writeNativeMessage(startOrForwardToBirdtray(request, requestType));
    } else if (requestType == QStringLiteral("close-request")) {
        writeNativeMessage(startOrForwardToBirdtray(request, requestType));
    } else {
        writeNativeMessage(unavailableResponse(QStringLiteral("unsupported-native-message")));
        return 1;
    }
    return 0;
}
