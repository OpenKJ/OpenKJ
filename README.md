OpenKJ
======

Cross-platform open source karaoke show hosting software.

KaraokeHost is a full featured karaoke hosting program.
A few features:
* Save/track/load regular singers
* Key changer
* End of track silence detection (after last CDG draw command)
* Rotation ticker on the CDG display
* Remotely fades BreakMusic in and out when karaoke tracks start/end
* Lots of other little things

It currently only handles mp3g+zip files (zip files containing an mp3 and cdg file) and paired mp3 and cdg files.  I'll be adding others in the future (wav+cdg, ogg+cdg, etc) if anyone expresses interest.  It does not support playing non cdg-based tracks (wmv, avi, mpg, etc) and I have zero plans to ever do so, as professional KJ's generally don't use them.

Database entries for the songs are based on the file naming scheme.  I've included the commone ones I've come across, if anyone needs something added just tell me or submit the appropriate code changes if you're a programmer.  They're pretty trivial to add.  I plan on adding the ability to do custom definitons in the future, but it's pretty far down on my list.

![Main window screen shot](/screenShots/KhMainWindow.png "Main KaraokeHost Window")
![Full screen CDG Display](/screenShots/KhCDGWindowFullScreen.png "Fullscreen CDG Display")

BreakMusic is a VERY basic bare bones media player.
The only real reason it exists is so that I could have something that would receive IPC commands from KaraokeHost to tell it to fade out when a karaoke track starts playing and fade back in when it is stopped. (I'm a lazy KJ like that).

Both are experimental but usable at this point.  I am using it to run my shows every weekend now, but if you do so and it kills kittens or eats your firstborn don't come screaming at me ;) Some features are still incomplete. To any other developers looking at this code, please don't laugh too hard, as I'm self taught primarily for the purpose of writing this.  Well, okay, you can laugh, but only if you're willing to fix the code that you're making fun of ;)

**Requirements to build KaraokeHost:**

* Qt 5.x
* gstreamer 1.x

**Requirements to build BreakMusic:**

* Qt 5.x
* gstreamer 1.x

**Linux**

I develop the software and host my shows on Linux (Fedora specifically), so it is known to build and work there.  (It "should" work similarly on any Linux distor or the BSD's.)  Everything needed will most likely be available via the package manager on any common distro.  On Fedora the packages are gstreamer-devel gstreamer gstreamer-plugins-good gstreamer-plugins-bad and the Qt5 stuff (I just yum install qt5-* because I'm lazy).  On Fedora you will also need to have the rpmfusion repo enabled to get mp3 support, as the app is pretty useless w/o it.  "qmake-qt5" or possibly just "qmake", depending on your distro, followed by a "make" should get it built. A "make install" will put the binaries in /usr/bin and copy .desktop files and icons into the appropriate places for it to appear in the app menu.  Tweak the KaraokeHost.pro file to enable or disable OpenGL support prior to building.  One thing to note, you'll probably need to turn off flat volumes in your pulseaudio config if you're using it, otherwise the applicaitons may mess with your system-wide volume instead of just the application volume.

**Mac**

Building now works on OS X.  You must install the homebrew version of the gstreamer packages.  The gst-plugins-bad package must be installed using the --with-sound-touch option if you want to enable the key chager.

* Install brew (https://brew.sh)
* brew install gstreamer gst-plugins-good gst-plugins-ugly gst-plugins-base
* brew install sound-touch
* brew install gst-plugins-bad --with-sound-touch

**Windows**

Karaokehost is building and working on Windows using the msvc 2015 build system (both 32 and 64 bit) with Qt Creator.  GStreamer works fine built against the gstreamer.com GStreamer SDK.  You will likely need to modify the paths in the KaraokeHost.pro file to match your devel environment.  Experimental build installers can be found at http://openkj.org/ if you just want to run the software and not build it yourself or help out with development.


The goal is to have it work consistently across all three platforms.

Things that are still work in progress or to do:

KaraokeHost:

* Regular singers - Name conflict resolution on import (Rename/Merge/Replace) 
* Regular singers - Name conflict resolution on save (Merge/Replace)
* Settings - Output device selection
* And a million more things I'm forgetting

BreakMusic:

* Work on tighter integration with KaraokeHost
* Work on general look/feel

libCDG:

* Handling of CDG scroll_copy instructions.  Right now these are pretty much unhandled, but the impact is minimal.  The only professionally produced CDG's that I've seen use this functionality only use it in the title sequence at the beginning of the song, and it has zero effect on actual lyric display.  Basically not a priority for me.
* Make libCDG only hand off the "safe" area of the CDG frames.  Right now it returns the whole thing, including the cdg border area.  Also not a big deal or priority, has virtually zero effect on what the singers see.  The background is just one CDG cell larger around the perimeter of the frame (6px on sides and 12px top and bottom before scaling)
* I plan to break off libCDG into it's own repo at some point and get it out of the KaraokeHost source tree
