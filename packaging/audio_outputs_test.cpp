#include "mediabackend.h"
#include <QApplication>
#include <QStandardPaths>
#include <cstdio>
#include <stdexcept>

std::ostream &operator<<(std::ostream &stream, const QString &text)
{
    return stream << text.toStdString();
}

// Virtual devices exercise routing without opening the host's audio hardware.
struct TestAudioDevice { GstDevice parent; };
struct TestAudioDeviceClass { GstDeviceClass parent; };
G_DEFINE_TYPE(TestAudioDevice, test_audio_device, GST_TYPE_DEVICE)
static QStringList created;
static GstElement *createOutput(GstDevice *device, const gchar *name)
{
    auto *displayName = gst_device_get_display_name(device);
    created.append(QString::fromUtf8(displayName));
    g_free(displayName);
    return gst_element_factory_make("fakesink", name);
}
static void test_audio_device_class_init(TestAudioDeviceClass *klass)
{
    GST_DEVICE_CLASS(klass)->create_element = createOutput;
}
static void test_audio_device_init(TestAudioDevice *) {}

struct TestAudioProvider { GstDeviceProvider parent; };
struct TestAudioProviderClass { GstDeviceProviderClass parent; };
G_DEFINE_TYPE(TestAudioProvider, test_audio_provider, GST_TYPE_DEVICE_PROVIDER)
static bool connected = true;
static GList *probeOutputs(GstDeviceProvider *)
{
    GList *devices = nullptr;
    auto *caps = gst_caps_from_string("audio/x-raw");
    if (connected) {
        for (const auto *name : {"Test Speakers", "Test USB"}) {
            auto *device = GST_DEVICE(g_object_new(test_audio_device_get_type(),
                "display-name", name, "device-class", "Audio/Sink", "caps", caps, nullptr));
            devices = g_list_append(devices, gst_object_ref_sink(device));
        }
    }
    auto *input = GST_DEVICE(g_object_new(test_audio_device_get_type(),
        "display-name", "Test Microphone", "device-class", "Audio/Source", "caps", caps, nullptr));
    gst_caps_unref(caps);
    return g_list_append(devices, gst_object_ref_sink(input));
}
static void test_audio_provider_class_init(TestAudioProviderClass *klass)
{
    auto *provider = GST_DEVICE_PROVIDER_CLASS(klass);
    provider->probe = probeOutputs;
    gst_device_provider_class_set_static_metadata(provider, "Test outputs", "Audio/Sink/Source",
                                                   "Isolated output routing test", "OpenKJ");
}
static void test_audio_provider_init(TestAudioProvider *) {}
static void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    gst_init(nullptr, nullptr);
    if (app.arguments().contains("--list-native")) {
        MediaBackend backend(nullptr, "Output probe", MediaBackend::SFX);
        for (const auto &name : backend.getOutputDevices()) {
            std::puts(name.toUtf8().constData());
            std::fflush(stdout);
        }
        return 0;
    }
    // Disable native providers; only the virtual devices should be probed.
    auto *providers = gst_device_provider_factory_list_get_device_providers(GST_RANK_NONE);
    for (auto *entry = providers; entry; entry = entry->next)
        gst_plugin_feature_set_rank(GST_PLUGIN_FEATURE(entry->data), GST_RANK_NONE);
    gst_plugin_feature_list_free(providers);
    gst_device_provider_register(nullptr, "testaudio", GST_RANK_PRIMARY,
                                 test_audio_provider_get_type());
    try {
        Settings settings;
        settings.setAudioOutputDevice("Default");
        settings.setAudioOutputDeviceBm("Default");
        settings.setAudioOutputBmFollowsKaraoke(false);
        MediaBackend karaoke(nullptr, "Test karaoke", MediaBackend::Karaoke);
        MediaBackend breaks(nullptr, "Test breaks", MediaBackend::BackgroundMusic);
        require(karaoke.getOutputDevices() == QStringList{"Default", "Test Speakers", "Test USB"},
                "Outputs must include real choices and exclude inputs");
        karaoke.setAudioOutputDevice("Test Speakers");
        breaks.setAudioOutputDevice("Test USB");
        require(created == QStringList{"Test Speakers", "Test USB"},
                "Each backend must create its own selected device sink");
        karaoke.play();
        karaoke.setAudioOutputDevice("Test USB");
        require(created.size() == 2, "Selection must not replace a running sink");
        karaoke.rawStop();
        karaoke.play();
        require(created.size() == 3 && created.last() == "Test USB",
                "Pending selection must apply on restart");
        karaoke.rawStop();
        karaoke.setAudioOutputDevice("Default");
        karaoke.setAudioOutputDevice("Test Speakers");
        require(created.size() == 4, "Returning from Default must allow explicit selection");
        connected = false;
        require(karaoke.getOutputDevices() == QStringList{"Default"},
                "Reopening Settings must refresh disconnected devices");
        karaoke.setAudioOutputDevice("Unavailable output");
        require(created.size() == 4, "Missing outputs must fall back to Default");
        connected = true;
        settings.setAudioOutputDevice("Test USB");
        MediaBackend restored(nullptr, "Test restored", MediaBackend::Karaoke);
        require(created.size() == 5 && created.last() == "Test USB",
                "Startup must apply the saved output choice");
        settings.setAudioOutputDeviceBm("Test Speakers");
        settings.setAudioOutputBmFollowsKaraoke(true);
        Settings savedSettings;
        require(savedSettings.audioOutputBmFollowsKaraoke(), "Follow preference must persist");
        MediaBackend following(nullptr, "Test following", MediaBackend::BackgroundMusic);
        require(created.size() == 6 && created.last() == "Test USB",
                "Break Music must start on the karaoke output when following");
        settings.setAudioOutputDevice("Default");
        require(settings.effectiveAudioOutputDeviceBm() == "Default",
                "Follow mode must reflect changes to the karaoke output");
        following.setAudioOutputDevice(settings.effectiveAudioOutputDeviceBm());
        require(settings.audioOutputDeviceBm() == "Test Speakers",
                "Following must preserve the independent Break Music choice");
        settings.setAudioOutputBmFollowsKaraoke(false);
        following.setAudioOutputDevice(settings.effectiveAudioOutputDeviceBm());
        require(created.size() == 7 && created.last() == "Test Speakers",
                "Disabling follow must restore the independent output");
        std::puts("Audio output routing checks passed");
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
