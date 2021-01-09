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
#include "audiorecorder.h"
#include <QScreen>


extern Settings settings;
extern OKJSongbookAPI *songbookApi;


DlgSettings::DlgSettings(MediaBackend *AudioBackend, MediaBackend *BmAudioBackend, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgSettings)
{
    pageSetupDone = false;
    kAudioBackend = AudioBackend;
    bmAudioBackend = BmAudioBackend;
    networkManager = new QNetworkAccessManager(this);
    ui->setupUi(this);
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
    audioOutputDevices = kAudioBackend->getOutputDevices();
    ui->comboBoxKAudioDevices->addItems(audioOutputDevices);
    int selDevice = audioOutputDevices.indexOf(settings.audioOutputDevice());
    if (selDevice == -1)
    {
        ui->comboBoxKAudioDevices->setCurrentIndex(0);
    }
    else
    {
        kAudioBackend->setOutputDevice(selDevice);
        ui->comboBoxKAudioDevices->setCurrentIndex(selDevice);
    }
    ui->comboBoxBAudioDevices->addItems(audioOutputDevices);
    selDevice = audioOutputDevices.indexOf(settings.audioOutputDeviceBm());
    if (selDevice == -1)
        ui->comboBoxBAudioDevices->setCurrentIndex(0);
    else
    {
        ui->comboBoxBAudioDevices->setCurrentIndex(selDevice);
        bmAudioBackend->setOutputDevice(selDevice);
    }
    ui->checkBoxProgressiveSearch->setChecked(settings.progressiveSearchEnabled());
    ui->horizontalSliderTickerSpeed->setValue(settings.tickerSpeed());
    QString ss = ui->pushButtonTextColor->styleSheet();
    QColor clr = settings.tickerTextColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," + QString::number(clr.blue())));
    ui->pushButtonTextColor->setStyleSheet(ss);
    ss = ui->pushButtonBgColor->styleSheet();
    clr = settings.tickerBgColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," + QString::number(clr.blue())));
    ui->pushButtonBgColor->setStyleSheet(ss);
    ss = ui->btnAlertTxtColor->styleSheet();
    clr = settings.alertTxtColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," + QString::number(clr.blue())));
    ui->btnAlertTxtColor->setStyleSheet(ss);
    ss = ui->btnAlertBgColor->styleSheet();
    clr = settings.alertBgColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," + QString::number(clr.blue())));
    ui->btnAlertBgColor->setStyleSheet(ss);

    ss = ui->btnDurationFontColor->styleSheet();
    clr = settings.cdgRemainTextColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," + QString::number(clr.blue())));
    ui->btnDurationFontColor->setStyleSheet(ss);

    ss = ui->btnDurationBgColor->styleSheet();
    clr = settings.cdgRemainBgColor();
    ss.replace("0,0,0", QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," + QString::number(clr.blue())));
    ui->btnDurationBgColor->setStyleSheet(ss);

    if (settings.tickerFullRotation())
    {
        ui->radioButtonFullRotation->setChecked(true);
        ui->spinBoxTickerSingers->setEnabled(false);
    }
    else
    {
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
    ui->spinBoxSystemId->setMaximum(songbookApi->entitledSystemCount());
    ui->spinBoxSystemId->setValue(settings.systemId());
    ui->spinBoxCdgOffsetTop->setValue(settings.cdgOffsetTop());
    ui->spinBoxCdgOffsetBottom->setValue(settings.cdgOffsetBottom());
    ui->spinBoxCdgOffsetLeft->setValue(settings.cdgOffsetLeft());
    ui->spinBoxCdgOffsetRight->setValue(settings.cdgOffsetRight());
    ui->spinBoxSlideshowInterval->setValue(settings.slideShowInterval());
    connect(ui->spinBoxCdgOffsetTop, SIGNAL(valueChanged(int)), &settings, SLOT(setCdgOffsetTop(int)));
    connect(ui->spinBoxCdgOffsetBottom, SIGNAL(valueChanged(int)), &settings, SLOT(setCdgOffsetBottom(int)));
    connect(ui->spinBoxCdgOffsetLeft, SIGNAL(valueChanged(int)), &settings, SLOT(setCdgOffsetLeft(int)));
    connect(ui->spinBoxCdgOffsetRight, SIGNAL(valueChanged(int)), &settings, SLOT(setCdgOffsetRight(int)));
    connect(ui->spinBoxVideoOffset, SIGNAL(valueChanged(int)), &settings, SLOT(setVideoOffsetMs(int)));
    connect(ui->spinBoxSlideshowInterval, SIGNAL(valueChanged(int)), &settings, SLOT(setSlideShowInterval(int)));
    AudioRecorder recorder;
 //   QAudioRecorder audioRecorder;
    QStringList inputs = recorder.getDeviceList();
    QStringList codecs = recorder.getCodecs();
//    QStringList containers = audioRecorder.supportedContainers();
    ui->groupBoxRecording->setChecked(settings.recordingEnabled());
    ui->comboBoxDevice->addItems(inputs);
    ui->comboBoxCodec->addItems(codecs);
//    ui->comboBoxContainer->addItems(containers);
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
    tickerShowRotationInfoChanged(settings.tickerShowRotationInfo());
    ui->groupBoxTicker->setChecked(settings.tickerEnabled());
    ui->lineEditTickerMessage->setText(settings.tickerCustomString());
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onNetworkReply(QNetworkReply*)));
    connect(networkManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(onSslErrors(QNetworkReply*)));
    connect(ui->cbxQueueRemovalWarning, SIGNAL(toggled(bool)), &settings, SLOT(setShowQueueRemovalWarning(bool)));
    connect(ui->cbxSingerRemovalWarning, SIGNAL(toggled(bool)), &settings, SLOT(setShowSingerRemovalWarning(bool)));
    connect(ui->cbxSongInterruptionWarning, SIGNAL(toggled(bool)), &settings, SLOT(setShowSongInterruptionWarning(bool)));
    connect(ui->cbxStopPauseWarning, SIGNAL(toggled(bool)), &settings, SLOT(setShowSongPauseStopWarning(bool)));
    connect(ui->cbxTickerShowRotationInfo, SIGNAL(clicked(bool)), &settings, SLOT(setTickerShowRotationInfo(bool)));
    connect(&settings, SIGNAL(tickerShowRotationInfoChanged(bool)), this, SLOT(tickerShowRotationInfoChanged(bool)));
    connect(songbookApi, SIGNAL(entitledSystemCountChanged(int)), this, SLOT(entitledSystemCountChanged(int)));
    connect(ui->cbxRotShowNextSong, SIGNAL(clicked(bool)), &settings, SLOT(setRotationShowNextSong(bool)));
    ui->fontComboBox->setFont(settings.applicationFont());
    ui->spinBoxAppFontSize->setValue(settings.applicationFont().pointSize());

    ui->checkBoxIncludeEmptySingers->setChecked(!settings.estimationSkipEmptySingers());
    ui->spinBoxDefaultPadTime->setValue(settings.estimationSingerPad());
    ui->spinBoxDefaultSongDuration->setValue(settings.estimationEmptySongLength());
    ui->checkBoxDisplayCurrentRotationPosition->setChecked(settings.rotationDisplayPosition());
    pageSetupDone = true;
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
    connect(&settings, SIGNAL(karaokeAutoAdvanceChanged(bool)), ui->checkBoxKAA, SLOT(setChecked(bool)));
    connect(&settings, SIGNAL(showQueueRemovalWarningChanged(bool)), ui->cbxQueueRemovalWarning, SLOT(setChecked(bool)));
    connect(&settings, SIGNAL(showSingerRemovalWarningChanged(bool)), ui->cbxSingerRemovalWarning, SLOT(setChecked(bool)));
    connect(&settings, SIGNAL(showSongInterruptionWarningChanged(bool)), ui->cbxSongInterruptionWarning, SLOT(setChecked(bool)));
    connect(&settings, SIGNAL(showSongStopPauseWarningChanged(bool)), ui->cbxStopPauseWarning, SLOT(setChecked(bool)));
    connect(ui->cbxIgnoreApos, SIGNAL(toggled(bool)), &settings, SLOT(setIgnoreAposInSearch(bool)));
    //connect(ui->spinBoxCdgOffset, SIGNAL(valueChanged(int)), settings, SLOT(setCdgDisplayOffset(int)));
    ui->cbxCrossFade->setChecked(settings.bmKCrossFade());
    connect(ui->cbxCrossFade, SIGNAL(clicked(bool)), &settings, SLOT(setBmKCrossfade(bool)));
    connect(ui->cbxCheckUpdates, SIGNAL(clicked(bool)), &settings, SLOT(setCheckUpdates(bool)));
    connect(ui->comboBoxUpdateBranch, SIGNAL(currentIndexChanged(int)), &settings, SLOT(setUpdatesBranch(int)));
    ui->lineEditDownloadsDir->setText(settings.storeDownloadDir());
    adjustSize();
    connect(ui->checkBoxDbSkipValidation, SIGNAL(toggled(bool)), &settings, SLOT(dbSetSkipValidation(bool)));
    connect(ui->checkBoxLazyLoadDurations, SIGNAL(toggled(bool)), &settings, SLOT(dbSetLazyLoadDurations(bool)));
    connect(ui->checkBoxMonitorDirs, SIGNAL(toggled(bool)), &settings, SLOT(dbSetDirectoryWatchEnabled(bool)));
    connect(ui->spinBoxSystemId, SIGNAL(valueChanged(int)), &settings, SLOT(setSystemId(int)));
    ui->checkBoxLogging->setChecked(settings.logEnabled());
    Settings tmpSettings;
    ui->lineEditLogDir->setText(tmpSettings.logDir());
    connect(ui->checkBoxLogging, SIGNAL(toggled(bool)), &settings, SLOT(setLogEnabled(bool)));
    ui->checkBoxEnforceAspectRatio->setChecked(settings.enforceAspectRatio());
}

DlgSettings::~DlgSettings()
{
    delete ui;
}

QStringList DlgSettings::getMonitors()
{
    QStringList screenStrings;
    for (int i=0; i < QGuiApplication::screens().size(); i++)
    {
        auto sWidth = QGuiApplication::screens().at(i)->size().width();
        auto sHeight = QGuiApplication::screens().at(i)->size().height();
        screenStrings << "Monitor " + QString::number(i) + " - " + QString::number(sWidth) + "x" + QString::number(sHeight);
    }
    return screenStrings;
}

void DlgSettings::onNetworkReply(QNetworkReply *reply)
{
    Q_UNUSED(reply);
}

void DlgSettings::onSslErrors(QNetworkReply *reply)
{
    if (settings.requestServerIgnoreCertErrors())
        reply->ignoreSslErrors();
    else
    {
        QMessageBox::warning(this, "SSL Error", "Unable to establish secure conneciton with server.");
    }
}



void DlgSettings::on_btnClose_clicked()
{
    close();
}

void DlgSettings::on_pushButtonFont_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, settings.tickerFont(), this, "Select ticker font");
    if (ok)
    {
        settings.setTickerFont(font);
    }
}

void DlgSettings::on_horizontalSliderTickerSpeed_valueChanged(int value)
{
    settings.setTickerSpeed(value);
}

void DlgSettings::on_pushButtonTextColor_clicked()
{
    QColor clr = QColorDialog::getColor(settings.tickerTextColor(),this,"Select ticker text color");
    if (clr.isValid())
    {
        QString ss = ui->pushButtonTextColor->styleSheet();
        QColor oclr = settings.tickerTextColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," + QString::number(oclr.blue())), QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," + QString::number(clr.blue())));
        ui->pushButtonTextColor->setStyleSheet(ss);
        settings.setTickerTextColor(clr);
    }
}

void DlgSettings::on_pushButtonBgColor_clicked()
{
    QColor clr = QColorDialog::getColor(settings.tickerBgColor(),this,"Select ticker background color");
    if (clr.isValid())
    {
        QString ss = ui->pushButtonBgColor->styleSheet();
        QColor oclr = settings.tickerBgColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," + QString::number(oclr.blue())), QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," + QString::number(clr.blue())));
        ui->pushButtonBgColor->setStyleSheet(ss);
        settings.setTickerBgColor(clr);
    }
}

void DlgSettings::on_radioButtonFullRotation_toggled(bool checked)
{
    settings.setTickerFullRotation(checked);
    ui->spinBoxTickerSingers->setEnabled(!checked);
}



void DlgSettings::on_spinBoxTickerSingers_valueChanged(int arg1)
{
    settings.setTickerShowNumSingers(arg1);
}

void DlgSettings::on_groupBoxTicker_toggled(bool arg1)
{
    settings.setTickerEnabled(arg1);
}

void DlgSettings::on_lineEditUrl_editingFinished()
{
    settings.setRequestServerUrl(ui->lineEditUrl->text());
}

void DlgSettings::on_checkBoxIgnoreCertErrors_toggled(bool checked)
{
    settings.setRequestServerIgnoreCertErrors(checked);
}

void DlgSettings::on_groupBoxRequestServer_toggled(bool arg1)
{
    settings.setRequestServerEnabled(arg1);
}

void DlgSettings::on_pushButtonBrowse_clicked()
{
    QString imageFile = QFileDialog::getOpenFileName(this,QString("Select image file"), QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), QString("Images (*.png *.jpg *.jpeg *.gif)"));
    if (imageFile != "")
    {
        QImage image(imageFile);
        if (!image.isNull())
        {
            settings.setCdgDisplayBackgroundImage(imageFile);
            ui->lineEditCdgBackground->setText(imageFile);
        }
        else
        {
            QMessageBox::warning(this, tr("Image load error"),QString("Unsupported or corrupt image file."));
        }
    }
}

void DlgSettings::on_checkBoxFader_toggled(bool checked)
{
    settings.setAudioUseFader(checked);
    emit audioUseFaderChanged(checked);
}

void DlgSettings::on_checkBoxFaderBm_toggled(bool checked)
{
    settings.setAudioUseFaderBm(checked);
    emit audioUseFaderChangedBm(checked);
}

void DlgSettings::on_checkBoxSilenceDetection_toggled(bool checked)
{
    settings.setAudioDetectSilence(checked);
    emit audioSilenceDetectChanged(checked);
}

void DlgSettings::on_checkBoxSilenceDetectionBm_toggled(bool checked)
{
    settings.setAudioDetectSilenceBm(checked);
    emit audioSilenceDetectChangedBm(checked);
}

void DlgSettings::on_checkBoxDownmix_toggled(bool checked)
{
    settings.setAudioDownmix(checked);
    emit audioDownmixChanged(checked);
}

void DlgSettings::on_checkBoxDownmixBm_toggled(bool checked)
{
    settings.setAudioDownmixBm(checked);
    emit audioDownmixChangedBm(checked);
}

void DlgSettings::on_comboBoxDevice_currentIndexChanged(const QString &arg1)
{
    if (pageSetupDone)
        settings.setRecordingInput(arg1);
}

void DlgSettings::on_comboBoxCodec_currentIndexChanged(const QString &arg1)
{
    if (pageSetupDone)
    {
        settings.setRecordingCodec(arg1);
        if (arg1 == "audio/mpeg")
        {
            settings.setRecordingRawExtension("mp3");
        }
    }
}

void DlgSettings::on_groupBoxRecording_toggled(bool arg1)
{
    if (pageSetupDone)
        settings.setRecordingEnabled(arg1);
}

void DlgSettings::on_buttonBrowse_clicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this, "Select the output directory",QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
    if (dirName != "")
    {
        settings.setRecordingOutputDir(dirName);
        ui->lineEditOutputDir->setText(dirName);
    }
}

void DlgSettings::on_pushButtonClearBgImg_clicked()
{
    settings.setCdgDisplayBackgroundImage(QString());
    ui->lineEditCdgBackground->setText(QString());
}

void DlgSettings::on_pushButtonSlideshowBrowse_clicked()
{
    QString initialPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (settings.bgSlideShowDir() != "")
        initialPath = settings.bgSlideShowDir();
    QString dirName = QFileDialog::getExistingDirectory(this, "Select the slideshow directory", initialPath);
    if (dirName != "")
    {
        settings.setBgSlideShowDir(dirName);
        ui->lineEditSlideshowDir->setText(dirName);
    }
}

void DlgSettings::on_rbSlideshow_toggled(bool checked)
{
    if (checked)
        settings.setBgMode(settings.BG_MODE_SLIDESHOW);
    else
        settings.setBgMode(settings.BG_MODE_IMAGE);
}

void DlgSettings::on_rbBgImage_toggled(bool checked)
{
    if (checked)
        settings.setBgMode(settings.BG_MODE_IMAGE);
    else
        settings.setBgMode(settings.BG_MODE_SLIDESHOW);
}

void DlgSettings::on_lineEditApiKey_editingFinished()
{
    settings.setRequestServerApiKey(ui->lineEditApiKey->text());
}

void DlgSettings::on_checkBoxShowKAAAlert_toggled(bool checked)
{
    settings.setKaraokeAAAlertEnabled(checked);
}

void DlgSettings::on_checkBoxKAA_toggled(bool checked)
{
    settings.setKaraokeAutoAdvance(checked);
}

void DlgSettings::on_spinBoxAADelay_valueChanged(int arg1)
{
    settings.setKaraokeAATimeout(arg1);
}

void DlgSettings::on_btnAlertFont_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, settings.karaokeAAAlertFont(), this, "Select alert font");
    if (ok)
    {
        settings.setKaraokeAAAlertFont(font);
    }
}

void DlgSettings::on_btnAlertTxtColor_clicked()
{
    QColor clr = QColorDialog::getColor(settings.alertTxtColor(),this,"Select alert text color");
    if (clr.isValid())
    {
        QString ss = ui->btnAlertTxtColor->styleSheet();
        QColor oclr = settings.alertTxtColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," + QString::number(oclr.blue())), QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," + QString::number(clr.blue())));
        ui->btnAlertTxtColor->setStyleSheet(ss);
        settings.setAlertTxtColor(clr);
    }
}

void DlgSettings::on_btnAlertBgColor_clicked()
{
    QColor clr = QColorDialog::getColor(settings.alertBgColor(),this,"Select alert background color");
    if (clr.isValid())
    {
        QString ss = ui->btnAlertBgColor->styleSheet();
        QColor oclr = settings.alertBgColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," + QString::number(oclr.blue())), QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," + QString::number(clr.blue())));
        ui->btnAlertBgColor->setStyleSheet(ss);
        settings.setAlertBgColor(clr);
    }
}

void DlgSettings::on_cbxBmAutostart_clicked(bool checked)
{
    settings.setBmAutoStart(checked);
}

void DlgSettings::on_spinBoxInterval_valueChanged(int arg1)
{
    if (pageSetupDone)
        settings.setRequestServerInterval(arg1);
}

void DlgSettings::tickerShowRotationInfoChanged(bool show)
{
    ui->radioButtonFullRotation->setEnabled(show);
    ui->radioButtonPartialRotation->setEnabled(show);
    ui->spinBoxTickerSingers->setEnabled(show);
    ui->label_5->setEnabled(show);
    ui->cbxTickerShowRotationInfo->setChecked(show);
}

void DlgSettings::on_cbxTheme_currentIndexChanged(int index)
{
    if (pageSetupDone)
        settings.setTheme(index);
}

void DlgSettings::on_btnBrowse_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(this, "Select directory to put store downloads in",settings.storeDownloadDir());
    if (fileName != "")
    {
        QFileInfo fi(fileName);
        if (!fi.isWritable() || !fi.isReadable())
        {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Directory not writable!");
            msgBox.setText("You do not have permission to write to the selected directory, aborting.");
            msgBox.exec();
        }
        settings.setStoreDownloadDir(fileName + QDir::separator());
        ui->lineEditDownloadsDir->setText(fileName + QDir::separator());
    }
}

void DlgSettings::on_fontComboBox_currentFontChanged(const QFont &f)
{
    if (!pageSetupDone)
        return;
    QFont font = f;
    font.setPointSize(ui->spinBoxAppFontSize->value());
    settings.setApplicationFont(font);
    setFont(font);
}

void DlgSettings::on_spinBoxAppFontSize_valueChanged(int arg1)
{
    if (!pageSetupDone)
        return;
    QFont font = settings.applicationFont();
    font.setPointSize(arg1);
    settings.setApplicationFont(font);
    setFont(font);
}

void DlgSettings::on_btnTestReqServer_clicked()
{
    OKJSongbookAPI *api = new OKJSongbookAPI(this);
    connect(api, SIGNAL(testFailed(QString)), this, SLOT(reqSvrTestError(QString)));
    connect(api, SIGNAL(testSslError(QString)), this, SLOT(reqSvrTestSslError(QString)));
    connect(api, SIGNAL(testPassed()), this, SLOT(reqSvrTestPassed()));
    api->test();

    delete api;
}

void DlgSettings::reqSvrTestError(QString error)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Request server test failed!");
    msgBox.setText("Request server connection test was unsuccessful!");
    msgBox.setInformativeText("Error msg:\n" + error);
    msgBox.exec();
}

void DlgSettings::reqSvrTestSslError(QString error)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Request server test failed!");
    msgBox.setText("Request server connection test was unsuccessful due to SSL errors!");
    msgBox.setInformativeText("Error msg:\n" + error);
    msgBox.exec();
}

void DlgSettings::reqSvrTestPassed()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Request server test passed");
    msgBox.setText("Request server connection test was successful.  Server info and API key appear to be valid");
    msgBox.exec();
}

void DlgSettings::on_checkBoxIncludeEmptySingers_clicked(bool checked)
{
    settings.setEstimationSkipEmptySingers(!checked);
}

void DlgSettings::on_spinBoxDefaultPadTime_valueChanged(int arg1)
{
    if (pageSetupDone)
    {
        settings.setEstimationSingerPad(arg1);
    }
}

void DlgSettings::on_spinBoxDefaultSongDuration_valueChanged(int arg1)
{
    if (pageSetupDone)
    {
        settings.setEstimationEmptySongLength(arg1);
    }
}

void DlgSettings::on_checkBoxDisplayCurrentRotationPosition_clicked(bool checked)
{
    settings.setRotationDisplayPosition(checked);

}

void DlgSettings::entitledSystemCountChanged(int count)
{
    ui->spinBoxSystemId->setMaximum(count);
    if (settings.systemId() <= count)
    {
        ui->spinBoxSystemId->setValue(settings.systemId());
    }
}

void DlgSettings::on_groupBoxShowDuration_clicked(bool checked)
{
    settings.setCdgRemainEnabled(checked);
}

void DlgSettings::on_btnDurationFont_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, settings.cdgRemainFont(), this, "Select CDG duration display font");
    if (ok)
    {
        settings.setCdgRemainFont(font);
    }
}

void DlgSettings::on_btnDurationFontColor_clicked()
{
    QColor clr = QColorDialog::getColor(settings.cdgRemainTextColor(),this,"Select CDG duration display text color");
    if (clr.isValid())
    {
        QString ss = ui->btnDurationFontColor->styleSheet();
        QColor oclr = settings.cdgRemainTextColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," + QString::number(oclr.blue())), QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," + QString::number(clr.blue())));
        ui->btnDurationFontColor->setStyleSheet(ss);
        settings.setCdgRemainTextColor(clr);
    }
}

void DlgSettings::on_btnDurationBgColor_clicked()
{
    QColor clr = QColorDialog::getColor(settings.cdgRemainBgColor(),this,"Select CDG duration display background color");
    if (clr.isValid())
    {
        QString ss = ui->btnDurationBgColor->styleSheet();
        QColor oclr = settings.cdgRemainBgColor();
        ss.replace(QString(QString::number(oclr.red()) + "," + QString::number(oclr.green()) + "," + QString::number(oclr.blue())), QString(QString::number(clr.red()) + "," + QString::number(clr.green()) + "," + QString::number(clr.blue())));
        ui->btnDurationBgColor->setStyleSheet(ss);
        settings.setCdgRemainBgColor(clr);
    }
}

void DlgSettings::on_btnLogDirBrowse_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(this, "Select directory to put logs in",settings.logDir());
    if (fileName != "")
    {
        QFileInfo fi(fileName);
        if (!fi.isWritable() || !fi.isReadable())
        {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Directory not writable!");
            msgBox.setText("You do not have permission to write to the selected directory, aborting.");
            msgBox.exec();
        }
        settings.setLogDir(fileName + QDir::separator());
        ui->lineEditLogDir->setText(fileName + QDir::separator());
    }
}

void DlgSettings::on_checkBoxProgressiveSearch_toggled(bool checked)
{
    settings.setProgressiveSearchEnabled(checked);
}

void DlgSettings::on_cbxPreviewEnabled_toggled(bool checked)
{
    settings.setPreviewEnabled(!checked);
}

void DlgSettings::on_comboBoxKAudioDevices_currentIndexChanged(int index)
{
    static int lastSelIndex = index;
    static QString lastSelItem = ui->comboBoxKAudioDevices->itemText(index);
    QString device = ui->comboBoxKAudioDevices->itemText(index);
    if (lastSelItem == device)
        return;
    if (kAudioBackend->state() == MediaBackend::PlayingState)
    {
        QMessageBox msgBox;
        msgBox.setText("Can not change audio device while audio is playing, please stop playback and try again");
        msgBox.exec();
        int selDevice = audioOutputDevices.indexOf(settings.audioOutputDevice());
        if (selDevice == -1)
            ui->comboBoxKAudioDevices->setCurrentIndex(0);
        else
            ui->comboBoxKAudioDevices->setCurrentIndex(lastSelIndex);
        return;
    }
    if (pageSetupDone)
    {
        settings.setAudioOutputDevice(device);
        int deviceIndex = audioOutputDevices.indexOf(QRegExp(device,Qt::CaseSensitive,QRegExp::FixedString));
        if (deviceIndex != -1)
            kAudioBackend->setOutputDevice(deviceIndex);
    }
    lastSelItem = device;
}

void DlgSettings::on_comboBoxBAudioDevices_currentIndexChanged(int index)
{
    static int lastSelIndex = index;
    static QString lastSelItem = ui->comboBoxBAudioDevices->itemText(index);
    QString device = ui->comboBoxBAudioDevices->itemText(index);
    if (lastSelIndex == index)
    {
        qInfo() << "BM audio device index change fired but index is the same, ignoring";
        return;
    }
    if (bmAudioBackend->state() == MediaBackend::PlayingState)
    {
        QMessageBox msgBox;
        msgBox.setText("Can not change audio device while audio is playing, please stop playback and try again");
        msgBox.exec();
        int selDevice = audioOutputDevices.indexOf(settings.audioOutputDevice());
        if (selDevice == -1)
            ui->comboBoxBAudioDevices->setCurrentIndex(0);
        else
            ui->comboBoxBAudioDevices->setCurrentIndex(lastSelIndex);
        return;
    }
    if (pageSetupDone)
    {
        settings.setAudioOutputDeviceBm(device);
        int deviceIndex = audioOutputDevices.indexOf(QRegExp(device,Qt::CaseSensitive,QRegExp::FixedString));
        if (deviceIndex != -1)
            bmAudioBackend->setOutputDevice(deviceIndex);
    }
    lastSelItem = device;
}

//void DlgSettings::on_comboBoxMonitors_currentIndexChanged(int index)
//{
//    if (!pageSetupDone)
//        return;
//    QDesktopWidget widget;
//    int appMonitor = widget.screenNumber(ui->tabWidget);
//    int selMonitor = index;
//    if ((selMonitor == appMonitor) && (settings.cdgWindowFullscreen()))
//    {
//        QMessageBox msgBox;
//        msgBox.setText("Warning: The selected CDG fullscreen display monitor is the same as the one that the main app is displayed on!");
//        msgBox.setInformativeText("This is probably not what you want. Are you sure you want to do this?");
//        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
//        msgBox.setDefaultButton(QMessageBox::Cancel);
//        int ret = msgBox.exec();
//        if (ret == QMessageBox::Cancel)
//            return;
//    }
//    settings.setCdgWindowFullscreenMonitor(selMonitor);
//}

void DlgSettings::on_checkBoxEnforceAspectRatio_clicked(bool checked)
{
    settings.setEnforceAspectRatio(checked);
}

void DlgSettings::on_pushButtonApplyTickerMsg_clicked()
{
    settings.setTickerCustomString(ui->lineEditTickerMessage->text());
}

void DlgSettings::on_pushButtonResetDurationPos_clicked()
{
    settings.resetDurationPosition();
}

void DlgSettings::on_lineEditTickerMessage_returnPressed()
{
    settings.setTickerCustomString(ui->lineEditTickerMessage->text());
}

void DlgSettings::on_checkBoxHardwareAccel_toggled(bool checked)
{
    settings.setHardwareAccelEnabled(checked);
}
