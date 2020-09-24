**Downloads**  
If you are looking for installers for Windows or macOS or binary packages for Fedora, Debian, or Ubuntu, please visit the Downloads section at https://openkj.org

If you need help with OpenKJ, you can reach out to support@openkj.org via email.

OpenKJ
======

Cross-platform open source karaoke show hosting software.

OpenKJ is a full featured karaoke hosting program.
A few features:
* Save/track/load regular singers
* Key changer
* Tempo control
* EQ
* End of track silence detection (after last CDG draw command)
* Rotation ticker on the CDG display
* Option to use a custom background or display a rotating slide show on the CDG output dialog while idle
* Fades break music in and out automatically when karaoke tracks start/end
* Remote requests server integration allowing singers to look up and submit songs via the web
* Automatic performance recording
* Autoplay karaoke mode
* Lots of other little things

It currently handles media+g zip files (zip files containing an mp3, wav, or ogg file and a cdg file) and paired mp3 and cdg files.  I'll be adding others in the future if anyone expresses interest.  It also can play non-cdg based video files (mkv, mp4, mpg, avi) for both break music and karaoke.

Database entries for the songs are based on the file naming scheme.  I've included the commone ones I've come across which should cover 90% of what's out there. Custom patterns can be also defined in the program using regular expressions.


**Screenshots**
  
See https://openkj.org  
  
**Requirements to build OpenKJ:**

* Qt 5.x
* gstreamer 1.4 or above

**Linux**

I develop the software and host my shows on Linux (Fedora specifically), so it is known to build and work there.  (It "should" work similarly on any Linux distro or the BSD's.)  Everything needed will most likely be available via the package manager on any common distro.  On Fedora the packages are gstreamer-devel gstreamer gstreamer-plugins-good gstreamer-plugins-bad and the Qt5 stuff (I just yum install qt5-* because I'm lazy).  On Fedora you will also need to have the rpmfusion repo enabled to get mp3 support, as the app is pretty useless w/o it.  "qmake-qt5" or possibly just "qmake", depending on your distro, followed by a "make" should get it built. A "make install" will put the binaries in /usr/bin and copy .desktop file and icon into the appropriate places for it to appear in the app menu.  Tweak the OpenKJ.pro file to enable or disable OpenGL support prior to building.  One thing to note, you'll probably need to turn off flat volumes in your pulseaudio config if you're using it, otherwise the applicaitons may mess with your system-wide volume instead of just the application volume.

**Contributed notes for building on Ubuntu 16.04 courtesy of Henry74**  

```
sudo apt install qt5-qmake  
sudo apt install libqt5svg5-dev
sudo apt install libgstreamer-plugins-base1.0-dev
```

In a terminal switch to the OpenKJ/OpenKJ directory in the repository.  
```
/usr/lib/x86_64-linux-gnu/qt5/bin/qmake
make
```

Suggest installing and using checkinstall to create a .deb file  
`sudo checkinstall`

Install the .deb file.

**Mac**

Building now works on OS X in Qt Creator using the native xcode compiler.  Use the latest stable version of the GStreamer SDK from http://gstreamer.freedesktop.org.


**Windows**

Building now works on Windows in Qt Creator using the msvc build system (both 32 and 64 bit).  Use the latest stable version of the GStreamer SDK from http://gstreamer.freedesktop.org.  You will likely need to modify the paths in the OpenKJ.pro file to match your devel environment.  Installers can be found at http://openkj.org/ if you just want to run the software and not build it yourself or help out with development.

