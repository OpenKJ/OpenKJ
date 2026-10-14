// Isolated build-environment check: never opens the host's settings or library.
#include <QApplication>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QWidget>
#include <gst/gst.h>
#include <zip.h>
#include <cstdio>
#include <QDataStream>
#include <QFile>
#include "localmediauri.h"

static void stage(const char *message)
{
    std::fprintf(stderr, "Runtime check: %s\n", message);
    std::fflush(stderr);
}

static bool checkZip()
{
    QTemporaryDir directory;
    if (!directory.isValid())
        return false;
    const auto path = directory.filePath(QString::fromUtf8("ZIP test café.zip")).toUtf8();
    int error = 0;
    auto *archive = zip_open(path.constData(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (!archive)
        return false;
    const QByteArray payload("OpenKJ ZIP round trip");
    auto *source = zip_source_buffer(archive, payload.constData(), payload.size(), 0);
    if (!source || zip_file_add(archive, "track.cdg", source, 0) < 0) {
        if (source)
            zip_source_free(source);
        zip_discard(archive);
        return false;
    }
    if (zip_close(archive) < 0) {
        zip_discard(archive);
        return false;
    }
    archive = zip_open(path.constData(), ZIP_RDONLY, &error);
    if (!archive)
        return false;
    auto *file = zip_fopen(archive, "track.cdg", 0);
    char buffer[64];
    const auto count = file ? zip_fread(file, buffer, sizeof(buffer)) : -1;
    const bool ok = count == payload.size() && QByteArray(buffer, count) == payload;
    if (file)
        zip_fclose(file);
    const bool closed = zip_close(archive) == 0;
    if (!closed)
        zip_discard(archive);
    return ok && closed;
}

static bool runPipeline(const QString &description)
{
    GError *error = nullptr;
    auto *pipeline = gst_parse_launch(description.toUtf8().constData(), &error);
    if (error || !pipeline) {
        qCritical() << "Pipeline creation failed:" << (error ? error->message : "no pipeline");
        g_clear_error(&error);
        if (pipeline)
            gst_object_unref(pipeline);
        return false;
    }
    auto *bus = gst_element_get_bus(pipeline);
    const auto state = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    auto *message = state == GST_STATE_CHANGE_FAILURE ? nullptr : gst_bus_timed_pop_filtered(
        bus, 10 * GST_SECOND, static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    const bool ok = message && GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS;
    if (message && GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
        gchar *debug = nullptr;
        gst_message_parse_error(message, &error, &debug);
        qCritical() << error->message << debug;
        g_free(debug);
        g_clear_error(&error);
    }
    if (!ok)
        qCritical() << "Pipeline did not reach EOS:" << description;
    if (message)
        gst_message_unref(message);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    return ok;
}

int main(int argc, char **argv)
{
    stage("initialize Qt GUI");
    qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &message) {
        std::fprintf(stderr, "%s\n", message.toUtf8().constData());
        std::fflush(stderr);
    });
    QApplication app(argc, argv);
    stage("ZIP creation and readback");
    if (!checkZip()) {
        qCritical() << "ZIP creation/readback failed";
        return 1;
    }
    stage("show offscreen widget");
    QWidget widget;
    widget.resize(320, 200);
    widget.show();
    app.processEvents();
    stage("SQLite plugin and queries");
    auto db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    if (!db.open()) {
        qCritical() << "SQLite plugin failed:" << db.lastError();
        return 1;
    }
    QSqlQuery query(db);
    if (!query.exec("CREATE TABLE songs (id INTEGER PRIMARY KEY, duration INTEGER)") ||
        !query.exec("INSERT INTO songs VALUES (1, 180000)") ||
        !query.exec("SELECT duration FROM songs WHERE id=1") || !query.next() ||
        query.value(0).toInt() != 180000) {
        qCritical() << "SQLite query failed:" << query.lastError();
        return 1;
    }

    stage("initialize GStreamer");
    gst_init(nullptr, nullptr);
    stage("find media elements");
    for (const auto *name : {"playbin", "pitch", "scaletempo", "equalizer-10bands",
             "audiopanorama", "volume", "audioconvert", "audioresample", "autoaudiosink", "appsink", "cdgparse",
             "cdgdec", "qtdemux", "avdec_h264"}) {
        auto *factory = gst_element_factory_find(name);
        if (!factory) {
            qCritical() << "Missing required GStreamer element:" << name;
            return 1;
        }
        gst_object_unref(factory);
    }

    stage("play a local file URI with spaces, Unicode and reserved characters");
    QTemporaryDir mediaDirectory;
    if (!mediaDirectory.isValid())
        return 1;
    const auto audioPath = mediaDirectory.filePath(QString::fromUtf8("song café #1 100%.wav"));
    QFile audioFile(audioPath);
    if (!audioFile.open(QIODevice::WriteOnly))
        return 1;
    // 100 ms of silent mono PCM, independent of encoders or external fixtures.
    constexpr quint32 sampleRate = 8000;
    const QByteArray silence(sampleRate / 10 * 2, '\0');
    QDataStream wav(&audioFile);
    wav.setByteOrder(QDataStream::LittleEndian);
    wav.writeRawData("RIFF", 4);
    wav << quint32(36 + silence.size());
    wav.writeRawData("WAVEfmt ", 8);
    wav << quint32(16) << quint16(1) << quint16(1) << sampleRate
        << quint32(sampleRate * 2) << quint16(2) << quint16(16);
    wav.writeRawData("data", 4);
    wav << quint32(silence.size());
    wav.writeRawData(silence.constData(), silence.size());
    if (wav.status() != QDataStream::Ok || !audioFile.flush())
        return 1;
    audioFile.close();
    const auto uri = localMediaUri(audioPath);
    const auto url = QUrl::fromEncoded(uri);
    if (!url.isLocalFile() || url.toLocalFile() != audioPath ||
        !uri.contains("%20") || !uri.contains("%23") || !uri.contains("%25")) {
        qCritical() << "Incorrect local media URI:" << uri;
        return 1;
    }
#ifdef Q_OS_WIN
    // A drive letter must be in the path, not parsed as a URI authority.
    if (audioPath.size() > 1 && audioPath[1] == ':' && !uri.startsWith("file:///"))
        return 1;
#endif
    if (!runPipeline("playbin uri=\"" + QString::fromUtf8(uri) +
                     "\" audio-sink=\"fakesink sync=false\" video-sink=fakesink"))
        return 1;
    stage("process SoundTouch and EQ audio");
    if (!runPipeline("audiotestsrc num-buffers=12 ! audioconvert ! pitch pitch=1.1 ! "
                     "scaletempo ! equalizer-10bands band0=3 ! audiopanorama ! volume ! fakesink"))
        return 1;

    stage("negotiate DSP output with a fixed-format audio device");
    // Unlike fakesink alone, these caps require real format/rate conversion
    // after SoundTouch/EQ, as a native output device may require on Windows.
    const QString fixedOutput =
        "pitch pitch=1.1 ! scaletempo ! equalizer-10bands band0=3 ! "
        "audiopanorama ! volume ! audioconvert ! audioresample ! "
        "audio/x-raw,format=S16LE,rate=48000,channels=2 ! fakesink sync=false";
    if (!runPipeline("audiotestsrc num-buffers=12 ! "
                     "audio/x-raw,format=F32LE,rate=44100,channels=2 ! " + fixedOutput))
        return 1;
    // Exercise playbin's negotiation into the same sink chain with the 8 kHz
    // mono WAV above, in addition to the stereo synthetic source.
    if (!runPipeline("playbin uri=\"" + QString::fromUtf8(uri) +
                     "\" audio-sink=\"" + fixedOutput + "\" video-sink=fakesink"))
        return 1;

    stage("decode CDG");
    QTemporaryFile cdg;
    if (!cdg.open())
        return 1;
    QByteArray packets(24 * 300, '\0');
    for (int i = 0; i < 300; ++i) {
        packets[i * 24] = 9; // CDG command
        packets[i * 24 + 1] = 1; // Memory preset
    }
    if (cdg.write(packets) != packets.size() || !cdg.flush())
        return 1;
    QString path = cdg.fileName();
    cdg.close(); // Release the writer before GStreamer opens the file on Windows.
    path.replace('\\', "\\\\");
    path.replace('"', "\\\"");
    if (!runPipeline("filesrc location=\"" + path +
                     "\" ! cdgparse ! cdgdec ! videoconvert ! fakesink sync=false"))
        return 1;
    qInfo() << "PASS: Qt GUI, ZIP round trip, SQLite, local-file URI playback, required media plugins, SoundTouch/EQ audio, output format negotiation, and CDG decoding";
    return 0;
}
