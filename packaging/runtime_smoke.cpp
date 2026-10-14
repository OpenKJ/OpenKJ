// Isolated build-environment check: never opens the host's settings or library.
#include <QApplication>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryFile>
#include <QWidget>
#include <gst/gst.h>

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
    QApplication app(argc, argv);
    QWidget widget;
    widget.resize(320, 200);
    widget.show();
    app.processEvents();
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

    gst_init(nullptr, nullptr);
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
    if (!runPipeline("audiotestsrc num-buffers=12 ! audioconvert ! pitch pitch=1.1 ! "
                     "scaletempo ! equalizer-10bands band0=3 ! audiopanorama ! volume ! fakesink"))
        return 1;

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
    path.replace('\\', "\\\\");
    path.replace('"', "\\\"");
    if (!runPipeline("filesrc location=\"" + path +
                     "\" ! cdgparse ! cdgdec ! videoconvert ! fakesink sync=false"))
        return 1;
    qInfo() << "PASS: Qt GUI, SQLite, required media plugins, SoundTouch/EQ audio, and CDG decoding";
    return 0;
}
