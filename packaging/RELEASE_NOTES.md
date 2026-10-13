# OpenKJ 2.2.2 — Apple Silicon Pre-release

Experimental native ARM64 build for Apple Silicon Macs, using Qt 6.

**This unofficial prerelease is ad-hoc signed. It is not Developer ID signed or notarized by Apple.**

## Changes in 2.2.2

- Background song-duration analysis now selects songs and saves results on a worker thread, using small batches to keep the interface responsive.
- Database searches run asynchronously. Progressive typing replaces obsolete searches with the latest query.
- Indexed song lookups eliminate full-library scans while scrolling singer history and updating durations.
- Removed the duplicate library refresh after imports and added timing diagnostics.
- Failed ZIP duration checks are attempted once per scan, with detailed warnings limited to five examples and remaining failures summarized.
- Regular singer exports include source-directory paths, with import path mapping for transferring libraries between machines.
- CDG preview playback and preview audio muting fixes.

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
- Initial library loading and explicit sorting still run on the UI thread and can pause it with large libraries.
- Additional media formats have not all been tested.
- Recording music volume and incremental window-resizing behavior remain known issues.
- Back up your existing database before trying a prerelease.

## Opening on macOS

If macOS blocks the downloaded app, attempt to open it, then use **System Settings → Privacy & Security → Open Anyway** if you trust this build. Do not disable Gatekeeper system-wide.

## Source and feedback

OpenKJ source and its GPL license are available in [this repository](https://github.com/EquinoxEarth/OpenKJ). Bundled third-party components retain their respective licenses.

Report reproducible bugs through GitHub Issues, including the Mac model, macOS version, reproduction steps, and relevant logs.
