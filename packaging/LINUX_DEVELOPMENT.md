# Fedora Qt 6 development

For an installable package on Aurora or another Flatpak desktop, use the
[Linux Flatpak preview](flatpak/README.md). That package includes the extra
media plugins and does not require host development packages or `env.sh`.

The first Linux target on `macos-qt6-test` is **Fedora 44 x86_64**. This uses
Fedora Qt 6, GStreamer and SoundTouch packages, preserving the same pitch, tempo,
EQ and playback pipelines as the Mac and Windows builds. No VLC SDK is needed.
Fedora 43 and other distributions are not covered by this workflow yet.

## Native setup

Use a normal user account on Fedora Workstation. For H.264/AAC support, enable
the [RPM Fusion Free repository](https://rpmfusion.org/Configuration) if it is
not already enabled:

```bash
sudo dnf install https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm
```

Fedora's `gstreamer1-plugin-libav` uses the distribution FFmpeg libraries. The
restricted codec selection may omit `avdec_h264`; OpenKJ checks for it rather
than allowing MP4 playback to fail later. Update the installed packages, then
install `libavcodec-freeworld` from RPM Fusion alongside Fedora's codec package:

```bash
sudo dnf upgrade --refresh
sudo dnf install libavcodec-freeworld
```

`libavcodec-freeworld` supplements Fedora's FFmpeg libraries; do not swap out
`libavcodec-free`, which other Fedora packages require at an exact version.
See the [RPM Fusion package definition](https://github.com/rpmfusion/ffmpeg/blob/f44/ffmpeg.spec).
Review DNF's proposed changes before accepting. If versions still cannot be
resolved after refreshing and updating, inspect the available package versions
before forcing removals or enabling testing repositories.

From this checkout:

```bash
bash packaging/setup_fedora.sh --install-deps
source build-linux-deps/env.sh
cmake --build build-linux-dev --parallel 4
ctest --test-dir build-linux-dev --output-on-failure
./build-linux-dev/openkj
```

Omit `--install-deps` on later runs when the packages are installed. The script
records exact RPM versions in `build-linux-deps/environment.txt`, verifies
SoundTouch `pitch`, the ten-band EQ, CDG and FFmpeg decoders, then configures
CMake. It never opens the application's settings or song database.

If CDG is missing, setup builds only `gst-plugin-cdg` from upstream
gst-plugins-rs 0.15.3, pinned at commit
`6302bea23b53e6461c104f9543df887a63b2d6ec`, using the upstream Cargo lockfile.
The plugin remains inside `build-linux-deps/plugins`; source `env.sh` before
running OpenKJ or the tests. This needs network access and the Fedora Rust/Cargo
toolchain. Existing system plugins take precedence when they already work.

## Automated build and test artifact

`linux-test.yml` runs in a Fedora 44 container on a Linux x86_64 runner. It
builds the app and runs isolated Qt, ZIP, SQLite, file-URI, audio negotiation,
SoundTouch/EQ and CDG checks. CI installs codecs into its disposable container;
it does not change a user's machine or publish a release.

The resulting `OpenKJ-Fedora44-x86_64-development` artifact contains a tarball
with the executable, desktop metadata, the local CDG plugin if one was built,
and an RPM version inventory. Extract it and run `./run-openkj.sh` on Fedora 44
with the dependencies above installed. It is **not a self-contained AppImage**.
The existing optional AppImage CMake path is not used by this workflow.

Before a Linux prerelease, test real MP3+G ZIPs and MP4s, pause/seek, Stop and
song changes, pitch/tempo, both EQ tabs, break music/crossfades, autoplay,
microphone recording, a full library import/search and singer rotation. Test
the video/singer windows on the actual Wayland/X11 desktop and external screens;
headless CI cannot validate hardware audio or window placement.
