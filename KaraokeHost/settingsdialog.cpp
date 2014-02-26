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

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    createIcons();
    QStringList screens = getMonitors();
    ui->listWidgetMonitors->addItems(screens);
    settings = new KhSettings(this);
    ui->checkBoxShowCdgWindow->setChecked(settings->showCdgWindow());
    ui->groupBoxMonitors->setChecked(settings->cdgWindowFullscreen());
    ui->listWidgetMonitors->setEnabled(settings->showCdgWindow());
    ui->groupBoxMonitors->setEnabled(settings->showCdgWindow());
    ui->listWidgetMonitors->item(settings->cdgWindowFullScreenMonitor())->setSelected(true);
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
