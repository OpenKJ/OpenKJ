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
#include <QDebug>
#include <QBuffer>
#include <QColor>


CDG::CDG()
{
    m_masks[0] = 0x20;
    m_masks[1] = 0x10;
    m_masks[2] = 0x08;
    m_masks[3] = 0x04;
    m_masks[4] = 0x02;
    m_masks[5] = 0x01;
    m_tempo = 100;
    reset();
}

bool CDG::open(QByteArray byteArray)
{
    reset();
    m_cdgData = byteArray;
    if (byteArray.size() == 0)
    {
        qWarning() << "CDG - Received zero bytes of CDG data";
        return false;
    }
    return true;
}

bool CDG::open(QString filename)
{
    QFile file(filename);
    file.open(QFile::ReadOnly);
    m_cdgData = file.readAll();
    file.close();
    return open(m_cdgData);
}

unsigned int CDG::position()
{
    float fpos = (m_position / 300.0) * 1000;
    return (int) fpos;
}

void CDG::reset()
{
    m_isOpen = false;
    m_needupdate = true;
    m_lastCDGCommandMS = 0;
    m_position = 0;
    m_cdgData = QByteArray();
    QVector<QRgb> palette;
    for (int i=0; i < 16; i++)
        palette.append(QColor(0,0,0).rgb());
    m_image = QImage(QSize(300,216),QImage::Format_Indexed8);
    m_image.setColorTable(palette);
    m_image.fill(0);
    m_frames.clear();
    m_skip.clear();
}

bool CDG::canSkipFrameByTime(unsigned int ms)
{
    int scaledMs = ms * ((float)m_tempo / 100.0);
    int frameno = scaledMs / 40;
    if (ms % 40 > 0) frameno++;
    if (frameno > m_frames.size())
        return false;
    bool skip = true;
    if (!m_skip.at(frameno - 1))
        skip = false;
    if (!m_skip.at(frameno))
        skip = false;
    if (!m_skip.at(frameno + 1))
        skip = false;
    return skip;
}

bool CDG::process()
{
    m_needupdate = true;
    bool retCode = true;
    CDG_SubCode subCode;
    int frameno = 0;
    QBuffer *ioDevice = new QBuffer(&m_cdgData);
    ioDevice->open(QIODevice::ReadOnly);
    while ((!ioDevice->atEnd()) && (ioDevice->isReadable()) && (ioDevice->size() > 0))
    {
        if (ioDevice->read((char *)&subCode, sizeof(subCode)) > 0)
        {
            m_needupdate = false;
            readCdgSubcodePacket(subCode);
            m_position++;
            if (((position() % 40) == 0) && position() >= 40)
            {
                if (m_needupdate)
                    m_lastCDGCommandMS = frameno * 40;
                m_skip.push_back(!m_needupdate);
                QVideoFrame frame(m_image.convertToFormat(QImage::Format_RGB32));
                frame.setStartTime(position());
                m_frames.push_back(frame);
                frameno++;
            }
        }
        else
        {
            qWarning() << "CDG - Error processing CDG data!";
            retCode = false;
        }
    }
    ioDevice->close();
    delete ioDevice;
    m_isOpen = true;
    return retCode;
}


void CDG::readCdgSubcodePacket(CDG_SubCode &subCode)
{
    if ((subCode.command & SC_MASK) != SC_CDG_COMMAND)
        return;
    switch (subCode.instruction & SC_MASK)
    {
    case CDG_MEMORYPRESET:
        cmdMemoryPreset(subCode.data);
        break;
    case CDG_BORDERPRESET:
        cmdBorderPreset(subCode.data);
        break;
    case CDG_TILEBLOCK:
        cmdTileBlock(subCode.data, false);
        break;
    case CDG_SCROLLPRESET:
        cmdScrollPreset(subCode.data);
        break;
    case CDG_SCROLLCOPY:
        cmdScrollCopy(subCode.data);
        break;
    case CDG_DEFINETRANS:
        cmdDefineTrans(subCode.data);
        break;
    case CDG_COLORSLOW:
        cmdColors(subCode.data, CDG_COLOR_TABLE_LOW);
        break;
    case CDG_COLORSHIGH:
        cmdColors(subCode.data, CDG_COLOR_TABLE_HIGH);
        break;
    case CDG_TILEBLOCKXOR:
        cmdTileBlock(subCode.data, true);
        break;
    }
}

void CDG::cmdBorderPreset(char data[16])
{
    CDG_Border_Preset preset;
    preset.color = (data[0] & 0x0F);
    if (preset.color > 15)
        return;
    // Top rows
    for (unsigned int y = 0; y < 12; y++)
    {
        for (unsigned int x=0; x < 300; x++)
            m_image.setPixel(x,y,preset.color);
    }
    // Bottom rows
    for (unsigned int y = 202; y < 216; y++)
    {
        for (unsigned int x=0; x < 300; x++)
            m_image.setPixel(x,y,preset.color);
    }
    // Sides
    for (unsigned int y = 11; y < 204; y++)
    {
        // Left
        for (unsigned int x = 0; x < 6; x++)
            m_image.setPixel(x,y,preset.color);
        // Right
        for (unsigned int x=294; x < 300; x++)
            m_image.setPixel(x,y,preset.color);
    }
    m_needupdate = true;
}

void CDG::cmdColors(char data[16], int Table)
{
    int colorIdx = 0;
    if (Table == CDG_COLOR_TABLE_HIGH)
        colorIdx = 8;
    for (int i=0; i < 15; i = i + 2)
    {
        char lowbyte = data[i];
        char highbyte = data[i + 1];
        int red = 0;
        int green = 0;
        int blue = 0;
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
        QRgb color = QColor(red * 17, green * 17, blue * 17).rgb();
        if (m_image.colorTable().at(colorIdx) != color)
        {
            m_image.setColor(colorIdx, color);
            m_needupdate = true;
        }
        colorIdx++;
    }
}

void CDG::cmdDefineTrans(char data[16])
{
    Q_UNUSED(data);
    // Unused CDG command from red book spec
}

void CDG::cmdMemoryPreset(char data[16])
{
    CDG_Memory_Preset preset;
    preset.color = (data[0] & 0x0F);
    if (preset.color > 15)
        return;
    m_image.fill(preset.color);
    m_needupdate = true;
}

void CDG::cmdScrollCopy(char data[16])
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
    m_needupdate = true;
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

void CDG::cmdScrollPreset(char data[16])
{
    CDG_Scroll_CMD scmd;
    scmd.color = (data[0] & 0x0F);
    scmd.hScroll = (data[1] & 0x3F);
    scmd.hSCmd = (scmd.hScroll & 0x30) >> 4;
    scmd.hSOffset = (scmd.hScroll & 0x07);
    scmd.vScroll = (data[2] & 0x3F);
    scmd.vSCmd = (scmd.vScroll & 0x30) >> 4;
    scmd.vSOffset = (scmd.vScroll & 0x07);
    m_needupdate = true;
//    qInfo() << "ScrollPreset command found";
//    qInfo() << "Color: " << scmd.color;
//    qInfo() << "hSCmd: " << scmd.hSCmd;
//    qInfo() << "hSOffset: " << scmd.hSOffset;
//    qInfo() << "vSCmd: " << scmd.vSCmd;
//    qInfo() << "vSOffset: " << scmd.vSOffset;
}

void CDG::cmdTileBlock(char data[16], bool XOR)
{
    CDG_Tile_Block tile;
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
    int top  = (tile.row    * 12);
    int left = (tile.column * 6);
    for (int i = 0; i <= 11; i++)
    {
        for (int j = 0; j <= 5; j++)
        {
            if (((top + i) <= 215) && ((left + j) <= 299) && (tile.color0 <= 15) && (tile.color1 <= 15))
            {
                int color = tile.color0;
                if ((tile.tilePixels[i] & m_masks[j]) > 0)
                    color = tile.color1;
                int idxnew;
                if (XOR)
                    idxnew = m_image.pixelIndex(left + j, top + i) ^ color;
                else
                    idxnew = color;
                int posx = left + j;
                int posy = top + i;
                m_image.setPixel(posx, posy, idxnew);
            }
        }
    }
    m_needupdate = true;
}

CDG::~CDG()
{

}

QVideoFrame CDG::videoFrameByTime(unsigned int ms)
{
    int scaledMs = ms * ((float)m_tempo / 100.0);
    int frameno = scaledMs / 40;
    if (ms % 40 > 0) frameno++;
    if (frameno > m_frames.size())
        return m_frames.at(m_frames.size() - 1);
    if (frameno < 0)
        return m_frames.at(0);
    return m_frames.at(frameno);
}

unsigned int CDG::duration()
{
    return m_cdgData.size() * 40;
}

bool CDG::isOpen() {
    return m_isOpen;
}

unsigned int CDG::lastCDGUpdate()
{
    return m_lastCDGCommandMS;
}

int CDG::tempo()
{
    return m_tempo;
}

void CDG::setTempo(int percent)
{
    m_tempo = percent;
}
