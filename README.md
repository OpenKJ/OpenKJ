[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/OpenKJ/OpenKJ.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/OpenKJ/OpenKJ/context:cpp)
[![Copr build status](https://copr.fedorainfracloud.org/coprs/openkj/OpenKJ-unstable/package/openkjtools/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/openkj/OpenKJ-unstable/package/openkjtools/)
[![Windows Build](https://github.com/OpenKJ/OpenKJ/actions/workflows/windows-test.yml/badge.svg)](https://github.com/OpenKJ/OpenKJ/actions/workflows/windows-test.yml)
[![Test building on macOS](https://github.com/OpenKJ/OpenKJ/actions/workflows/macos-test.yml/badge.svg)](https://github.com/OpenKJ/OpenKJ/actions/workflows/macos-test.yml)

## **Actively Maintained Fork**

Hey, I'm EquinoxEarth. I actively use this software to run my shows, and so I really don't want it to die over time. I'm putting my own time in to update it to newer frameworks and make sure it will continue to run on newer operating systems well into the future. If you decide to use my fork, please report issues so that I can work on them and keep this project alive. I really like this program, and find it way easier to use than most of the other options out there.

Thank you Isaac Lightburn for creating this project, and putting your heart into building it. I will keep your dream alive as long as I can.
Thank you Phillip Kirk for getting this project part of the way to updated!

OpenKJ
======

Cross-platform open source karaoke show hosting software.

OpenKJ is a fully featured karaoke hosting program.
A few features:
* Save/track/load regular singers
* Key changer
* Tempo control
* EQ
* End of track silence detection (after last CDG draw command)
* Rotation ticker on the CDG display
* Option to use a custom background or display a rotating slide show on the CDG output dialog while idle
* Fades break music in and out automatically when karaoke tracks start/end
* Remote request server integration allowing singers to look up and submit songs via the web or mobile apps
* Automatic performance recording
* Autoplay karaoke mode
* Lots of other little things

It currently handles media+g zip files (zip files containing an mp3, wav, or ogg file and a cdg file) and paired mp3 and cdg files. It also can play non-cdg based video files (mkv, mp4, mpg, avi) for both break music and karaoke.

Database entries for the songs are based on the file naming scheme. Custom patterns can also be defined in the program using regular expressions.

---

## Building OpenKJ

OpenKJ is built using CMake and requires a C++20 compliant compiler, Qt 6, and GStreamer (including the SoundTouch pitch plugin).
*Note: `spdlog`, `libzip`, and `taglib` are automatically fetched and built statically using CPM (CMake Package Manager) during configuration.*

### Requirements
- **Qt 6** (Core, Gui, Sql, Network, NetworkAuth, Widgets, Concurrent, Svg, PrintSupport, Multimedia, WebSockets)
- **GStreamer** (the macOS setup uses 1.28.x), with audio/video plugins and SoundTouch
- **CMake** (3.14+)
- **Ninja** or **Make**

---

### **macOS (Apple Silicon)**

Install Apple's Command Line Tools and native Homebrew, then run:

```bash
./packaging/setup_macos.sh
source build-macos-deps/env.sh
cmake --build build-macos-dev --parallel 4
```

The setup builds the required SoundTouch plugin inside the checkout, configures
Qt/GStreamer, and records installed dependency versions. See
[macOS development and automated releases](packaging/MACOS_DEVELOPMENT.md)
for first-machine setup, IDE use, checks, standalone app packaging, and DMGs.

---

### **Linux (legacy instructions; not validated against this branch)**

#### Ubuntu / Debian
1. **Install Dependencies**:
   ```bash
   sudo apt update
   sudo apt install build-essential cmake ninja-build pkg-config \
                    qt6-base-dev qt6-svg-dev qt6-multimedia-dev \
                    qt6-networkauth-dev qt6-websockets-dev libvlc-dev
   ```

#### Fedora
1. **Install Dependencies** (Note: `vlc-devel` requires the **RPM Fusion Free** repository to be enabled):
   ```bash
   sudo dnf install cmake ninja-build gcc-c++ pkgconfig \
                    qt6-qtbase-devel qt6-qtsvg-devel qt6-qtmultimedia-devel \
                    qt6-qtnetworkauth-devel qt6-qtwebsockets-devel vlc-devel
   ```

#### Build Commands
```bash
# Configure the build directory
cmake -DSPDLOG_USE_BUNDLED=true -DCMAKE_BUILD_TYPE=Release -B build -G Ninja

# Compile the application
cmake --build build
```

---

### **Windows (initial Qt 6 development build)**

The initial Windows x64 build uses a consistent MSYS2 UCRT64 compiler, Qt 6 and
GStreamer stack. See [Windows development](packaging/WINDOWS_DEVELOPMENT.md)
for setup commands, CI checks, and the native testing checklist. This is not yet
a standalone Windows release or installer.
