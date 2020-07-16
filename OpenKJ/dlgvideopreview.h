#ifndef DLGVIDEOPREVIEW_H
#define DLGVIDEOPREVIEW_H

#include <QDialog>
#include <QTemporaryDir>
#include <gst/gst.h>
#include <libCDG/include/libCDG.h>

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
    bool m_cdgMode{false};
    QString m_mediaFilename;
    QTemporaryDir tmpDir;
    GstElement *pipeline;
    GstElement *playBin;
    GstElement *videoSink;
    CdgParser parser;
    guint64 position{0};
    unsigned int curFrame{0};
    static void cb_need_data(GstElement *appsrc, guint unused_size, gpointer user_data);
    static void cb_seek_data(GstElement *appsrc, guint64 position, gpointer user_data);
    void playCdg(const QString &filename);
    void playVideo(const QString &filename);
};

#endif // DLGVIDEOPREVIEW_H
