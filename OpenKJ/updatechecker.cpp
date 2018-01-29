#include "updatechecker.h"
#include <QApplication>
#include <QDebug>
#include <QNetworkReply>


QString UpdateChecker::getOS() const
{
    return OS;
}

void UpdateChecker::setOS(const QString &value)
{
    OS = value;
}

QString UpdateChecker::getChannel() const
{
    return channel;
}

void UpdateChecker::setChannel(const QString &value)
{
    channel = value;
}

UpdateChecker::UpdateChecker(QObject *parent) : QObject(parent)
{
    OS = "unknown";
    channel = "stable";
    manager = new QNetworkAccessManager(this);
    currentVer = GIT_VERSION;
#ifdef Q_OS_WIN32
    OS = "Win32";
#endif
#ifdef Q_OS_WIN64
    OS = "Win64";
#endif
#ifdef Q_OS_MACOS
    OS = "MacOS";
#endif
#ifdef Q_OS_LINUX
    OS = "Linux";
#endif

#ifdef OKJ_UNSTABLE
    channel = "unstable";
#endif
#ifdef OKJ_BETA
    channel = "beta";
#endif
#ifdef OKJ_STABLE
    channel = "stable";
#endif
//    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onNetworkReply(QNetworkReply*)));
}

void UpdateChecker::checkForUpdates()
{
    qWarning() << "Requesting current version info";
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onNetworkReply(QNetworkReply*)));
    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl("http://openkj.org/downloads/" + OS + "-" + channel + "-curversion.txt")));
    while (!reply->isFinished())
        QApplication::processEvents();
    qWarning() << "Request completed";
}

void UpdateChecker::onNetworkReply(QNetworkReply *reply)
{
    qWarning() << "Received network reply";
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << reply->errorString();
        //output some meaningful error msg
        return;
    }
    availVersion = QString(reply->readAll());
    availVersion = availVersion.trimmed();
    QStringList curVersionParts = currentVer.split(".");
    QStringList availVersionParts = availVersion.split(".");
    if (availVersionParts.size() != 3 || curVersionParts.size() != 3)
        return;
    int availMajor = availVersionParts.at(0).toInt();
    int availMinor = availVersionParts.at(1).toInt();
    int availRevis = availVersionParts.at(2).toInt();
    int curMajor = curVersionParts.at(0).toInt();
    int curMinor = curVersionParts.at(1).toInt();
    int curRevis = curVersionParts.at(2).toInt();
    if (availMajor > curMajor)
        emit newVersionAvailable(availVersion);
    else if (availMajor == curMajor && availMinor > curMinor)
        emit newVersionAvailable(availVersion);
    else if (availMajor == curMajor && availMinor == curMinor && availRevis > curRevis)
        emit newVersionAvailable(availVersion);
    qWarning() << "Received version: " << availVersion << " Current version: " << currentVer;
    reply->deleteLater();
}

void UpdateChecker::downloadInstaller()
{
    QString url;
    if (channel == "unstable")
    {
        if (OS == "Win64")
            url = "http://openkj.org/downloads/unstable/Windows/OpenKJ-" + availVersion + "-64bit-setup.exe";
        if (OS == "Win32")
            url = "http://openkj.org/downloads/unstable/Windows/OpenKJ-" + availVersion + "-32bit-setup.exe";
        if (OS == "MacOS")
            url = "https://openkj.org/downloads/unstable/MacOS/OpenKJ-" + availVersion + "-unstable-osx-installer.dmg";
    }
}
