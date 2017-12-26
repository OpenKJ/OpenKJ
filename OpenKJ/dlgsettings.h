/*
 * Copyright (c) 2013-2017 Thomas Isaac Lightburn
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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QList>
#include <QNetworkAccessManager>
#include "settings.h"
#include "abstractaudiobackend.h"
#include "okjsongbookapi.h"

namespace Ui {
class DlgSettings;
}

class DlgSettings : public QDialog
{
    Q_OBJECT

private:
    Ui::DlgSettings *ui;
    QStringList getMonitors();
    AbstractAudioBackend *kAudioBackend;
    AbstractAudioBackend *bmAudioBackend;
    QNetworkAccessManager *networkManager;
    bool pageSetupDone;
    QStringList audioOutputDevices;

public:
    explicit DlgSettings(AbstractAudioBackend *AudioBackend, AbstractAudioBackend *BmAudioBackend, QWidget *parent = 0);
    ~DlgSettings();
    void createIcons();

signals:
    void audioUseFaderChanged(bool);
    void audioUseFaderChangedBm(bool);
    void audioSilenceDetectChanged(bool);
    void audioSilenceDetectChangedBm(bool);
    void audioDownmixChanged(bool);
    void audioDownmixChangedBm(bool);

private slots:
    void on_btnClose_clicked();
    void on_checkBoxShowCdgWindow_stateChanged(int arg1);
    void on_listWidgetMonitors_itemSelectionChanged();
    void on_pushButtonFont_clicked();
    void on_spinBoxTickerHeight_valueChanged(int arg1);
    void on_horizontalSliderTickerSpeed_valueChanged(int value);
    void on_pushButtonTextColor_clicked();
    void on_pushButtonBgColor_clicked();
    void on_radioButtonFullRotation_toggled(bool checked);
    void on_spinBoxTickerSingers_valueChanged(int arg1);
    void on_groupBoxTicker_toggled(bool arg1);
    void on_lineEditUrl_editingFinished();
    void on_checkBoxIgnoreCertErrors_toggled(bool checked);
    void on_groupBoxRequestServer_toggled(bool arg1);
    void on_pushButtonBrowse_clicked();
    void on_checkBoxFader_toggled(bool checked);
    void on_checkBoxFaderBm_toggled(bool checked);
    void on_checkBoxSilenceDetection_toggled(bool checked);
    void on_checkBoxSilenceDetectionBm_toggled(bool checked);
    void on_checkBoxDownmix_toggled(bool checked);
    void on_checkBoxDownmixBm_toggled(bool checked);
    void on_listWidgetAudioDevices_itemSelectionChanged();
    void on_listWidgetAudioDevicesBm_itemSelectionChanged();
    void on_comboBoxDevice_currentIndexChanged(const QString &arg1);
    void on_comboBoxCodec_currentIndexChanged(const QString &arg1);
    void on_groupBoxRecording_toggled(bool arg1);
    void on_buttonBrowse_clicked();
    void onNetworkReply(QNetworkReply* reply);
    void onSslErrors(QNetworkReply * reply);

    void on_pushButtonClearBgImg_clicked();
    void on_pushButtonSlideshowBrowse_clicked();
    void on_rbSlideshow_toggled(bool checked);
    void on_rbBgImage_toggled(bool checked);
    void on_lineEditApiKey_editingFinished();
    void on_lineEditTickerMessage_textChanged(const QString &arg1);
    void on_checkBoxCdgFullscreen_toggled(bool checked);
    void on_checkBoxShowKAAAlert_toggled(bool checked);
    void on_checkBoxKAA_toggled(bool checked);
    void on_spinBoxAADelay_valueChanged(int arg1);
    void on_btnAlertFont_clicked();
    void on_btnAlertTxtColor_clicked();
    void on_btnAlertBgColor_clicked();
    void on_cbxBmAutostart_clicked(bool checked);
};

#endif // SETTINGSDIALOG_H
