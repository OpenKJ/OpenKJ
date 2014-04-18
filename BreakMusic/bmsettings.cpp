#include "bmsettings.h"
#include <QCoreApplication>

BmSettings::BmSettings(QObject *parent) :
    QObject(parent)
{
    QCoreApplication::setOrganizationName("OpenKJ");
    QCoreApplication::setOrganizationDomain("OpenKJ.org");
    QCoreApplication::setApplicationName("BreakMusic");
    settings = new QSettings(this);
}

void BmSettings::saveWindowState(QWidget *window)
{
    settings->beginGroup(window->objectName());
    settings->setValue("size", window->size());
    settings->setValue("pos", window->pos());
    settings->endGroup();
}

void BmSettings::restoreWindowState(QWidget *window)
{
    settings->beginGroup(window->objectName());
    window->resize(settings->value("size", QSize(640, 480)).toSize());
    window->move(settings->value("pos", QPoint(100, 100)).toPoint());
    settings->endGroup();
}

bool BmSettings::showFilenames()
{
    return settings->value("showFilenames", false).toBool();
}

void BmSettings::setShowFilenames(bool show)
{
    settings->setValue("showFilenames", show);
}

bool BmSettings::showMetadata()
{
    return settings->value("showMetadata", true).toBool();
}

void BmSettings::setShowMetadata(bool show)
{
    settings->setValue("showMetadata", show);
}

int BmSettings::volume()
{
    return settings->value("volume", 50).toInt();
}

void BmSettings::setVolume(int volume)
{
    settings->setValue("volume", volume);
}

int BmSettings::playlistIndex()
{
    return settings->value("playlistIndex",0).toInt();
}

void BmSettings::setPlaylistIndex(int index)
{
    settings->setValue("playlistIndex", index);
}
