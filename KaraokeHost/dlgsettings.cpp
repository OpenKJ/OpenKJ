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

#include "dlgsettings.h"
#include "ui_dlgsettings.h"
#include <QDebug>
#include <QApplication>
#include <QDesktopWidget>
#include <QFontDialog>
#include <QColorDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QAudioRecorder>
#include <QSqlQuery>
#include <QtSql>
#include <QXmlStreamWriter>
#include <QNetworkReply>
#include <QAuthenticator>

extern KhSettings *settings;


DlgSettings::DlgSettings(KhAudioBackends *AudioBackends, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgSettings)
{
    pageSetupDone = false;
    audioBackends = AudioBackends;
    audioBackend = AudioBackends->at(settings->audioBackend());
    networkManager = new QNetworkAccessManager(this);
    ui->setupUi(this);
    for (int i=0; i < audioBackends->size(); i++)
    {
        ui->comboBoxBackend->addItem(audioBackends->at(i)->backendName());
    }
    ui->comboBoxBackend->setCurrentIndex(settings->audioBackend());
    createIcons();
    ui->listWidget->setCurrentRow(0);
    QStringList screens = getMonitors();
    ui->listWidgetMonitors->addItems(screens);
    ui->checkBoxShowCdgWindow->setChecked(settings->showCdgWindow());
    ui->groupBoxMonitors->setChecked(settings->cdgWindowFullscreen());
    ui->listWidgetMonitors->setEnabled(settings->showCdgWindow());
    ui->groupBoxMonitors->setEnabled(settings->showCdgWindow());
    if (screens.count() > settings->cdgWindowFullScreenMonitor())
        ui->listWidgetMonitors->item(settings->cdgWindowFullScreenMonitor())->setSelected(true);
    else
        settings->setCdgWindowFullscreen(false);
    ui->spinBoxTickerHeight->setValue(settings->tickerHeight());
    ui->horizontalSliderTickerSpeed->setValue(settings->tickerSpeed());
    QPalette txtpalette = ui->pushButtonTextColor->palette();
    txtpalette.setColor(QPalette::Button, settings->tickerTextColor());
    ui->pushButtonTextColor->setPalette(txtpalette);
    QPalette bgpalette = ui->pushButtonBgColor->palette();
    bgpalette.setColor(QPalette::Button, settings->tickerBgColor());
    ui->pushButtonBgColor->setPalette(bgpalette);
    if (settings->tickerFullRotation())
    {
        ui->radioButtonFullRotation->setChecked(true);
        ui->spinBoxTickerSingers->setEnabled(false);
    }
    else
    {
        ui->radioButtonPartialRotation->setChecked(true);
        ui->spinBoxTickerSingers->setEnabled(true);
    }
    ui->spinBoxTickerSingers->setValue(settings->tickerShowNumSingers());
    ui->groupBoxRequestServer->setChecked(settings->requestServerEnabled());
    ui->lineEditUrl->setText(settings->requestServerUrl());
    ui->lineEditUsername->setText(settings->requestServerUsername());
    ui->lineEditPassword->setText(settings->requestServerPassword());
    ui->checkBoxIgnoreCertErrors->setChecked(settings->requestServerIgnoreCertErrors());
    ui->lineEditCdgBackground->setText(settings->cdgDisplayBackgroundImage());
    ui->checkBoxFader->setChecked(settings->audioUseFader());
    ui->checkBoxDownmix->setChecked(settings->audioDownmix());
    ui->checkBoxSilenceDetection->setChecked(settings->audioDetectSilence());
    QAudioRecorder audioRecorder;
    QStringList inputs = audioRecorder.audioInputs();
    QStringList codecs = audioRecorder.supportedAudioCodecs();
    QStringList containers = audioRecorder.supportedContainers();
    ui->groupBoxRecording->setChecked(settings->recordingEnabled());
    ui->comboBoxDevice->addItems(inputs);
    ui->comboBoxCodec->addItems(codecs);
    ui->comboBoxContainer->addItems(containers);
    ui->comboBoxDevice->setCurrentIndex(ui->comboBoxDevice->findText(settings->recordingInput()));
    ui->comboBoxCodec->setCurrentIndex(ui->comboBoxCodec->findText(settings->recordingCodec()));
    ui->comboBoxContainer->setCurrentIndex(ui->comboBoxContainer->findText(settings->recordingContainer()));
    ui->lineEditExtension->setText(settings->recordingRawExtension());
    ui->lineEditOutputDir->setText(settings->recordingOutputDir());
    if (settings->recordingContainer() != "raw")
    {
        ui->lineEditExtension->hide();
        ui->labelExtension->hide();
    }
    audioBackendChanged(settings->audioBackend());
    connect(settings, SIGNAL(audioBackendChanged(int)), this, SLOT(audioBackendChanged(int)));

    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onNetworkReply(QNetworkReply*)));
    connect(networkManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(onSslErrors(QNetworkReply*)));
    connect(networkManager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), this, SLOT(setAuth(QNetworkReply*,QAuthenticator*)));

    pageSetupDone = true;
}

DlgSettings::~DlgSettings()
{
    delete ui;
}

QStringList DlgSettings::getMonitors()
{
    QStringList screenStrings;
    for (int i=0; i < QApplication::desktop()->screenCount(); i++)
    {
        int sWidth = QApplication::desktop()->screenGeometry(i).width();
        int sHeight = QApplication::desktop()->screenGeometry(i).height();
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
    if (settings->requestServerIgnoreCertErrors())
        reply->ignoreSslErrors();
    else
    {
        QMessageBox::warning(this, "SSL Error", "Unable to establish secure conneciton with server.");
    }
}

void DlgSettings::setAuth(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(reply);
    static bool firstTry = true;
    static QString lastUser;
    static QString lastPass;
    static QString lastUrl;
    if ((lastUser != settings->requestServerUsername()) || (lastPass != settings->requestServerPassword()) || (lastUrl != settings->requestServerUrl()))
    {
        firstTry = true;
    }
    if (!firstTry)
    {
        QMessageBox::warning(this, "Authentication Error", "Server rejected the provided username and password.");
        return;
    }
    qDebug() << "RequestsClient - Received authentication request, sending username and password";
    authenticator->setUser(settings->requestServerUsername());
    authenticator->setPassword(settings->requestServerPassword());
    lastUser = settings->requestServerUsername();
    lastPass = settings->requestServerPassword();
    lastUrl = settings->requestServerUrl();
    firstTry = false;
}


void DlgSettings::createIcons()
{
    QListWidgetItem *audioButton = new QListWidgetItem(ui->listWidget);
    audioButton->setIcon(QIcon(":/icons/Icons/audio-card.png"));
    audioButton->setText(tr("Audio"));
    audioButton->setTextAlignment(Qt::AlignHCenter);
    audioButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    QListWidgetItem *videoButton = new QListWidgetItem(ui->listWidget);
    videoButton->setIcon(QIcon(":/icons/Icons/video-display.png"));
    videoButton->setText(tr("Video"));
    videoButton->setTextAlignment(Qt::AlignHCenter);
    videoButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    QListWidgetItem *networkButton = new QListWidgetItem(ui->listWidget);
    networkButton->setIcon(QIcon(":/icons/Icons/network-wired.png"));
    networkButton->setText(tr("Network"));
    networkButton->setTextAlignment(Qt::AlignHCenter);
    networkButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

void DlgSettings::on_btnClose_clicked()
{
    close();
}

void DlgSettings::on_checkBoxShowCdgWindow_stateChanged(int arg1)
{
    settings->setShowCdgWindow(arg1);
    emit showCdgWindowChanged(arg1);
    ui->listWidgetMonitors->setEnabled(arg1);
    ui->groupBoxMonitors->setEnabled(arg1);
}

void DlgSettings::on_groupBoxMonitors_toggled(bool arg1)
{
    settings->setCdgWindowFullscreen(arg1);
    emit cdgWindowFullScreenChanged(arg1);
}

void DlgSettings::on_listWidgetMonitors_itemSelectionChanged()
{
    int selMonitor = ui->listWidgetMonitors->selectionModel()->selectedIndexes().at(0).row();
    settings->setCdgWindowFullscreenMonitor(selMonitor);
    emit cdgWindowFullScreenMonitorChanged(selMonitor);
}

void DlgSettings::on_pushButtonFont_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, settings->tickerFont(), this, "Select ticker font");
    if (ok)
    {
        settings->setTickerFont(font);
    }
}

void DlgSettings::on_spinBoxTickerHeight_valueChanged(int arg1)
{
    settings->setTickerHeight(arg1);
}

void DlgSettings::on_horizontalSliderTickerSpeed_valueChanged(int value)
{
    settings->setTickerSpeed(value);
}

void DlgSettings::on_pushButtonTextColor_clicked()
{
    QColor color = QColorDialog::getColor(settings->tickerTextColor(),this,"Select ticker text color");
    if (color.isValid())
    {
        settings->setTickerTextColor(color);
        QPalette palette = ui->pushButtonTextColor->palette();
        palette.setColor(ui->pushButtonTextColor->backgroundRole(), color);
        ui->pushButtonTextColor->setPalette(palette);
    }
}

void DlgSettings::on_pushButtonBgColor_clicked()
{
    QColor color = QColorDialog::getColor(settings->tickerTextColor(),this,"Select ticker background color");
    if (color.isValid())
    {
        settings->setTickerBgColor(color);
        QPalette palette = ui->pushButtonBgColor->palette();
        palette.setColor(ui->pushButtonBgColor->backgroundRole(), color);
        ui->pushButtonBgColor->setPalette(palette);
    }
}

void DlgSettings::on_radioButtonFullRotation_toggled(bool checked)
{
    settings->setTickerFullRotation(checked);
    ui->spinBoxTickerSingers->setEnabled(!checked);
}



void DlgSettings::on_spinBoxTickerSingers_valueChanged(int arg1)
{
    settings->setTickerShowNumSingers(arg1);
}

void DlgSettings::on_groupBoxTicker_toggled(bool arg1)
{
    settings->setTickerEnabled(arg1);
}

void DlgSettings::on_lineEditUrl_editingFinished()
{
    settings->setRequestServerUrl(ui->lineEditUrl->text());
}

void DlgSettings::on_checkBoxIgnoreCertErrors_toggled(bool checked)
{
    settings->setRequestServerIgnoreCertErrors(checked);
}

void DlgSettings::on_lineEditUsername_editingFinished()
{
    settings->setRequestServerUsername(ui->lineEditUsername->text());
}

void DlgSettings::on_lineEditPassword_editingFinished()
{
    settings->setRequestServerPassword(ui->lineEditPassword->text());
}

void DlgSettings::on_groupBoxRequestServer_toggled(bool arg1)
{
    settings->setRequestServerEnabled(arg1);
}

void DlgSettings::on_pushButtonBrowse_clicked()
{
    QString imageFile = QFileDialog::getOpenFileName(this,tr("Select image file"), QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), tr("Images (*.png *.jpg *.jpeg *.gif)"));
    if (imageFile != "")
    {
        QImage image(imageFile);
        if (!image.isNull())
        {
            settings->setCdgDisplayBackgroundImage(imageFile);
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
    settings->setAudioUseFader(checked);
    emit audioUseFaderChanged(checked);
}

void DlgSettings::on_checkBoxSilenceDetection_toggled(bool checked)
{
    settings->setAudioDetectSilence(checked);
    emit audioSilenceDetectChanged(checked);
}

void DlgSettings::on_checkBoxDownmix_toggled(bool checked)
{
    settings->setAudioDownmix(checked);
    emit audioDownmixChanged(checked);
}

void DlgSettings::on_listWidgetAudioDevices_itemSelectionChanged()
{
    if (pageSetupDone)
    {
        QString device = ui->listWidgetAudioDevices->selectedItems().at(0)->text();
        settings->setAudioOutputDevice(device);
        int deviceIndex = audioBackend->getOutputDevices().indexOf(QRegExp(device,Qt::CaseSensitive,QRegExp::FixedString));
        if (deviceIndex != -1)
            audioBackend->setOutputDevice(deviceIndex);
    }
}

void DlgSettings::on_comboBoxBackend_currentIndexChanged(int index)
{
    if (pageSetupDone)
    {
        qDebug() << "SettingsDialog::on_comboBoxBackend_currentIndexChanged(" << index << ") fired";
        if (audioBackend->state() != KhAbstractAudioBackend::StoppedState)
        {
            QMessageBox::warning(this, "Unable to complete action","You can not change the audio backend while there is an active song playing or paused.  Please stop the current song and try again");
            ui->comboBoxBackend->setCurrentIndex(settings->audioBackend());
        }
        else
            settings->setAudioBackend(index);
    }
}

void DlgSettings::audioBackendChanged(int index)
{
    pageSetupDone = false;
    audioBackend = audioBackends->at(index);
    ui->checkBoxSilenceDetection->setHidden(!audioBackend->canDetectSilence());
    ui->checkBoxDownmix->setHidden(!audioBackend->canDownmix());
    ui->checkBoxFader->setHidden(!audioBackend->canFade());
    ui->listWidgetAudioDevices->clear();
    ui->listWidgetAudioDevices->addItems(audioBackend->getOutputDevices());
    pageSetupDone = true;
    if (audioBackend->getOutputDevices().contains(settings->audioOutputDevice()))
    {
        ui->listWidgetAudioDevices->findItems(settings->audioOutputDevice(),Qt::MatchExactly).at(0)->setSelected(true);
    }
    else
        ui->listWidgetAudioDevices->item(0)->setSelected(true);
    if (!audioBackend->downmixChangeRequiresRestart())
        ui->checkBoxDownmix->setText("Downmix to mono");
    else
        ui->checkBoxDownmix->setText("Dowmix to mono (requires restart)");
}

void DlgSettings::on_comboBoxDevice_currentIndexChanged(const QString &arg1)
{
    if (pageSetupDone)
        settings->setRecordingInput(arg1);
}

void DlgSettings::on_comboBoxCodec_currentIndexChanged(const QString &arg1)
{
    if (pageSetupDone)
    {
        settings->setRecordingCodec(arg1);
        if (arg1 == "audio/mpeg")
        {
            ui->lineEditExtension->setText("mp3");
            settings->setRecordingRawExtension("mp3");
        }
    }
}

void DlgSettings::on_comboBoxContainer_currentIndexChanged(const QString &arg1)
{
    if (pageSetupDone)
    {
        settings->setRecordingContainer(arg1);
        if (arg1 == "raw")
        {
            ui->labelExtension->show();
            ui->lineEditExtension->show();
            if (settings->recordingCodec() == "audio/mpeg")
            {
                ui->lineEditExtension->setText("mp3");
                settings->setRecordingRawExtension("mp3");
            }
            else
                ui->lineEditExtension->setText(settings->recordingRawExtension());

        }
        else
        {
            ui->labelExtension->hide();
            ui->lineEditExtension->hide();
        }
    }
}

void DlgSettings::on_groupBoxRecording_toggled(bool arg1)
{
    if (pageSetupDone)
        settings->setRecordingEnabled(arg1);
}

void DlgSettings::on_lineEditExtension_editingFinished()
{
    settings->setRecordingRawExtension(ui->lineEditExtension->text());
}

void DlgSettings::on_buttonBrowse_clicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this, "Select the output directory",QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
    if (dirName != "")
    {
        settings->setRecordingOutputDir(dirName);
        ui->lineEditOutputDir->setText(dirName);
    }
}

void DlgSettings::on_pushButtonUpdateRemoteDb_clicked()
{
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setStandardButtons(0);
    msgBox->setText("Updating remote database.  Please wait...");
    msgBox->show();
    msgBox->setInformativeText("Generating data for transmission...");
    QTemporaryDir dir;
    QFile xmlFile(dir.path() + QDir::separator() + "songdata.xml");
    xmlFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    QXmlStreamWriter xml(&xmlFile);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("songs");

    QApplication::processEvents();
    QSqlQuery query;
    query.exec("SELECT DISTINCT artist,title FROM dbsongs ORDER BY artist ASC, title ASC");
    while (query.next())
    {
        QApplication::processEvents();
        xml.writeStartElement("song");
        xml.writeAttribute("artist", query.value(0).toString());
        xml.writeAttribute("title", query.value(1).toString());
        xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndDocument();
    xmlFile.close();
    msgBox->setInformativeText("Uploading data to server...");
    QNetworkRequest request(QUrl(settings->requestServerUrl() + "/updateSongs.php"));
    QString bound="margin";
    QByteArray data(QString("--" + bound + "\r\n").toLocal8Bit());
    data.append("Content-Disposition: form-data; name=\"action\"\r\n\r\n");
    data.append("updateSongs.php\r\n");
    data.append("--" + bound + "\r\n");
    data.append("Content-Disposition: form-data; name=\"uploaded\"; filename=\"songData.xml\"\r\n");
    data.append("Content-Type: text/xml\r\n\r\n");
    if (!xmlFile.open(QIODevice::ReadOnly))
        return;
    data.append(xmlFile.readAll());
    data.append("\r\n");
    data.append("--" + bound + "--\r\n");
    request.setRawHeader(QString("Content-Type").toLocal8Bit(),QString("multipart/form-data; boundary=" + bound).toLocal8Bit());
    request.setRawHeader(QString("Content-Length").toLocal8Bit(), QString::number(data.length()).toLocal8Bit());
    QNetworkReply *reply = networkManager->post(request,data);
    while (!reply->isFinished())
        QApplication::processEvents();
    msgBox->close();
    delete msgBox;
    QMessageBox::information(this, "Update complete", "Remote database update complete.");
}
