# OpenKJ 2.2.3 — Apple Silicon Pre-release

Experimental native ARM64 build for Apple Silicon Macs, using Qt 6.

**This unofficial prerelease is ad-hoc signed. It is not Developer ID signed or notarized by Apple.**

## Changes in 2.2.3

- Initial library loading and reloading run asynchronously, with indexed lookups for large libraries.
- Editing, renaming, or downloading individual songs updates the affected records without reloading the entire library.
- Fixed file renaming when configured source-directory paths have trailing separators or equivalent path forms.
- Song preparation, ZIP extraction, video copying, and playback startup move blocking work off the UI thread.
- Improved CDG/audio startup synchronization and prevented late CDG frames from covering the idle logo after stopping playback.
- Break music continues silently during karaoke playback and returns afterward, including when crossfade is enabled.
- Fixed autoplay advancement and added a status-bar countdown for the next song.
- Fixed rotation-duration totals to use available song durations.
- Audio recording initializes capture only when requested, avoiding unnecessary microphone access while recording is disabled.
- Added library-loading and background duration-analysis status, including the remaining queue count, on the right side of the status bar.

Responsiveness improvements were verified with a synthetic 450,000-song library and user testing with a similarly sized library during playback.

## Included

- MP3+G and MP4 karaoke playback
- Pitch and tempo adjustment through the bundled custom SoundTouch plugin
- Collapsible ten-band EQ with separate Karaoke and Break Music controls
- Singer rotation, queues, history, and regular singer transfer
- Bundled Qt, GStreamer, FFmpeg, and SoundTouch dependencies

## Compatibility and limitations

- Apple Silicon Macs only; no Intel, Windows, or Linux installer in this prerelease.
- Minimum supported macOS version has not yet been established.
- Sleeping external drives can still delay song loading while they spin up.
- Additional media formats have not all been tested.
- Recording music volume and incremental window-resizing behavior remain known issues.
- Back up your existing database before trying a prerelease.

## Opening on macOS

If macOS blocks the downloaded app, attempt to open it, then use **System Settings → Privacy & Security → Open Anyway** if you trust this build. Do not disable Gatekeeper system-wide.

## Source and feedback

OpenKJ source and its GPL license are available in [this repository](https://github.com/EquinoxEarth/OpenKJ). Bundled third-party components retain their respective licenses.

Report reproducible bugs through GitHub Issues, including the Mac model, macOS version, reproduction steps, and relevant logs.
