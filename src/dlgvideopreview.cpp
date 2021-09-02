#include "dlgvideopreview.h"
#include "ui_dlgvideopreview.h"
#include <QMessageBox>
#include <utility>
#include "mzarchive.h"
#include "okjutil.h"


DlgVideoPreview::DlgVideoPreview(QString mediaFilePath, QWidget *parent) :
        QDialog(parent), ui(new Ui::DlgVideoPreview), m_mediaFilename(std::move(mediaFilePath)) {
    m_logger = spdlog::get("logger");
    m_logger->trace("{} Constructor called with media path: {}", m_loggingPrefix, m_mediaFilename);

    ui->setupUi(this);

    connect(ui->pushButtonClose, &QPushButton::clicked, [&]() {
        close();
    });
    if (!QFile(m_mediaFilename).exists()) {
        m_logger->warn("{} Bad karaoke file - file missing - {}", m_loggingPrefix, m_mediaFilename);
        QMessageBox::warning(nullptr, tr("Bad karaoke file"), tr("File missing."), QMessageBox::Ok);
        QTimer::singleShot(250, [&] () { if (ui) close(); });
        return;
    }
    m_mediaBackend.setVideoOutputWidgets({ui->videoDisplay});
    m_mediaBackend.setUseSilenceDetection(false);
    if (m_mediaFilename.endsWith(".zip", Qt::CaseInsensitive)) {
        MzArchive archive(m_mediaFilename);
        if ((archive.checkCDG()) && (archive.checkAudio())) {
            if (archive.checkAudio()) {
                if (!archive.extractAudio(m_tmpDir.path(), "tmp" + archive.audioExtension())) {
                    // We're not going to use it, but we extract it just to catch bad files during preview
                    m_logger->warn("{} Bad karaoke file - Failed to extract audio file from archive: {}",
                                   m_loggingPrefix, m_mediaFilename);
                    QMessageBox::warning(nullptr, tr("Bad karaoke file"), tr("Failed to extract audio file."),
                                         QMessageBox::Ok);
                    QTimer::singleShot(250, [&] () { if (ui) close(); });
                    return;
                }
                if (!archive.extractCdg(m_tmpDir.path(), "tmp.cdg")) {
                    m_logger->warn("{} Bad karaoke file - Failed to extract CDG file from archive: {}", m_loggingPrefix,
                                   m_mediaFilename);
                    QMessageBox::warning(nullptr, tr("Bad karaoke file"), tr("Failed to extract CDG file."),
                                         QMessageBox::Ok);
                    QTimer::singleShot(250, [&] () { if (ui) close(); });
                    return;
                }
                m_logger->info("{} Decompression successful - starting preview playback of: {}", m_loggingPrefix,
                               m_mediaFilename);
                playCdg(m_tmpDir.path() + QDir::separator() + "tmp.cdg");
            }
        } else {
            QMessageBox::warning(nullptr, tr("Bad karaoke file"),
                                 tr("Zip file does not contain a valid karaoke track.  CDG or audio file missing or corrupt."),
                                 QMessageBox::Ok);
            QTimer::singleShot(250, [&] () { if (ui) close(); });
            return;
        }
    } else if (m_mediaFilename.endsWith(".cdg", Qt::CaseInsensitive)) {
        QFile cdgFile(m_mediaFilename);
        if (cdgFile.size() == 0) {
            m_logger->warn("{} Bad karaoke file - CDG file contains no data - {}", m_loggingPrefix, m_mediaFilename);
            QMessageBox::warning(nullptr, tr("Bad karaoke file"), tr("CDG file contains no data"), QMessageBox::Ok);
            QTimer::singleShot(250, [&] () { if (ui) close(); });
            return;
        }
        QString audioFilePath = findMatchingAudioFile(m_mediaFilename);
        if (audioFilePath == "") {
            m_logger->warn("{} Bad karaoke file - No matching audio file found - {}", m_loggingPrefix, m_mediaFilename);
            QMessageBox::warning(nullptr, tr("Bad karaoke file"), tr("Audio file missing."), QMessageBox::Ok);
            QTimer::singleShot(250, [&] () { if (ui) close(); });
            return;
        }
        QFile audioFile(audioFilePath);
        if (audioFile.size() == 0) {
            m_logger->warn("{} Bad karaoke file - Audio file contains no data - {}", m_loggingPrefix, m_mediaFilename);
            QMessageBox::warning(nullptr, tr("Bad karaoke file"), tr("Audio file contains no data"), QMessageBox::Ok);
            QTimer::singleShot(250, [&] () { if (ui) close(); });
            return;
        }
        m_logger->info("{} Starting preview playback of media file: {}", m_loggingPrefix, m_mediaFilename);
        cdgFile.copy(m_tmpDir.path() + QDir::separator() + "tmp.cdg");
        playCdg(m_tmpDir.path() + QDir::separator() + "tmp.cdg");
    } else {
        m_logger->info("{} Starting preview playback of media file: {}", m_loggingPrefix, m_mediaFilename);
        playVideo(m_mediaFilename);
    }
}

DlgVideoPreview::~DlgVideoPreview() {
    m_logger->trace("{} Destructor called, stopping playback", m_loggingPrefix);
    m_mediaBackend.rawStop();
    while (m_mediaBackend.state() != MediaBackend::StoppedState)
        QApplication::processEvents();
    m_logger->trace("{} Playback stopped successfully", m_loggingPrefix);
}

void DlgVideoPreview::playCdg(const QString &filename) {
    m_mediaBackend.setMediaCdg(filename, nullptr);
    m_mediaBackend.play();
}

void DlgVideoPreview::playVideo(const QString &filename) {
    m_mediaBackend.setMedia(filename);
    m_mediaBackend.play();

}

void DlgVideoPreview::setPlaybackTimeLimit(int playSecs) {
    if (playSecs != 0)
        QTimer::singleShot(playSecs * 1000, [&]() {
            if (ui)
                close();
            else
                m_logger->warn("{} UI object already destroyed when time limit expired", m_loggingPrefix);
        });
}

