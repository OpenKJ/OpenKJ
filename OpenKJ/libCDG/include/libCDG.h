/*
 * Copyright (c) 2013-2019 Thomas Isaac Lightburn
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

#include <QByteArray>
#include <QBuffer>
#include <QVector>
#include <QImage>
#include <QColor>
#include <QDebug>
#include <QVideoFrame>
\
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


struct CDG_SubCode
{
	char	command;
	char	instruction;
	char	parityQ[2];
	char	data[16];
	char	parityP[4];
};


struct CDG_Memory_Preset
{
	char	color;
	char	repeat;
	char	filler[14];
};


struct CDG_Border_Preset
{
	char	color;
	char	filler[15];
};


struct CDG_Tile_Block
{
	char	color0;
	char	color1;
	char	row;
	char	column;
	char	tilePixels[12];
};

struct CDG_Scroll_CMD
{
	char	color;
	char	hScroll;
    char    hSCmd;
    char    hSOffset;
	char	vScroll;
    char    vSCmd;
    char    vSOffset;
};

class CDG
{
public:
	CDG();
	virtual ~CDG();
    bool FileOpen(QByteArray byteArray);
    bool FileOpen(QString filename);
	bool Process(bool clear = true);
	void VideoClose();
    QVideoFrame getQVideoFrameByTime(unsigned int ms);
    unsigned int GetDuration();
    bool IsOpen();
    unsigned int GetLastCDGUpdate() { return LastCDGCommandMS; }
    int tempo();
    void setTempo(int percent);
protected:
private:
    int m_tempo;
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
    unsigned int duration;
    bool Open;
	bool CDGFileOpened;
    QByteArray cdgData;
	unsigned int CurPos;
	char masks[6];
	bool needupdate;
    QVector<QVideoFrame> frames;
    QImage image;
};

#endif // LIBCDG_H
