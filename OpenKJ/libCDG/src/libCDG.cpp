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

#include "../include/libCDG.h"
#include <QFile>

CDG::CDG()
{
    masks[0] = 0x20;
    masks[1] = 0x10;
    masks[2] = 0x08;
    masks[3] = 0x04;
    masks[4] = 0x02;
    masks[5] = 0x01;
    LastCDGCommandMS = 0;
    Open = false;
    CurPos = 0;
    needupdate = true;
    m_tempo = 100;
    QVector<QRgb> palette;
    for (int i=0; i < 16; i++)
        palette.append(QColor(0,0,0).rgb());
    image = QImage(QSize(300,216),QImage::Format_Indexed8);
    image.setColorTable(palette);
    image.fill(0);
    frames.clear();
}

bool CDG::FileOpen(QByteArray byteArray)
{
    QVector<QRgb> palette;
    for (int i=0; i < 16; i++)
        palette.append(QColor(0,0,0).rgb());
    image = QImage(QSize(300,216),QImage::Format_Indexed8);
    image.setColorTable(palette);
    image.fill(0);
    frames.clear();
    CurPos = 0;
    cdgData = byteArray;
    if (byteArray.size() > 0)
    {
        return true;
    }
    else
    {
        qCritical() << "Received zero bytes of CDG data";
        return false;
    }
}

bool CDG::FileOpen(QString filename)
{
    CurPos = 0;
    QVector<QRgb> palette;
    for (int i=0; i < 16; i++)
        palette.append(QColor(0,0,0).rgb());
    image = QImage(QSize(300,216),QImage::Format_Indexed8);
    image.setColorTable(palette);
    image.fill(0);
    frames.clear();
    QFile file(filename);
    file.open(QFile::ReadOnly);
    cdgData = file.readAll();
    file.close();
    if (cdgData.size() > 0)
        return true;
    else
    {
        qCritical() << "Received zero bytes of CDG data";
        return false;
    }
}

void CDG::VideoClose()
{
    frames.clear();
    CurPos = 0;
    Open = false;
}

unsigned int CDG::GetPosMS()
{
    float fpos = (CurPos / 300.0) * 1000;
    return (int) fpos;
}

bool CDG::Process(bool clear)
{
    needupdate = true;
    CDG_SubCode SubCode;
    static int frameno = 0;
    if (clear)
    {
        frameno = 0;
    }
    QBuffer *ioDevice = new QBuffer(&cdgData);
    ioDevice->open(QIODevice::ReadOnly);
    while ((!ioDevice->atEnd()) && (ioDevice->isReadable()) && (ioDevice->size() > 0))
    {
        if (ioDevice->read((char *)&SubCode, sizeof(SubCode)) > 0)
        {
            CDG_Read_SubCode_Packet(SubCode);
            CurPos++;
            if (((GetPosMS() % 40) == 0) && GetPosMS() >= 40)
            {
                LastCDGCommandMS = frameno * 40;
                needupdate = false;
                QVideoFrame frame(image.convertToFormat(QImage::Format_RGB32));
                frame.setStartTime(GetPosMS());
                frames.push_back(frame);
                frameno++;
            }
        }
        else
        {
            ioDevice->close();
            delete ioDevice;
            Open = true;
            return true;
        }
    }
    ioDevice->close();
    delete ioDevice;
    Open = true;
    return true;
}


void CDG::CDG_Read_SubCode_Packet(CDG_SubCode &SubCode)
{
    if ((SubCode.command & SC_MASK) == SC_CDG_COMMAND)
    {
        switch (SubCode.instruction & SC_MASK)
        {
        case CDG_MEMORYPRESET:
            needupdate = true;
            CMDMemoryPreset(SubCode.data);
            break;
        case CDG_BORDERPRESET:
            needupdate = true;
            CMDBorderPreset(SubCode.data);
            break;
        case CDG_TILEBLOCK:
            needupdate = true;
            CMDTileBlock(SubCode.data, false);
            break;
        case CDG_SCROLLPRESET:
            needupdate = true;
            CMDScrollPreset(SubCode.data);
            break;
        case CDG_SCROLLCOPY:
            needupdate = true;
            CMDScrollCopy(SubCode.data);
            break;
        case CDG_DEFINETRANS:
            needupdate = true;
            CMDDefineTrans(SubCode.data);
            break;
        case CDG_COLORSLOW:
            needupdate = true;
            CMDColors(SubCode.data, CDG_COLOR_TABLE_LOW);
            break;
        case CDG_COLORSHIGH:
            needupdate = true;
            CMDColors(SubCode.data, CDG_COLOR_TABLE_HIGH);
            break;
        case CDG_TILEBLOCKXOR:
            needupdate = true;
            CMDTileBlock(SubCode.data, true);
            break;
        }
    }
}

void CDG::CMDBorderPreset(char data[16])
{
    CDG_Border_Preset preset;
    preset.color = (data[0] & 0x0F);
    if (preset.color <= 15)
    {
        // Top rows
        for (unsigned int y = 0; y < 12; y++)
        {
            for (unsigned int x=0; x < 300; x++)
            {
                image.setPixel(x,y,preset.color);
            }
        }
        // Bottom rows

        for (unsigned int y = 202; y < 216; y++)
        {
            for (unsigned int x=0; x < 300; x++)
            {
                image.setPixel(x,y,preset.color);
            }
        }
        // Sides
        for (unsigned int y = 11; y < 204; y++)
        {
            // Left
            for (unsigned int x = 0; x < 6; x++)
            {
                image.setPixel(x,y,preset.color);
            }
            // Right
            for (unsigned int x=294; x < 300; x++)
            {
                image.setPixel(x,y,preset.color);
            }
        }
    }
}

void CDG::CMDColors(char data[16], int Table)
{
    char highbyte, lowbyte;
    int i, j, red, green, blue;
    i = 0;
    if (Table == CDG_COLOR_TABLE_HIGH)
        j = 8;
    else
        j = 0;
    while (i < 15)
    {
        lowbyte = data[i];
        highbyte = data[i + 1];
        red = 0;
        green = 0;
        blue = 0;
        if ((lowbyte & 0x20) > 0)
            red = 8;
        if ((lowbyte & 0x10) > 0)
            red = red + 4;
        if ((lowbyte & 0x08) > 0)
            red = red + 2;
        if ((lowbyte & 0x04) > 0)
            red = red + 1;
        if ((lowbyte & 0x02) > 0)
            green = 8;
        if ((lowbyte & 0x01) > 0)
            green = green + 4;
        if ((highbyte & 0x20) > 0)
            green = green + 2;
        if ((highbyte & 0x10) > 0)
            green = green + 1;
        if ((highbyte & 0x08) > 0)
            blue = 8;
        if ((highbyte & 0x04) > 0)
            blue = blue + 4;
        if ((highbyte & 0x02) > 0)
            blue = blue + 2;
        if ((highbyte & 0x01) > 0)
            blue = blue + 1;
        blue  = blue  * 17;
        red   = red   * 17;
        green = green * 17;
        i++;
        i++;
        QColor color = QColor(red, green, blue);
        image.setColor(j, color.rgb());
        j++;
    }
}

void CDG::CMDDefineTrans(char data[16])
{
    Q_UNUSED(data);
    // Unused CDG command from red book spec
}

void CDG::CMDMemoryPreset(char data[16])
{
    CDG_Memory_Preset preset;
    preset.color = (data[0] & 0x0F);
    preset.repeat = (data[1] & 0x0F);
    if (preset.color <= 15)
    {
        image.fill(preset.color);
    }
}

void CDG::CMDScrollCopy(char data[16])
{
//    static int hOffset = 0;
//    static int vOffset = 0;
    CDG_Scroll_CMD scmd;
    scmd.color = (data[0] & 0x0F);
    scmd.hScroll = (data[1] & 0x3F);
    scmd.hSCmd = (scmd.hScroll & 0x30) >> 4;
    scmd.hSOffset = (scmd.hScroll & 0x07);
    scmd.vScroll = (data[2] & 0x3F);
    scmd.vSCmd = (scmd.vScroll & 0x30) >> 4;
    scmd.vSOffset = (scmd.vScroll & 0x07);

//    qInfo() << "ScrollCopy command found";
//    qInfo() << "Color: " << (int)scmd.color;
////    qInfo() << "hSCmd: " << (int)scmd.hSCmd;
////    qInfo() << "hSOffset: " << (int)scmd.hSOffset;
//    qInfo() << "vSCmd: " << (int)scmd.vSCmd;
//    qInfo() << "vSOffset: " << (int)scmd.vSOffset;
//    if (scmd.hSCmd == 0 && scmd.hSOffset == 0)
//        qInfo() << "Not scrolling horizontally";
//    if (scmd.hSCmd == 1)
//    {
//        qInfo() << "Scrolling 6px rt";
//        QImage image2 = image.copy(-6,0,300,216);
//        image.swap(image2);
//        needupdate = true;
//    }
//    if (scmd.hSCmd == 2)
//    {
//        qInfo() << "Scrolling 6px lft";
//        QImage image2 = image.copy(6,0,300,216);
//        image.swap(image2);
//        needupdate = true;
//    }
//    if (scmd.hSOffset > 0)
//    {
//        qInfo() << "Scrolling " << (int)scmd.hSOffset << "to the left?";
//    }
}

void CDG::CMDScrollPreset(char data[16])
{
    CDG_Scroll_CMD scmd;
    scmd.color = (data[0] & 0x0F);
    scmd.hScroll = (data[1] & 0x3F);
    scmd.hSCmd = (scmd.hScroll & 0x30) >> 4;
    scmd.hSOffset = (scmd.hScroll & 0x07);
    scmd.vScroll = (data[2] & 0x3F);
    scmd.vSCmd = (scmd.vScroll & 0x30) >> 4;
    scmd.vSOffset = (scmd.vScroll & 0x07);

//    qInfo() << "ScrollPreset command found";
//    qInfo() << "Color: " << scmd.color;
//    qInfo() << "hSCmd: " << scmd.hSCmd;
//    qInfo() << "hSOffset: " << scmd.hSOffset;
//    qInfo() << "vSCmd: " << scmd.vSCmd;
//    qInfo() << "vSOffset: " << scmd.vSOffset;
}

void CDG::CMDTileBlock(char data[16], bool XOR)
{
    CDG_Tile_Block tile;
    int top, left, i, j;
    tile.color0 = (data[0] & 0x0F);
    tile.color1 = (data[1] & 0x0F);
    tile.row = (data[2] & 0x1F);
    tile.column = (data[3] & 0x3F);
    tile.tilePixels[0]  = data[4];
    tile.tilePixels[1]  = data[5];
    tile.tilePixels[2]  = data[6];
    tile.tilePixels[3]  = data[7];
    tile.tilePixels[4]  = data[8];
    tile.tilePixels[5]  = data[9];
    tile.tilePixels[6]  = data[10];
    tile.tilePixels[7]  = data[11];
    tile.tilePixels[8]  = data[12];
    tile.tilePixels[9]  = data[13];
    tile.tilePixels[10] = data[14];
    tile.tilePixels[11] = data[15];
    top  = (tile.row    * 12);
    left = (tile.column * 6);
    for (i = 0; i <= 11; i++)
    {
        for (j = 0; j <= 5; j++)
        {
            if (((top + i) <= 215) && ((left + j) <= 299) && (tile.color0 <= 15) && (tile.color1 <= 15))
            {
                int color = tile.color0;
                if ((tile.tilePixels[i] & masks[j]) > 0)
                    color = tile.color1;
                int idxnew;
                if (XOR)
                {
                    idxnew = image.pixelIndex(left + j, top + i) ^ color;
                }
                else
                {
                    idxnew = color;
                }
                int posx = left + j;
                int posy = top + i;
                image.setPixel(posx, posy, idxnew);
            }
        }
    }
}

CDG::~CDG()
{

}

QVideoFrame CDG::getQVideoFrameByTime(unsigned int ms)
{
    int scaledMs = ms * ((float)m_tempo / 100.0);
    int frameno = scaledMs / 40;
    if (ms % 40 > 0) frameno++;
    if (frameno < frames.size())
        return frames.at(frameno);
    else
        return frames.at(frames.size() - 1);
}

unsigned int CDG::GetDuration()
{
    return cdgData.size() * 40;
}

bool CDG::IsOpen() {
    return Open;
}

int CDG::tempo()
{
    return m_tempo;
}

void CDG::setTempo(int percent)
{
    m_tempo = percent;
}
