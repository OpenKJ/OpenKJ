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
        //  m_filename = filename;
#ifdef Q_OS_WIN
        logFilePath = logDir + QDir::separator() + filename;
#else
        logFilePath = logDir + QDir::separator() + filename;
#endif
        //    logFile = new QFile(logFilePath);
        logFile.setFileName(logFilePath);
        logFile.open(QFile::WriteOnly);
        logStream.setDevice(&logFile);
    }


    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "DEBG: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "DEBG: " << localMsg << " (" << context.function << ")\n";
        logContents.append(QString::number(elapsed) + QString(" - DEBG: " + localMsg + " (" + context.function + ")"));
        break;
    case QtInfoMsg:
        fprintf(stderr, "INFO: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "INFO: " << localMsg << " (" << context.function << ")\n";
        logContents.append(QString::number(elapsed) + QString(" - INFO: " + localMsg + " (" + context.function + ")"));
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
    filter = new IdleDetect;
    a.installEventFilter(filter);
    qputenv("GST_DEBUG", "*:3");
    //qputenv("GST_DEBUG_DUMP_DOT_DIR", "/tmp");
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    if (settings.theme() == 1)
    {
        //a.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
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
//    QFile file(":/QTDark.css");
//    QString stylesheet;
//    QString line;
//    if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
//        QTextStream stream(&file);
//        stylesheet = stream.readAll();
//    }
//    file.close();
//    stylesheet = "* { background: #191919; color: #DDDDDD; border: 1px solid #5A5A5A;}";
//    a.setStyleSheet(stylesheet);

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
//#ifdef MACPLATFORM
//    if (!QFile::exists("/Library/Frameworks/GStreamer.framework"))
//    {
//        QMessageBox msgBox;
//        msgBox.setText("Required library GStreamer is not installed!");
//        msgBox.setTextFormat(Qt::RichText);
//        msgBox.setInformativeText("OpenKJ no longer bundles the GStreamer multimedia library with the application.<br><br>Please download and install <a href=https://gstreamer.freedesktop.org/data/pkg/osx/1.16.2/gstreamer-1.0-1.16.2-x86_64.pkg>this version</a> and re-run OpenKJ.<br><br>Exiting now.");
//        msgBox.setIcon(QMessageBox::Critical);
//        msgBox.exec();
//        return 1;
//    }
//#endif

    MainWindow w;
    w.show();

    return a.exec();
}
