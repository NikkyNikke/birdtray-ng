#ifndef BIRDTRAY_CONTROL_SOCKET_H
#define BIRDTRAY_CONTROL_SOCKET_H

#include <QFileInfo>
#include <QStandardPaths>
#include <QString>

inline QString birdtrayControlSocketName()
{
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    const QString userPart = QFileInfo(runtimeDir).fileName();
    return QStringLiteral("birdtray-wayland-control-") + userPart;
}

#endif
