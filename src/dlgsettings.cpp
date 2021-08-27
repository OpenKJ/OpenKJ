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

#include "dlgsettings.h"
#include "ui_dlgsettings.h"
#include <QDebug>
#include <QGuiApplication>
#include <QDesktopWidget>
#include <QFontDialog>
#include <QColorDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QSqlQuery>
#include <QtSql>
#include <QXmlStreamWriter>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QKeySequenceEdit>
#include "audiorecorder.h"
#include <QScreen>




DlgSettings::DlgSettings(MediaBackend &AudioBackend, MediaBackend &BmAudioBackend, OKJSongbookAPI &songbookAPI,
                         QWidget *parent) :
        QDialog(parent),
        kAudioBackend(AudioBackend),
        bmAudioBackend(BmAudioBackend),
        songbookApi(songbookAPI),
        ui(new Ui::DlgSettings) {
    m_pageSetupDone = false;
    networkManager = new QNetworkAccessManager(this);
    ui->setupUi(this);
    m_settings.restoreWindowState(this);
    ui->checkBoxHardwareAccel->setChecked(m_settings.hardwareAccelEnabled());
#ifdef Q_OS_MACOS
    ui->checkBoxHardwareAccel->setHidden(true);
#endif
    ui->tabWidgetMain->setCurrentIndex(0);
    ui->checkBoxDbSkipValidation->setChecked(m_settings.dbSkipValidation());
    ui->checkBoxLazyLoadDurations->setChecked(m_settings.dbLazyLoadDurations());
    ui->checkBoxMonitorDirs->setChecked(m_settings.dbDirectoryWatchEnabled());
    ui->groupBoxShowDuration->setChecked(m_settings.cdgRemainEnabled());
    ui->cbxRotShowNextSong->setChecked(m_settings.rotationShowNextSong());
    ui->checkBoxCdgPrescaling->setChecked(m_settings.cdgPrescalingEnabled());
    ui->checkBoxCurrentSingerTop->setChecked(m_settings.rotationAltSortOrder());
    audioOutputDevices = kAudioBackend.getOutputDevices();
    ui->comboBoxKAudioDevices->addItems(audioOutputDevices);
    ui->checkBoxShowAddDlgOnDbDblclk->setChecked(m_settings.dbDoubleClickAddsSong());
    int selDevice = audioOutputDevices.indexOf(m_settings.audioOutputDevice());
    if (selDevice == -1) {
        ui->comboBoxKAudioDevices->setCurrentIndex(0);
    } else {
        ui->comboBoxKAudioDevices->setCurrentIndex(selDevice);
    }
    ui->comboBoxBAudioDevices->addItems(audioOutputDevices);
    selDevice = audioOutputDevices.indexOf(m_settings.audioOutputDeviceBm());
    if (selDevice == -1)
        ui->comboBoxBAudioDevices->setCurrentIndex(0);
    else {
        ui->comboBoxBAudioDevices->setCurrentIndex(selDevice);
    }
    ui->checkBoxProgressiveSearch->setChecked(m_settings.progressiveSearchEnabled());
    ui->horizontalSliderTickerSpeed->setValue(m_settings.tickerSpeed());
    QString ss = ui->pushButtonTextColor->styleSheet();
    QColor clr = m_settings.tickerTextColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                                QString::number(clr.blue())));
    ui->pushButtonTextColor->setStyleSheet(ss);
    ss = ui->pushButtonBgColor->styleSheet();
    clr = m_settings.tickerBgColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                                QString::number(clr.blue())));
    ui->pushButtonBgColor->setStyleSheet(ss);
    ss = ui->btnAlertTxtColor->styleSheet();
    clr = m_settings.alertTxtColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                                QString::number(clr.blue())));
    ui->btnAlertTxtColor->setStyleSheet(ss);
    ss = ui->btnAlertBgColor->styleSheet();
    clr = m_settings.alertBgColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                                QString::number(clr.blue())));
    ui->btnAlertBgColor->setStyleSheet(ss);

    ss = ui->btnDurationFontColor->styleSheet();
    clr = m_settings.cdgRemainTextColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                                QString::number(clr.blue())));
    ui->btnDurationFontColor->setStyleSheet(ss);

    ss = ui->btnDurationBgColor->styleSheet();
    clr = m_settings.cdgRemainBgColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                                QString::number(clr.blue())));
    ui->btnDurationBgColor->setStyleSheet(ss);

    if (m_settings.tickerFullRotation()) {
        ui->radioButtonFullRotation->setChecked(true);
        ui->spinBoxTickerSingers->setEnabled(false);
    } else {
        ui->radioButtonPartialRotation->setChecked(true);
        ui->spinBoxTickerSingers->setEnabled(true);
    }
    ui->spinBoxTickerSingers->setValue(m_settings.tickerShowNumSingers());
    ui->groupBoxRequestServer->setChecked(m_settings.requestServerEnabled());
    ui->lineEditUrl->setText(m_settings.requestServerUrl());
    ui->lineEditApiKey->setText(m_settings.requestServerApiKey());
    ui->checkBoxIgnoreCertErrors->setChecked(m_settings.requestServerIgnoreCertErrors());
    if ((m_settings.bgMode() == m_settings.BG_MODE_IMAGE) || (m_settings.bgSlideShowDir() == ""))
        ui->rbBgImage->setChecked(true);
    else
        ui->rbSlideshow->setChecked(true);
    ui->lineEditCdgBackground->setText(m_settings.cdgDisplayBackgroundImage());
    ui->lineEditSlideshowDir->setText(m_settings.bgSlideShowDir());
    ui->checkBoxFader->setChecked(m_settings.audioUseFader());
    ui->checkBoxDownmix->setChecked(m_settings.audioDownmix());
    ui->checkBoxSilenceDetection->setChecked(m_settings.audioDetectSilence());
    ui->checkBoxFaderBm->setChecked(m_settings.audioUseFaderBm());
    ui->checkBoxDownmixBm->setChecked(m_settings.audioDownmixBm());
    ui->checkBoxSilenceDetectionBm->setChecked(m_settings.audioDetectSilenceBm());
    ui->spinBoxInterval->setValue(m_settings.requestServerInterval());
    ui->spinBoxSystemId->setMaximum(songbookApi.entitledSystemCount());
    ui->spinBoxSystemId->setValue(m_settings.systemId());
    ui->spinBoxCdgOffsetTop->setValue(m_settings.cdgOffsetTop());
    ui->spinBoxCdgOffsetBottom->setValue(m_settings.cdgOffsetBottom());
    ui->spinBoxCdgOffsetLeft->setValue(m_settings.cdgOffsetLeft());
    ui->spinBoxCdgOffsetRight->setValue(m_settings.cdgOffsetRight());
    ui->spinBoxSlideshowInterval->setValue(m_settings.slideShowInterval());

    AudioRecorder recorder;
    QStringList inputs = recorder.getDeviceList();
    QStringList codecs = recorder.getCodecs();
    ui->groupBoxRecording->setChecked(m_settings.recordingEnabled());
    ui->comboBoxDevice->addItems(inputs);
    ui->comboBoxCodec->addItems(codecs);
    QString recordingInput = m_settings.recordingInput();
    if (recordingInput == "undefined")
        ui->comboBoxDevice->setCurrentIndex(0);
    else
        ui->comboBoxDevice->setCurrentIndex(ui->comboBoxDevice->findText(m_settings.recordingInput()));
    QString recordingCodec = m_settings.recordingCodec();
    if (recordingCodec == "undefined")
        ui->comboBoxCodec->setCurrentIndex(1);
    else
        ui->comboBoxCodec->setCurrentIndex(ui->comboBoxCodec->findText(m_settings.recordingCodec()));
    ui->comboBoxUpdateBranch->addItem("Stable");
    ui->comboBoxUpdateBranch->addItem("Development");
    ui->cbxTheme->addItem("OS Native");
    ui->cbxTheme->addItem("Fusion Dark");
    ui->cbxTheme->addItem("Fusion Light");
    ui->cbxTheme->setCurrentIndex(m_settings.theme());
    ui->lineEditOutputDir->setText(m_settings.recordingOutputDir());
    ui->cbxTickerShowRotationInfo->setChecked(m_settings.tickerShowRotationInfo());
    ui->groupBoxTicker->setChecked(m_settings.tickerEnabled());
    ui->lineEditTickerMessage->setText(m_settings.tickerCustomString());
    ui->fontComboBox->setFont(m_settings.applicationFont());
    ui->spinBoxAppFontSize->setValue(m_settings.applicationFont().pointSize());
    ui->checkBoxIncludeEmptySingers->setChecked(!m_settings.estimationSkipEmptySingers());
    ui->spinBoxDefaultPadTime->setValue(m_settings.estimationSingerPad());
    ui->spinBoxDefaultSongDuration->setValue(m_settings.estimationEmptySongLength());
    ui->checkBoxDisplayCurrentRotationPosition->setChecked(m_settings.rotationDisplayPosition());
    ui->spinBoxAADelay->setValue(m_settings.karaokeAATimeout());
    ui->checkBoxKAA->setChecked(m_settings.karaokeAutoAdvance());
    ui->checkBoxShowKAAAlert->setChecked(m_settings.karaokeAAAlertEnabled());
    ui->cbxQueueRemovalWarning->setChecked(m_settings.showQueueRemovalWarning());
    ui->cbxSingerRemovalWarning->setChecked(m_settings.showSingerRemovalWarning());
    ui->cbxSongInterruptionWarning->setChecked(m_settings.showSongInterruptionWarning());
    ui->cbxBmAutostart->setChecked(m_settings.bmAutoStart());
    ui->cbxIgnoreApos->setChecked(m_settings.ignoreAposInSearch());
    ui->spinBoxVideoOffset->setValue(m_settings.videoOffsetMs());
    ui->cbxStopPauseWarning->setChecked(m_settings.showSongPauseStopWarning());
    ui->cbxCheckUpdates->setChecked(m_settings.checkUpdates());
    ui->comboBoxUpdateBranch->setCurrentIndex(m_settings.updatesBranch());
    ui->lineEditDownloadsDir->setText(m_settings.storeDownloadDir());
    ui->checkBoxLogging->setChecked(m_settings.logEnabled());
    ui->lineEditLogDir->setText(m_settings.logDir());
    ui->checkBoxEnforceAspectRatio->setChecked(m_settings.enforceAspectRatio());
    ui->checkBoxTreatAllSingersAsRegs->setChecked(m_settings.treatAllSingersAsRegs());
    ui->cbxCrossFade->setChecked(m_settings.bmKCrossFade());
    adjustSize();
    tickerFontChanged();
    tickerBgColorChanged();
    tickerTextColorChanged();
    tickerOutputModeChanged();
    tickerCustomStringChanged();
    tickerSpeedChanged();
    connect(ui->checkBoxTreatAllSingersAsRegs, &QCheckBox::toggled, this, &DlgSettings::treatAllSingersAsRegsChanged);
    connect(ui->checkBoxEnforceAspectRatio, &QCheckBox::toggled, this, &DlgSettings::enforceAspectRatioChanged);
    connect(ui->spinBoxCdgOffsetTop, qOverload<int>(&QSpinBox::valueChanged), &m_settings, &Settings::setCdgOffsetTop);
    connect(ui->spinBoxCdgOffsetTop, qOverload<int>(&QSpinBox::valueChanged), this, &DlgSettings::cdgOffsetsChanged);
    connect(ui->spinBoxCdgOffsetBottom, qOverload<int>(&QSpinBox::valueChanged), &m_settings, &Settings::setCdgOffsetBottom);
    connect(ui->spinBoxCdgOffsetBottom, qOverload<int>(&QSpinBox::valueChanged), this, &DlgSettings::cdgOffsetsChanged);
    connect(ui->spinBoxCdgOffsetLeft, qOverload<int>(&QSpinBox::valueChanged), &m_settings, &Settings::setCdgOffsetLeft);
    connect(ui->spinBoxCdgOffsetLeft, qOverload<int>(&QSpinBox::valueChanged), this, &DlgSettings::cdgOffsetsChanged);
    connect(ui->spinBoxCdgOffsetRight, qOverload<int>(&QSpinBox::valueChanged), &m_settings, &Settings::setCdgOffsetRight);
    connect(ui->spinBoxCdgOffsetRight, qOverload<int>(&QSpinBox::valueChanged), this, &DlgSettings::cdgOffsetsChanged);
    connect(ui->spinBoxVideoOffset, qOverload<int>(&QSpinBox::valueChanged), &m_settings, &Settings::setVideoOffsetMs);
    connect(ui->spinBoxVideoOffset, qOverload<int>(&QSpinBox::valueChanged), this, &DlgSettings::videoOffsetChanged);
    connect(ui->spinBoxSlideshowInterval, qOverload<int>(&QSpinBox::valueChanged), &m_settings, &Settings::setSlideShowInterval);
    connect(ui->spinBoxSlideshowInterval, qOverload<int>(&QSpinBox::valueChanged), this, &DlgSettings::slideShowIntervalChanged);
    connect(ui->checkBoxKAA, &QCheckBox::toggled, this, &DlgSettings::karaokeAutoAdvanceChanged);
    connect(&m_settings, &Settings::showQueueRemovalWarningChanged, ui->cbxQueueRemovalWarning, &QCheckBox::setChecked);
    connect(&m_settings, &Settings::showSingerRemovalWarningChanged, ui->cbxSingerRemovalWarning, &QCheckBox::setChecked);
    connect(&m_settings, &Settings::showSongInterruptionWarningChanged, ui->cbxSongInterruptionWarning, &QCheckBox::setChecked);
    connect(&m_settings, &Settings::showSongStopPauseWarningChanged, ui->cbxStopPauseWarning, &QCheckBox::setChecked);
    connect(ui->cbxIgnoreApos, &QCheckBox::toggled, &m_settings, &Settings::setIgnoreAposInSearch);
    connect(ui->cbxCrossFade, &QCheckBox::toggled, &m_settings, &Settings::setBmKCrossfade);
    connect(ui->cbxCheckUpdates, &QCheckBox::toggled, &m_settings, &Settings::setCheckUpdates);
    connect(ui->comboBoxUpdateBranch, qOverload<int>(&QComboBox::currentIndexChanged), &m_settings, &Settings::setUpdatesBranch);
    connect(ui->checkBoxDbSkipValidation, &QCheckBox::toggled, &m_settings, &Settings::dbSetSkipValidation);
    connect(ui->checkBoxLazyLoadDurations, &QCheckBox::toggled, &m_settings, &Settings::dbSetLazyLoadDurations);
    connect(ui->checkBoxMonitorDirs, &QCheckBox::toggled, &m_settings, &Settings::dbSetDirectoryWatchEnabled);
    connect(ui->spinBoxSystemId, qOverload<int>(&QSpinBox::valueChanged), &m_settings, &Settings::setSystemId);
    connect(ui->checkBoxLogging, &QCheckBox::toggled, &m_settings, &Settings::setLogEnabled);
    connect(ui->checkBoxTreatAllSingersAsRegs, &QAbstractButton::toggled, &m_settings,
            &Settings::setTreatAllSingersAsRegs);
    connect(ui->checkBoxShowAddDlgOnDbDblclk, &QCheckBox::stateChanged, [&](auto state) {
        if (state == 0)
            m_settings.setDbDoubleClickAddsSong(false);
        else
            m_settings.setDbDoubleClickAddsSong(true);
    });
    connect(networkManager, &QNetworkAccessManager::finished, this, &DlgSettings::onNetworkReply);
    connect(networkManager, &QNetworkAccessManager::sslErrors, this, &DlgSettings::onSslErrors);
    connect(ui->cbxQueueRemovalWarning, &QCheckBox::toggled, &m_settings, &Settings::setShowQueueRemovalWarning);
    connect(ui->cbxSingerRemovalWarning, &QCheckBox::toggled, &m_settings, &Settings::setShowSingerRemovalWarning);
    connect(ui->cbxSongInterruptionWarning, &QCheckBox::toggled, &m_settings, &Settings::setShowSongInterruptionWarning);
    connect(ui->cbxStopPauseWarning, &QCheckBox::toggled, &m_settings, &Settings::setShowSongPauseStopWarning);
    connect(ui->cbxTickerShowRotationInfo, &QCheckBox::toggled, &m_settings, &Settings::setTickerShowRotationInfo);
    connect(ui->cbxTickerShowRotationInfo, &QCheckBox::toggled, this, &DlgSettings::tickerOutputModeChanged);
    connect(&songbookApi, &OKJSongbookAPI::entitledSystemCountChanged, this, &DlgSettings::entitledSystemCountChanged);
    connect(ui->cbxRotShowNextSong, &QCheckBox::toggled, &m_settings, &Settings::setRotationShowNextSong);
    connect(ui->cbxRotShowNextSong, &QCheckBox::toggled, this, &DlgSettings::rotationShowNextSongChanged);
    setupHotkeysForm();
    m_pageSetupDone = true;
}

DlgSettings::~DlgSettings() {
    delete ui;
}

QStringList DlgSettings::getMonitors() {
    QStringList screenStrings;
    for (int i = 0; i < QGuiApplication::screens().size(); i++) {
        auto sWidth = QGuiApplication::screens().at(i)->size().width();
        auto sHeight = QGuiApplication::screens().at(i)->size().height();
        screenStrings
                << "Monitor " + QString::number(i) + " - " + QString::number(sWidth) + "x" + QString::number(sHeight);
    }
    return screenStrings;
}

void DlgSettings::setupHotkeysForm() {
    std::vector<KeyboardShortcut> shortcuts;
    shortcuts.emplace_back(KeyboardShortcut{
            "Karaoke - Volume up",
            "kVolUp"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Karaoke - Volume down",
            "kVolDn"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Karaoke - Volume mute/unmute",
            "kVolMute"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Karaoke - Pause/unpause",
            "kPause"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Karaoke - Stop",
            "kStop"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Karaoke - Jump back (5s)",
            "kRwnd"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Karaoke - Jump forward (5s)",
            "kFfwd"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Karaoke - Restart track",
            "kRestartSong"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Karaoke - Play next unsung queued karaoke song",
            "kPlayNextUnsung"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Karaoke - Select next singer w/ unsung queued songs",
            "kSelectNextSinger"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Break Music - Volume up",
            "bVolUp"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Break Music - Volume down",
            "bVolDn"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Break Music - Volume mute/unmute",
            "bVolMute"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Break Music - Pause/unpause",
            "bPause"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Break Music - Stop",
            "bStop"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Break Music - Jump back (5s)",
            "bRwnd"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Break Music - Jump forward (5s)",
            "bFfwd"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Break Music - Restart track",
            "bRestartSong"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Toggle singer/video window visibility",
            "toggleSingerWindow"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Show add singer dialog",
            "addSinger"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Show regular singers dialog",
            "loadRegularSinger"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Show the incoming requests dialog (if enabled)",
            "showIncomingRequests"
    });
    shortcuts.emplace_back(KeyboardShortcut{
            "Jump to karaoke search (second press clears search)",
            "jumpToSearch"
    });

    for (auto &shortcut : shortcuts) {
        auto clearButton = new QPushButton(this);
        auto sequenceEdit = new QKeySequenceEdit(this);
        sequenceEdit->setObjectName(shortcut.sequenceName);
        sequenceEdit->setKeySequence(m_settings.loadShortcutKeySequence(shortcut.sequenceName));
        clearButton->setIcon(QIcon(":/theme/Icons/okjbreeze-dark/actions/22/edit-clear.svg"));
        connect(sequenceEdit, &QKeySequenceEdit::keySequenceChanged, this, &DlgSettings::keySequenceEditChanged);
        connect(clearButton, &QPushButton::pressed, sequenceEdit, &QKeySequenceEdit::clear);
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget(sequenceEdit);
        layout->addWidget(clearButton);
        ui->formLayoutHotkeys->addRow(new QLabel(shortcut.description, this), layout);
    }
}

void DlgSettings::keySequenceEditChanged(QKeySequence sequence) {
    m_settings.saveShortcutKeySequence(sender()->objectName(), sequence);
    emit shortcutsChanged();
}


void DlgSettings::onNetworkReply(QNetworkReply *reply) {
    Q_UNUSED(reply);
}

void DlgSettings::onSslErrors(QNetworkReply *reply) {
    if (m_settings.requestServerIgnoreCertErrors())
        reply->ignoreSslErrors();
    else {
        QMessageBox::warning(this, "SSL Error", "Unable to establish secure conneciton with server.");
    }
}


void DlgSettings::on_btnClose_clicked() {
    close();
}

void DlgSettings::on_pushButtonFont_clicked() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_settings.tickerFont(), this, "Select ticker font");
    if (ok) {
        m_settings.setTickerFont(font);
        emit tickerFontChanged();
    }
}

void DlgSettings::on_horizontalSliderTickerSpeed_valueChanged(int value) {
    if (!m_pageSetupDone)
        return;
    m_settings.setTickerSpeed(value);
    emit tickerSpeedChanged();
}

void DlgSettings::on_pushButtonTextColor_clicked() {
    QColor clr = QColorDialog::getColor(m_settings.tickerTextColor(), this, "Select ticker text color");
    if (clr.isValid()) {
        QString ss = ui->pushButtonTextColor->styleSheet();
        QColor oclr = m_settings.tickerTextColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," +
                           QString::number(oclr.blue())),
                   QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                           QString::number(clr.blue())));
        ui->pushButtonTextColor->setStyleSheet(ss);
        m_settings.setTickerTextColor(clr);
        emit tickerTextColorChanged();
    }
}

void DlgSettings::on_pushButtonBgColor_clicked() {
    QColor clr = QColorDialog::getColor(m_settings.tickerBgColor(), this, "Select ticker background color");
    if (clr.isValid()) {
        QString ss = ui->pushButtonBgColor->styleSheet();
        QColor oclr = m_settings.tickerBgColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," +
                           QString::number(oclr.blue())),
                   QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                           QString::number(clr.blue())));
        ui->pushButtonBgColor->setStyleSheet(ss);
        m_settings.setTickerBgColor(clr);
        emit tickerBgColorChanged();
    }
}

void DlgSettings::on_radioButtonFullRotation_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    m_settings.setTickerFullRotation(checked);
    ui->spinBoxTickerSingers->setEnabled(!checked);
    emit tickerOutputModeChanged();
}


void DlgSettings::on_spinBoxTickerSingers_valueChanged(int arg1) {
    if (!m_pageSetupDone)
        return;
    m_settings.setTickerShowNumSingers(arg1);
    emit tickerOutputModeChanged();
}

void DlgSettings::on_groupBoxTicker_toggled(bool arg1) {
    if (!m_pageSetupDone)
        return;
    m_settings.setTickerEnabled(arg1);
    emit tickerEnableChanged();
}

void DlgSettings::on_lineEditUrl_editingFinished() {
    m_settings.setRequestServerUrl(ui->lineEditUrl->text());
}

void DlgSettings::on_checkBoxIgnoreCertErrors_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    m_settings.setRequestServerIgnoreCertErrors(checked);
}

void DlgSettings::on_groupBoxRequestServer_toggled(bool arg1) {
    if (!m_pageSetupDone)
        return;
    m_settings.setRequestServerEnabled(arg1);
    emit requestServerEnableChanged(arg1);
}

void DlgSettings::on_pushButtonBrowse_clicked() {
    QString imageFile = QFileDialog::getOpenFileName(this, QString("Select image file"),
                                                     QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                                     QString("Images (*.png *.jpg *.jpeg *.gif)"), nullptr, QFileDialog::DontUseNativeDialog);
    if (imageFile != "") {
        QImage image(imageFile);
        if (!image.isNull()) {
            m_settings.setCdgDisplayBackgroundImage(imageFile);
            emit cdgBgImageChanged();
            ui->lineEditCdgBackground->setText(imageFile);
        } else {
            QMessageBox::warning(this, tr("Image load error"), QString("Unsupported or corrupt image file."));
        }
    }
}

void DlgSettings::on_checkBoxFader_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    m_settings.setAudioUseFader(checked);
    emit audioUseFaderChanged(checked);
}

void DlgSettings::on_checkBoxFaderBm_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    m_settings.setAudioUseFaderBm(checked);
    emit audioUseFaderChangedBm(checked);
}

void DlgSettings::on_checkBoxSilenceDetection_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    m_settings.setAudioDetectSilence(checked);
    emit audioSilenceDetectChanged(checked);
}

void DlgSettings::on_checkBoxSilenceDetectionBm_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    m_settings.setAudioDetectSilenceBm(checked);
    emit audioSilenceDetectChangedBm(checked);
}

void DlgSettings::on_checkBoxDownmix_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    m_settings.setAudioDownmix(checked);
    emit audioDownmixChanged(checked);
}

void DlgSettings::on_checkBoxDownmixBm_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    m_settings.setAudioDownmixBm(checked);
    emit audioDownmixChangedBm(checked);
}

void DlgSettings::on_comboBoxDevice_currentIndexChanged(const QString &arg1) {
    if (!m_pageSetupDone)
        return;
    m_settings.setRecordingInput(arg1);
}

void DlgSettings::on_comboBoxCodec_currentIndexChanged(const QString &arg1) {
    if (!m_pageSetupDone)
        return;
    m_settings.setRecordingCodec(arg1);
    if (arg1 == "audio/mpeg")
        m_settings.setRecordingRawExtension("mp3");
}

void DlgSettings::on_groupBoxRecording_toggled(bool arg1) {
    if (!m_pageSetupDone)
        return;
    m_settings.setRecordingEnabled(arg1);
}

void DlgSettings::on_buttonBrowse_clicked() {
    QString dirName = QFileDialog::getExistingDirectory(
            this,
            "Select output directory",
            QStandardPaths::standardLocations(QStandardPaths::MusicLocation).at(0),
            QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog
            );
    if (dirName != "") {
        m_settings.setRecordingOutputDir(dirName);
        ui->lineEditOutputDir->setText(dirName);
    }
}

void DlgSettings::on_pushButtonClearBgImg_clicked() {
    m_settings.setCdgDisplayBackgroundImage(QString());
    ui->lineEditCdgBackground->setText(QString());
    emit cdgBgImageChanged();
}

void DlgSettings::on_pushButtonSlideshowBrowse_clicked() {
    QString initialPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (m_settings.bgSlideShowDir() != "")
        initialPath = m_settings.bgSlideShowDir();
    QString dirName = QFileDialog::getExistingDirectory(
            this,
            "Select the slideshow directory",
            initialPath,
            QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly
            );
    if (dirName != "") {
        m_settings.setBgSlideShowDir(dirName);
        emit bgSlideShowDirChanged(dirName);
        ui->lineEditSlideshowDir->setText(dirName);
    }
}

void DlgSettings::on_rbSlideshow_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    Settings::BgMode mode = (checked) ? m_settings.BG_MODE_SLIDESHOW : m_settings.BG_MODE_IMAGE;
    m_settings.setBgMode(mode);
    emit bgModeChanged(mode);
}

void DlgSettings::on_rbBgImage_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    Settings::BgMode mode = (checked) ? m_settings.BG_MODE_IMAGE : m_settings.BG_MODE_SLIDESHOW;
    m_settings.setBgMode(mode);
    emit bgModeChanged(mode);
}

void DlgSettings::on_lineEditApiKey_editingFinished() {
    m_settings.setRequestServerApiKey(ui->lineEditApiKey->text());
}

void DlgSettings::on_checkBoxShowKAAAlert_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    m_settings.setKaraokeAAAlertEnabled(checked);
}

void DlgSettings::on_checkBoxKAA_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    m_settings.setKaraokeAutoAdvance(checked);
}

void DlgSettings::on_spinBoxAADelay_valueChanged(int arg1) {
    if (!m_pageSetupDone)
        return;
    m_settings.setKaraokeAATimeout(arg1);
}

void DlgSettings::on_btnAlertFont_clicked() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_settings.karaokeAAAlertFont(), this, "Select alert font");
    if (ok) {
        m_settings.setKaraokeAAAlertFont(font);
        emit karaokeAAAlertFontChanged(font);
    }
}

void DlgSettings::on_btnAlertTxtColor_clicked() {
    QColor clr = QColorDialog::getColor(m_settings.alertTxtColor(), this, "Select alert text color");
    if (clr.isValid()) {
        QString ss = ui->btnAlertTxtColor->styleSheet();
        QColor oclr = m_settings.alertTxtColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," +
                           QString::number(oclr.blue())),
                   QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                           QString::number(clr.blue())));
        ui->btnAlertTxtColor->setStyleSheet(ss);
        m_settings.setAlertTxtColor(clr);
        emit alertTxtColorChanged(clr);
    }
}

void DlgSettings::on_btnAlertBgColor_clicked() {
    QColor clr = QColorDialog::getColor(m_settings.alertBgColor(), this, "Select alert background color");
    if (clr.isValid()) {
        QString ss = ui->btnAlertBgColor->styleSheet();
        QColor oclr = m_settings.alertBgColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," +
                           QString::number(oclr.blue())),
                   QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                           QString::number(clr.blue())));
        ui->btnAlertBgColor->setStyleSheet(ss);
        m_settings.setAlertBgColor(clr);
        emit alertBgColorChanged(clr);
    }
}

void DlgSettings::on_cbxBmAutostart_clicked(bool checked) {
    m_settings.setBmAutoStart(checked);
}

void DlgSettings::on_spinBoxInterval_valueChanged(int arg1) {
    if (!m_pageSetupDone)
        return;
    m_settings.setRequestServerInterval(arg1);
    emit requestServerIntervalChanged(arg1);
}

void DlgSettings::on_cbxTheme_currentIndexChanged(int index) {
    if (!m_pageSetupDone)
        return;
    m_settings.setTheme(index);
}

void DlgSettings::on_btnBrowse_clicked() {
    QString fileName = QFileDialog::getExistingDirectory(
            this,
            "Select directory to put store downloads in",
            m_settings.storeDownloadDir(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog
            );
    if (fileName != "") {
        QFileInfo fi(fileName);
        if (!fi.isWritable() || !fi.isReadable()) {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Directory not writable!");
            msgBox.setText("You do not have permission to write to the selected directory, aborting.");
            msgBox.exec();
        }
        m_settings.setStoreDownloadDir(fileName + QDir::separator());
        ui->lineEditDownloadsDir->setText(fileName + QDir::separator());
    }
}

void DlgSettings::on_fontComboBox_currentFontChanged(const QFont &f) {
    if (!m_pageSetupDone)
        return;
    QFont font = f;
    font.setPointSize(ui->spinBoxAppFontSize->value());
    m_settings.setApplicationFont(font);
    emit applicationFontChanged(font);
    setFont(font);
}

void DlgSettings::on_spinBoxAppFontSize_valueChanged(int arg1) {
    if (!m_pageSetupDone)
        return;
    QFont font = m_settings.applicationFont();
    font.setPointSize(arg1);
    m_settings.setApplicationFont(font);
    emit applicationFontChanged(font);
    setFont(font);
}

void DlgSettings::on_btnTestReqServer_clicked() {
    auto api = new OKJSongbookAPI(this);
    connect(api, &OKJSongbookAPI::testFailed, this, &DlgSettings::reqSvrTestError);
    connect(api, &OKJSongbookAPI::testSslError, this, &DlgSettings::reqSvrTestSslError);
    connect(api, &OKJSongbookAPI::testPassed, this, &DlgSettings::reqSvrTestPassed);
    api->test();

    delete api;
}

void DlgSettings::reqSvrTestError(QString error) {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Request server test failed!");
    msgBox.setText("Request server connection test was unsuccessful!");
    msgBox.setInformativeText("Error msg:\n" + error);
    msgBox.exec();
}

void DlgSettings::reqSvrTestSslError(QString error) {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Request server test failed!");
    msgBox.setText("Request server connection test was unsuccessful due to SSL errors!");
    msgBox.setInformativeText("Error msg:\n" + error);
    msgBox.exec();
}

void DlgSettings::reqSvrTestPassed() {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Request server test passed");
    msgBox.setText("Request server connection test was successful.  Server info and API key appear to be valid");
    msgBox.exec();
}

void DlgSettings::on_checkBoxIncludeEmptySingers_clicked(bool checked) {
    m_settings.setEstimationSkipEmptySingers(!checked);
    emit rotationDurationSettingsModified();
}

void DlgSettings::on_spinBoxDefaultPadTime_valueChanged(int arg1) {
    if (!m_pageSetupDone)
        return;
    m_settings.setEstimationSingerPad(arg1);
    emit rotationDurationSettingsModified();
}

void DlgSettings::on_spinBoxDefaultSongDuration_valueChanged(int arg1) {
    if (!m_pageSetupDone)
        return;
    m_settings.setEstimationEmptySongLength(arg1);
    emit rotationDurationSettingsModified();
}

void DlgSettings::on_checkBoxDisplayCurrentRotationPosition_clicked(bool checked) {
    m_settings.setRotationDisplayPosition(checked);
}

void DlgSettings::entitledSystemCountChanged(int count) {
    ui->spinBoxSystemId->setMaximum(count);
    if (m_settings.systemId() <= count) {
        ui->spinBoxSystemId->setValue(m_settings.systemId());
    }
}

void DlgSettings::on_groupBoxShowDuration_clicked(bool checked) {
    m_settings.setCdgRemainEnabled(checked);
    emit cdgRemainEnabledChanged(checked);
}

void DlgSettings::on_btnDurationFont_clicked() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_settings.cdgRemainFont(), this, "Select CDG duration display font");
    if (ok) {
        m_settings.setCdgRemainFont(font);
        emit cdgRemainFontChanged(font);
    }
}

void DlgSettings::on_btnDurationFontColor_clicked() {
    QColor clr = QColorDialog::getColor(m_settings.cdgRemainTextColor(), this, "Select CDG duration display text color");
    if (clr.isValid()) {
        QString ss = ui->btnDurationFontColor->styleSheet();
        QColor oclr = m_settings.cdgRemainTextColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," +
                           QString::number(oclr.blue())),
                   QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                           QString::number(clr.blue())));
        ui->btnDurationFontColor->setStyleSheet(ss);
        m_settings.setCdgRemainTextColor(clr);
        emit cdgRemainTextColorChanged(clr);
    }
}

void DlgSettings::on_btnDurationBgColor_clicked() {
    QColor clr = QColorDialog::getColor(m_settings.cdgRemainBgColor(), this,
                                        "Select CDG duration display background color");
    if (clr.isValid()) {
        QString ss = ui->btnDurationBgColor->styleSheet();
        QColor oclr = m_settings.cdgRemainBgColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," +
                           QString::number(oclr.blue())),
                   QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                           QString::number(clr.blue())));
        ui->btnDurationBgColor->setStyleSheet(ss);
        m_settings.setCdgRemainBgColor(clr);
        emit cdgRemainBgColorChanged(clr);
    }
}

void DlgSettings::on_btnLogDirBrowse_clicked() {
    QString fileName = QFileDialog::getExistingDirectory(
            this,
            "Select directory to put logs in",
            m_settings.logDir(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog
            );
    if (fileName != "") {
        QFileInfo fi(fileName);
        if (!fi.isWritable() || !fi.isReadable()) {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Directory not writable!");
            msgBox.setText("You do not have permission to write to the selected directory, aborting.");
            msgBox.exec();
        }
        m_settings.setLogDir(fileName + QDir::separator());
        ui->lineEditLogDir->setText(fileName + QDir::separator());
    }
}

void DlgSettings::on_checkBoxProgressiveSearch_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    m_settings.setProgressiveSearchEnabled(checked);
}

void DlgSettings::on_cbxPreviewEnabled_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    m_settings.setPreviewEnabled(!checked);
}

void DlgSettings::on_comboBoxKAudioDevices_currentIndexChanged(int index) {
    if (!m_pageSetupDone)
        return;
    QString device = ui->comboBoxKAudioDevices->itemText(index);
    if (m_settings.audioOutputDevice() == device)
        return;
    qInfo() << "Changing karaoke audio output device to: " << index << ")" << device;
    m_settings.setAudioOutputDevice(device);
    kAudioBackend.setAudioOutputDevice(device);
}

void DlgSettings::on_comboBoxBAudioDevices_currentIndexChanged(int index) {
    if (!m_pageSetupDone)
        return;
    QString device = ui->comboBoxBAudioDevices->itemText(index);
    if (m_settings.audioOutputDeviceBm() == device)
        return;
    qInfo() << "Changing karaoke audio output device to: " << index << ")" << device;
    m_settings.setAudioOutputDeviceBm(device);
    bmAudioBackend.setAudioOutputDevice(device);
}

void DlgSettings::on_checkBoxEnforceAspectRatio_clicked(bool checked) {
    m_settings.setEnforceAspectRatio(checked);
}

void DlgSettings::on_pushButtonApplyTickerMsg_clicked() {
    m_settings.setTickerCustomString(ui->lineEditTickerMessage->text());
    emit tickerCustomStringChanged();
}

void DlgSettings::on_pushButtonResetDurationPos_clicked() {
    m_settings.resetDurationPosition();
    emit durationPositionReset();
}

void DlgSettings::on_lineEditTickerMessage_returnPressed() {
    m_settings.setTickerCustomString(ui->lineEditTickerMessage->text());
    emit tickerCustomStringChanged();
}

void DlgSettings::on_checkBoxHardwareAccel_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    m_settings.setHardwareAccelEnabled(checked);
}

void DlgSettings::on_checkBoxCdgPrescaling_stateChanged(int arg1) {
    if (!m_pageSetupDone)
        return;
    if (arg1 == 0)
        m_settings.setCdgPrescalingEnabled(false);
    else
        m_settings.setCdgPrescalingEnabled(true);
}

void DlgSettings::on_checkBoxCurrentSingerTop_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    m_settings.setRotationAltSortOrder(checked);
}


void DlgSettings::closeEvent([[maybe_unused]]QCloseEvent *event) {
    m_settings.saveWindowState(this);
    deleteLater();
}

void DlgSettings::done(int) {
    close();
}
