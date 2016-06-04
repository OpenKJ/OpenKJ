/*
 * Copyright (c) 2013-2014 Thomas Isaac Lightburn
 *
 *
 * This file is part of libCDG.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LIBCDG_H
#define LIBCDG_H

#include <string.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include "libCDG_Frame_Image.h"
#include "libCDG_Color.h"
//#include <boost/scoped_ptr.hpp>
#include <QByteArray>
#include <QBuffer>

#define SC_MASK           0x3F
#define SC_CDG_COMMAND    0x09
#define CDG_MEMORYPRESET     1
#define CDG_BORDERPRESET     2
#define CDG_TILEBLOCK	     6
#define CDG_SCROLLPRESET    20
#define CDG_SCROLLCOPY      24
#define CDG_DEFINETRANS     28
#define CDG_COLORSLOW       30
#define CDG_COLORSHIGH      31
#define CDG_TILEBLOCKXOR    38
#define CDG_COLOR_TABLE_LOW  0
#define CDG_COLOR_TABLE_HIGH 1

#define MODE_FILE           0
#define MODE_QIODEVICE       1

using namespace std;

//! A struct representing the data contained in a cdg packet
/*!
    This is used internally by libCDG and is not meant for direct access or use.
*/
struct CDG_SubCode
{
	char	command;
	char	instruction;
	char	parityQ[2];
	char	data[16];
	char	parityP[4];
};

//! A struct representing the data contained in a cdg memory preset command
/*!
    This is used internally by libCDG and is not meant for direct access or use.
*/
struct CDG_Memory_Preset
{
	char	color;
	char	repeat;
	char	filler[14];
};

//! A struct representing the data contained in a cdg border preset command
/*!
    This is used internally by libCDG and is not meant for direct access or use.
*/
struct CDG_Border_Preset
{
	char	color;
	char	filler[15];
};

//! A struct representing the data contained in a cdg tile
/*!
    This is used internally by libCDG and is not meant for direct access or use.
*/
struct CDG_Tile_Block
{
	char	color0;
	char	color1;
	char	row;
	char	column;
	char	tilePixels[12];
};

//! A struct representing the data contained in a cdg scroll copy or scroll preset command
/*!
    This is used internally by libCDG and is not meant for direct access or use.
*/
struct CDG_Scroll_CMD
{
	char	color;
	char	hScroll;
	char	vScroll;
};

//! A C++ class for decoding cdg (karaoke graphics) files
/*!
    A class that decodes CDG files into a series of bitmaps which can be displayed in a program.
    Normally you will begin by creating the object, then using the FileOpen(), Process(), and GetImageByTime() functions.
*/
class CDG
{
public:
	//! Default constructor
	CDG();
	//! Default destructor
	virtual ~CDG();
	//! Open a cdg file
	/*!
	      Opens a cdg file to be processed.  You MUST open a file with this funciton before calling any other member functions of this class.
	      \return true on success or false on failure
	*/
	bool FileOpen(string filename);
    //! Open a QIODevice
    /*!
        Opens a QT IO Device to be processed.
        \return true on success or false on failure
    */
    bool FileOpen(QByteArray byteArray);
	//! Process an opened CDG file
	/*!
	      Processes the contents of a CDG file to generate libCDG's internal
	      frame array.  On a moderately powered computer, this should take under half a second.
	      Once this finishes, libCDG is ready to serve up images for display in your program.
	      \return true on success and false on failure
	*/
	bool Process(bool clear = true);
    //! Determine whether frame at position ms is a duplicate of the previous frame
    /*!
        This function is used to determine whether the frame at postion ms is a duplicate of the previous frame.  This is useful
        for determining whether or not you need to retreive and display the frame or can skip it.  For programs that scale the
        output from libCDG this can save quite a bit of CPU time by avoiding unneccesary expensive image scaling routines
        \param ms Position in milliseconds at which to check for duplication.
        \return true if frame is a duplicate of the last, false if not.
    */
	bool SkipFrame(int ms);
	//! Clear all cdg data and free all memory used by the previously opened cdg file
	/*!
        Should be called after you are done with the previously opened cdg file.  This resets all internal data, frees the memory used by
        the frame array, and gets the object ready to handle a new cdg file.
	*/
	void VideoClose();
	//! Retrieve a frame as RGB data
	/*!
        Retreives a video frame at the specified position in milliseconds.  Returns a pointer to an array of data containg the RGB values
        for the frame.  The image is 300px wide by 216px high.  Data is in RGBRGBRGB... format, where the first RGB value is for the top
        left pixel in the image.
        \param ms Position to get a video frame for, in milliseconds
        \return a pointer to an array of RGB values representing the video frame image
	*/
	unsigned char *GetImageByTime(unsigned int ms);
	//! Retrieve a frame as RGB data
	/*!
        Retreives a video frame at the specified position in milliseconds.  Takes a pointer to an array of unsigned char and fills it
        with the RGB values for the frame.  The array should be malloc'd with a size of 300 * 216 * 3.  Data populated is in
        RGBRGBRGB... format, where the first RGB value is for the top left pixel in the image.
        \param ms Position to get a video frame for, in milliseconds
        \param pRGB Pointer to a malloc'd array of unsigned char
	*/
	void GetImageByTime(unsigned int ms, unsigned char * pRGB);
	//! Get the length of the cdg file, in milliseconds
	/*!
        Gets the length of the currently opened and processed cdg file, in milliseconds.
        \return an unsigned integer containing the cdg's length in milliseconds
	*/
	unsigned int GetDuration()
	{
		return CDGVideo.size() * 40;
	}
	bool IsOpen() {
        return Open;
	}
    unsigned int GetLastCDGUpdate() { return LastCDGCommandMS; }
    unsigned char *GetCDGRowByTime(unsigned int ms, unsigned int row);
    bool RowNeedsUpdate(unsigned int ms, int row);
    bool AllNeedUpdate(unsigned int ms);
protected:
private:
	void FileClose();
	void CDG_Read_SubCode_Packet(CDG_SubCode &SubCode);
	void CMDScrollPreset(char data[16]);
	void CMDScrollCopy(char data[16]);
	void CMDDefineTrans(char data[16]);
	void CMDMemoryPreset(char data[16]);
	void CMDBorderPreset(char data[16]);
	void CMDTileBlock(char data[16], bool XOR = false);
	void CMDColors(char data[16], int Table);
	unsigned int GetPosMS();
	unsigned int LastCDGCommandMS;
    bool Open;
	bool CDGFileOpened;
	FILE *CDGFile;
    QByteArray cdgData;
    QBuffer *buffer;
	unsigned int CurPos;
    CDG_Frame_Image *CDGImage;
	char masks[6];
	bool needupdate;
	CDG_Color colors[16];
	vector<CDG_Frame_Image *> CDGVideo;
	vector<int> ChangedRows;
	bool NeedFullUpdate;
	vector<bool> SkipFrames;
    int mode;
};

#endif // LIBCDG_H
