/*
 * Copyright (c) 2013-2014 Thomas Isaac Lightburn
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

#include "khsettings.h"
#include <QCoreApplication>
#include <QApplication>
#include <QDesktopWidget>

KhSettings::KhSettings(QObject *parent) :
    QObject(parent)
{
    QCoreApplication::setOrganizationName("OpenKJ");
    QCoreApplication::setOrganizationDomain("OpenKJ.org");
    QCoreApplication::setApplicationName("KaraokeHost");
    settings = new QSettings(this);
}

bool KhSettings::cdgWindowFullscreen()
{
    return settings->value("cdgWindowFullscreen", false).toBool();
}

void KhSettings::setCdgWindowFullscreen(bool fullScreen)
{
    settings->setValue("cdgWindowFullscreen", fullScreen);
}


bool KhSettings::showCdgWindow()
{
    return settings->value("showCdgWindow", false).toBool();
}

void KhSettings::setShowCdgWindow(bool show)
{
    settings->setValue("showCdgWindow", show);
}

void KhSettings::setCdgWindowFullscreenMonitor(int monitor)
{
    settings->setValue("cdgWindowFullScreenMonitor", monitor);
}

int KhSettings::cdgWindowFullScreenMonitor()
{
    //We default to the highest mointor present, by default, rather than the primary display.  Seems to make more sense
    //and will help prevent people from popping up a full screen window in front of the main window and getting confused.
    return settings->value("cdgWindowFullScreenMonitor", QApplication::desktop()->screenCount() - 1).toInt();
}

void KhSettings::saveWindowState(QWidget *window)
{
    settings->beginGroup(window->objectName());
    settings->setValue("size", window->size());
    settings->setValue("pos", window->pos());
    settings->endGroup();
}

void KhSettings::restoreWindowState(QWidget *window)
{
    settings->beginGroup(window->objectName());
    window->resize(settings->value("size", QSize(640, 480)).toSize());
    window->move(settings->value("pos", QPoint(100, 100)).toPoint());
    settings->endGroup();
}
