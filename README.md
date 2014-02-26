OpenKJ
======

Cross-platform open source karaoke show hosting software.


Not anywhere near ready for professional use at this point.  Many features are incomplete, and the code is full of ugly.
To any other developers looking at this code, please don't laugh too hard, as I'm self taught primarily for the purpose
of writing this.  Well, okay, you can laugh, just not where I can hear you ;)

There were previous versions of this software that I wrote and used to host my shows, but they were based on proprietary
libraries and couldn't be open sourced.  This is my attempt at re-implementing my previous work in an FOSS friendly way.

The oldest and cruftiest code is in the libCDG part of it, that was some of the first code I wrote while figuring out
C++.  It could probably use heavy improvements.

My reason for this and its predecessors is because I'm a Linux geek, and I was unhappy with the lack of available Linux
based hosting software.  I'm a bit of a purist, so I couldn't abide running Windows to run my shows.  

If you are building this and want to use the pitch shift/key change functionality, you will unfortunately have to build
it with USE_FMOD defined and download and install a copy of the FMOD Api (fmod.org).  I haven't figured out how to get
key changing working with any of the open source audio libraries.  If anyone knows how to get this working, feel free 
to subclass KhAbstractAudioBackend, it should be pretty self explanatory.  By default this uses a QMediaPlayer based
audio backend, which isn't very featureful.

This project requires Qt 5.x, Taglib, and minizip to build.
