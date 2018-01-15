/*
 * Copyright (c) 2013-2016 Thomas Isaac Lightburn
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

#include "settings.h"
#include <QCoreApplication>
#include <QApplication>
#include <QDesktopWidget>
#include <QStandardPaths>
#include <QDebug>

Settings::Settings(QObject *parent) :
    QObject(parent)
{
    QCoreApplication::setOrganizationName("OpenKJ");
    QCoreApplication::setOrganizationDomain("OpenKJ.org");
    QCoreApplication::setApplicationName("OpenKJ");
    settings = new QSettings(this);
}

bool Settings::cdgWindowFullscreen()
{
    return settings->value("cdgWindowFullscreen", false).toBool();
}

void Settings::setCdgWindowFullscreen(bool fullScreen)
{
    settings->setValue("cdgWindowFullscreen", fullScreen);
    emit cdgWindowFullscreenChanged(fullScreen);
}


bool Settings::showCdgWindow()
{
    return settings->value("showCdgWindow", false).toBool();
}

void Settings::setShowCdgWindow(bool show)
{
    settings->setValue("showCdgWindow", show);
    emit cdgShowCdgWindowChanged(show);
}

void Settings::setCdgWindowFullscreenMonitor(int monitor)
{
    settings->setValue("cdgWindowFullScreenMonitor", monitor);
    emit cdgWindowFullscreenMonitorChanged(monitor);
}

int Settings::cdgWindowFullScreenMonitor()
{
    //We default to the highest mointor present, by default, rather than the primary display.  Seems to make more sense
    //and will help prevent people from popping up a full screen window in front of the main window and getting confused.
    return settings->value("cdgWindowFullScreenMonitor", QApplication::desktop()->screenCount() - 1).toInt();
}

void Settings::saveWindowState(QWidget *window)
{
    settings->beginGroup(window->objectName());
    settings->setValue("size", window->size());
    settings->setValue("pos", window->pos());
    settings->endGroup();
}

void Settings::restoreWindowState(QWidget *window)
{

    settings->beginGroup(window->objectName());
    if (settings->contains("size") && settings->contains("pos"))
    {
        window->resize(settings->value("size", QSize(640, 480)).toSize());
        window->move(settings->value("pos", QPoint(100, 100)).toPoint());
    }
    settings->endGroup();
}

void Settings::saveColumnWidths(QTreeView *treeView)
{
    settings->beginGroup(treeView->objectName());
    settings->setValue("headerState", treeView->header()->saveState());
    settings->setValue("hiddenSections", treeView->header()->hiddenSectionCount());
    settings->setValue("sections", treeView->header()->count());
    settings->endGroup();
}

void Settings::saveColumnWidths(QTableView *tableView)
{
    settings->beginGroup(tableView->objectName());
    for (int i=0; i < tableView->horizontalHeader()->count(); i++)
    {
        settings->beginGroup(QString::number(i));
        settings->setValue("size", tableView->horizontalHeader()->sectionSize(i));
        settings->setValue("hidden", tableView->horizontalHeader()->isSectionHidden(i));
        settings->endGroup();
    }
    settings->endGroup();
}

void Settings::restoreColumnWidths(QTreeView *treeView)
{
    settings->beginGroup(treeView->objectName());
    if ((settings->contains("headerState")) && (settings->value("hiddenSections").toInt() == treeView->header()->hiddenSectionCount()) && (settings->value("sections").toInt() == treeView->header()->count()))
        treeView->header()->restoreState(settings->value("headerState").toByteArray());
    settings->endGroup();
}

void Settings::restoreColumnWidths(QTableView *tableView)
{
    settings->beginGroup(tableView->objectName());
    QStringList headers = settings->childGroups();
    for (int i=0; i < headers.size(); i++)
    {
        settings->beginGroup(headers.at(i));
        int section = headers.at(i).toInt();
        bool hidden = settings->value("hidden", false).toBool();
        int size = settings->value("size", 0).toInt();
        tableView->horizontalHeader()->resizeSection(section, size);
        tableView->horizontalHeader()->setSectionHidden(section, hidden);
        settings->endGroup();
    }
    settings->endGroup();
}

void Settings::saveSplitterState(QSplitter *splitter)
{
    settings->beginGroup(splitter->objectName());
    settings->setValue("splitterState", splitter->saveState());
    settings->endGroup();
}

void Settings::restoreSplitterState(QSplitter *splitter)
{
    settings->beginGroup(splitter->objectName());
    if (settings->contains("splitterState"))
        splitter->restoreState(settings->value("splitterState").toByteArray());
    settings->endGroup();
}

void Settings::setTickerFont(QFont font)
{
    settings->setValue("tickerFont", font.toString());
    emit tickerFontChanged();
}

QFont Settings::tickerFont()
{
    QFont font;
    font.fromString(settings->value("tickerFont", QApplication::font().toString()).toString());
    return font;
}

int Settings::tickerHeight()
{
    return settings->value("tickerHeight", 25).toInt();
}

void Settings::setTickerHeight(int height)
{
    settings->setValue("tickerHeight", height);
    emit tickerHeightChanged(height);
}

int Settings::tickerSpeed()
{
    return settings->value("tickerSpeed", 50).toInt();
}

void Settings::setTickerSpeed(int speed)
{
    settings->setValue("tickerSpeed", speed);
    emit tickerSpeedChanged();
}

QColor Settings::tickerTextColor()
{
    return settings->value("tickerTextColor", QApplication::palette().foreground().color()).value<QColor>();
}

void Settings::setTickerTextColor(QColor color)
{
    settings->setValue("tickerTextColor", color);
    emit tickerTextColorChanged();
}

QColor Settings::tickerBgColor()
{
    return settings->value("tickerBgColor", QApplication::palette().background().color()).value<QColor>();
}

void Settings::setTickerBgColor(QColor color)
{
    settings->setValue("tickerBgColor", color);
    emit tickerBgColorChanged();
}

bool Settings::tickerFullRotation()
{
    return settings->value("tickerFullRotation", true).toBool();
}

void Settings::setTickerFullRotation(bool full)
{
    settings->setValue("tickerFullRotation", full);
    emit tickerOutputModeChanged();
}

int Settings::tickerShowNumSingers()
{
    return settings->value("tickerShowNumSingers", 10).toInt();
}

void Settings::setTickerShowNumSingers(int limit)
{
    settings->setValue("tickerShowNumSingers", limit);
    emit tickerOutputModeChanged();
}

void Settings::setTickerEnabled(bool enable)
{
    settings->setValue("tickerEnabled", enable);
    emit tickerEnableChanged();
}

bool Settings::tickerEnabled()
{
    return settings->value("tickerEnabled", false).toBool();
}

QString Settings::tickerCustomString()
{
    return settings->value("tickerCustomString", "").toString();
}

void Settings::setTickerCustomString(QString value)
{
    settings->setValue("tickerCustomString", value);
    emit tickerCustomStringChanged();
}

bool Settings::tickerShowRotationInfo()
{
    return settings->value("tickerShowRotationInfo", true).toBool();
}

bool Settings::requestServerEnabled()
{
    return settings->value("requestServerEnabled", false).toBool();
}

void Settings::setRequestServerEnabled(bool enable)
{
    settings->setValue("requestServerEnabled", enable);
}

QString Settings::requestServerUrl()
{
    return settings->value("requestServerUrl", "https://songbook.openkj.org/api").toString();
}

void Settings::setRequestServerUrl(QString url)
{
    settings->setValue("requestServerUrl", url);
}

int Settings::requestServerVenue()
{
    return settings->value("requestServerVenue", 0).toInt();
}

void Settings::setRequestServerVenue(int venueId)
{
    settings->setValue("requestServerVenue", venueId);
    emit requestServerVenueChanged(venueId);
}

QString Settings::requestServerApiKey()
{
    return settings->value("requestServerApiKey","").toString();
}

void Settings::setRequestServerApiKey(QString apiKey)
{
    settings->setValue("requestServerApiKey", apiKey);
}

bool Settings::requestServerIgnoreCertErrors()
{
    return settings->value("requestServerIgnoreCertErrors", false).toBool();
}

void Settings::setRequestServerIgnoreCertErrors(bool ignore)
{
    settings->setValue("requestServerIgnoreCertErrors", ignore);
}

bool Settings::audioUseFader()
{
    return settings->value("audioUseFader", true).toBool();
}

bool Settings::audioUseFaderBm()
{
    return settings->value("audioUseFaderBm", true).toBool();
}

void Settings::setAudioUseFader(bool fader)
{
    settings->setValue("audioUseFader", fader);
}

void Settings::setAudioUseFaderBm(bool fader)
{
    settings->setValue("audioUseFaderBm", fader);
}

int Settings::audioVolume()
{
    return settings->value("audioVolume", 50).toInt();
}

void Settings::setAudioVolume(int volume)
{
    settings->setValue("audioVolume", volume);
}

QString Settings::cdgDisplayBackgroundImage()
{
    return settings->value("cdgDisplayBackgroundImage", QString()).toString();
}

void Settings::setCdgDisplayBackgroundImage(QString imageFile)
{
    if (imageFile == "")
        settings->remove("cdgDisplayBackgroundImage");
    else
        settings->setValue("cdgDisplayBackgroundImage", imageFile);
    emit cdgBgImageChanged();
}

Settings::BgMode Settings::bgMode()
{
    return (Settings::BgMode)settings->value("bgMode", 0).toInt();
}

void Settings::setBgMode(Settings::BgMode mode)
{
    settings->setValue("bgMode", mode);
    emit bgModeChanged(mode);
}

QString Settings::bgSlideShowDir()
{
    return settings->value("bgSlideShowDir", QString()).toString();
}

void Settings::setBgSlideShowDir(QString dir)
{
    settings->setValue("bgSlideShowDir", dir);
    emit bgSlideShowDirChanged(dir);
}

bool Settings::audioDownmix()
{
    return settings->value("audioDownmix", false).toBool();
}

void Settings::setAudioDownmix(bool downmix)
{
    settings->setValue("audioDownmix", downmix);
}

bool Settings::audioDownmixBm()
{
    return settings->value("audioDownmixBm", false).toBool();
}

void Settings::setAudioDownmixBm(bool downmix)
{
    settings->setValue("audioDownmixBm", downmix);
}

bool Settings::audioDetectSilence()
{
    return settings->value("audioDetectSilence", false).toBool();
}

bool Settings::audioDetectSilenceBm()
{
    return settings->value("audioDetectSilenceBm", false).toBool();
}

void Settings::setAudioDetectSilence(bool enabled)
{
    settings->setValue("audioDetectSilence", enabled);
}

void Settings::setAudioDetectSilenceBm(bool enabled)
{
    settings->setValue("audioDetectSilenceBm", enabled);
}

QString Settings::audioOutputDevice()
{
    return settings->value("audioOutputDevice", 0).toString();
}

QString Settings::audioOutputDeviceBm()
{
    return settings->value("audioOutputDeviceBm", 0).toString();
}

void Settings::setAudioOutputDevice(QString device)
{
    settings->setValue("audioOutputDevice", device);
}

void Settings::setAudioOutputDeviceBm(QString device)
{
    settings->setValue("audioOutputDeviceBm", device);
}

int Settings::audioBackend()
{
    return settings->value("audioBackend", 0).toInt();
}

void Settings::setAudioBackend(int index)
{
    settings->setValue("audioBackend", index);
    emit audioBackendChanged(index);
}

QString Settings::recordingContainer()
{
    return settings->value("recordingContainer", "ogg").toString();
}

void Settings::setRecordingContainer(QString container)
{
    settings->setValue("recordingContainer", container);
    emit recordingSetupChanged();
}

QString Settings::recordingCodec()
{
    return settings->value("recordingCodec", "undefined").toString();
}

void Settings::setRecordingCodec(QString codec)
{
    settings->setValue("recordingCodec", codec);
    emit recordingSetupChanged();
}

QString Settings::recordingInput()
{
    return settings->value("recordingInput", "undefined").toString();
}

void Settings::setRecordingInput(QString input)
{
    settings->setValue("recordingInput", input);
    emit recordingSetupChanged();
}

QString Settings::recordingOutputDir()
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    return settings->value("recordingOutputDir", defaultPath).toString();
}

void Settings::setRecordingOutputDir(QString path)
{
    settings->setValue("recordingOutputDir", path);
    emit recordingSetupChanged();
}

bool Settings::recordingEnabled()
{
    return settings->value("recordingEnabled", false).toBool();
}

void Settings::setRecordingEnabled(bool enabled)
{
    settings->setValue("recordingEnabled", enabled);
    emit recordingSetupChanged();
}

QString Settings::recordingRawExtension()
{
    return settings->value("recordingRawExtension", QString()).toString();
}

void Settings::setRecordingRawExtension(QString extension)
{
    settings->setValue("recordingRawExtension", extension);
}

int Settings::cdgVOffset()
{
    return settings->value("cdgVOffset", 0).toInt();
}

void Settings::setCdgVOffset(int pixels)
{
    settings->setValue("cdgVOffset", pixels);
    emit cdgVOffsetChanged(pixels);
}

void Settings::setShowQueueRemovalWarning(bool show)
{
    settings->setValue("showQueueRemovalWarning", show);
    emit showQueueRemovalWarningChanged(show);
}

void Settings::setShowSingerRemovalWarning(bool show)
{
    settings->setValue("showSingerRemovalWarning", show);
    emit showSingerRemovalWarningChanged(show);
}

int Settings::cdgHOffset()
{
    return settings->value("cdgHOffset", 0).toInt();
}

void Settings::setCdgHOffset(int pixels)
{
    settings->setValue("cdgHOffset", pixels);
    emit cdgHOffsetChanged(pixels);
}

int Settings::cdgVSizeAdjustment()
{
    return settings->value("cdgVSizeAdjustment", 0).toInt();
}

void Settings::setCdgVSizeAdjustment(int pixels)
{
    settings->setValue("cdgVSizeAdjustment", pixels);
    emit cdgVSizeAdjustmentChanged(pixels);
}

int Settings::cdgHSizeAdjustment()
{
    return settings->value("cdgHSizeAdjustment", 0).toInt();
}

bool Settings::ignoreAposInSearch()
{
    return settings->value("ignoreAposInSearch", false).toBool();
}

void Settings::setIgnoreAposInSearch(bool ignore)
{
    settings->setValue("ignoreAposInSearch", ignore);
}

void Settings::setCdgDisplayOffset(int offset)
{
    settings->setValue("CDGDisplayOffset", offset);
    emit cdgDisplayOffsetChanged(offset);
}

void Settings::setShowSongPauseStopWarning(bool enabled)
{
    settings->setValue("showStopPauseInterruptWarning", enabled);
    emit showSongStopPauseWarningChanged(enabled);
    
}

void Settings::setBookCreatorHeaderFont(QFont font)
{
    settings->setValue("bookCreatorHeaderFont", font.toString());
}

void Settings::setBookCreatorItemFont(QFont font)
{
    settings->setValue("bookCreatorItemFont", font.toString());
}

void Settings::setBookCreatorSortCol(int col)
{
    settings->setValue("bookCreatorSortCol", col);
}

void Settings::setBookCreatorMarginRt(double margin)
{
    settings->setValue("bookCreatorMarginRt", margin);
}

void Settings::setBookCreatorMarginLft(double margin)
{
    settings->setValue("bookCreatorMarginLft", margin);
}

void Settings::setBookCreatorMarginTop(double margin)
{
    settings->setValue("bookCreatorMarginTop", margin);
}

void Settings::setBookCreatorMarginBtm(double margin)
{
    settings->setValue("bookCreatorMarginBtm", margin);
}

void Settings::setEqKBypass(bool bypass)
{
    settings->setValue("eqKBypass", bypass);
    emit eqKBypassChanged(bypass);
}

void Settings::setEqBBypass(bool bypass)
{
    settings->setValue("eqBBypass", bypass);
    emit eqBBypassChanged(bypass);
}

void Settings::setEqKLevel1(int level)
{
    settings->setValue("eqKLevel1", level);
    emit eqKLevel1Changed(level);
}

void Settings::setEqKLevel2(int level)
{
    settings->setValue("eqKLevel2", level);
    emit eqKLevel2Changed(level);
}

void Settings::setEqKLevel3(int level)
{
    settings->setValue("eqKLevel3", level);
    emit eqKLevel3Changed(level);
}

void Settings::setEqKLevel4(int level)
{
    settings->setValue("eqKLevel4", level);
    emit eqKLevel4Changed(level);
}

void Settings::setEqKLevel5(int level)
{
    settings->setValue("eqKLevel5", level);
    emit eqKLevel5Changed(level);
}

void Settings::setEqKLevel6(int level)
{
    settings->setValue("eqKLevel6", level);
    emit eqKLevel6Changed(level);
}

void Settings::setEqKLevel7(int level)
{
    settings->setValue("eqKLevel7", level);
    emit eqKLevel7Changed(level);
}

void Settings::setEqKLevel8(int level)
{
    settings->setValue("eqKLevel8", level);
    emit eqKLevel8Changed(level);
}

void Settings::setEqKLevel9(int level)
{
    settings->setValue("eqKLevel9", level);
    emit eqKLevel9Changed(level);
}

void Settings::setEqKLevel10(int level)
{
    settings->setValue("eqKLevel10", level);
    emit eqKLevel10Changed(level);
}

void Settings::setEqBLevel1(int level)
{
    settings->setValue("eqBLevel1", level);
    emit eqBLevel1Changed(level);
}

void Settings::setEqBLevel2(int level)
{
    settings->setValue("eqBLevel2", level);
    emit eqBLevel2Changed(level);
}

void Settings::setEqBLevel3(int level)
{
    settings->setValue("eqBLevel3", level);
    emit eqBLevel3Changed(level);
}

void Settings::setEqBLevel4(int level)
{
    settings->setValue("eqBLevel4", level);
    emit eqBLevel4Changed(level);
}

void Settings::setEqBLevel5(int level)
{
    settings->setValue("eqBLevel5", level);
    emit eqBLevel5Changed(level);
}

void Settings::setEqBLevel6(int level)
{
    settings->setValue("eqBLevel6", level);
    emit eqBLevel6Changed(level);
}

void Settings::setEqBLevel7(int level)
{
    settings->setValue("eqBLevel7", level);
    emit eqBLevel7Changed(level);
}

void Settings::setEqBLevel8(int level)
{
    settings->setValue("eqBLevel8", level);
    emit eqBLevel8Changed(level);
}

void Settings::setEqBLevel9(int level)
{
    settings->setValue("eqBLevel9", level);
    emit eqBLevel9Changed(level);
}

void Settings::setEqBLevel10(int level)
{
    settings->setValue("eqBLevel10", level);
    emit eqBLevel10Changed(level);
}

void Settings::setRequestServerInterval(int interval)
{
    settings->setValue("requestServerInterval", interval);
    emit requestServerIntervalChanged(interval);
}

void Settings::setTickerShowRotationInfo(bool show)
{
    settings->setValue("tickerShowRotationInfo", show);
    emit tickerShowRotationInfoChanged(show);
    emit tickerOutputModeChanged();
}

void Settings::setCdgHSizeAdjustment(int pixels)
{
    settings->setValue("cdgHSizeAdjustment", pixels);
    emit cdgHSizeAdjustmentChanged(pixels);
}

bool Settings::bmShowFilenames()
{
    return settings->value("showFilenames", false).toBool();
}

void Settings::bmSetShowFilenames(bool show)
{
    settings->setValue("showFilenames", show);
}

bool Settings::bmShowMetadata()
{
    return settings->value("showMetadata", true).toBool();
}

void Settings::bmSetShowMetadata(bool show)
{
    settings->setValue("showMetadata", show);
}

int Settings::bmVolume()
{
    return settings->value("volume", 50).toInt();
}

void Settings::bmSetVolume(int volume)
{
    settings->setValue("volume", volume);
}

int Settings::bmPlaylistIndex()
{
    return settings->value("playlistIndex",0).toInt();
}

void Settings::bmSetPlaylistIndex(int index)
{
    settings->setValue("playlistIndex", index);
}

int Settings::mplxMode()
{
    return settings->value("mplxMode", 0).toInt();
}

void Settings::setMplxMode(int mode)
{
    settings->setValue("mplxMode", mode);
    emit mplxModeChanged(mode);
}

bool Settings::karaokeAutoAdvance()
{
    return settings->value("karaokeAutoAdvance", false).toBool();
}

void Settings::setKaraokeAutoAdvance(bool enabled)
{
    settings->setValue("karaokeAutoAdvance", enabled);
    emit karaokeAutoAdvanceChanged(enabled);
}

void Settings::setShowSongInterruptionWarning(bool enabled)
{
    settings->setValue("showSongInterruptionWarning", enabled);
    emit showSongInterruptionWarningChanged(enabled);
}

void Settings::setAlertBgColor(QColor color)
{
    settings->setValue("alertBgColor", color);
    emit alertBgColorChanged(color);
}

void Settings::setAlertTxtColor(QColor color)
{
    settings->setValue("alertTxtColor", color);
    emit alertTxtColorChanged(color);
}

int Settings::karaokeAATimeout()
{
    return settings->value("karaokeAATimeout", 30).toInt();
}

void Settings::setKaraokeAATimeout(int secs)
{
    settings->setValue("karaokeAATimeout", secs);
}

bool Settings::karaokeAAAlertEnabled()
{
    return settings->value("karaokeAAAlertEnabled", false).toBool();
}

void Settings::setKaraokeAAAlertEnabled(bool enabled)
{
    settings->setValue("karaokeAAAlertEnabled", enabled);
}

QFont Settings::karaokeAAAlertFont()
{
    QFont font;
    font.fromString(settings->value("karaokeAAAlertFont", QApplication::font().toString()).toString());
    return font;
}

void Settings::setKaraokeAAAlertFont(QFont font)
{
    settings->setValue("karaokeAAAlertFont", font.toString());
    emit karaokeAAAlertFontChanged(font);
}

bool Settings::showQueueRemovalWarning()
{
    return settings->value("showQueueRemovalWarning", true).toBool();
}

bool Settings::showSingerRemovalWarning()
{
    return settings->value("showSingerRemovalWarning", true).toBool();
}

bool Settings::showSongInterruptionWarning()
{
    return settings->value("showSongInterruptionWarning", true).toBool();
}

bool Settings::showSongPauseStopWarning()
{
    return settings->value("showStopPauseInterruptWarning", false).toBool();
}



QColor Settings::alertTxtColor()
{
    return settings->value("alertTxtColor", QApplication::palette().foreground().color()).value<QColor>();
}

QColor Settings::alertBgColor()
{
    return settings->value("alertBgColor", QApplication::palette().background().color()).value<QColor>();
}

bool Settings::bmAutoStart()
{
    return settings->value("bmAutoStart", false).toBool();
}

void Settings::setBmAutoStart(bool enabled)
{
    settings->setValue("bmAutoStart", enabled);
}

int Settings::cdgDisplayOffset()
{
    return settings->value("CDGDisplayOffset", 0).toInt();
}

QFont Settings::bookCreatorItemFont()
{
    QFont font;
    font.fromString(settings->value("bookCreatorItemFont", QApplication::font().toString()).toString());
    return font;
}

QFont Settings::bookCreatorHeaderFont()
{
    QFont font;
    font.fromString(settings->value("bookCreatorHeaderFont", QApplication::font().toString()).toString());
    return font;
}

int Settings::bookCreatorSortCol()
{
    return settings->value("bookCreatorSortCol", 0).toInt();
}

double Settings::bookCreatorMarginRt()
{
    return settings->value("bookCreatorMarginRt", 0.25).toDouble();
}

double Settings::bookCreatorMarginLft()
{
    return settings->value("bookCreatorMarginLft", 0.25).toDouble();
}

double Settings::bookCreatorMarginTop()
{
    return settings->value("bookCreatorMarginTop", 0.25).toDouble();
}

double Settings::bookCreatorMarginBtm()
{
    return settings->value("bookCreatorMarginBtm", 0.25).toDouble();
}

bool Settings::eqKBypass()
{
    return settings->value("eqKBypass", true).toBool();
}

bool Settings::eqBBypass()
{
    return settings->value("eqBBypass", true).toBool();
}

int Settings::eqKLevel1()
{
    return settings->value("eqKLevel1", 0).toInt();
}

int Settings::eqKLevel2()
{
    return settings->value("eqKLevel2", 0).toInt();
}

int Settings::eqKLevel3()
{
    return settings->value("eqKLevel3", 0).toInt();
}

int Settings::eqKLevel4()
{
    return settings->value("eqKLevel4", 0).toInt();
}

int Settings::eqKLevel5()
{
    return settings->value("eqKLevel5", 0).toInt();
}

int Settings::eqKLevel6()
{
    return settings->value("eqKLevel6", 0).toInt();
}

int Settings::eqKLevel7()
{
    return settings->value("eqKLevel7", 0).toInt();
}

int Settings::eqKLevel8()
{
    return settings->value("eqKLevel8", 0).toInt();
}

int Settings::eqKLevel9()
{
    return settings->value("eqKLevel9", 0).toInt();
}

int Settings::eqKLevel10()
{
    return settings->value("eqKLevel10", 0).toInt();
}

int Settings::eqBLevel1()
{
    return settings->value("eqBLevel1", 0).toInt();
}

int Settings::eqBLevel2()
{
    return settings->value("eqBLevel2", 0).toInt();
}

int Settings::eqBLevel3()
{
    return settings->value("eqBLevel3", 0).toInt();
}

int Settings::eqBLevel4()
{
    return settings->value("eqBLevel4", 0).toInt();
}

int Settings::eqBLevel5()
{
    return settings->value("eqBLevel5", 0).toInt();
}

int Settings::eqBLevel6()
{
    return settings->value("eqBLevel6", 0).toInt();
}

int Settings::eqBLevel7()
{
    return settings->value("eqBLevel7", 0).toInt();
}

int Settings::eqBLevel8()
{
    return settings->value("eqBLevel8", 0).toInt();
}

int Settings::eqBLevel9()
{
    return settings->value("eqBLevel9", 0).toInt();
}

int Settings::eqBLevel10()
{
    return settings->value("eqBLevel10", 0).toInt();
}

int Settings::requestServerInterval()
{
    return settings->value("requestServerInterval", 30).toInt();
}

bool Settings::bmKCrossFade()
{
    return settings->value("bmKCrossFade", true).toBool();
}

void Settings::setBmKCrossfade(bool enabled)
{
    settings->setValue("bmKCrossFade", enabled);
}
