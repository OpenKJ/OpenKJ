/*
 * Copyright (c) 2013-2021 Thomas Isaac Lightburn
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
#include <QApplication>
#include "mainwindow.h"
#include <QStyleFactory>
#include <QSplashScreen>
#include <QStringList>
#include <QMessageBox>
#include "settings.h"
#include "idledetect.h"
#include "runguard/runguard.h"
#include "okjversion.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async_logger.h>
#include <spdlog/async.h>


Settings settings;
IdleDetect *filter;

//todo: This should be me moved into main after migration to spdlog is complete
//      It's currently only global for use by the QDebug callback
std::shared_ptr<spdlog::async_logger> logger;


void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    bool loggingEnabled = settings.logEnabled();
    std::string logMsg = msg.toStdString();
    if (context.function) {
        logMsg.append(" [");
        logMsg.append(context.function);
        logMsg.append("]");
    }
    switch (type) {
        case QtDebugMsg:
            if (!loggingEnabled)
                return;
            logger->debug(logMsg);
            break;
        case QtInfoMsg:
            logger->info(logMsg);
            break;
        case QtWarningMsg:
            logger->warn(logMsg);
            break;
        case QtCriticalMsg:
            logger->critical(logMsg);
            break;
        case QtFatalMsg:
            logger->critical(logMsg);
            abort();
    }
}

int main(int argc, char *argv[]) {
    QString logDir = settings.logDir();
    QDir dir;
    QString logFilePath;
    QString filename = "openkj-debug-" + QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".log";
    dir.mkpath(logDir);
    logFilePath = logDir + QDir::separator() + filename;

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.toStdString(), false);
    console_sink->set_level(spdlog::level::trace);
    console_sink->set_pattern("[%^%l%$] %v");
    if (settings.logEnabled())
        file_sink->set_level(spdlog::level::debug);
    else
        file_sink->set_level(spdlog::level::off);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    spdlog::init_thread_pool(8192, 1);
    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    logger = std::make_shared<spdlog::async_logger>("logger", sinks.begin(), sinks.end(), spdlog::thread_pool(),
                                                    spdlog::async_overflow_policy::block);
    logger->set_level(spdlog::level::trace);
    spdlog::register_logger(logger);
    spdlog::flush_every(std::chrono::seconds(1));
    logger->flush_on(spdlog::level::err);


    logger->info("OpenKJ version {} starting up", OKJ_VERSION_STRING);

    //QLoggingCategory::setFilterRules("*.debug=true");
    qInstallMessageHandler(myMessageOutput);
    QApplication a(argc, argv);

#ifdef MAC_OVERRIDE_GST
    // This points GStreamer paths to the framework contained in the app bundle.  Not needed on brew installs.
    QString appDir = QCoreApplication::applicationDirPath();
    qInfo() << "Application dir: " << appDir;
    appDir.remove(appDir.length() - 5, 5);
    qputenv("GST_PLUGIN_SYSTEM_PATH", QString(appDir + "Frameworks/GStreamer.framework/Versions/Current/lib/gstreamer-1.0").toLocal8Bit());
    qputenv("GST_PLUGIN_SCANNER", QString(appDir + "Frameworks/GStreamer.framework/Versions/Current/libexec/gstreamer-1.0/gst-plugin-scanner").toLocal8Bit());
    qputenv("GTK_PATH", QString(appDir + "Frameworks/GStreamer.framework/Versions/Current/").toLocal8Bit());
    qputenv("GIO_EXTRA_MODULES", QString(appDir + "Frameworks/GStreamer.framework/Versions/Current/lib/gio/modules").toLocal8Bit());
    qWarning() << "MacOS detected, changed GST env vars to point to the bundled framework";
    qWarning() << qgetenv("GST_PLUGIN_SYSTEM_PATH") << endl << qgetenv("GST_PLUGIN_SCANNER") << endl << qgetenv("GTK_PATH") << endl << qgetenv("GIO_EXTRA_MODULES") << endl;
#endif

    filter = new IdleDetect;
    a.installEventFilter(filter);
    qputenv("GST_DEBUG", "*:3");
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    if (settings.theme() == 1) {
        QPalette palette;
        QApplication::setStyle(QStyleFactory::create("Fusion"));
        palette.setColor(QPalette::Window, QColor(53, 53, 53));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
        palette.setColor(QPalette::Base, QColor(42, 42, 42));
        palette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, QColor(53, 53, 53));
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
        palette.setColor(QPalette::Dark, QColor(35, 35, 35));
        palette.setColor(QPalette::Shadow, QColor(20, 20, 20));
        palette.setColor(QPalette::Button, QColor(53, 53, 53));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(42, 130, 218));
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127, 127, 127));
        QApplication::setPalette(palette);
    } else if (settings.theme() == 2) {
        QApplication::setStyle(QStyleFactory::create("Fusion"));
    }
//    else
//    {
//
//    }
    QApplication::setFont(settings.applicationFont(), "QWidget");
    QApplication::setFont(settings.applicationFont(), "QMenu");

    // RunGuard seems to be broken by flatpak, ignore it if running in a flatpak sandbox
    logger->info("App dir path {}", QCoreApplication::applicationDirPath().toStdString());
    if (QCoreApplication::applicationDirPath() == "/app/bin") {
        logger->info("RunGuard disabled due to flatpak sandbox");
    } else {
        logger->debug("Checking for other instances");
        RunGuard guard("SharedMemorySingleInstanceProtectorOpenKJ");
        if (!guard.tryToRun()) {
            QMessageBox msgBox;
            msgBox.setText("OpenKJ is already running!");
            msgBox.setInformativeText(
                    "In order to protect the database, you can only run one instance of OpenKJ at a time.\nExiting now.");
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.exec();
            return 1;
        }
    }
    if (!settings.lastStartupOk()) {
        QMessageBox msgBox;
        msgBox.setText("OpenKJ appears to have failed to startup on the last run.");
        msgBox.setInformativeText("Would you like to attempt to recover by loading safe settings?");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        if (msgBox.exec() == QMessageBox::Yes)
            settings.setSafeStartupMode(true);
        else
            settings.setSafeStartupMode(false);
    }
#ifdef Q_OS_DARWIN
    if (settings.lastRunVersion() != OKJ_VERSION_STRING)
        settings.setSafeStartupMode(true);
#endif
    settings.setLastRunVersion(OKJ_VERSION_STRING);
    settings.setStartupOk(false);
    MainWindow w;
    w.show();
    return QApplication::exec();
}
