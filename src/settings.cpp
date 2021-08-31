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

#include "settings.h"
#include <QCoreApplication>
#include <QApplication>
#include <QDesktopWidget>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDataStream>
#include "simplecrypt.h"
#include <QStandardPaths>
#include <QDir>
#include <QDataStream>
#include <QFontDatabase>
#include <QUuid>
#include <fstream>

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <sysinfoapi.h>
#endif



bool Settings::lastStartupOk() const
{
    return settings->value("startupOk", true).toBool();
}

void Settings::setStartupOk(const bool ok)
{
    settings->setValue("startupOk", ok);
}

QString Settings::lastRunVersion() const
{
    return settings->value("lastRunVersion", "0.0.0").toString();
}

void Settings::setLastRunVersion(const QString &version)
{
    settings->setValue("lastRunVersion", version);
}

bool Settings::safeStartupMode() const
{
    return m_safeStartupMode;
}

void Settings::setSafeStartupMode(const bool safeMode)
{
    m_safeStartupMode = safeMode;
}

int Settings::historyDblClickAction() const
{
    return settings->value("historyDblClickAction", 0).toInt();
}

void Settings::setHistoryDblClickAction(const int index)
{
    settings->setValue("historyDblClickAction", index);
}

int Settings::getSystemRamSize()
{
#if defined(Q_OS_LINUX)
    std::string token;
    std::ifstream file("/proc/meminfo");
    while(file >> token) {
        if(token == "MemTotal:") {
            unsigned long mem;
            if(file >> mem) {
                return mem;
            } else {
                return 0;
            }
        }
        // ignore rest of the line
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return 0; // nothing found
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx (&statex);
    return statex.ullTotalPhys / 1024;
#endif
    return 0;
}

int Settings::remainRtOffset()
{
    return settings->value("remainRtOffset", 5).toInt();
}

int Settings::remainBtmOffset()
{
    return settings->value("remainBtmOffset", 5).toInt();
}

qint64 Settings::hash(const QString &str)
{
    QByteArray hash = QCryptographicHash::hash(
      QByteArray::fromRawData((const char*)str.utf16(), str.length()*2),
      QCryptographicHash::Md5
    );
    Q_ASSERT(hash.size() == 16);
    QDataStream stream(hash);
    qint64 a, b;
    stream >> a >> b;
    return a ^ b;
}

bool Settings::progressiveSearchEnabled()
{
    return settings->value("progressiveSearchEnabled", true).toBool();
}

QString Settings::storeDownloadDir()
{
    return settings->value("storeDownloadDir", QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + QDir::separator() + "OpenKJ_Downloads" + QDir::separator()).toString();
}

QString Settings::logDir()
{
    return settings->value("logDir", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + "OpenKJ_Logs" + QDir::separator()).toString();
}

bool Settings::logShow()
{
    return settings->value("logVisible", false).toBool();
}

bool Settings::logEnabled()
{
    return settings->value("logEnabled", false).toBool();
}

void Settings::setStoreDownloadDir(QString path)
{
    settings->setValue("storeDownloadDir", path);
}

void Settings::setLogEnabled(bool enabled)
{
    settings->setValue("logEnabled", enabled);
}

void Settings::setLogVisible(bool visible)
{
    settings->setValue("logVisible", visible);
}

void Settings::setLogDir(QString path)
{
    settings->setValue("logDir", path);
}

void Settings::setCurrentRotationPosition(int position)
{
    settings->setValue("currentRotationPosition", position);
}

void Settings::dbSetDirectoryWatchEnabled(bool val)
{
    settings->setValue("directoryWatchEnabled", val);
}

void Settings::setPassword(QString password)
{
    qint64 passHash = this->hash(password);
    SimpleCrypt simpleCrypt(passHash);
    QString pchk = simpleCrypt.encryptToString(QString("testpass"));
    settings->setValue("pchk", pchk);
}

void Settings::clearPassword()
{
    settings->remove("pchk");
    clearCC();
    clearKNAccount();
}

bool Settings::chkPassword(QString password)
{
    qint64 passHash = this->hash(password);
    SimpleCrypt simpleCrypt(passHash);
    QString pchk = simpleCrypt.decryptToString(settings->value("pchk", QString()).toString());
    if (pchk == "testpass")
        return true;
    else
        return false;
}

bool Settings::passIsSet()
{
    if (settings->contains("pchk"))
        return true;
    return false;
}

void Settings::setCC(QString ccn, QString month, QString year, QString ccv, QString passwd)
{
    QString cc = ccn + "," + month + "," + year + "," + ccv;
    SimpleCrypt simpleCrypt(this->hash(passwd));
    settings->setValue("cc", simpleCrypt.encryptToString(cc));
}

void Settings::setSaveCC(bool save)
{
    settings->setValue("saveCC", save);
}

bool Settings::saveCC()
{
    return settings->value("saveCC", false).toBool();
}

void Settings::clearCC()
{
    settings->remove("cc");
}

void Settings::clearKNAccount()
{
    settings->remove("karaokeDotNetUser");
    settings->remove("karaokeDotNetPass");
}



void Settings::setSaveKNAccount(bool save)
{
    settings->setValue("saveKNAccount", save);
}

bool Settings::saveKNAccount()
{
    return settings->value("saveKNAccount", false).toBool();
}

bool Settings::testingEnabled()
{
    return settings->value("testingEnabled", false).toBool();
}

bool Settings::hardwareAccelEnabled()
{
    bool hwAccelDefault{true};
#ifdef Q_OS_MACOS
    hwAccelDefault = false;
#endif
    return settings->value("hardwareAccelEnabled", hwAccelDefault).toBool();
}

bool Settings::dbDoubleClickAddsSong()
{
    return settings->value("dbDoubleClickAddsSong", false).toBool();
}

QString Settings::getCCN(const QString &password)
{
    SimpleCrypt simpleCrypt(this->hash(password));
    QString encrypted = settings->value("cc", QString()).toString();
    if (encrypted == QString())
        return QString();
    QString cc = simpleCrypt.decryptToString(encrypted);
    QStringList parts = cc.split(",");
    return parts.at(0);
}

QString Settings::getCCM(const QString &password)
{
    SimpleCrypt simpleCrypt(this->hash(password));
    QString encrypted = settings->value("cc", QString()).toString();
    if (encrypted == QString())
        return QString();
    QString cc = simpleCrypt.decryptToString(encrypted);
    QStringList parts = cc.split(",");
    return parts.at(1);
}

QString Settings::getCCY(const QString &password)
{
    SimpleCrypt simpleCrypt(this->hash(password));
    QString encrypted = settings->value("cc", QString()).toString();
    if (encrypted == QString())
        return QString();
    QString cc = simpleCrypt.decryptToString(encrypted);
    QStringList parts = cc.split(",");
    return parts.at(2);
}

QString Settings::getCCV(const QString &password)
{
    SimpleCrypt simpleCrypt(this->hash(password));
    QString encrypted = settings->value("cc", QString()).toString();
    if (encrypted == QString())
        return QString();
    QString cc = simpleCrypt.decryptToString(encrypted);
    QStringList parts = cc.split(",");
    return parts.at(3);
}

void Settings::setKaroakeDotNetUser(const QString &username, const QString &password)
{
    SimpleCrypt simpleCrypt(this->hash(password));
    settings->setValue("karaokeDotNetUser", simpleCrypt.encryptToString(username));
}

void Settings::setKaraokeDotNetPass(const QString &KDNPassword, const QString &password)
{
    SimpleCrypt simpleCrypt(this->hash(password));
    settings->setValue("karaokeDotNetPass", simpleCrypt.encryptToString(KDNPassword));
}

QString Settings::karoakeDotNetUser(const QString &password)
{
    SimpleCrypt simpleCrypt(this->hash(password));
    QString encrypted = settings->value("karaokeDotNetUser", QString()).toString();
    if (encrypted == QString())
        return QString();
    QString username = simpleCrypt.decryptToString(encrypted);
    return username;
}

QString Settings::karoakeDotNetPass(const QString &password)
{
    SimpleCrypt simpleCrypt(this->hash(password));
    QString encrypted = settings->value("karaokeDotNetPass", QString()).toString();
    if (encrypted == QString())
        return QString();
    QString KDNpassword = simpleCrypt.decryptToString(encrypted);
    return KDNpassword;
}

Settings::Settings(QObject *parent) :
    QObject(parent)
{
    QCoreApplication::setOrganizationName("OpenKJ");
    QCoreApplication::setOrganizationDomain("OpenKJ.org");
    QCoreApplication::setApplicationName("OpenKJ");
#ifdef Q_OS_LINUX
    settings = new QSettings(this);
#else
    QDir khDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    if (!khDir.exists())
    {
        khDir.mkpath(khDir.absolutePath());
    }
    settings = new QSettings(khDir.absolutePath() + QDir::separator() + "openkj.ini", QSettings::IniFormat);
#endif
}

bool Settings::cdgWindowFullscreen()
{
    if (m_safeStartupMode)
        return false;
    return settings->value("cdgWindowFullscreen", false).toBool();
}

void Settings::setCdgWindowFullscreen(bool fullScreen)
{
    settings->setValue("cdgWindowFullscreen", fullScreen);
}


bool Settings::showCdgWindow()
{
    if (m_safeStartupMode)
        return false;
    return settings->value("showCdgWindow", false).toBool();
}

void Settings::setShowCdgWindow(bool show)
{
    settings->setValue("showCdgWindow", show);
}

void Settings::setCdgWindowFullscreenMonitor(int monitor)
{
    settings->setValue("cdgWindowFullScreenMonitor", monitor);
}

int Settings::cdgWindowFullScreenMonitor()
{
    //We default to the highest mointor present, by default, rather than the primary display.  Seems to make more sense
    //and will help prevent people from popping up a full screen window in front of the main window and getting confused.
    return settings->value("cdgWindowFullScreenMonitor", QGuiApplication::screens().count() - 1).toInt();
}

void Settings::saveWindowState(QWidget *window)
{
    settings->beginGroup(window->objectName());
    //settings->setValue("size", window->size());
    //settings->setValue("pos", window->pos());
    settings->setValue("geometry", window->saveGeometry());
    settings->endGroup();
}

void Settings::restoreWindowState(QWidget *window)
{
    if (m_safeStartupMode)
        return;
    settings->beginGroup(window->objectName());
    if (settings->contains("geometry"))
    {
        window->restoreGeometry(settings->value("geometry").toByteArray());
    }
    else if (settings->contains("size") && settings->contains("pos"))
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
    if (m_safeStartupMode)
        return;
    settings->beginGroup(treeView->objectName());
    if ((settings->contains("headerState")) && (settings->value("hiddenSections").toInt() == treeView->header()->hiddenSectionCount()) && (settings->value("sections").toInt() == treeView->header()->count()))
        treeView->header()->restoreState(settings->value("headerState").toByteArray());
    settings->endGroup();
}

void Settings::restoreColumnWidths(QTableView *tableView)
{
    if (m_safeStartupMode)
        return;
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
    if (m_safeStartupMode)
        return;
    settings->beginGroup(splitter->objectName());
    if (settings->contains("splitterState"))
        splitter->restoreState(settings->value("splitterState").toByteArray());
    settings->endGroup();
}

void Settings::setTickerFont(const QFont &font)
{
    settings->setValue("tickerFont", font.toString());
}

void Settings::setApplicationFont(const QFont &font)
{
    settings->setValue("applicationFont", font.toString());
    QApplication::setFont(font, "QWidget");
    QApplication::setFont(font, "QMenu");
}

QFont Settings::tickerFont()
{
    QFontDatabase fdb;
    QFont font;
    QFont defaultFont = QApplication::font();
    if (fdb.hasFamily("Roboto Medium"))
        defaultFont = QFont("Roboto Medium");
    else if (fdb.hasFamily("Verdana"))
        defaultFont = QFont("Verdana");
    defaultFont.setPointSize(48);
    font.fromString(settings->value("tickerFont", defaultFont.toString()).toString());
    return font;
}

QFont Settings::applicationFont() const
{
    QFontDatabase fdb;
    QFont font;
    QFont defaultFont = QApplication::font();
    if (fdb.hasFamily("Roboto"))
        defaultFont = QFont("Roboto");
    else if (fdb.hasFamily("Verdana"))
        defaultFont = QFont("Verdana");
    defaultFont.setPointSize(14);
    font.fromString(settings->value("applicationFont", defaultFont.toString()).toString());
    return font;
}

int Settings::tickerHeight()
{
    return settings->value("tickerHeight", 25).toInt();
}

void Settings::setTickerHeight(int height)
{
    settings->setValue("tickerHeight", height);
}

int Settings::tickerSpeed()
{
    if (settings->value("tickerSpeed").toInt() > 50)
        return 25;
    return settings->value("tickerSpeed", 25).toInt();
}

void Settings::setTickerSpeed(int speed)
{
    settings->setValue("tickerSpeed", speed);
}

QColor Settings::tickerTextColor()
{
    return settings->value("tickerTextColor", QApplication::palette().windowText().color()).value<QColor>();
}

void Settings::setTickerTextColor(QColor color)
{
    settings->setValue("tickerTextColor", color);
}

QFont Settings::cdgRemainFont()
{
    QFontDatabase fdb;
    QFont font;
    QFont defaultFont = QApplication::font();
    if (fdb.hasFamily("Roboto Mono Medium"))
        defaultFont = QFont("Roboto Mono Medium");
    else if (fdb.hasFamily("Source Code Pro Medium"))
        defaultFont = QFont("Source Code Pro Medium");
    else if (fdb.hasFamily("Verdana"))
        defaultFont = QFont("Verdana");
    defaultFont.setPointSize(48);
    font.fromString(settings->value("cdgRemainFont", defaultFont.toString()).toString());
    return font;
}

QColor Settings::cdgRemainTextColor()
{
    return settings->value("cdgRemainTextColor", QApplication::palette().windowText().color()).value<QColor>();
}

QColor Settings::cdgRemainBgColor()
{
    return settings->value("cdgRemainBgColor", QApplication::palette().window().color()).value<QColor>();

}

bool Settings::rotationShowNextSong() const
{
    return settings->value("rotationShowNextSong", false).toBool();
}

void Settings::sync()
{
    settings->sync();
}

bool Settings::previewEnabled()
{
    return settings->value("previewEnabled", true).toBool();
}

bool Settings::showMainWindowVideo()
{
    return settings->value("showMainWindowVideo", true).toBool();
}

void Settings::setShowMainWindowVideo(bool show)
{
    settings->setValue("showMainWindowVideo", show);
}

bool Settings::showMainWindowSoundClips()
{
    return settings->value("showMainWindowSoundClips", false).toBool();
}

void Settings::setShowMplxControls(const bool show)
{
     settings->setValue("showMplxControls", show);
}

bool Settings::showMplxControls()
{
    return settings->value("showMplxControls", true).toBool();
}

void Settings::setShowMainWindowSoundClips(const bool &show)
{
    settings->setValue("showMainWindowSoundClips", show);
}

bool Settings::showMainWindowNowPlaying()
{
    return settings->value("showMainWindowNowPlaying", true).toBool();
}

void Settings::setShowMainWindowNowPlaying(const bool &show)
{
    settings->setValue("showMainWindowNowPlaying", show);
}

int Settings::mainWindowVideoSize()
{
    return settings->value("mainWindowVideoSize", 0).toInt();
}

void Settings::setMainWindowVideoSize(const Settings::PreviewSize &size)
{
    settings->setValue("mainWindowVideoSize", size);
}

bool Settings::enforceAspectRatio()
{
    return settings->value("enforceAspectRatio", true).toBool();
}

void Settings::setEnforceAspectRatio(const bool &enforce)
{
    settings->setValue("enforceAspectRatio", enforce);
    emit enforceAspectRatioChanged(enforce);
}

QString Settings::auxTickerFile()
{
    return settings->value("auxTickerFile", QString()).toString();
}

QString Settings::uuid()
{
    if (!settings->contains("uuid"))
    {
        QString uuid = QUuid::createUuid().toString();
        settings->setValue("uuid", uuid);
    }
    return settings->value("uuid", QVariant()).toString();
}

uint Settings::slideShowInterval()
{
    return settings->value("slideShowInterval", 15).toUInt();
}

int Settings::lastSingerAddPositionType()
{
    return settings->value("lastSingerAddPositionType", 0).toInt();
}

void Settings::saveShortcutKeySequence(const QString &name, const QKeySequence &sequence)
{
    settings->setValue("shortcutKeySequence-" + name, sequence);
    emit shortcutsChanged();
}

QKeySequence Settings::loadShortcutKeySequence(const QString &name)
{
    return settings->value("shortcutKeySequence-" + name, QString()).toString();
}

bool Settings::cdgPrescalingEnabled()
{
    return settings->value("cdgPrescaling", false).toBool();
}

bool Settings::rotationAltSortOrder() const
{
    return settings->value("rotationAltSortOrder", true).toBool();
}

bool Settings::treatAllSingersAsRegs()
{
    return settings->value("treatAllSingersAsRegs", false).toBool();
}

void Settings::setTreatAllSingersAsRegs(const bool enabled)
{
    settings->setValue("treatAllSingersAsRegs", enabled);
    emit treatAllSingersAsRegsChanged(enabled);
}

void Settings::setRotationAltSortOrder(bool enabled)
{
    settings->setValue("rotationAltSortOrder", enabled);
}

void Settings::setCdgPrescalingEnabled(bool enabled)
{
    settings->setValue("cdgPrescaling", enabled);
}

void Settings::setSlideShowInterval(int secs)
{
    if (secs <= 5)
    {
        settings->setValue("slideShowInterval", 5);
        return;
    }
    settings->setValue("slideShowInterval", secs);
}

void Settings::setHardwareAccelEnabled(const bool enabled)
{
#ifdef Q_OS_MACOS
    return;
#endif
    settings->setValue("hardwareAccelEnabled", enabled);
}

void Settings::setDbDoubleClickAddsSong(const bool enabled)
{
    settings->setValue("dbDoubleClickAddsSong", enabled);
}

void Settings::setDurationPosition(const QPoint pos)
{
    settings->setValue("DurationPosition", pos);
}

void Settings::resetDurationPosition()
{
}

void Settings::setRemainRtOffset(int offset)
{
    settings->setValue("remainRtOffset", offset);
    emit remainOffsetChanged(remainRtOffset(), remainBtmOffset());
}

void Settings::setRemainBtmOffset(int offset)
{
    settings->setValue("remainBtmOffset", offset);
    emit remainOffsetChanged(remainRtOffset(), remainBtmOffset());
}

bool Settings::cdgRemainEnabled()
{
    return settings->value("cdgRemainEnabled", false).toBool();
}

void Settings::setCdgRemainFont(QFont font)
{
    settings->setValue("cdgRemainFont", font.toString());
}

void Settings::setCdgRemainTextColor(QColor color)
{
    settings->setValue("cdgRemainTextColor", color);
}

void Settings::setCdgRemainBgColor(QColor color)
{
    settings->setValue("cdgRemainBgColor", color);
}

void Settings::setRotationShowNextSong(bool show)
{
    settings->setValue("rotationShowNextSong", show);
    emit rotationShowNextSongChanged(show);
}

void Settings::setProgressiveSearchEnabled(bool enabled)
{
    settings->setValue("progressiveSearchEnabled", enabled);
}

void Settings::setPreviewEnabled(bool enabled)
{
    settings->setValue("previewEnabled", enabled);
    emit previewEnabledChanged(enabled);
}

void Settings::setVideoOffsetMs(int offset)
{
    settings->setValue("videoOffsetMs", offset);
    emit videoOffsetChanged(offset);
}

void Settings::setLastSingerAddPositionType(const int type)
{
    settings->setValue("lastSingerAddPositionType", type);
    emit lastSingerAddPositionTypeChanged(type);
}

QColor Settings::tickerBgColor()
{
    return settings->value("tickerBgColor", QApplication::palette().window().color()).value<QColor>();
}

void Settings::setTickerBgColor(QColor color)
{
    settings->setValue("tickerBgColor", color);
}

bool Settings::tickerFullRotation()
{
    return settings->value("tickerFullRotation", true).toBool();
}

void Settings::setTickerFullRotation(bool full)
{
    settings->setValue("tickerFullRotation", full);
}

int Settings::tickerShowNumSingers()
{
    return settings->value("tickerShowNumSingers", 10).toInt();
}

void Settings::setTickerShowNumSingers(int limit)
{
    settings->setValue("tickerShowNumSingers", limit);
}

void Settings::setTickerEnabled(bool enable)
{
    settings->setValue("tickerEnabled", enable);
}

bool Settings::tickerEnabled()
{
    return settings->value("tickerEnabled", false).toBool();
}

QString Settings::tickerCustomString()
{
    return settings->value("tickerCustomString", "").toString();
}

void Settings::setTickerCustomString(const QString &value)
{
    settings->setValue("tickerCustomString", value);
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
    emit requestServerEnabledChanged(enable);
}

QString Settings::requestServerUrl()
{
    QString url = settings->value("requestServerUrl", "https://api.okjsongbook.com").toString();
    if (url == "https://songbook.openkj.org/api")
    {
        url = "https://api.okjsongbook.com";
        setRequestServerUrl(url);
    }
    return url;
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
}

Settings::BgMode Settings::bgMode()
{
    return (Settings::BgMode)settings->value("bgMode", 0).toInt();
}

void Settings::setBgMode(Settings::BgMode mode)
{
    settings->setValue("bgMode", mode);
}

QString Settings::bgSlideShowDir()
{
    return settings->value("bgSlideShowDir", QString()).toString();
}

void Settings::setBgSlideShowDir(QString dir)
{
    settings->setValue("bgSlideShowDir", dir);
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
}

QString Settings::recordingContainer()
{
    return settings->value("recordingContainer", "ogg").toString();
}

void Settings::setRecordingContainer(QString container)
{
    settings->setValue("recordingContainer", container);
}

QString Settings::recordingCodec()
{
    return settings->value("recordingCodec", "undefined").toString();
}

void Settings::setRecordingCodec(QString codec)
{
    settings->setValue("recordingCodec", codec);
}

QString Settings::recordingInput()
{
    return settings->value("recordingInput", "undefined").toString();
}

void Settings::setRecordingInput(QString input)
{
    settings->setValue("recordingInput", input);
}

QString Settings::recordingOutputDir()
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    return settings->value("recordingOutputDir", defaultPath).toString();
}

void Settings::setRecordingOutputDir(QString path)
{
    settings->setValue("recordingOutputDir", path);
}

bool Settings::recordingEnabled()
{
    return settings->value("recordingEnabled", false).toBool();
}

void Settings::setRecordingEnabled(bool enabled)
{
    settings->setValue("recordingEnabled", enabled);
}

QString Settings::recordingRawExtension()
{
    return settings->value("recordingRawExtension", QString()).toString();
}

void Settings::setRecordingRawExtension(QString extension)
{
    settings->setValue("recordingRawExtension", extension);
}

void Settings::setCdgOffsetTop(int pixels)
{
    settings->setValue("cdgOffsetTop", pixels);
}

void Settings::setCdgOffsetBottom(int pixels)
{
    settings->setValue("cdgOffsetBottom", pixels);
}

void Settings::setCdgOffsetLeft(int pixels)
{
    settings->setValue("cdgOffsetLeft", pixels);
}

void Settings::setCdgOffsetRight(int pixels)
{
    settings->setValue("cdgOffsetRight", pixels);
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

int Settings::cdgOffsetTop()
{
    return settings->value("cdgOffsetTop", 0).toInt();
}

int Settings::cdgOffsetBottom()
{
    return settings->value("cdgOffsetBottom", 0).toInt();
}

int Settings::cdgOffsetLeft()
{
    return settings->value("cdgOffsetLeft", 0).toInt();
}

int Settings::cdgOffsetRight()
{
    return settings->value("cdgOffsetRight", 0).toInt();
}
bool Settings::ignoreAposInSearch()
{
    return settings->value("ignoreAposInSearch", false).toBool();
}

int Settings::videoOffsetMs()
{
    return settings->value("videoOffsetMs", 0).toInt();
}

void Settings::setIgnoreAposInSearch(bool ignore)
{
    settings->setValue("ignoreAposInSearch", ignore);
}

void Settings::setShowSongPauseStopWarning(bool enabled)
{
    settings->setValue("showStopPauseInterruptWarning", enabled);
    emit showSongStopPauseWarningChanged(enabled);

}

void Settings::setBookCreatorArtistFont(QFont font)
{
    settings->setValue("bookCreatorArtistFont", font.toString());
}

void Settings::setBookCreatorTitleFont(QFont font)
{
    settings->setValue("bookCreatorTitleFont", font.toString());
}

void Settings::setBookCreatorHeaderFont(QFont font)
{
    settings->setValue("bookCreatorHeaderFont", font.toString());
}

void Settings::setBookCreatorFooterFont(QFont font)
{
    settings->setValue("bookCreatorFooterFont", font.toString());
}

void Settings::setBookCreatorHeaderText(QString text)
{
    settings->setValue("bookCreatorHeaderText", text);
}

void Settings::setBookCreatorFooterText(QString text)
{
    settings->setValue("bookCreatorFooterText", text);
}

void Settings::setBookCreatorPageNumbering(bool show)
{
    settings->setValue("bookCreatorPageNumbering", show);
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
}

void Settings::setEqKLevel(int band, int level)
{
    // eq bands in settings are indexed 1-10
    settings->setValue(QString("eqKLevel%1").arg(band + 1), level);
}

void Settings::setEqBBypass(bool bypass)
{
    settings->setValue("eqBBypass", bypass);
}

void Settings::setEqBLevel(int band, int level)
{
    // eq bands in settings are indexed 1-10
    settings->setValue(QString("eqBLevel%1").arg(band + 1), level);
}

void Settings::setRequestServerInterval(int interval)
{
    settings->setValue("requestServerInterval", interval);
    emit requestServerIntervalChanged(interval);
}

void Settings::setTickerShowRotationInfo(bool show)
{
    settings->setValue("tickerShowRotationInfo", show);
}

void Settings::setRequestRemoveOnRotAdd(bool remove)
{
    settings->setValue("requestRemoveOnRotAdd", remove);
}

void Settings::setRequestDialogAutoShow(bool enabled)
{
    settings->setValue("requestDialogAutoShow", enabled);
}

void Settings::setCheckUpdates(bool enabled)
{
    return settings->setValue("checkUpdates", enabled);
}

void Settings::setUpdatesBranch(int index)
{
    settings->setValue("updatesBranch", index);
}

void Settings::setTheme(int theme)
{
    settings->setValue("theme", theme);
}

void Settings::setBookCreatorCols(int cols)
{
    settings->setValue("bookCreatorCols", cols);
}

void Settings::setBookCreatorPageSize(int size)
{
    settings->setValue("bookCreatorPageSize", size);
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
}

void Settings::setAlertTxtColor(QColor color)
{
    settings->setValue("alertTxtColor", color);
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
    return settings->value("alertTxtColor", QApplication::palette().windowText().color()).value<QColor>();
}

QColor Settings::alertBgColor()
{
    return settings->value("alertBgColor", QApplication::palette().window().color()).value<QColor>();
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

QFont Settings::bookCreatorTitleFont()
{
    QFont font;
    font.fromString(settings->value("bookCreatorTitleFont", QApplication::font().toString()).toString());
    return font;
}

QFont Settings::bookCreatorArtistFont()
{
    QFont font;
    font.fromString(settings->value("bookCreatorArtistFont", QApplication::font().toString()).toString());
    return font;
}

QFont Settings::bookCreatorHeaderFont()
{
    QFont font;
    font.fromString(settings->value("bookCreatorHeaderFont", QApplication::font().toString()).toString());
    return font;
}

QFont Settings::bookCreatorFooterFont()
{
    QFont font;
    font.fromString(settings->value("bookCreatorFooterFont", QApplication::font().toString()).toString());
    return font;
}

QString Settings::bookCreatorHeaderText()
{
    return settings->value("bookCreatorHeaderText", QString()).toString();
}

QString Settings::bookCreatorFooterText()
{
    return settings->value("bookCreatorFooterText", QString()).toString();
}

bool Settings::bookCreatorPageNumbering()
{
    return settings->value("bookCreatorPageNumbering", false).toBool();
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

int Settings::bookCreatorCols()
{
    return settings->value("bookCreatorCols", 2).toInt();
}

int Settings::bookCreatorPageSize()
{
    return settings->value("bookCreatorPageSize", 0).toInt();
}

bool Settings::eqKBypass()
{
    return settings->value("eqKBypass", true).toBool();
}

int Settings::getEqKLevel(int band)
{
    // eq bands in settings are indexed 1-10
    return settings->value(QString("eqKLevel%1").arg(band + 1), 0).toInt();
}

bool Settings::eqBBypass()
{
    return settings->value("eqBBypass", true).toBool();
}

int Settings::getEqBLevel(int band)
{
    // eq bands in settings are indexed 1-10
    return settings->value(QString("eqBLevel%1").arg(band + 1), 0).toInt();
}

int Settings::requestServerInterval()
{
    return settings->value("requestServerInterval", 30).toInt();
}

bool Settings::bmKCrossFade()
{
    return settings->value("bmKCrossFade", true).toBool();
}

bool Settings::requestRemoveOnRotAdd()
{
    return settings->value("requestRemoveOnRotAdd", false).toBool();
}

bool Settings::requestDialogAutoShow()
{
    return settings->value("requestDialogAutoShow", true).toBool();
}

bool Settings::checkUpdates()
{
    return settings->value("checkUpdates", true).toBool();
}

int Settings::updatesBranch()
{
    return settings->value("updatesBranch", 0).toInt();
}

int Settings::theme() const
{
    return settings->value("theme", 1).toInt();
}

const QPoint Settings::durationPosition()
{
    return settings->value("DurationPosition", QPoint(0,0)).toPoint();
}

bool Settings::dbDirectoryWatchEnabled()
{
    return settings->value("directoryWatchEnabled", false).toBool();
}

SfxEntryList Settings::getSfxEntries()
{
    QStringList buttons = settings->value("sfxEntryButtons", QStringList()).toStringList();
    QStringList paths = settings->value("sfxEntryPaths", QStringList()).toStringList();
    SfxEntryList list;
    for (int i=0; i < buttons.size(); i++)
    {
        SfxEntry entry;
        entry.name = buttons.at(i);
        entry.path = paths.at(i);
        list.append(entry);
    }
    return list;
}

void Settings::addSfxEntry(SfxEntry entry)
{
    SfxEntryList list = getSfxEntries();
    list.append(entry);
    setSfxEntries(list);
}

void Settings::setSfxEntries(SfxEntryList entries)
{
    QStringList buttons;
    QStringList paths;
    foreach (SfxEntry entry, entries) {
       buttons.append(entry.name);
       paths.append(entry.path);
    }
    QVariant v = QVariant::fromValue(entries).toList();
    settings->setValue("sfxEntryButtons", buttons);
    settings->setValue("sfxEntryPaths", paths);
}

int Settings::estimationSingerPad() const
{
    return settings->value("estimationSingerPad", 60).toInt();
}

void Settings::setEstimationSingerPad(int secs)
{
    settings->setValue("estimationSingerPad", secs);
    emit rotationDurationSettingsModified();
}

int Settings::estimationEmptySongLength() const
{
    return settings->value("estimationEmptySongLength", 240).toInt();
}

void Settings::setEstimationEmptySongLength(int secs)
{
    settings->setValue("estimationEmptySongLength", secs);
    emit rotationDurationSettingsModified();
}

bool Settings::estimationSkipEmptySingers() const
{
    return settings->value("estimationSkipEmptySingers", false).toBool();
}

void Settings::setEstimationSkipEmptySingers(bool skip)
{
    settings->setValue("estimationSkipEmptySingers", skip);
    emit rotationDurationSettingsModified();
}

bool Settings::rotationDisplayPosition() const
{
    return settings->value("rotationDisplayPosition", false).toBool();
}

void Settings::setRotationDisplayPosition(bool show)
{
    settings->setValue("rotationDisplayPosition", show);
    emit rotationDisplayPositionChanged(show);
}

int Settings::currentRotationPosition()
{
    return settings->value("currentRotationPosition", -1).toInt();
}

bool Settings::dbSkipValidation()
{
    return settings->value("dbSkipValidation", true).toBool();
}

void Settings::dbSetSkipValidation(bool val)
{
    settings->setValue("dbSkipValidation", val);
}

bool Settings::dbLazyLoadDurations()
{
    return settings->value("dbLazyLoadDurations", true).toBool();
}

void Settings::dbSetLazyLoadDurations(bool val)
{
    settings->setValue("dbLazyLoadDurations", val);
}

void Settings::setBmKCrossfade(bool enabled)
{
    settings->setValue("bmKCrossFade", enabled);
}

SfxEntry::SfxEntry()
{

}

int Settings::systemId()
{
    return settings->value("systemId", 1).toInt();
}

void Settings::setSystemId(int id)
{
    return settings->setValue("systemId", id);
}

void Settings::setCdgRemainEnabled(bool enabled)
{
    settings->setValue("cdgRemainEnabled", enabled);
}

int Settings::lastRunRotationTopSingerId() {
    return settings->value("lastRunRotationTopSingerId", -1).toInt();
}

void Settings::setLastRunRotationTopSingerId(const int id) {
    settings->setValue("lastRunRotationTopSingerId", id);
}

int Settings::getConsoleLogLevel() {
    return settings->value("consoleLogLevel", LOG_LEVEL_WARNING).toInt();
}

int Settings::getFileLogLevel() {
    return settings->value("fileLogLevel", LOG_LEVEL_DISABLED).toInt();
}

void Settings::setConsoleLogLevel(int level) {
    settings->setValue("consoleLogLevel", level);
}

void Settings::setFileLogLevel(int level) {
    settings->setValue("fileLogLevel", level);
}

bool Settings::tickerReducedCpuMode() {
    return settings->value("tickerReducedCpuMode", true).toBool();
}

void Settings::setTickerReducedCpuMode(bool enabled) {
    settings->setValue("tickerReducedCpuMode", enabled);
}
