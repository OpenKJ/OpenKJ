# OpenKJ Linux x86_64 Flatpak preview

This experimental package targets Linux x86_64 distributions with Flatpak,
including Aurora, Fedora, Ubuntu, Debian, Arch and openSUSE. The same bundle
uses its Flatpak runtime across distributions; it does not link against each
distribution's Qt or multimedia packages. It uses the KDE 6.11 / Qt 6 runtime, with the SoundTouch pitch and Rust
CDG plugins included in the application. No Fedora RPM Fusion packages,
development SDK, build tools or sourced environment file are needed to run it.
The KDE runtime is downloaded through Flathub at installation if needed.

## Install on Linux (including Aurora)

Download the `OpenKJ-Linux-x86_64-flatpak-preview` GitHub Actions artifact and
extract it. From the extracted folder:

```bash
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user ./OpenKJ-Linux-x86_64-preview.flatpak
flatpak run org.openkj.OpenKJ//preview
```

The application also appears in the desktop launcher. The preview is a locally
built, unsigned Flatpak bundle; it is not a Flathub publication. To install a
newer preview of the same branch, run the install command with `--reinstall`.
The `.flatpak.sha256` file supplies the download checksum:

```bash
sha256sum --check OpenKJ-Linux-x86_64-preview.flatpak.sha256
```

The package supports Wayland and X11/XWayland, sound playback and microphone
recording, graphics rendering and network features. It grants host-filesystem
access so the existing library browser, imports, downloads and file renaming
can work with home folders, `/run/media`, `/media`, `/mnt` and other mounted
libraries. `/var/mnt` is also exposed for static mounts on atomic desktops.
Access remains subject to your normal user permissions.

Flatpak keeps application configuration/data under
`~/.var/app/org.openkj.OpenKJ/`. A native installation's settings/database are
not imported automatically. An existing Flatpak with the same application ID
shares these directories across branches; back up existing OpenKJ data before
testing. Removing the application without `--delete-data` retains its data.

## Test on the actual host

Check MP3+G ZIPs and MP4s, pitch/tempo, both EQ tabs, break music/crossfades,
microphone recording, seeking/Stop, autoplay, a full library import/search and
singer rotation. Test song editing and renaming on the actual external drive.
Check full-screen output and singer windows on your real monitors and desktop.
Headless CI validates media decoding/DSP with fake outputs; physical audio and
display behavior require this native test.

## Build the package

On an x86_64 Linux build machine, install `flatpak`, `flatpak-builder`, `git`,
`tar` and Python 3.11+, then run from the checkout:

```bash
bash packaging/build_flatpak.sh
```

This packages **committed HEAD**, excluding local build trees and user files.
It downloads the KDE SDK/runtime and Rust SDK extension, builds the additional
plugins and app, installs the preview per user, tests its Platform runtime, and
writes the bundle/checksum/manifest to `releases/flatpak/`. CI uses the same
script in the official KDE SDK build container. The exported bundle is then
installed and tested in clean Ubuntu and Fedora containers with the Platform
runtime and no development SDK. Other distributions are intended targets;
their hardware audio and display behavior still need native testing.

The SoundTouch plugin is built from GStreamer 1.26.11 to match the KDE runtime's
GStreamer ABI. It uses the existing `pitch` element and SoundTouch 2.4.1; DSP
pipelines/settings are unchanged. Rust CDG uses pinned gst-plugins-rs 0.15.3,
as tested in the Windows and Fedora development builds. Static C++ dependencies
and CPM itself are supplied as pinned sources; Cargo uses the upstream lockfile
and network access at build time. This preview recipe is not yet an offline
Flathub submission. Included library notices are in `/app/share/licenses/openkj`.
