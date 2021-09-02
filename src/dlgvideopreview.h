#ifndef DLGVIDEOPREVIEW_H
#define DLGVIDEOPREVIEW_H

#include <QDialog>
#include <QTemporaryDir>
#include <mediabackend.h>
#include <gst/gst.h>
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>

std::ostream& operator<<(std::ostream& os, const QString& s);

namespace Ui {
class DlgVideoPreview;
}

class DlgVideoPreview : public QDialog
{
    Q_OBJECT

public:
    explicit DlgVideoPreview(QString mediaFilePath, QWidget *parent = nullptr);
    ~DlgVideoPreview() override;
    // Intended for use with torture testing modes.  Closes the dialog after playing back the specified number
    // of seconds of playback
    void setPlaybackTimeLimit(int playSecs);

private:
    std::string m_loggingPrefix{"[PreviewDialog]"};
    std::shared_ptr<spdlog::logger> m_logger;
    std::unique_ptr<Ui::DlgVideoPreview> ui;
    QTemporaryDir m_tmpDir;
    QString m_mediaFilename;
    MediaBackend m_mediaBackend { this, "PREVIEW", MediaBackend::VideoPreview };

    void playCdg(const QString &filename);
    void playVideo(const QString &filename);

};

#endif // DLGVIDEOPREVIEW_H
