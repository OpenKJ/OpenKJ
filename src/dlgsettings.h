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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QKeySequenceEdit>
#include <QList>
#include <QNetworkAccessManager>
#include <QPushButton>
#include "settings.h"
#include "mediabackend.h"
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
    MediaBackend &kAudioBackend;
    MediaBackend &bmAudioBackend;
    QNetworkAccessManager *networkManager;
    bool m_pageSetupDone;
    QStringList audioOutputDevices;
    void setupHotkeysForm();
    struct KeyboardShortcut
    {
        QString description;
        QString sequenceName;
    };
    OKJSongbookAPI &songbookApi;

public:
    explicit DlgSettings(MediaBackend &AudioBackend, MediaBackend &BmAudioBackend, OKJSongbookAPI &songbookAPI,
                         QWidget *parent = nullptr);
    ~DlgSettings() override;

signals:
    void audioUseFaderChanged(bool);
    void audioUseFaderChangedBm(bool);
    void audioSilenceDetectChanged(bool);
    void audioSilenceDetectChangedBm(bool);
    void audioDownmixChanged(bool);
    void audioDownmixChangedBm(bool);
    void applicationFontChanged(const QFont &font);

    void alertTxtColorChanged(QColor color);
    void alertBgColorChanged(QColor color);
    void bgModeChanged(Settings::BgMode mode);
    void bgSlideShowDirChanged(QString dir);
    void cdgBgImageChanged();
    void cdgOffsetsChanged();
    void cdgRemainBgColorChanged(QColor color);
    void cdgRemainEnabledChanged(bool enabled);
    void cdgRemainFontChanged(QFont font);
    void cdgRemainTextColorChanged(QColor color);
    void durationPositionReset();
    void karaokeAAAlertFontChanged(QFont font);
    void slideShowIntervalChanged(uint secs);
    void tickerBgColorChanged();
    void tickerEnableChanged();
    void tickerFontChanged();
    void tickerSpeedChanged();
    void tickerTextColorChanged();
    void tickerCustomStringChanged();
    void tickerOutputModeChanged();
    void shortcutsChanged();

private slots:
    void on_btnClose_clicked();
    void on_pushButtonFont_clicked();
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
    void on_checkBoxShowKAAAlert_toggled(bool checked);
    void on_checkBoxKAA_toggled(bool checked);
    void on_spinBoxAADelay_valueChanged(int arg1);
    void on_btnAlertFont_clicked();
    void on_btnAlertTxtColor_clicked();
    void on_btnAlertBgColor_clicked();
    void on_cbxBmAutostart_clicked(bool checked);
    void on_spinBoxInterval_valueChanged(int arg1);
    void on_cbxTheme_currentIndexChanged(int index);
    void on_btnBrowse_clicked();
    void on_fontComboBox_currentFontChanged(const QFont &f);
    void on_spinBoxAppFontSize_valueChanged(int arg1);
    void on_btnTestReqServer_clicked();
    void reqSvrTestError(QString error);
    void reqSvrTestSslError(QString error);
    void reqSvrTestPassed();
    void on_checkBoxIncludeEmptySingers_clicked(bool checked);
    void on_spinBoxDefaultPadTime_valueChanged(int arg1);
    void on_spinBoxDefaultSongDuration_valueChanged(int arg1);
    void on_checkBoxDisplayCurrentRotationPosition_clicked(bool checked);
    void entitledSystemCountChanged(int count);
    void on_groupBoxShowDuration_clicked(bool checked);
    void on_btnDurationFont_clicked();
    void on_btnDurationFontColor_clicked();
    void on_btnDurationBgColor_clicked();
    void on_btnLogDirBrowse_clicked();
    void on_checkBoxProgressiveSearch_toggled(bool checked);
    void on_cbxPreviewEnabled_toggled(bool checked);
    void on_comboBoxKAudioDevices_currentIndexChanged(int index);
    void on_comboBoxBAudioDevices_currentIndexChanged(int index);
    void on_checkBoxEnforceAspectRatio_clicked(bool checked);
    void on_pushButtonApplyTickerMsg_clicked();
    void on_pushButtonResetDurationPos_clicked();
    void on_lineEditTickerMessage_returnPressed();
    void on_checkBoxHardwareAccel_toggled(bool checked);
    void on_checkBoxCdgPrescaling_stateChanged(int arg1);
    void on_checkBoxCurrentSingerTop_toggled(bool checked);
    void keySequenceEditChanged(QKeySequence sequence);
    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;

    // QDialog interface
public slots:
    void done(int) override;
};

#endif // SETTINGSDIALOG_H
