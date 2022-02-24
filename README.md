[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/OpenKJ/OpenKJ.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/OpenKJ/OpenKJ/context:cpp)
[![Copr build status](https://copr.fedorainfracloud.org/coprs/openkj/OpenKJ-unstable/package/openkjtools/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/openkj/OpenKJ-unstable/package/openkjtools/)
[![Windows Build](https://github.com/OpenKJ/OpenKJ/actions/workflows/windows-test.yml/badge.svg)](https://github.com/OpenKJ/OpenKJ/actions/workflows/windows-test.yml)
[![Test building on macOS](https://github.com/OpenKJ/OpenKJ/actions/workflows/macos-test.yml/badge.svg)](https://github.com/OpenKJ/OpenKJ/actions/workflows/macos-test.yml)

**Downloads**  
If you are looking for installers for Windows or macOS, please visit the Downloads section at https://openkj.org

Linux users can grab OpenKJ stable versions from flathub: https://flathub.org/apps/details/org.openkj.OpenKJ

If you would like to install Linux versions of the unstable builds, please refer to the OpenKJ documentation wiki.

Documentation can be found at https://docs.openkj.org

* Flatpak based install on Ubuntu 18.10 or later:

1. Install flatpak on Ubuntu:

```
  $ sudo apt install flatpak
  $ sudo add-apt-repository ppa:flatpak/stable
  $ sudo apt update
  $ sudo apt install flatpak
  $ sudo apt install gnome-software-plugin-flatpak
```
2. Choose Stable or Unstable
  * To Add OpenKJ Stable:
 ``` 
  $ flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
  $ flatpak install flathub org.openkj.OpenKJ
```
  * To Instead Add OpenKJ Unstable:
```
  $ flatpak remote-add --if-not-exists flathub-beta https://flathub.org/beta-repo/flathub-beta.flatpakrepo
  $ flatpak install flathub-beta org.openkj.OpenKJ
```
3. Run OpenKJ
```
  $ flatpak run org.openkj.OpenKJ
```

If you need help with OpenKJ, you can reach out to support@openkj.org via email.

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

It currently handles media+g zip files (zip files containing an mp3, wav, or ogg file and a cdg file) and paired mp3 and cdg files.  I'll be adding others in the future if anyone expresses interest.  It also can play non-cdg based video files (mkv, mp4, mpg, avi) for both break music and karaoke.

Database entries for the songs are based on the file naming scheme.  I've included the common ones I've come across which should cover 90% of what's out there. Custom patterns can be also defined in the program using regular expressions.



**Requirements to build OpenKJ:**

* Qt 5.x
* gstreamer 1.4 or above
* spdlog
* taglib

**Linux**

Build using cmake from the command line or in your IDE of choice

**Mac**

Building now works on OS X in Qt Creator using the native xcode compiler.  Use the latest stable version of the GStreamer SDK from http://gstreamer.freedesktop.org.


**Windows**

Building now works on Windows in Qt Creator using the msvc build system (both 32 and 64 bit).  Use the latest stable version of the GStreamer SDK from http://gstreamer.freedesktop.org.  You will likely need to modify the paths in the OpenKJ.pro file to match your devel environment.  Installers can be found at http://openkj.org/ if you just want to run the software and not build it yourself or help out with development.

