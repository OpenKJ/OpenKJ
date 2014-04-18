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

void KhSettings::setTickerFont(QFont font)
{
    settings->setValue("tickerFont", font.toString());
    emit tickerFontChanged();
}

QFont KhSettings::tickerFont()
{
    QFont font;
    font.fromString(settings->value("tickerFont", QApplication::font().toString()).toString());
    return font;
}

int KhSettings::tickerHeight()
{
    return settings->value("tickerHeight", 25).toInt();
}

void KhSettings::setTickerHeight(int height)
{
    settings->setValue("tickerHeight", height);
    emit tickerHeightChanged();
}

int KhSettings::tickerSpeed()
{
    return settings->value("tickerSpeed", 50).toInt();
}

void KhSettings::setTickerSpeed(int speed)
{
    settings->setValue("tickerSpeed", speed);
    emit tickerSpeedChanged();
}

QColor KhSettings::tickerTextColor()
{
    return settings->value("tickerTextColor", QApplication::palette().foreground().color()).value<QColor>();
}

void KhSettings::setTickerTextColor(QColor color)
{
    settings->setValue("tickerTextColor", color);
    emit tickerTextColorChanged();
}

QColor KhSettings::tickerBgColor()
{
    return settings->value("tickerBgColor", QApplication::palette().background().color()).value<QColor>();
}

void KhSettings::setTickerBgColor(QColor color)
{
    settings->setValue("tickerBgColor", color);
    emit tickerBgColorChanged();
}

bool KhSettings::tickerFullRotation()
{
    return settings->value("tickerFullRotation", true).toBool();
}

void KhSettings::setTickerFullRotation(bool full)
{
    settings->setValue("tickerFullRotation", full);
    emit tickerOutputModeChanged();
}

int KhSettings::tickerShowNumSingers()
{
    return settings->value("tickerShowNumSingers", 10).toInt();
}

void KhSettings::setTickerShowNumSingers(int limit)
{
    settings->setValue("tickerShowNumSingers", limit);
    emit tickerOutputModeChanged();
}

void KhSettings::setTickerEnabled(bool enable)
{
    settings->setValue("tickerEnabled", enable);
    emit tickerEnableChanged();
}

bool KhSettings::tickerEnabled()
{
    return settings->value("tickerEnabled", false).toBool();
}

bool KhSettings::requestServerEnabled()
{
    return settings->value("requestServerEnabled", false).toBool();
}

void KhSettings::setRequestServerEnabled(bool enable)
{
    settings->setValue("requestServerEnabled", enable);
}

QString KhSettings::requestServerUrl()
{
    return settings->value("requestServerUrl", "").toString();
}

void KhSettings::setRequestServerUrl(QString url)
{
    settings->setValue("requestServerUrl", url);
}

QString KhSettings::requestServerUsername()
{
    return settings->value("requestServerUsername","").toString();
}

void KhSettings::setRequestServerUsername(QString username)
{
    settings->setValue("requestServerUsername", username);
}

QString KhSettings::requestServerPassword()
{
    return settings->value("requestServerPassword", "").toString();
}

void KhSettings::setRequestServerPassword(QString password)
{
    settings->setValue("requestServerPassword", password);
}

bool KhSettings::requestServerIgnoreCertErrors()
{
    return settings->value("requestServerIgnoreCertErrors", false).toBool();
}

void KhSettings::setRequestServerIgnoreCertErrors(bool ignore)
{
    settings->setValue("requestServerIgnoreCertErrors", ignore);
}

bool KhSettings::audioUseFader()
{
    return settings->value("audioUseFader", true).toBool();
}

void KhSettings::setAudioUseFader(bool fader)
{
    settings->setValue("audioUseFader", fader);
}

int KhSettings::audioVolume()
{
    return settings->value("audioVolume", 50).toInt();
}

void KhSettings::setAudioVolume(int volume)
{
    settings->setValue("audioVolume", volume);
}
