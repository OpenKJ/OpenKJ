#ifndef DLGVIDEOPREVIEW_H
#define DLGVIDEOPREVIEW_H

#include <QDialog>
#include <QTemporaryDir>
#include <mediabackend.h>
#include <gst/gst.h>
//#include <libCDG/include/libCDG.h>
#include "libCDG/src/cdgfilereader.h"

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
    //bool m_cdgMode{false};
    QString m_mediaFilename;
    QTemporaryDir tmpDir;
    //GstElement *pipeline{nullptr};
    //GstElement *playBin{nullptr};
    //GstElement *videoSink{nullptr};
    //GstElement *appSrc{nullptr};
    //CdgParser parser;
    guint64 position{0};
    //unsigned int curFrame{0};
    //static void cb_need_data(GstElement *appsrc, guint unused_size, gpointer user_data);
    //static gboolean cb_seek_data(GstElement *appsrc, guint64 position, gpointer user_data);
    void playCdg(const QString &filename);
    void playVideo(const QString &filename);
};

#endif // DLGVIDEOPREVIEW_H
