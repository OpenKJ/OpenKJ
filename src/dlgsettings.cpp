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


extern Settings settings;


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
    settings.restoreWindowState(this);
    ui->checkBoxHardwareAccel->setChecked(settings.hardwareAccelEnabled());
#ifdef Q_OS_MACOS
    ui->checkBoxHardwareAccel->setHidden(true);
#endif
    ui->tabWidgetMain->setCurrentIndex(0);
    ui->checkBoxDbSkipValidation->setChecked(settings.dbSkipValidation());
    ui->checkBoxLazyLoadDurations->setChecked(settings.dbLazyLoadDurations());
    ui->checkBoxMonitorDirs->setChecked(settings.dbDirectoryWatchEnabled());
    ui->groupBoxShowDuration->setChecked(settings.cdgRemainEnabled());
    ui->cbxRotShowNextSong->setChecked(settings.rotationShowNextSong());
    ui->checkBoxCdgPrescaling->setChecked(settings.cdgPrescalingEnabled());
    ui->checkBoxCurrentSingerTop->setChecked(settings.rotationAltSortOrder());
    audioOutputDevices = kAudioBackend.getOutputDevices();
    ui->comboBoxKAudioDevices->addItems(audioOutputDevices);
    ui->checkBoxShowAddDlgOnDbDblclk->setChecked(settings.dbDoubleClickAddsSong());
    int selDevice = audioOutputDevices.indexOf(settings.audioOutputDevice());
    if (selDevice == -1) {
        ui->comboBoxKAudioDevices->setCurrentIndex(0);
    } else {
        ui->comboBoxKAudioDevices->setCurrentIndex(selDevice);
    }
    ui->comboBoxBAudioDevices->addItems(audioOutputDevices);
    selDevice = audioOutputDevices.indexOf(settings.audioOutputDeviceBm());
    if (selDevice == -1)
        ui->comboBoxBAudioDevices->setCurrentIndex(0);
    else {
        ui->comboBoxBAudioDevices->setCurrentIndex(selDevice);
    }
    ui->checkBoxProgressiveSearch->setChecked(settings.progressiveSearchEnabled());
    ui->horizontalSliderTickerSpeed->setValue(settings.tickerSpeed());
    QString ss = ui->pushButtonTextColor->styleSheet();
    QColor clr = settings.tickerTextColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                                QString::number(clr.blue())));
    ui->pushButtonTextColor->setStyleSheet(ss);
    ss = ui->pushButtonBgColor->styleSheet();
    clr = settings.tickerBgColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                                QString::number(clr.blue())));
    ui->pushButtonBgColor->setStyleSheet(ss);
    ss = ui->btnAlertTxtColor->styleSheet();
    clr = settings.alertTxtColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                                QString::number(clr.blue())));
    ui->btnAlertTxtColor->setStyleSheet(ss);
    ss = ui->btnAlertBgColor->styleSheet();
    clr = settings.alertBgColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                                QString::number(clr.blue())));
    ui->btnAlertBgColor->setStyleSheet(ss);

    ss = ui->btnDurationFontColor->styleSheet();
    clr = settings.cdgRemainTextColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                                QString::number(clr.blue())));
    ui->btnDurationFontColor->setStyleSheet(ss);

    ss = ui->btnDurationBgColor->styleSheet();
    clr = settings.cdgRemainBgColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                                QString::number(clr.blue())));
    ui->btnDurationBgColor->setStyleSheet(ss);

    if (settings.tickerFullRotation()) {
        ui->radioButtonFullRotation->setChecked(true);
        ui->spinBoxTickerSingers->setEnabled(false);
    } else {
        ui->radioButtonPartialRotation->setChecked(true);
        ui->spinBoxTickerSingers->setEnabled(true);
    }
    ui->spinBoxTickerSingers->setValue(settings.tickerShowNumSingers());
    ui->groupBoxRequestServer->setChecked(settings.requestServerEnabled());
    ui->lineEditUrl->setText(settings.requestServerUrl());
    ui->lineEditApiKey->setText(settings.requestServerApiKey());
    ui->checkBoxIgnoreCertErrors->setChecked(settings.requestServerIgnoreCertErrors());
    if ((settings.bgMode() == settings.BG_MODE_IMAGE) || (settings.bgSlideShowDir() == ""))
        ui->rbBgImage->setChecked(true);
    else
        ui->rbSlideshow->setChecked(true);
    ui->lineEditCdgBackground->setText(settings.cdgDisplayBackgroundImage());
    ui->lineEditSlideshowDir->setText(settings.bgSlideShowDir());
    ui->checkBoxFader->setChecked(settings.audioUseFader());
    ui->checkBoxDownmix->setChecked(settings.audioDownmix());
    ui->checkBoxSilenceDetection->setChecked(settings.audioDetectSilence());
    ui->checkBoxFaderBm->setChecked(settings.audioUseFaderBm());
    ui->checkBoxDownmixBm->setChecked(settings.audioDownmixBm());
    ui->checkBoxSilenceDetectionBm->setChecked(settings.audioDetectSilenceBm());
    ui->spinBoxInterval->setValue(settings.requestServerInterval());
    ui->spinBoxSystemId->setMaximum(songbookApi.entitledSystemCount());
    ui->spinBoxSystemId->setValue(settings.systemId());
    ui->spinBoxCdgOffsetTop->setValue(settings.cdgOffsetTop());
    ui->spinBoxCdgOffsetBottom->setValue(settings.cdgOffsetBottom());
    ui->spinBoxCdgOffsetLeft->setValue(settings.cdgOffsetLeft());
    ui->spinBoxCdgOffsetRight->setValue(settings.cdgOffsetRight());
    ui->spinBoxSlideshowInterval->setValue(settings.slideShowInterval());

    AudioRecorder recorder;
    QStringList inputs = recorder.getDeviceList();
    QStringList codecs = recorder.getCodecs();
    ui->groupBoxRecording->setChecked(settings.recordingEnabled());
    ui->comboBoxDevice->addItems(inputs);
    ui->comboBoxCodec->addItems(codecs);
    QString recordingInput = settings.recordingInput();
    if (recordingInput == "undefined")
        ui->comboBoxDevice->setCurrentIndex(0);
    else
        ui->comboBoxDevice->setCurrentIndex(ui->comboBoxDevice->findText(settings.recordingInput()));
    QString recordingCodec = settings.recordingCodec();
    if (recordingCodec == "undefined")
        ui->comboBoxCodec->setCurrentIndex(1);
    else
        ui->comboBoxCodec->setCurrentIndex(ui->comboBoxCodec->findText(settings.recordingCodec()));
    ui->comboBoxUpdateBranch->addItem("Stable");
    ui->comboBoxUpdateBranch->addItem("Development");
    ui->cbxTheme->addItem("OS Native");
    ui->cbxTheme->addItem("Fusion Dark");
    ui->cbxTheme->addItem("Fusion Light");
    ui->cbxTheme->setCurrentIndex(settings.theme());
    ui->lineEditOutputDir->setText(settings.recordingOutputDir());
    ui->cbxTickerShowRotationInfo->setChecked(settings.tickerShowRotationInfo());
    ui->groupBoxTicker->setChecked(settings.tickerEnabled());
    ui->lineEditTickerMessage->setText(settings.tickerCustomString());
    ui->fontComboBox->setFont(settings.applicationFont());
    ui->spinBoxAppFontSize->setValue(settings.applicationFont().pointSize());
    ui->checkBoxIncludeEmptySingers->setChecked(!settings.estimationSkipEmptySingers());
    ui->spinBoxDefaultPadTime->setValue(settings.estimationSingerPad());
    ui->spinBoxDefaultSongDuration->setValue(settings.estimationEmptySongLength());
    ui->checkBoxDisplayCurrentRotationPosition->setChecked(settings.rotationDisplayPosition());
    ui->spinBoxAADelay->setValue(settings.karaokeAATimeout());
    ui->checkBoxKAA->setChecked(settings.karaokeAutoAdvance());
    ui->checkBoxShowKAAAlert->setChecked(settings.karaokeAAAlertEnabled());
    ui->cbxQueueRemovalWarning->setChecked(settings.showQueueRemovalWarning());
    ui->cbxSingerRemovalWarning->setChecked(settings.showSingerRemovalWarning());
    ui->cbxSongInterruptionWarning->setChecked(settings.showSongInterruptionWarning());
    ui->cbxBmAutostart->setChecked(settings.bmAutoStart());
    ui->cbxIgnoreApos->setChecked(settings.ignoreAposInSearch());
    ui->spinBoxVideoOffset->setValue(settings.videoOffsetMs());
    ui->cbxStopPauseWarning->setChecked(settings.showSongPauseStopWarning());
    ui->cbxCheckUpdates->setChecked(settings.checkUpdates());
    ui->comboBoxUpdateBranch->setCurrentIndex(settings.updatesBranch());
    ui->lineEditDownloadsDir->setText(settings.storeDownloadDir());
    ui->checkBoxLogging->setChecked(settings.logEnabled());
    ui->lineEditLogDir->setText(settings.logDir());
    ui->checkBoxEnforceAspectRatio->setChecked(settings.enforceAspectRatio());
    ui->checkBoxTreatAllSingersAsRegs->setChecked(settings.treatAllSingersAsRegs());
    ui->cbxCrossFade->setChecked(settings.bmKCrossFade());
    adjustSize();
    tickerFontChanged();
    tickerBgColorChanged();
    tickerTextColorChanged();
    tickerOutputModeChanged();
    tickerCustomStringChanged();
    tickerSpeedChanged();
    connect(ui->spinBoxCdgOffsetTop, qOverload<int>(&QSpinBox::valueChanged), &settings, &Settings::setCdgOffsetTop);
    connect(ui->spinBoxCdgOffsetTop, qOverload<int>(&QSpinBox::valueChanged), this, &DlgSettings::cdgOffsetsChanged);
    connect(ui->spinBoxCdgOffsetBottom, qOverload<int>(&QSpinBox::valueChanged), &settings, &Settings::setCdgOffsetBottom);
    connect(ui->spinBoxCdgOffsetBottom, qOverload<int>(&QSpinBox::valueChanged), this, &DlgSettings::cdgOffsetsChanged);
    connect(ui->spinBoxCdgOffsetLeft, qOverload<int>(&QSpinBox::valueChanged), &settings, &Settings::setCdgOffsetLeft);
    connect(ui->spinBoxCdgOffsetLeft, qOverload<int>(&QSpinBox::valueChanged), this, &DlgSettings::cdgOffsetsChanged);
    connect(ui->spinBoxCdgOffsetRight, qOverload<int>(&QSpinBox::valueChanged), &settings, &Settings::setCdgOffsetRight);
    connect(ui->spinBoxCdgOffsetRight, qOverload<int>(&QSpinBox::valueChanged), this, &DlgSettings::cdgOffsetsChanged);
    connect(ui->spinBoxVideoOffset, qOverload<int>(&QSpinBox::valueChanged), &settings, &Settings::setVideoOffsetMs);
    connect(ui->spinBoxSlideshowInterval, qOverload<int>(&QSpinBox::valueChanged), &settings, &Settings::setSlideShowInterval);
    connect(ui->spinBoxSlideshowInterval, qOverload<int>(&QSpinBox::valueChanged), this, &DlgSettings::slideShowIntervalChanged);
    connect(&settings, &Settings::karaokeAutoAdvanceChanged, ui->checkBoxKAA, &QCheckBox::setChecked);
    connect(&settings, &Settings::showQueueRemovalWarningChanged, ui->cbxQueueRemovalWarning, &QCheckBox::setChecked);
    connect(&settings, &Settings::showSingerRemovalWarningChanged, ui->cbxSingerRemovalWarning, &QCheckBox::setChecked);
    connect(&settings, &Settings::showSongInterruptionWarningChanged, ui->cbxSongInterruptionWarning, &QCheckBox::setChecked);
    connect(&settings, &Settings::showSongStopPauseWarningChanged, ui->cbxStopPauseWarning, &QCheckBox::setChecked);
    connect(ui->cbxIgnoreApos, &QCheckBox::toggled, &settings, &Settings::setIgnoreAposInSearch);
    connect(ui->cbxCrossFade, &QCheckBox::toggled, &settings, &Settings::setBmKCrossfade);
    connect(ui->cbxCheckUpdates, &QCheckBox::toggled, &settings, &Settings::setCheckUpdates);
    connect(ui->comboBoxUpdateBranch, qOverload<int>(&QComboBox::currentIndexChanged), &settings, &Settings::setUpdatesBranch);
    connect(ui->checkBoxDbSkipValidation, &QCheckBox::toggled, &settings, &Settings::dbSetSkipValidation);
    connect(ui->checkBoxLazyLoadDurations, &QCheckBox::toggled, &settings, &Settings::dbSetLazyLoadDurations);
    connect(ui->checkBoxMonitorDirs, &QCheckBox::toggled, &settings, &Settings::dbSetDirectoryWatchEnabled);
    connect(ui->spinBoxSystemId, qOverload<int>(&QSpinBox::valueChanged), &settings, &Settings::setSystemId);
    connect(ui->checkBoxLogging, &QCheckBox::toggled, &settings, &Settings::setLogEnabled);
    connect(ui->checkBoxTreatAllSingersAsRegs, &QAbstractButton::toggled, &settings,
            &Settings::setTreatAllSingersAsRegs);
    connect(ui->checkBoxShowAddDlgOnDbDblclk, &QCheckBox::stateChanged, [&](auto state) {
        if (state == 0)
            settings.setDbDoubleClickAddsSong(false);
        else
            settings.setDbDoubleClickAddsSong(true);
    });
    connect(networkManager, &QNetworkAccessManager::finished, this, &DlgSettings::onNetworkReply);
    connect(networkManager, &QNetworkAccessManager::sslErrors, this, &DlgSettings::onSslErrors);
    connect(ui->cbxQueueRemovalWarning, &QCheckBox::toggled, &settings, &Settings::setShowQueueRemovalWarning);
    connect(ui->cbxSingerRemovalWarning, &QCheckBox::toggled, &settings, &Settings::setShowSingerRemovalWarning);
    connect(ui->cbxSongInterruptionWarning, &QCheckBox::toggled, &settings, &Settings::setShowSongInterruptionWarning);
    connect(ui->cbxStopPauseWarning, &QCheckBox::toggled, &settings, &Settings::setShowSongPauseStopWarning);
    connect(ui->cbxTickerShowRotationInfo, &QCheckBox::toggled, &settings, &Settings::setTickerShowRotationInfo);
    connect(ui->cbxTickerShowRotationInfo, &QCheckBox::toggled, this, &DlgSettings::tickerOutputModeChanged);
    connect(&songbookApi, &OKJSongbookAPI::entitledSystemCountChanged, this, &DlgSettings::entitledSystemCountChanged);
    connect(ui->cbxRotShowNextSong, &QCheckBox::toggled, &settings, &Settings::setRotationShowNextSong);
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
        sequenceEdit->setKeySequence(settings.loadShortcutKeySequence(shortcut.sequenceName));
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
    settings.saveShortcutKeySequence(sender()->objectName(), sequence);
}


void DlgSettings::onNetworkReply(QNetworkReply *reply) {
    Q_UNUSED(reply);
}

void DlgSettings::onSslErrors(QNetworkReply *reply) {
    if (settings.requestServerIgnoreCertErrors())
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
    QFont font = QFontDialog::getFont(&ok, settings.tickerFont(), this, "Select ticker font");
    if (ok) {
        settings.setTickerFont(font);
        emit tickerFontChanged();
    }
}

void DlgSettings::on_horizontalSliderTickerSpeed_valueChanged(int value) {
    if (!m_pageSetupDone)
        return;
    settings.setTickerSpeed(value);
    emit tickerSpeedChanged();
}

void DlgSettings::on_pushButtonTextColor_clicked() {
    QColor clr = QColorDialog::getColor(settings.tickerTextColor(), this, "Select ticker text color");
    if (clr.isValid()) {
        QString ss = ui->pushButtonTextColor->styleSheet();
        QColor oclr = settings.tickerTextColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," +
                           QString::number(oclr.blue())),
                   QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                           QString::number(clr.blue())));
        ui->pushButtonTextColor->setStyleSheet(ss);
        settings.setTickerTextColor(clr);
        emit tickerTextColorChanged();
    }
}

void DlgSettings::on_pushButtonBgColor_clicked() {
    QColor clr = QColorDialog::getColor(settings.tickerBgColor(), this, "Select ticker background color");
    if (clr.isValid()) {
        QString ss = ui->pushButtonBgColor->styleSheet();
        QColor oclr = settings.tickerBgColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," +
                           QString::number(oclr.blue())),
                   QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                           QString::number(clr.blue())));
        ui->pushButtonBgColor->setStyleSheet(ss);
        settings.setTickerBgColor(clr);
        emit tickerBgColorChanged();
    }
}

void DlgSettings::on_radioButtonFullRotation_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    settings.setTickerFullRotation(checked);
    ui->spinBoxTickerSingers->setEnabled(!checked);
    emit tickerOutputModeChanged();
}


void DlgSettings::on_spinBoxTickerSingers_valueChanged(int arg1) {
    if (!m_pageSetupDone)
        return;
    settings.setTickerShowNumSingers(arg1);
    emit tickerOutputModeChanged();
}

void DlgSettings::on_groupBoxTicker_toggled(bool arg1) {
    if (!m_pageSetupDone)
        return;
    settings.setTickerEnabled(arg1);
    emit tickerEnableChanged();
}

void DlgSettings::on_lineEditUrl_editingFinished() {
    settings.setRequestServerUrl(ui->lineEditUrl->text());
}

void DlgSettings::on_checkBoxIgnoreCertErrors_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    settings.setRequestServerIgnoreCertErrors(checked);
}

void DlgSettings::on_groupBoxRequestServer_toggled(bool arg1) {
    if (!m_pageSetupDone)
        return;
    settings.setRequestServerEnabled(arg1);
}

void DlgSettings::on_pushButtonBrowse_clicked() {
    QString imageFile = QFileDialog::getOpenFileName(this, QString("Select image file"),
                                                     QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                                     QString("Images (*.png *.jpg *.jpeg *.gif)"), nullptr, QFileDialog::DontUseNativeDialog);
    if (imageFile != "") {
        QImage image(imageFile);
        if (!image.isNull()) {
            settings.setCdgDisplayBackgroundImage(imageFile);
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
    settings.setAudioUseFader(checked);
    emit audioUseFaderChanged(checked);
}

void DlgSettings::on_checkBoxFaderBm_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    settings.setAudioUseFaderBm(checked);
    emit audioUseFaderChangedBm(checked);
}

void DlgSettings::on_checkBoxSilenceDetection_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    settings.setAudioDetectSilence(checked);
    emit audioSilenceDetectChanged(checked);
}

void DlgSettings::on_checkBoxSilenceDetectionBm_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    settings.setAudioDetectSilenceBm(checked);
    emit audioSilenceDetectChangedBm(checked);
}

void DlgSettings::on_checkBoxDownmix_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    settings.setAudioDownmix(checked);
    emit audioDownmixChanged(checked);
}

void DlgSettings::on_checkBoxDownmixBm_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    settings.setAudioDownmixBm(checked);
    emit audioDownmixChangedBm(checked);
}

void DlgSettings::on_comboBoxDevice_currentIndexChanged(const QString &arg1) {
    if (!m_pageSetupDone)
        return;
    settings.setRecordingInput(arg1);
}

void DlgSettings::on_comboBoxCodec_currentIndexChanged(const QString &arg1) {
    if (!m_pageSetupDone)
        return;
    settings.setRecordingCodec(arg1);
    if (arg1 == "audio/mpeg")
        settings.setRecordingRawExtension("mp3");
}

void DlgSettings::on_groupBoxRecording_toggled(bool arg1) {
    if (!m_pageSetupDone)
        return;
    settings.setRecordingEnabled(arg1);
}

void DlgSettings::on_buttonBrowse_clicked() {
    QString dirName = QFileDialog::getExistingDirectory(
            this,
            "Select output directory",
            QStandardPaths::standardLocations(QStandardPaths::MusicLocation).at(0),
            QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog
            );
    if (dirName != "") {
        settings.setRecordingOutputDir(dirName);
        ui->lineEditOutputDir->setText(dirName);
    }
}

void DlgSettings::on_pushButtonClearBgImg_clicked() {
    settings.setCdgDisplayBackgroundImage(QString());
    ui->lineEditCdgBackground->setText(QString());
    emit cdgBgImageChanged();
}

void DlgSettings::on_pushButtonSlideshowBrowse_clicked() {
    QString initialPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (settings.bgSlideShowDir() != "")
        initialPath = settings.bgSlideShowDir();
    QString dirName = QFileDialog::getExistingDirectory(
            this,
            "Select the slideshow directory",
            initialPath,
            QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly
            );
    if (dirName != "") {
        settings.setBgSlideShowDir(dirName);
        emit bgSlideShowDirChanged(dirName);
        ui->lineEditSlideshowDir->setText(dirName);
    }
}

void DlgSettings::on_rbSlideshow_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    Settings::BgMode mode = (checked) ? settings.BG_MODE_SLIDESHOW : settings.BG_MODE_IMAGE;
    settings.setBgMode(mode);
    emit bgModeChanged(mode);
}

void DlgSettings::on_rbBgImage_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    Settings::BgMode mode = (checked) ? settings.BG_MODE_IMAGE : settings.BG_MODE_SLIDESHOW;
    settings.setBgMode(mode);
    emit bgModeChanged(mode);
}

void DlgSettings::on_lineEditApiKey_editingFinished() {
    settings.setRequestServerApiKey(ui->lineEditApiKey->text());
}

void DlgSettings::on_checkBoxShowKAAAlert_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    settings.setKaraokeAAAlertEnabled(checked);
}

void DlgSettings::on_checkBoxKAA_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    settings.setKaraokeAutoAdvance(checked);
}

void DlgSettings::on_spinBoxAADelay_valueChanged(int arg1) {
    if (!m_pageSetupDone)
        return;
    settings.setKaraokeAATimeout(arg1);
}

void DlgSettings::on_btnAlertFont_clicked() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, settings.karaokeAAAlertFont(), this, "Select alert font");
    if (ok) {
        settings.setKaraokeAAAlertFont(font);
        emit karaokeAAAlertFontChanged(font);
    }
}

void DlgSettings::on_btnAlertTxtColor_clicked() {
    QColor clr = QColorDialog::getColor(settings.alertTxtColor(), this, "Select alert text color");
    if (clr.isValid()) {
        QString ss = ui->btnAlertTxtColor->styleSheet();
        QColor oclr = settings.alertTxtColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," +
                           QString::number(oclr.blue())),
                   QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                           QString::number(clr.blue())));
        ui->btnAlertTxtColor->setStyleSheet(ss);
        settings.setAlertTxtColor(clr);
        emit alertTxtColorChanged(clr);
    }
}

void DlgSettings::on_btnAlertBgColor_clicked() {
    QColor clr = QColorDialog::getColor(settings.alertBgColor(), this, "Select alert background color");
    if (clr.isValid()) {
        QString ss = ui->btnAlertBgColor->styleSheet();
        QColor oclr = settings.alertBgColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," +
                           QString::number(oclr.blue())),
                   QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                           QString::number(clr.blue())));
        ui->btnAlertBgColor->setStyleSheet(ss);
        settings.setAlertBgColor(clr);
        emit alertBgColorChanged(clr);
    }
}

void DlgSettings::on_cbxBmAutostart_clicked(bool checked) {
    settings.setBmAutoStart(checked);
}

void DlgSettings::on_spinBoxInterval_valueChanged(int arg1) {
    if (!m_pageSetupDone)
        return;
    settings.setRequestServerInterval(arg1);
}

void DlgSettings::on_cbxTheme_currentIndexChanged(int index) {
    if (!m_pageSetupDone)
        return;
    settings.setTheme(index);
}

void DlgSettings::on_btnBrowse_clicked() {
    QString fileName = QFileDialog::getExistingDirectory(
            this,
            "Select directory to put store downloads in",
            settings.storeDownloadDir(),
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
        settings.setStoreDownloadDir(fileName + QDir::separator());
        ui->lineEditDownloadsDir->setText(fileName + QDir::separator());
    }
}

void DlgSettings::on_fontComboBox_currentFontChanged(const QFont &f) {
    if (!m_pageSetupDone)
        return;
    QFont font = f;
    font.setPointSize(ui->spinBoxAppFontSize->value());
    settings.setApplicationFont(font);
    emit applicationFontChanged(font);
    setFont(font);
}

void DlgSettings::on_spinBoxAppFontSize_valueChanged(int arg1) {
    if (!m_pageSetupDone)
        return;
    QFont font = settings.applicationFont();
    font.setPointSize(arg1);
    settings.setApplicationFont(font);
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
    settings.setEstimationSkipEmptySingers(!checked);
}

void DlgSettings::on_spinBoxDefaultPadTime_valueChanged(int arg1) {
    if (!m_pageSetupDone)
        return;
    settings.setEstimationSingerPad(arg1);
}

void DlgSettings::on_spinBoxDefaultSongDuration_valueChanged(int arg1) {
    if (!m_pageSetupDone)
        return;
    settings.setEstimationEmptySongLength(arg1);
}

void DlgSettings::on_checkBoxDisplayCurrentRotationPosition_clicked(bool checked) {
    settings.setRotationDisplayPosition(checked);
}

void DlgSettings::entitledSystemCountChanged(int count) {
    ui->spinBoxSystemId->setMaximum(count);
    if (settings.systemId() <= count) {
        ui->spinBoxSystemId->setValue(settings.systemId());
    }
}

void DlgSettings::on_groupBoxShowDuration_clicked(bool checked) {
    settings.setCdgRemainEnabled(checked);
    emit cdgRemainEnabledChanged(checked);
}

void DlgSettings::on_btnDurationFont_clicked() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, settings.cdgRemainFont(), this, "Select CDG duration display font");
    if (ok) {
        settings.setCdgRemainFont(font);
        emit cdgRemainFontChanged(font);
    }
}

void DlgSettings::on_btnDurationFontColor_clicked() {
    QColor clr = QColorDialog::getColor(settings.cdgRemainTextColor(), this, "Select CDG duration display text color");
    if (clr.isValid()) {
        QString ss = ui->btnDurationFontColor->styleSheet();
        QColor oclr = settings.cdgRemainTextColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," +
                           QString::number(oclr.blue())),
                   QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                           QString::number(clr.blue())));
        ui->btnDurationFontColor->setStyleSheet(ss);
        settings.setCdgRemainTextColor(clr);
        emit cdgRemainTextColorChanged(clr);
    }
}

void DlgSettings::on_btnDurationBgColor_clicked() {
    QColor clr = QColorDialog::getColor(settings.cdgRemainBgColor(), this,
                                        "Select CDG duration display background color");
    if (clr.isValid()) {
        QString ss = ui->btnDurationBgColor->styleSheet();
        QColor oclr = settings.cdgRemainBgColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," +
                           QString::number(oclr.blue())),
                   QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," +
                           QString::number(clr.blue())));
        ui->btnDurationBgColor->setStyleSheet(ss);
        settings.setCdgRemainBgColor(clr);
        emit cdgRemainBgColorChanged(clr);
    }
}

void DlgSettings::on_btnLogDirBrowse_clicked() {
    QString fileName = QFileDialog::getExistingDirectory(
            this,
            "Select directory to put logs in",
            settings.logDir(),
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
        settings.setLogDir(fileName + QDir::separator());
        ui->lineEditLogDir->setText(fileName + QDir::separator());
    }
}

void DlgSettings::on_checkBoxProgressiveSearch_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    settings.setProgressiveSearchEnabled(checked);
}

void DlgSettings::on_cbxPreviewEnabled_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    settings.setPreviewEnabled(!checked);
}

void DlgSettings::on_comboBoxKAudioDevices_currentIndexChanged(int index) {
    if (!m_pageSetupDone)
        return;
    QString device = ui->comboBoxKAudioDevices->itemText(index);
    if (settings.audioOutputDevice() == device)
        return;
    qInfo() << "Changing karaoke audio output device to: " << index << ")" << device;
    settings.setAudioOutputDevice(device);
    kAudioBackend.setAudioOutputDevice(device);
}

void DlgSettings::on_comboBoxBAudioDevices_currentIndexChanged(int index) {
    if (!m_pageSetupDone)
        return;
    QString device = ui->comboBoxBAudioDevices->itemText(index);
    if (settings.audioOutputDeviceBm() == device)
        return;
    qInfo() << "Changing karaoke audio output device to: " << index << ")" << device;
    settings.setAudioOutputDeviceBm(device);
    bmAudioBackend.setAudioOutputDevice(device);
}

void DlgSettings::on_checkBoxEnforceAspectRatio_clicked(bool checked) {
    settings.setEnforceAspectRatio(checked);
}

void DlgSettings::on_pushButtonApplyTickerMsg_clicked() {
    settings.setTickerCustomString(ui->lineEditTickerMessage->text());
    emit tickerCustomStringChanged();
}

void DlgSettings::on_pushButtonResetDurationPos_clicked() {
    settings.resetDurationPosition();
    emit durationPositionReset();
}

void DlgSettings::on_lineEditTickerMessage_returnPressed() {
    settings.setTickerCustomString(ui->lineEditTickerMessage->text());
    emit tickerCustomStringChanged();
}

void DlgSettings::on_checkBoxHardwareAccel_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    settings.setHardwareAccelEnabled(checked);
}

void DlgSettings::on_checkBoxCdgPrescaling_stateChanged(int arg1) {
    if (!m_pageSetupDone)
        return;
    if (arg1 == 0)
        settings.setCdgPrescalingEnabled(false);
    else
        settings.setCdgPrescalingEnabled(true);
}

void DlgSettings::on_checkBoxCurrentSingerTop_toggled(bool checked) {
    if (!m_pageSetupDone)
        return;
    settings.setRotationAltSortOrder(checked);
}


void DlgSettings::closeEvent([[maybe_unused]]QCloseEvent *event) {
    settings.saveWindowState(this);
    deleteLater();
}

void DlgSettings::done(int) {
    close();
}
