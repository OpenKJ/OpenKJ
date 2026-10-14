#ifndef BUNDLEDRUNTIME_H
#define BUNDLEDRUNTIME_H

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

// Call after creating QCoreApplication and before gst_init(). Development
// builds without this layout continue to use the installed GStreamer SDK.
inline void configureWindowsBundledGStreamer()
{
#ifdef Q_OS_WIN
    const QDir directory(QCoreApplication::applicationDirPath());
    const auto plugins = directory.filePath("gstreamer-1.0");
    const auto scanner = directory.filePath("gst-plugin-scanner.exe");
    if (!QFileInfo::exists(QDir(plugins).filePath("libgstcoreelements.dll")) ||
        !QFileInfo::exists(scanner))
        return;

    qputenv("GST_PLUGIN_PATH_1_0", QByteArray());
    qputenv("GST_PLUGIN_SYSTEM_PATH_1_0", QFile::encodeName(plugins));
    qputenv("GST_PLUGIN_SCANNER_1_0", QFile::encodeName(scanner));
#endif
}

#endif // BUNDLEDRUNTIME_H
