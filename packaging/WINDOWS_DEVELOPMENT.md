# Initial Windows Qt 6 build

The first Windows target from `macos-qt6-test` is **Windows x64, MSYS2 UCRT64**.
Compiler, Qt 6 and GStreamer must come from that same environment. Do not mix
MSVC Qt libraries with MinGW GStreamer, as the old workflow did.

This is a development build, not a Windows release or installer. It needs native
Windows testing before being used to host a show. The macOS pipeline and custom
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
Windows runtime bundling and an installer follow native playback validation.
