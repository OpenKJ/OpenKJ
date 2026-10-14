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
             "audiopanorama", "volume", "autoaudiosink", "appsink", "cdgparse",
             "cdgdec", "qtdemux", "avdec_h264"}) {
        auto *factory = gst_element_factory_find(name);
        if (!factory) {
            qCritical() << "Missing required GStreamer element:" << name;
            return 1;
        }
        gst_object_unref(factory);
    }
    stage("process SoundTouch and EQ audio");
    if (!runPipeline("audiotestsrc num-buffers=12 ! audioconvert ! pitch pitch=1.1 ! "
                     "scaletempo ! equalizer-10bands band0=3 ! audiopanorama ! volume ! fakesink"))
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
    qInfo() << "PASS: Qt GUI, ZIP round trip, SQLite, required media plugins, SoundTouch/EQ audio, and CDG decoding";
    return 0;
}
