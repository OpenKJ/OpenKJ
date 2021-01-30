#include "dlgvideopreview.h"
#include "ui_dlgvideopreview.h"
#include <QDebug>
#include <QMessageBox>
#include "mzarchive.h"
#include "okjutil.h"
#include <gst/video/videooverlay.h>
#include <gst/app/gstappsrc.h>

DlgVideoPreview::DlgVideoPreview(const QString &mediaFilePath, QWidget *parent) :
    QDialog(parent), ui(new Ui::DlgVideoPreview), m_mediaFilename(mediaFilePath)
{
    ui->setupUi(this);
    connect(ui->pushButtonClose, &QPushButton::clicked, [&] () {
       close();
       deleteLater();
    });

    //m_mediaBackend.setVolume(0);
    m_mediaBackend.setHWVideoOutputDevices({ui->videoDisplay->winId()});
    // todo: should we set silence detection off?

    if (m_mediaFilename.endsWith(".zip", Qt::CaseInsensitive))
    {
        MzArchive archive(m_mediaFilename);
        if ((archive.checkCDG()) && (archive.checkAudio()))
        {
            if (archive.checkAudio())
            {
                if (!archive.extractAudio(tmpDir.path(), "tmp" + archive.audioExtension()))
                {
                    // We're not gonna use it, but we extract it just to catch bad files during preview
                    QMessageBox::warning(this, tr("Bad karaoke file"), tr("Failed to extract audio file."),QMessageBox::Ok);
                    return;
                }
                if (!archive.extractCdg(tmpDir.path(), "tmp.cdg"))
                {
                    QMessageBox::warning(this, tr("Bad karaoke file"), tr("Failed to extract CDG file."),QMessageBox::Ok);
                    return;
                }
                playCdg(tmpDir.path() + QDir::separator() + "tmp.cdg");
                //m_mediaBackend.setMediaCdg(tmpDir.path() + QDir::separator() + "tmp.cdg", tmpDir.path() + QDir::separator() + "tmp" + archive.audioExtension());
                //m_mediaBackend.play();
            }
        }
        else
        {
            QMessageBox::warning(this, tr("Bad karaoke file"),tr("Zip file does not contain a valid karaoke track.  CDG or audio file missing or corrupt."),QMessageBox::Ok);
            return;
        }
    }
    else if (m_mediaFilename.endsWith(".cdg", Qt::CaseInsensitive))
    {
        QString cdgTmpFile = "tmp.cdg";
        QFile cdgFile(m_mediaFilename);
        if (!cdgFile.exists())
        {
            QMessageBox::warning(this, tr("Bad karaoke file"), tr("CDG file missing."),QMessageBox::Ok);
            return;
        }
        else if (cdgFile.size() == 0)
        {
            QMessageBox::warning(this, tr("Bad karaoke file"), tr("CDG file contains no data"),QMessageBox::Ok);
            return;
        }
        QString audiofn = findMatchingAudioFile(m_mediaFilename);
        if (audiofn == "")
        {
            QMessageBox::warning(this, tr("Bad karaoke file"), tr("Audio file missing."),QMessageBox::Ok);
            return;
        }
        QFile audioFile(audiofn);
        if (audioFile.size() == 0)
        {
            QMessageBox::warning(this, tr("Bad karaoke file"), tr("Audio file contains no data"),QMessageBox::Ok);
            return;
        }
        cdgFile.copy(tmpDir.path() + QDir::separator() + "tmp.cdg");
        playCdg(tmpDir.path() + QDir::separator() + "tmp.cdg");

    }
    else
    {
        playVideo(m_mediaFilename);
    }
}

DlgVideoPreview::~DlgVideoPreview()
{
    delete ui;
}

void DlgVideoPreview::playCdg(const QString &filename)
{
    m_mediaBackend.setMediaCdg(filename, nullptr);
    m_mediaBackend.play();
}

void DlgVideoPreview::playVideo(const QString &filename)
{
    m_mediaBackend.setMedia(filename);
    m_mediaBackend.play();

}

