# OpenKJ 2.2.3 — Windows x64 preview

Experimental, unsigned Windows x64 build using Qt 6 and GStreamer.

- Native Windows playback, full-library import and singer rotation have been
  tested by the maintainer.
- Correct Windows local-file URLs, including spaces and Unicode filenames.
- Audio format/rate conversion between the existing SoundTouch/EQ chain and
  native output devices.
- Portable ZIP with Qt, GStreamer, FFmpeg, SoundTouch and runtime DLLs;
  MSYS2 is not required on the destination computer.
- Isolated checks cover SQLite, ZIP I/O, file playback, DSP/output negotiation,
  CDG decoding and bundled plugin discovery without development SDK paths.

Extract the **entire ZIP**, then open `openkj.exe`. Keep all DLLs and plugin
folders together. This uses your normal OpenKJ settings/library; back up your
existing data before trying a preview. Windows may prompt because the build is
unsigned. No installer or automatic updater is included.

Automated tests use offscreen windows and fake audio/video outputs; they do not
certify every audio device, microphone or multi-monitor configuration. Please
report playback, display, recording or library issues with the application log.

The archive includes dependency versions, license notices and a file manifest.
Source and build scripts: [EquinoxEarth/OpenKJ](https://github.com/EquinoxEarth/OpenKJ).
