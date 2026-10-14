#ifndef LOCALMEDIAURI_H
#define LOCALMEDIAURI_H

#include <QByteArray>
#include <QFileInfo>
#include <QUrl>

// GStreamer expects an encoded URI, including the third slash for Windows
// drive paths and escaping for characters such as spaces, '#' and '%'.
inline QByteArray localMediaUri(const QString &filename)
{
    return QUrl::fromLocalFile(QFileInfo(filename).absoluteFilePath()).toEncoded();
}

#endif // LOCALMEDIAURI_H
