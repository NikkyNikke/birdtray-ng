#include "native_messaging.h"
#include "control_socket.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QtEndian>

#include <iostream>

namespace {
void writeMarker(const QString &path, const QByteArray &contents)
{
    QFile marker(path);
    if (marker.open(QIODevice::WriteOnly)) {
        marker.write(contents);
        marker.close();
    }
}

int runHelperChild(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const bool legacy = QCoreApplication::arguments().contains(QStringLiteral("--legacy-helper"));
    const QString markerPath = QString::fromLocal8Bit(qgetenv("BIRDTRAY_TEST_HELPER_MARKER"));
    QLocalServer server;
#if defined(Q_OS_LINUX)
    server.setSocketOptions(QLocalServer::AbstractNamespaceOption);
#endif
    if (!server.listen(birdtrayControlSocketName())) {
        writeMarker(markerPath + QStringLiteral(".error"), server.errorString().toUtf8());
        return 2;
    }
    writeMarker(markerPath + QStringLiteral(".listening"), "listening");

    QObject::connect(&server, &QLocalServer::newConnection, &app, [&]() {
        QLocalSocket *socket = server.nextPendingConnection();
        if (!socket)
            return;
        const auto handleRequest = [&, socket]() {
            if (!socket->canReadLine())
                return;
            const QJsonDocument request = QJsonDocument::fromJson(socket->readLine());
            const QString requestType = request.object().value(QStringLiteral("type")).toString();
            QByteArray response;
            if (legacy) {
                response = "{\"action\":\"fallback\"}\n";
            } else if (requestType == QStringLiteral("ensure-running")) {
                response = "{\"action\":\"running\"}\n";
            } else if (requestType == QStringLiteral("close-request")) {
                response = "{\"action\":\"hidden\"}\n";
            } else {
                response = "{\"action\":\"fallback\"}\n";
            }
            socket->write(response);
            socket->waitForBytesWritten(1000);
            writeMarker(markerPath + QStringLiteral(".ready"), requestType.toUtf8());
            QTimer::singleShot(100, &app, [&app, markerPath]() {
                writeMarker(markerPath + QStringLiteral(".stopped"), "stopped");
                app.quit();
            });
        };
        QObject::connect(socket, &QLocalSocket::readyRead, &app, handleRequest);
        if (socket->canReadLine())
            handleRequest();
    });
    return app.exec();
}

QByteArray makeNativeFrame(const QByteArray &payload)
{
    QByteArray frame(static_cast<qsizetype>(sizeof(quint32)), Qt::Uninitialized);
    qToLittleEndian(static_cast<quint32>(payload.size()), frame.data());
    frame.append(payload);
    return frame;
}

bool prepareRuntime(QTemporaryDir &temporaryRoot, QString &markerPath)
{
    if (!temporaryRoot.isValid()) {
        std::cerr << "could not create a temporary test directory\n";
        return false;
    }
    const QString runtimeDirectory = QDir(temporaryRoot.path()).filePath("runtime-birdtray-test");
    if (!QDir().mkpath(runtimeDirectory)
        || !QFile::setPermissions(runtimeDirectory,
                                  QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                      | QFileDevice::ExeOwner)) {
        std::cerr << "could not prepare a private runtime directory\n";
        return false;
    }
    markerPath = QDir(temporaryRoot.path()).filePath("helper");
    qputenv("XDG_RUNTIME_DIR", runtimeDirectory.toLocal8Bit());
    qputenv("BIRDTRAY_TEST_HELPER_MARKER", markerPath.toLocal8Bit());
    return true;
}

bool waitForMarker(const QString &path, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    while (!QFile::exists(path) && timer.elapsed() < timeoutMs)
        QThread::msleep(25);
    return QFile::exists(path);
}

bool launchNativeHost(const QString &executable, const QString &requestType,
                      const QString &expectedAction)
{
    QProcess host;
    host.setProgram(executable);
    host.setArguments({QStringLiteral("--native-messaging-host")});
    host.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    host.start();
    if (!host.waitForStarted(3000)) {
        std::cerr << "native host test process did not start\n";
        return false;
    }

    const QByteArray request = QJsonDocument(QJsonObject{
        {QStringLiteral("type"), requestType}}).toJson(QJsonDocument::Compact);
    host.write(makeNativeFrame(request));
    host.closeWriteChannel();
    if (!host.waitForFinished(15000)) {
        host.kill();
        host.waitForFinished();
        std::cerr << "native host did not finish its startup handshake\n";
        return false;
    }

    const QByteArray output = host.readAllStandardOutput();
    if (host.exitCode() != 0 || output.size() < static_cast<qsizetype>(sizeof(quint32))) {
        std::cerr << "native host failed: " << host.readAllStandardError().toStdString() << '\n';
        return false;
    }
    const quint32 responseSize = qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(output.constData()));
    if (output.size() != static_cast<qsizetype>(sizeof(quint32) + responseSize)) {
        std::cerr << "native host returned an invalid frame\n";
        return false;
    }
    const QJsonDocument response = QJsonDocument::fromJson(
        output.mid(sizeof(quint32), static_cast<qsizetype>(responseSize)));
    if (response.object().value(QStringLiteral("action")).toString() != expectedAction) {
        std::cerr << "native host returned " << response.toJson(QJsonDocument::Compact).toStdString()
                  << " for " << requestType.toStdString() << '\n';
        return false;
    }
    return true;
}

bool runMissingHelperCase(const QString &executable, const QString &requestType,
                          const QString &expectedAction)
{
    QTemporaryDir temporaryRoot;
    QString markerPath;
    if (!prepareRuntime(temporaryRoot, markerPath))
        return false;
    if (!launchNativeHost(executable, requestType, expectedAction))
        return false;
    if (!waitForMarker(markerPath + QStringLiteral(".ready"), 3000)
        || !waitForMarker(markerPath + QStringLiteral(".stopped"), 3000)) {
        std::cerr << "detached fake helper did not start and exit cleanly\n";
        return false;
    }
    return true;
}

bool runLegacyHelperCase(const QString &executable)
{
    QTemporaryDir temporaryRoot;
    QString markerPath;
    if (!prepareRuntime(temporaryRoot, markerPath))
        return false;

    QProcess legacyHelper;
    legacyHelper.setProgram(executable);
    legacyHelper.setArguments({QStringLiteral("--legacy-helper")});
    legacyHelper.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    legacyHelper.start();
    if (!legacyHelper.waitForStarted(3000)
        || !waitForMarker(markerPath + QStringLiteral(".listening"), 3000)) {
        std::cerr << "legacy helper did not listen on the temporary test socket\n";
        legacyHelper.kill();
        legacyHelper.waitForFinished();
        return false;
    }

    if (!launchNativeHost(executable, QStringLiteral("ensure-running"),
                          QStringLiteral("running"))) {
        legacyHelper.kill();
        legacyHelper.waitForFinished();
        return false;
    }
    if (!legacyHelper.waitForFinished(3000)) {
        legacyHelper.kill();
        legacyHelper.waitForFinished();
        std::cerr << "legacy helper did not exit after the readiness request\n";
        return false;
    }
    return !QFile::exists(markerPath + QStringLiteral(".error"));
}

bool runSelfTest(const QString &executable)
{
    if (!runMissingHelperCase(executable, QStringLiteral("ensure-running"),
                              QStringLiteral("running")))
        return false;
    if (!runMissingHelperCase(executable, QStringLiteral("close-request"),
                              QStringLiteral("hidden")))
        return false;
    if (!runLegacyHelperCase(executable))
        return false;
    return true;
}
}

int main(int argc, char *argv[])
{
    const QStringList arguments = [&]() {
        QStringList values;
        for (int i = 0; i < argc; ++i)
            values.append(QString::fromLocal8Bit(argv[i]));
        return values;
    }();

    if (arguments.contains(QStringLiteral("--native-messaging-host"))) {
        QCoreApplication app(argc, argv);
        return runNativeMessagingHost();
    }
    if (arguments.contains(QStringLiteral("--legacy-helper")))
        return runHelperChild(argc, argv);
    if (arguments.contains(QStringLiteral("--self-test"))) {
        QCoreApplication app(argc, argv);
        return runSelfTest(QFileInfo(QString::fromLocal8Bit(argv[0])).absoluteFilePath()) ? 0 : 1;
    }
    return runHelperChild(argc, argv);
}
