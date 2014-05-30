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

BreakMusic is a VERY basic bare bones media player.
The only real reason it exists is so that I could have something that would receive IPC commands from KaraokeHost to tell it to fade out when a karaoke track starts playing and fade back in when it is stopped. (I'm a lazy KJ like that).

Both are experimental but usable at this point.  I am using it to run my shows every weekend now, but if you do so and it kills kittens or eats your firstborn don't come screaming at me ;) Some features are still incomplete, and the code is full of ugly. To any other developers looking at this code, please don't laugh too hard, as I'm self taught primarily for the purpose of writing this.  Well, okay, you can laugh, but only if you're willing to fix the code that you're making fun of ;)

Requirements to build KaraokeHost:

* Qt 5.x
* minizip

Strongly recommended:
* gstreamer (1.0+)
* gst-plugins-ugly (for key changer)

Requirements to build BreakMusic:

* Qt 5.x
* Taglib

I develop the software and host my shows on Linux (Fedora specifically), so it is known to build and work there.  (It "should" work similarly on the BSD's.)
I have verified that it will build and run on Mac OS X, though only with the QMediaPlayer backend.  The brew gstreamer-1.0 packages were missing too much stuff for the GStreamer backend to work properly.  I haven't tried it with the GStreamer project hosted installer.
It should build and work on Windows as well, but I have little experience there and have had problems getting the dependencies set up properly.  If anyone builds it successfully on Windows please do let me know.  

The goal is to have it work on all three platforms.

The KaraokeHost audio backends

GStreamer (recommended)

* Fader - working
* Downmix - working
* Silence detect - working
* Key changer - working
* Output device selection - Not implemented (waiting for upcoming features in GStreamer)

QMediaPlayer (very limited, feature wise because of QMediaPlayer itself)

* Fader - Working
* Downmix - Not implemented
* Silence detect - Not implemented
* Key changer - Not implemented
* Output device selection - Not implemented

Fmod (Closed source, not really working on this backend anymore, but it's still there if anyone wants to play)

* Fader - Working
* Downmix - Working
* Silence detect - Working
* Key changer - Working
* Output device selection - Working (mostly)



Things that are still work in progress or to do:

KaraokeHost:

* Regular singers - Name conflict resolution on import (Rename/Merge/Replace) 
* Regular singers - Name conflict resolution on save (Merge/Replace)
* Ticker - Make height auto-adapt to font size
* And a million more things I'm forgetting

BreakMusic:

* Work on tighter integration with KaraokeHost
* Work on general look/feel

libCDG:

* Handling of CDG scroll_copy instructions.  Right now these are pretty much unhandled, but the impact is minimal.  The only professionally produced CDG's that I've seen use this functionality only use it in the title sequence at the beginning of the song, and it has zero effect on actual lyric display.  Basically not a priority for me.
* Make libCDG only hand off the "safe" area of the CDG frames.  Right now it returns the whole thing, including the cdg border area.  Also not a big deal or priority, has virtually zero effect on what the singers see.  The background is just one CDG cell larger around the perimeter of the frame (6px on sides and 12px top and bottom before scaling)
* I plan to break off libCDG into it's own repo at some point and get it out of the KaraokeHost source tree
