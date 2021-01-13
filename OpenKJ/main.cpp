/*
 * Copyright (c) 2013-2019 Thomas Isaac Lightburn
 *
 *
 * This file is part of OpenKJ.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


//#include <QtGui/QApplication>
#include <QApplication>
#include "mainwindow.h"
#include <QStyleFactory>
#include <QSplashScreen>
#include <QStringList>
#include <QDebug>
#include <QMessageBox>
#include "settings.h"
#include "idledetect.h"
#include "runguard/runguard.h"

QDataStream &operator<<(QDataStream &out, const SfxEntry &obj)
{
    out << obj.name << obj.path;
    qInfo() << "returning " << obj.name << " " << obj.path;
    return out;
}

QDataStream &operator>>(QDataStream &in, SfxEntry &obj)
{
   qInfo() << "setting " << obj.name << " " << obj.path;
   in >> obj.name >> obj.path;
   return in;
}

Settings settings;
IdleDetect *filter;

QFile logFile;
QTextStream logStream;
QStringList logContents;
auto startTime = std::chrono::high_resolution_clock::now();

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    bool loggingEnabled = settings.logEnabled();
    auto currentTime = std::chrono::high_resolution_clock::now();
    unsigned int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
    if (loggingEnabled && !logFile.isOpen())
    {
        QString logDir = settings.logDir();
        QDir dir;
        QString logFilePath;
        QString filename = "openkj-debug-" + QDateTime::currentDateTime().toString("yyyy-MM-dd-hhmm") + "-log";
        dir.mkpath(logDir);
        logFilePath = logDir + QDir::separator() + filename;
        logFile.setFileName(logFilePath);
        logFile.open(QFile::WriteOnly);
        logStream.setDevice(&logFile);
    }


    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "DEBG: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "DEBG: " << localMsg << " (" << context.function << ")\n";
        if (loggingEnabled) logContents.append(QString::number(elapsed) + QString(" - DEBG: " + localMsg + " (" + context.function + ")"));
        break;
    case QtInfoMsg:
        fprintf(stderr, "INFO: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "INFO: " << localMsg << " (" << context.function << ")\n";
        if (loggingEnabled) logContents.append(QString::number(elapsed) + QString(" - INFO: " + localMsg + " (" + context.function + ")"));
        break;
    case QtWarningMsg:
        fprintf(stderr, "WARN: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "WARN: " << localMsg << " (" << context.function << ")\n";
        logContents.append(QString::number(elapsed) + QString(" - WARN: " + localMsg + " (" + context.function + ")"));
        break;
    case QtCriticalMsg:
        fprintf(stderr, "CRIT: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "CRIT: " << localMsg << " (" << context.function << ")\n";
        logContents.append(QString::number(elapsed) + QString(" - CRIT: " + localMsg + " (" + context.function + ")"));
        break;
    case QtFatalMsg:
        fprintf(stderr, "FATAL!!: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "FATAL!!: " << localMsg << " (" << context.function << ")\n";
        logContents.append(QString::number(elapsed) + QString(" - FATAL!!: " + localMsg + " (" + context.function + ")"));
        if (loggingEnabled) logStream.flush();
        abort();
    }
    if (loggingEnabled) logStream.flush();
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(myMessageOutput);
    qRegisterMetaType<SfxEntry>("SfxEntry");
    qRegisterMetaTypeStreamOperators<SfxEntry>("SfxEntry");
    qRegisterMetaType<QList<SfxEntry> >("QList<SfxEntry>");
    qRegisterMetaTypeStreamOperators<QList<SfxEntry> >("QList<SfxEntry>");
    QApplication a(argc, argv);

#ifdef MAC_OVERRIDE_GST
    // This points GStreamer paths to the framework contained in the app bundle.  Not needed on brew installs.
    QString appDir = QCoreApplication::applicationDirPath();
    appDir.remove(appDir.length() - 5, 5);
    qputenv("GST_PLUGIN_SYSTEM_PATH", QString(appDir + "/Frameworks/GStreamer.framework/Versions/Current/lib/gstreamer-1.0").toLocal8Bit());
    qputenv("GST_PLUGIN_SCANNER", QString(appDir + "/Frameworks/GStreamer.framework/Versions/Current/libexec/gstreamer-1.0/gst-plugin-scanner").toLocal8Bit());
    qputenv("GTK_PATH", QString(appDir + "/Frameworks/GStreamer.framework/Versions/Current/").toLocal8Bit());
    qputenv("GIO_EXTRA_MODULES", QString(appDir + "/Frameworks/GStreamer.framework/Versions/Current/lib/gio/modules").toLocal8Bit());
    qWarning() << "MacOS detected, changed GST env vars to point to the bundled framework";
    qInfo() << "Application dir: " << appDir;
    qWarning() << qgetenv("GST_PLUGIN_SYSTEM_PATH") << endl << qgetenv("GST_PLUGIN_SCANNER") << endl << qgetenv("GTK_PATH") << endl << qgetenv("GIO_EXTRA_MODULES") << endl;
#endif

    filter = new IdleDetect;
    a.installEventFilter(filter);
    qputenv("GST_DEBUG", "*:3");
    //qputenv("GST_DEBUG_DUMP_DOT_DIR", "/tmp");
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    if (settings.theme() == 1)
    {
        QPalette palette;
        a.setStyle(QStyleFactory::create("Fusion"));
        palette.setColor(QPalette::Window,QColor(53,53,53));
        palette.setColor(QPalette::WindowText,Qt::white);
        palette.setColor(QPalette::Disabled,QPalette::WindowText,QColor(127,127,127));
        palette.setColor(QPalette::Base,QColor(42,42,42));
        palette.setColor(QPalette::AlternateBase,QColor(66,66,66));
        palette.setColor(QPalette::ToolTipBase,Qt::white);
        palette.setColor(QPalette::ToolTipText,QColor(53,53,53));
        palette.setColor(QPalette::Text,Qt::white);
        palette.setColor(QPalette::Disabled,QPalette::Text,QColor(127,127,127));
        palette.setColor(QPalette::Dark,QColor(35,35,35));
        palette.setColor(QPalette::Shadow,QColor(20,20,20));
        palette.setColor(QPalette::Button,QColor(53,53,53));
        palette.setColor(QPalette::ButtonText,Qt::white);
        palette.setColor(QPalette::Disabled,QPalette::ButtonText,QColor(127,127,127));
        palette.setColor(QPalette::BrightText,Qt::red);
        palette.setColor(QPalette::Link,QColor(42,130,218));
        palette.setColor(QPalette::Highlight,QColor(42,130,218));
        palette.setColor(QPalette::Disabled,QPalette::Highlight,QColor(80,80,80));
        palette.setColor(QPalette::HighlightedText,Qt::white);
        palette.setColor(QPalette::Disabled,QPalette::HighlightedText,QColor(127,127,127));
        a.setPalette(palette);
    }
    else if (settings.theme() == 2)
    {
        a.setStyle(QStyleFactory::create("Fusion"));
    }
    else
    {

    }
    a.setFont(settings.applicationFont(), "QWidget");
    a.setFont(settings.applicationFont(), "QMenu");

    RunGuard guard("SharedMemorySingleInstanceProtectorOpenKJ");
     if (!guard.tryToRun())
     {
         QMessageBox msgBox;
         msgBox.setText("OpenKJ is already running!");
         msgBox.setInformativeText("In order to protect the database, you can only run one instance of OpenKJ at a time.\nExiting now.");
         msgBox.setIcon(QMessageBox::Critical);
         msgBox.exec();
         return 1;
     }

    MainWindow w;
    w.show();

    return a.exec();
}
