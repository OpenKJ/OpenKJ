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

#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QDebug>
#include <QApplication>
#include <QDesktopWidget>
#include <QFontDialog>
#include <QColorDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>

extern KhSettings *settings;


SettingsDialog::SettingsDialog(KhAudioBackends *AudioBackends, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    pageSetupDone = false;
    audioBackends = AudioBackends;
    audioBackend = AudioBackends->at(settings->audioBackend());
    ui->setupUi(this);
    for (int i=0; i < audioBackends->size(); i++)
    {
        ui->comboBoxBackend->addItem(audioBackends->at(i)->backendName());
    }
    ui->comboBoxBackend->setCurrentIndex(settings->audioBackend());
    createIcons();
    QStringList screens = getMonitors();
    ui->listWidgetMonitors->addItems(screens);
    ui->checkBoxShowCdgWindow->setChecked(settings->showCdgWindow());
    ui->groupBoxMonitors->setChecked(settings->cdgWindowFullscreen());
    ui->listWidgetMonitors->setEnabled(settings->showCdgWindow());
    ui->groupBoxMonitors->setEnabled(settings->showCdgWindow());
    ui->listWidgetMonitors->item(settings->cdgWindowFullScreenMonitor())->setSelected(true);
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
    audioBackendChanged(settings->audioBackend());
    connect(settings, SIGNAL(audioBackendChanged(int)), this, SLOT(audioBackendChanged(int)));
//    ui->checkBoxSilenceDetection->setHidden(!audioBackend->canDetectSilence());
//    ui->checkBoxDownmix->setHidden(!audioBackend->canDownmix());
//    ui->checkBoxFader->setHidden(!audioBackend->canFade());
//    ui->listWidgetAudioDevices->addItems(audioBackend->getOutputDevices());
//    if (audioBackend->getOutputDevices().contains(settings->audioOutputDevice()))
//    {
//        ui->listWidgetAudioDevices->findItems(settings->audioOutputDevice(),Qt::MatchExactly).at(0)->setSelected(true);
//    }
//    else
//        ui->listWidgetAudioDevices->item(0)->setSelected(true);
    pageSetupDone = true;
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

QStringList SettingsDialog::getMonitors()
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


void SettingsDialog::createIcons()
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

void SettingsDialog::on_btnClose_clicked()
{
    close();
}

void SettingsDialog::on_checkBoxShowCdgWindow_stateChanged(int arg1)
{
    settings->setShowCdgWindow(arg1);
    emit showCdgWindowChanged(arg1);
    ui->listWidgetMonitors->setEnabled(arg1);
    ui->groupBoxMonitors->setEnabled(arg1);
}

void SettingsDialog::on_groupBoxMonitors_toggled(bool arg1)
{
    settings->setCdgWindowFullscreen(arg1);
    emit cdgWindowFullScreenChanged(arg1);
}

void SettingsDialog::on_listWidgetMonitors_itemSelectionChanged()
{
    int selMonitor = ui->listWidgetMonitors->selectionModel()->selectedIndexes().at(0).row();
    settings->setCdgWindowFullscreenMonitor(selMonitor);
    emit cdgWindowFullScreenMonitorChanged(selMonitor);
}

void SettingsDialog::on_pushButtonFont_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, settings->tickerFont(), this, "Select ticker font");
    if (ok)
    {
        settings->setTickerFont(font);
    }
}

void SettingsDialog::on_spinBoxTickerHeight_valueChanged(int arg1)
{
    settings->setTickerHeight(arg1);
}

void SettingsDialog::on_horizontalSliderTickerSpeed_valueChanged(int value)
{
    settings->setTickerSpeed(value);
}

void SettingsDialog::on_pushButtonTextColor_clicked()
{
    QColor color = QColorDialog::getColor(settings->tickerTextColor(),this,"Select ticker text color");
    if (color.isValid())
    {
        settings->setTickerTextColor(color);
        QPalette palette = ui->pushButtonTextColor->palette();
        palette.setColor(ui->pushButtonTextColor->backgroundRole(), color);
        ui->pushButtonTextColor->setPalette(palette);
    }

//    QColor color = QColorDialog::getColor()
}

void SettingsDialog::on_pushButtonBgColor_clicked()
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

void SettingsDialog::on_radioButtonFullRotation_toggled(bool checked)
{
    settings->setTickerFullRotation(checked);
    ui->spinBoxTickerSingers->setEnabled(!checked);
}



void SettingsDialog::on_spinBoxTickerSingers_valueChanged(int arg1)
{
    settings->setTickerShowNumSingers(arg1);
}

void SettingsDialog::on_groupBoxTicker_toggled(bool arg1)
{
    settings->setTickerEnabled(arg1);
}

void SettingsDialog::on_lineEditUrl_editingFinished()
{
    settings->setRequestServerUrl(ui->lineEditUrl->text());
}

void SettingsDialog::on_checkBoxIgnoreCertErrors_toggled(bool checked)
{
    settings->setRequestServerIgnoreCertErrors(checked);
}

void SettingsDialog::on_lineEditUsername_editingFinished()
{
    settings->setRequestServerUsername(ui->lineEditUsername->text());
}

void SettingsDialog::on_lineEditPassword_editingFinished()
{
    settings->setRequestServerPassword(ui->lineEditPassword->text());
}

void SettingsDialog::on_groupBoxRequestServer_toggled(bool arg1)
{
    settings->setRequestServerEnabled(arg1);
}

void SettingsDialog::on_pushButtonBrowse_clicked()
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

void SettingsDialog::on_checkBoxFader_toggled(bool checked)
{
    settings->setAudioUseFader(checked);
    emit audioUseFaderChanged(checked);
}

void SettingsDialog::on_checkBoxSilenceDetection_toggled(bool checked)
{
    settings->setAudioDetectSilence(checked);
    emit audioSilenceDetectChanged(checked);
}

void SettingsDialog::on_checkBoxDownmix_toggled(bool checked)
{
    settings->setAudioDownmix(checked);
    emit audioDownmixChanged(checked);
}

void SettingsDialog::on_listWidgetAudioDevices_itemSelectionChanged()
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

void SettingsDialog::on_comboBoxBackend_currentIndexChanged(int index)
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

void SettingsDialog::audioBackendChanged(int index)
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
}
