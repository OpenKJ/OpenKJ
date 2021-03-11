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
    explicit DlgVideoPreview(const QString &mediaFilePath, QWidget *parent = nullptr);
    ~DlgVideoPreview();

private:
    Ui::DlgVideoPreview *ui;
    MediaBackend m_mediaBackend { this, "PREVIEW", MediaBackend::VideoPreview };
    QString m_mediaFilename;
    QTemporaryDir tmpDir;
    guint64 position{0};
    void playCdg(const QString &filename);
    void playVideo(const QString &filename);
};

#endif // DLGVIDEOPREVIEW_H
