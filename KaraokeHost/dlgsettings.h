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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QList>
#include "khsettings.h"
#include "khabstractaudiobackend.h"

namespace Ui {
class DlgSettings;
}

class DlgSettings : public QDialog
{
    Q_OBJECT
    
public:
    explicit DlgSettings(KhAudioBackends *AudioBackends, QWidget *parent = 0);
    ~DlgSettings();
    
    void createIcons();
private slots:

    void on_btnClose_clicked();

    void on_checkBoxShowCdgWindow_stateChanged(int arg1);

    void on_groupBoxMonitors_toggled(bool arg1);

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

    void on_lineEditUsername_editingFinished();

    void on_lineEditPassword_editingFinished();

    void on_groupBoxRequestServer_toggled(bool arg1);

    void on_pushButtonBrowse_clicked();

    void on_checkBoxFader_toggled(bool checked);

    void on_checkBoxSilenceDetection_toggled(bool checked);

    void on_checkBoxDownmix_toggled(bool checked);

    void on_listWidgetAudioDevices_itemSelectionChanged();

    void on_comboBoxBackend_currentIndexChanged(int index);

    void audioBackendChanged(int index);

    void on_comboBoxDevice_currentIndexChanged(const QString &arg1);

    void on_comboBoxCodec_currentIndexChanged(const QString &arg1);

    void on_comboBoxContainer_currentIndexChanged(const QString &arg1);

    void on_groupBoxRecording_toggled(bool arg1);

signals:
    void showCdgWindowChanged(bool);
    void cdgWindowFullScreenChanged(bool);
    void cdgWindowFullScreenMonitorChanged(int);
    void audioUseFaderChanged(bool);
    void audioSilenceDetectChanged(bool);
    void audioDownmixChanged(bool);

private:
    Ui::DlgSettings *ui;
    QStringList getMonitors();
    KhAbstractAudioBackend *audioBackend;
    KhAudioBackends *audioBackends;
    bool pageSetupDone;
//    KhSettings *settings;
};

#endif // SETTINGSDIALOG_H
