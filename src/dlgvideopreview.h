#ifndef DLGVIDEOPREVIEW_H
#define DLGVIDEOPREVIEW_H

#include <QDialog>
#include <QTemporaryDir>
#include <mediabackend.h>
#include <gst/gst.h>

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
    Ui::DlgVideoPreview *ui;
    MediaBackend m_mediaBackend { this, "PREVIEW", MediaBackend::VideoPreview };
    QString m_mediaFilename;
    QTemporaryDir tmpDir;
    guint64 position{0};
    int m_playbackLimit{0};
    void playCdg(const QString &filename);
    void playVideo(const QString &filename);

};

#endif // DLGVIDEOPREVIEW_H
