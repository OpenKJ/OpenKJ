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

#include "dlgcdgpreview.h"
#include "ui_dlgcdgpreview.h"
#include <QMessageBox>
#include <QDebug>
#include "okarchive.h"

DlgCdgPreview::DlgCdgPreview(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgCdgPreview)
{
    ui->setupUi(this);
    cdg = new CDG();
    timer = new QTimer(this);
    timer->stop();
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    cdgPosition = 0;
    cdgTempDir = NULL;
}

DlgCdgPreview::~DlgCdgPreview()
{
    delete ui;
    delete cdg;
}

void DlgCdgPreview::setSourceFile(QString srcFile)
{
    m_srcFile = srcFile;
}

void DlgCdgPreview::preview()
{
    timer->stop();
    cdgPosition = 0;
    if (cdg->isOpen()) cdg->reset();
    setVisible(true);
    QApplication::processEvents();
    if (m_srcFile.endsWith(".zip", Qt::CaseInsensitive))
    {
        OkArchive archive(m_srcFile);
        if ((!archive.checkCDG()) || (!archive.checkAudio()))
        {
            QMessageBox::warning(this, tr("Bad karaoke file"),tr("Zip file does not contain a valid karaoke track.  CDG or mp3 file missing or corrupt."),QMessageBox::Ok);
            close();
            return;
        }
        cdg->open(archive.getCDGData());
        cdg->process();
        timer->start(40);
    }
    else if (m_srcFile.endsWith(".cdg", Qt::CaseInsensitive))
    {
        QFile cdgFile(m_srcFile);
        if (cdgFile.size() == 0)
        {
            QMessageBox::warning(this, tr("Bad karaoke file"), tr("CDG file contains no data"),QMessageBox::Ok);
            close();
            return;
        }
        cdg->open(m_srcFile);
        cdg->process();
        timer->start(40);
    }
    else if (m_srcFile.endsWith(".mkv", Qt::CaseInsensitive) || m_srcFile.endsWith(".avi", Qt::CaseInsensitive) || m_srcFile.endsWith(".wmv", Qt::CaseInsensitive) || m_srcFile.endsWith(".mp4", Qt::CaseInsensitive) || m_srcFile.endsWith(".mpg", Qt::CaseInsensitive) || m_srcFile.endsWith(".mpeg", Qt::CaseInsensitive))
    {
        AudioBackendGstreamer *mediaBackend = new AudioBackendGstreamer(false, this, "Preview");
        connect(mediaBackend, SIGNAL(newVideoFrame(QImage, QString)), this, SLOT(videoFrameReceived(QImage, QString)));
        mediaBackend->setVolume(0);
        mediaBackend->setMedia(m_srcFile);
        mediaBackend->play();
    }

}


void DlgCdgPreview::timerTimeout()
{
    if (cdg->isOpen())
    {
        if (cdg->lastCDGUpdate() >= cdgPosition)
        {
            ui->cdgVideoWidget->videoSurface()->present(cdg->videoFrameByTime(cdgPosition));
            cdgPosition = cdgPosition + timer->interval();
        }
        else
        {
            timer->stop();
            cdg->reset();
            cdgPosition = 0;
            close();
        }
    }
}



void DlgCdgPreview::on_pushButtonClose_clicked()
{
    timer->stop();
    cdg->reset();
    cdgPosition = 0;
    close();
    deleteLater();
}

void DlgCdgPreview::videoFrameReceived(QImage frame, QString backendName)
{
    Q_UNUSED(backendName)
    ui->cdgVideoWidget->videoSurface()->present(QVideoFrame(frame));
}


void DlgCdgPreview::closeEvent(QCloseEvent *)
{
    timer->stop();
    cdg->reset();
    cdgPosition = 0;
    close();
    deleteLater();
}


void DlgCdgPreview::reject()
{
    close();
}
