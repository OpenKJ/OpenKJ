OpenKJ
======

Cross-platform open source karaoke show hosting software.


Not anywhere near ready for professional use at this point.  Many features are incomplete, and the code is full of ugly. To any other developers looking at this code, please don't laugh too hard, as I'm self taught primarily for the purpose of writing this.  Well, okay, you can laugh, just not where I can hear you ;)

There are previous versions of this software that I wrote and currently use to host my shows, but they are based on proprietary libraries and couldn't be open sourced.  This is my attempt at re-implementing my previous work in an FOSS friendly way.  As soon as it reaches feature parity with what I'm using now, I'll start dogfooding it.

The oldest and cruftiest code is in the libCDG part of it, that was some of the first code I wrote while figuring out C++.  It could probably use heavy improvements.

My reason for this and its predecessors is because I'm a Linux geek, and I was unhappy with the lack of available Linux based hosting software.  I'm a bit of a purist, so I couldn't abide running Windows to run my shows.  

If you are building this and want to use the pitch shift/key change functionality, you will unfortunately have to build it with USE_FMOD defined and download and install a copy of the FMOD Api (fmod.org).  I haven't figured out how to get key changing working with any of the open source audio libraries.  If anyone knows how to get this working, feel free to subclass KhAbstractAudioBackend, it should be pretty self explanatory.  By default this uses a QMediaPlayer based audio backend, which isn't very featureful.

This project requires Qt 5.x, boost, Taglib, and minizip to build.

Things that work so far:

libCDG:
Opening and processing of cdg files into sequences of timestamps and rgb data to be displayed
libCDG is fairly stable and I've been dogfooding it for years at this point every weekend in pre-OSS karaoke programs of mine.

KaraokeHost:
Blank database creation if nonexistant on program startup
Database management (scanning for and importing karaoke zip files)
Database searching
Adding rotation singers
Deleting rotation singers
Renameing rotation singers
Moving rotation singers via drag reordering
Current singer tracking
Adding songs to a singer's queue (via double click or drag'n'drop from the db search results)
Deleting singer's queued songs
Moving singer's queued songs via drag reordering
Audio output (QMediaPlayer or FMOD backend)
On the fly key changes (FMOD backend ONLY)
CDG Graphics output in the main window
Scaled CDG Graphics output in the optional CDG display window (requires working OpenGL video drivers)
Settings for CDG window (enabled/disabled, fullscreen, monitor for fullscreen output)
Playing songs from the rotation or queue lists (double click)
Media controls (pause, unpause, stop)
Regular singer management (save/track new regular singer - Pretty useless right now)
Notification of BreakMusic via IPC that karaoke is starting playback or stopping (used for auto-fade of break music)

BreakMusic:
Fading in/out on IPC notification from KaraokeHost
Fading out/in when switching songs
Database management (scanning for and importing mp3's for now, will add more formats and make configurable later)
Database search
Playlist creation
Playlist selection
Adding songs to playlist via drag'n'drop from search results
Adding songs to playlist via double click.
Playing songs in playlist
Media controls
Display of songs via either metadata or filename (or both)

Things that are still work in progress or to do:

KaraokeHost:
Regular singer management (load/export/import/etc)
Setting key changes on queued songs
Convert main program CDG display to OpenGL rendering like the CDG window
Automatically adapting the models/views to contents and window size
Detection of bad zip files and user notification.
User notification of many different error conditions.
Fading in/out on stop, pause, and unpause.
Find a way to do keychanges without requiring a closed souce library
And a million more things I'm forgetting

BreakMusic:
Work on tighter integration with KaraokeHost
Work on general look/feel

libCDG:
Handling of CDG scroll_copy instructions.  Right now these are pretty much unhandled, but the impact is minimal.  The only professionally produced CDG's that I've seen use this functionality only use it in the title sequence at the beginning of the song, and it has zero effect on actual lyric display.  Basically not a priority for me.
Make libCDG only hand off the "safe" area of the CDG frames.  Right now it returns the whole thing, including the cdg border area.  Also not a big deal or priority, has virtually zero effect on what the singers see.  The background is just one CDG cell larger around the perimeter of the frame (6px on sides and 12px top and bottom before scaling)
