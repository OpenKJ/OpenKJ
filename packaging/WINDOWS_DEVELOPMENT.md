# Initial Windows Qt 6 build

The first Windows target from `macos-qt6-test` is **Windows x64, MSYS2 UCRT64**.
Compiler, Qt 6 and GStreamer must come from that same environment. Do not mix
MSVC Qt libraries with MinGW GStreamer, as the old workflow did.

The native development build has passed the host's full-library import and
singer-rotation testing. A portable experimental preview is now staged in CI;
the ZIP still needs a clean-machine playback check before public release. The macOS pipeline and custom
SoundTouch build remain unchanged. MSYS2 supplies the same upstream SoundTouch
`pitch` element on Windows; the workflow verifies that it is available.
Windows builds pin libzip 1.11.4 for corrected Windows callback types accepted
by current compilers; the existing macOS libzip pin is preserved.

## Local build

Install [MSYS2](https://www.msys2.org/) and launch its **UCRT64** terminal.
Update MSYS2 first with `pacman -Syu` (restart the terminal and repeat if asked).
Install:

```bash
pacman -S --needed git \
  mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-svg \
  mingw-w64-ucrt-x86_64-qt6-networkauth mingw-w64-ucrt-x86_64-qt6-multimedia \
  mingw-w64-ucrt-x86_64-qt6-websockets mingw-w64-ucrt-x86_64-gstreamer \
  mingw-w64-ucrt-x86_64-gst-plugins-base mingw-w64-ucrt-x86_64-gst-plugins-good \
  mingw-w64-ucrt-x86_64-gst-plugins-bad mingw-w64-ucrt-x86_64-gst-plugins-rs \
  mingw-w64-ucrt-x86_64-gst-libav

git clone --branch macos-qt6-test https://github.com/EquinoxEarth/OpenKJ.git
cd OpenKJ
cmake -S . -B build-windows-dev -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DOPENKJ_BUILD_SMOKE_TESTS=ON
cmake --build build-windows-dev --parallel 4
export QT_PLUGIN_PATH="$(qmake6 -query QT_INSTALL_PLUGINS)"
ctest --test-dir build-windows-dev --output-on-failure
./build-windows-dev/openkj.exe
```

Run the executable from UCRT64 so it can find the installed GStreamer runtime.
The Rust plugin package supplies `cdgparse` and `cdgdec`; it is required for CDG
playback even though OpenKJ itself is a C++ application.
The CI artifact contains the development executable, Qt deployment files and
diagnostics, **not a standalone GStreamer distribution**. Downloading just the
executable onto a clean Windows PC is not sufficient. MSYS2 package versions
are recorded by `pacman -Q` in each CI run; they are not frozen.

## CI and testing

`windows-test.yml` builds on `windows-2025` for branch pushes and pull requests
targeting `macos-qt6-test`, and supports manual runs. It checks Qt widgets,
ZIP creation/readback, SQLite database access, required media factories, SoundTouch/EQ processing, and
synthetic CDG decoding without opening the real application database.

After compilation and smoke checks succeed, test MP3+G ZIPs and MP4s, pause/seek,
startup synchronization, Stop/track changes, pitch/tempo, both EQ tabs, break
music/crossfades, secondary displays, autoplay, recording permissions, large
library search/edits, and media paths containing spaces and non-ASCII text.
The preview ZIP below is the first distribution format; an installer can follow
after testing the portable runtime on a machine without the development SDK.

## Portable preview ZIP

The `OpenKJ-Windows-x64-preview` CI artifact contains a ZIP and its SHA-256 hash.
Extract the whole ZIP, then double-click `openkj.exe` inside the extracted
folder. This bundle includes Qt, the GStreamer plugins/scanner, FFmpeg,
SoundTouch and their native DLL dependencies; it does not need an MSYS2 install.
It is unsigned and experimental. Keep all DLLs and subfolders beside the EXE.
It uses your normal OpenKJ settings and database; back them up before testing.

To stage the same folder locally from UCRT64, install its native Python:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-python
python packaging/package_windows.py --prefix /ucrt64
powershell.exe -NoProfile -ExecutionPolicy Bypass -File packaging/check_windows_bundle.ps1
```

Staging requires a fresh `dist/OpenKJ-Windows-x64-preview` destination. It copies
the deployed Qt plugins and installed GStreamer plugin set, follows PE imports
recursively, and fails on unresolved dependencies or MSYS runtime binaries.
It includes license notices, package versions and a per-file hash manifest.
CI checks this exact folder using only Windows system paths, a fresh GStreamer
registry and the bundled plugin scanner, then archives it. The check exercises
the shared bundle-path configuration used by the application.

The automated check uses offscreen Qt and fake audio/video outputs. Repeat
physical playback, recording and display tests on a Windows PC without MSYS2
before treating the portable preview as release-ready. Public distribution
also requires reviewing the included dependency notices/source requirements.

## Preview releases

The same workflow builds and publishes GitHub prereleases for tags such as
`windows-v2.2.3-prerelease.1`. The numeric version must match `okjversion.h` and
`WINDOWS_RELEASE_NOTES.md`. Publishing waits for the build and isolated bundle
check, verifies the ZIP checksum, then attaches the ZIP and SHA-256 file. It
does not overwrite existing assets on reruns. Branch pushes only make artifacts.
The legacy Windows installer and auto-version workflows skip this tag namespace.
