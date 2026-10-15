# OpenKJ 2.2.4 — Cross-platform Pre-release

Experimental Qt 6 / GStreamer builds from the same source for Windows x64,
Apple Silicon macOS, and Linux x86_64. The modern codebase now lives on `master`.

## Downloads

| Platform | Package | Installation |
| --- | --- | --- |
| Windows x64 | `OpenKJ-2.2.4-windows-x64.msi` | Run the installer; administrator approval is required. |
| Windows x64 portable | `OpenKJ-2.2.4-windows-x64.zip` | Extract the entire folder and open `openkj.exe`. |
| macOS Apple Silicon | `OpenKJ-2.2.4-macos-arm64.dmg` | Open the DMG and drag OpenKJ to Applications. |
| Linux x86_64 | `OpenKJ-2.2.4-linux-x86_64.flatpak` | Install with Flatpak using the commands below. |

SHA-256 files and `SHA256SUMS` accompany the packages. Build metadata records the
source commit and Linux runtime/dependency recipe. The Windows ZIP and installed
app include dependency versions, license notices and a payload hash manifest.

## Changes since 2.2.3

- Windows x64 portable package bundles Qt, GStreamer, FFmpeg, SoundTouch and CDG;
  MSYS2 and development SDKs are not needed on the destination computer.
- New Windows MSI adds a Start Menu entry, optional desktop shortcut, upgrade
  handling, repair and uninstall. Settings, databases and media are retained.
- Fixed Windows local-file URI handling and audio output format negotiation.
- Linux Flatpak includes SoundTouch/CDG and uses the KDE 6.11 runtime across
  compatible distributions. Native playback has been tested on Bazzite;
  clean installation/runtime checks also run on Ubuntu and Fedora containers.
- Reproducible platform setup/build scripts and isolated runtime checks cover
  Qt, SQLite, ZIPs, local media playback, pitch/EQ and CDG decoding.
- All platform packages are built from one release commit and published together
  after their packaging checks pass. Windows checks include MSI upgrade,
  repair, downgrade rejection, uninstall and user-data preservation.
- Kept the asynchronous library loading, search, background scanning and song
  preparation improvements introduced in the 2.2.3 macOS prerelease.
- Fixed a build compatibility issue with older Qt 6 database connection APIs.

## Windows

Both packages are unsigned. Windows may prompt before running the installer or
application. The MSI upgrades this MSI product family; it does not remove older
Inno Setup installations or portable copies. The ZIP remains available for
installation without administrator privileges. No automatic updater is included.

## macOS

**Apple Silicon only. The app is ad-hoc signed, not Developer ID signed or
notarized.** If macOS blocks it, attempt to open it, then use **System Settings →
Privacy & Security → Open Anyway** if you trust the build. Do not disable
Gatekeeper system-wide. Intel Macs are not included, and the minimum supported
macOS version has not yet been established on physical hardware.

## Linux

Requires x86_64 Linux with a compatible Flatpak installation. The KDE runtime is
installed from Flathub as needed; host Qt/GStreamer development packages and
Fedora RPM Fusion packages are not required.

```bash
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user ./OpenKJ-2.2.4-linux-x86_64.flatpak
flatpak run org.openkj.OpenKJ//preview
```

For an existing preview, add `--reinstall` to the install command. Wayland and
X11/XWayland are supported. This is an unsigned local bundle, not a Flathub
publication. Flatpak settings/database are stored under
`~/.var/app/org.openkj.OpenKJ/`; native app data is not imported automatically.
Filesystem access includes mounted song libraries, subject to user permissions.

## Testing and limitations

Back up your existing library/settings before installing a prerelease. Native
Windows testing covered full-library import, playback and singer rotation;
Linux development testing used Fedora and the packaged Flatpak was tested on
Bazzite. macOS playback and large-library responsiveness were tested during
ongoing development. The new MSI wizard still benefits from a manual Windows
check alongside its automated lifecycle checks.

Headless checks use fake audio/video outputs and do not certify every audio
device, microphone, monitor layout or desktop. Sleeping external drives can
still delay song loading. Recording music volume and incremental window-resizing
behavior remain known issues. Additional media formats have not all been tested.

Source and GPL license: [EquinoxEarth/OpenKJ](https://github.com/EquinoxEarth/OpenKJ).
Bundled third-party components retain their respective licenses. Please report
reproducible bugs through GitHub Issues with OS/hardware details and relevant logs.
