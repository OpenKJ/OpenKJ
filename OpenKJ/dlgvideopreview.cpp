#include "dlgvideopreview.h"
#include "ui_dlgvideopreview.h"
#include <QDebug>
#include <QMessageBox>
#include "okarchive.h"
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

    if (!gst_is_initialized())
    {
        qInfo() << "VideoPreview - gst not initialized - initializing";
        gst_init(nullptr,nullptr);
    }

    if (m_mediaFilename.endsWith(".zip", Qt::CaseInsensitive))
    {
        m_cdgMode = true;
        OkArchive archive(m_mediaFilename);
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
        m_cdgMode = true;
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
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    if (!m_cdgMode)
    {
        gst_element_set_state (playBin, GST_STATE_NULL);
        gst_object_unref (playBin);
    }
}

void DlgVideoPreview::playCdg(const QString &filename)
{
    parser.open(filename);
    parser.process();
    pipeline = gst_pipeline_new("videoPreviewPl");
    auto appSrc = gst_element_factory_make("appsrc", "videoPreviewAS");
    g_object_set(G_OBJECT(appSrc), "caps",
                 gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB16", "width", G_TYPE_INT, 288, "height",
                                     G_TYPE_INT, 192, "framerate", GST_TYPE_FRACTION, 1, 30, NULL),
                 NULL);
    g_object_set(G_OBJECT(appSrc), "stream-type", 1, "format", GST_FORMAT_TIME, NULL);
    auto videoConvert = gst_element_factory_make("videoconvert", "videoPreviewVC");
#if defined(Q_OS_LINUX)
    videoSink = gst_element_factory_make("xvimagesink", "videoPreviewVS");
#elif defined(Q_OS_WIN)
    videoSink = gst_element_factory_make("d3dvideosink", "videoPreviewVS");
#elif defined(Q_OS_MAC)
    videoSink = gst_element_factory_make("glimagesink", "videoPreviewVS");
#else
    qWarning() << "Unknown platform, defaulting to OpenGL video output";
    videoSink = gst_element_factory_make("glimagesink", "videoPreviewVS");
#endif
    gst_bin_add_many(reinterpret_cast<GstBin *>(pipeline),appSrc,videoConvert,videoSink,NULL);
    gst_element_link_many(appSrc,videoConvert,videoSink,nullptr);
    g_signal_connect(appSrc, "need-data", G_CALLBACK(cb_need_data), this);
    g_signal_connect(appSrc, "seek-data", G_CALLBACK(cb_seek_data), this);
    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(videoSink), ui->videoDisplay->winId());
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void DlgVideoPreview::playVideo(const QString &filename)
{
    pipeline = gst_pipeline_new("videoPreviewPl");
    auto videoConvert = gst_element_factory_make("videoconvert", "videoPreviewVC");
#if defined(Q_OS_LINUX)
    videoSink = gst_element_factory_make("xvimagesink", "videoPreviewVS");
#elif defined(Q_OS_WIN)
    videoSink = gst_element_factory_make("d3dvideosink", "videoPreviewVS");
#elif defined(Q_OS_MAC)
    videoSink = gst_element_factory_make("glimagesink", "videoPreviewVS");
#else
    qWarning() << "Unknown platform, defaulting to OpenGL video output";
    videoSink = gst_element_factory_make("glimagesink", "videoPreviewVS");
#endif
    gst_bin_add_many(reinterpret_cast<GstBin *>(pipeline), videoConvert, videoSink, NULL);
    playBin = gst_element_factory_make("playbin", "videoPreviewPB");
    gst_element_link(videoConvert,videoSink);
    auto *pad = gst_element_get_static_pad(videoConvert, "sink");
    auto ghostPad = gst_ghost_pad_new("sink", pad);
    gst_pad_set_active(ghostPad, true);
    gst_element_add_pad(pipeline, ghostPad);
    gst_object_unref(pad);
    g_object_set(playBin, "audio-sink", gst_element_factory_make("fakesink", nullptr), nullptr);
    g_object_set(playBin, "video-sink", pipeline, nullptr);
    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(videoSink), ui->videoDisplay->winId());
    g_object_set(playBin, "mute", true, nullptr);
    auto uri = gst_filename_to_uri(filename.toLocal8Bit(), nullptr);
    g_object_set(playBin, "uri", uri, nullptr);
    g_free(uri);
    gst_element_set_state(playBin, GST_STATE_PLAYING);
}

void DlgVideoPreview::cb_seek_data([[maybe_unused]]GstElement *appsrc, guint64 position, gpointer user_data)
{
    auto dlg = reinterpret_cast<DlgVideoPreview *>(user_data);
    if (position == 0)
    {
        dlg->curFrame = 0;
        dlg->position = 0;
        return;
    }
    dlg->curFrame = position / 40000000;
    dlg->position = position;
}

void DlgVideoPreview::cb_need_data(GstElement *appsrc, [[maybe_unused]]guint unused_size, gpointer user_data)
{
    auto dlg = reinterpret_cast<DlgVideoPreview *>(user_data);
    auto buffer = gst_buffer_new_and_alloc(dlg->parser.videoFrameByIndex(dlg->curFrame).sizeInBytes());
    gst_buffer_fill(buffer,
                    0,
                    dlg->parser.videoFrameByIndex(dlg->curFrame).constBits(),
                    dlg->parser.videoFrameByTime(dlg->curFrame).sizeInBytes()
                    );
    GST_BUFFER_PTS(buffer) = dlg->position;
    GST_BUFFER_DURATION(buffer) = 40000000; // 40ms
    dlg->position += GST_BUFFER_DURATION(buffer);
    dlg->curFrame++;
    gst_app_src_push_buffer(reinterpret_cast<GstAppSrc *>(appsrc), buffer);
}
