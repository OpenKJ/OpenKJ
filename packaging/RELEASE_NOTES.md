# OpenKJ 2.1.39 — macOS Apple Silicon Preview

This is an experimental native Apple Silicon build of OpenKJ, updated to work with modern macOS and Qt 6.

**This is an unofficial development preview and is not yet signed with an Apple Developer ID or notarized by Apple.**

## What's included

- Native ARM64 application for Apple Silicon Macs
- Updated Qt 6 interface
- Bundled GStreamer and FFmpeg multimedia libraries
- MP3+G and MP4 karaoke playback
- Pitch and tempo adjustment
- Fixed 10-band audio equalizer
- Microphone recording

### macOS security warning

This preview has not been notarized by Apple. macOS may prevent the application from opening automatically.

If you trust the downloaded build, attempt to open it, then go to **System Settings → Privacy & Security → Open Anyway** and follow the prompts.

Do not disable Gatekeeper system-wide.

## Compatibility

- Apple Silicon Macs (ARM64)
- Tested successfully on two Apple Silicon systems running MacOS 27.0.1
- Minimum supported macOS version has not yet been established

## Known limitations

- Not Developer ID signed or notarized
- Legacy databases containing large numbers of missing media files may cause lengthy startup delays
- **Intel Mac compatibility is not included in this build**
- Additional media formats have not all been tested
- When using the built in recording, the music volume is extremely low
- Resizing the window can misbehave. It will let you resize small amounts at a time.

## Feedback

Please report reproducible bugs through GitHub Issues, including your macOS version, Mac model, reproduction steps, and relevant application logs.