# OpenKJ 2.2.6 — Cross Platform Beta

Experimental Qt 6 / GStreamer builds from the same source for Windows x64,
Apple Silicon macOS, and Linux x86_64. The modern codebase now lives on `master`.

## Downloads

| Platform | Package | Installation |
| --- | --- | --- |
| Windows x64 | `OpenKJ-2.2.6-windows-x64.msi` | Run the installer; administrator approval is required. |
| Windows x64 portable | `OpenKJ-2.2.6-windows-x64.zip` | Extract the entire folder and open `openkj.exe`. |
| macOS Apple Silicon | `OpenKJ-2.2.6-macos-arm64.dmg` | Open the DMG and drag OpenKJ to Applications. |
| Linux x86_64 | `OpenKJ-2.2.6-linux-x86_64.flatpak` | Install with Flatpak using the commands below. |

SHA-256 files and `SHA256SUMS` accompany the packages. Build metadata records the
source commit and Linux runtime/dependency recipe. The macOS app also includes
dependency notices, Homebrew package metadata and its build environment. The
Windows ZIP and installed app include dependency versions, license notices and
a payload hash manifest.

## Changes since 2.2.5

- Fixed the audio output device lists so available outputs can be selected.
- Added an option for break music to follow the karaoke audio output while
  retaining separate output selection when desired.
- Replaced the existing-singer dropdown in Add Song To Queue with an
  alphabetical list that wraps after ten singers per column.
- Singer columns share a width based on the longest name and expand evenly
  to fill the available space. Wider rotations scroll horizontally.
- The dialog adapts to the screen width and font size, with Singer Position
  and Key Change moving between stacked and side-by-side layouts.
- Improved dialog spacing and selection highlighting. Height now follows
  the contents automatically; manual resizing is horizontal only.

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
flatpak install --user ./OpenKJ-2.2.6-linux-x86_64.flatpak
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
