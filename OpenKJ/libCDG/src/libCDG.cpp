/*
 * Copyright (c) 2013-2020 Thomas Isaac Lightburn
 *
 *
 * This file is part of OpenKJ.
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
#include <QCryptographicHash>
#include <chrono>

CdgParser::CdgParser()
{
    blank.fill(0);
    reset();
}

bool CdgParser::open(const QByteArray &byteArray, const bool &bypassReset)
{
    //qInfo() << "libCDG - Opening byte array for processing";
    if (!bypassReset)
        reset();
    m_cdgData = byteArray;
    if (byteArray.size() == 0)
    {
        qWarning() << "libCDG - Received zero bytes of CDG data";
        return false;
    }
    //qInfo() << "libCDG - Byte array opened successfully";
    //m_frameArrays.reserve(byteArray.size() / 24);
    return true;
}

bool CdgParser::open(const QString &filename)
{
    qInfo() << "libCDG - Opening file: " << filename;
    reset();
    QFile file(filename);
    file.open(QFile::ReadOnly);
    m_cdgData = file.readAll();
    file.close();
    return open(m_cdgData, true);
}

unsigned int CdgParser::position()
{
    float fpos = (m_position / 300.0) * 1000;
    return (int) fpos;
}

void CdgParser::reset()
{
    qDebug() << "libCDG - CDG::reset() called, freeing memory and setting isOpen to false";
    m_isOpen = false;
    m_lastCmdWasMempreset = false;
    m_lastCDGCommandMS = 0;
    m_position = 0;
    m_curHOffset = 0;
    m_curVOffset = 0;
    m_cdgData = QByteArray();
    QVector<QRgb> palette;
    for (int i=0; i < 16; i++)
        palette.append(QColor(0,0,0).rgb());
    m_image = QImage(cdg::FRAME_DIM_FULL, QImage::Format_Indexed8);
    m_bytesPerPixel = m_image.pixelFormat().bitsPerPixel() / 8;
    m_borderLRBytes = m_bytesPerPixel * 6;
    m_borderRBytesOffset = 294 * m_bytesPerPixel;
    m_image.setColorTable(palette);
    m_image.fill(0);
    m_frameArrays.clear();
    m_frameLookupTable.clear();
    m_frameArrays.shrink_to_fit();
    m_frameLookupTable.shrink_to_fit();
    m_tempo = 100;

    // Uncomment the following to help test for memory leaks,
    //m_frames.shrink_to_fit();
    //m_skip.shrink_to_fit();
}

bool CdgParser::process()
{
    qInfo() << "libCDG - Beginning processing of CDG data";
    auto t1 = std::chrono::high_resolution_clock::now();
    cdg::CDG_SubCode subCode;
    int frameno = 0;
    bool needUpdate{true};
    QBuffer ioDevice(&m_cdgData);
    if (!ioDevice.open(QIODevice::ReadOnly))
        return false;
    while (ioDevice.read((char *)&subCode, sizeof(subCode)) > 0)
    {
        if (readCdgSubcodePacket(subCode))
            needUpdate = true;
        if (needUpdate)
            m_lastCDGCommandMS = frameno * 40;
        m_position++;
        if (((position() % 40) == 0) && position() >= 40)
        {
            if (needUpdate)
            {
                m_frameArrays.emplace_back(getCroppedImagedata());
            }

            if (m_frameArrays.size() > 0)
                m_frameLookupTable.emplace_back(m_frameArrays.size() - 1);
            else
                m_frameLookupTable.emplace_back(0);
            frameno++;
            needUpdate = false;
        }
    }

    ioDevice.close();
    m_cdgData.clear();
    m_isOpen = true;
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();
    qInfo() << "libCDG - Processed CDG file in " << duration << "ms";
    return true;
}

std::array<uchar, cdg::CDG_IMAGE_SIZE> CdgParser::getCroppedImagedata()
{
    uchar* src = m_image.bits();

    std::array<uchar, cdg::CDG_IMAGE_SIZE> cropped;
    uchar* croppedpos = cropped.data();

    for (auto y=0; y < cdg::FRAME_DIM_CROPPED.height(); y++)
    {
        auto curSrcLineNum = y + m_curVOffset;
        auto srcLineOffset = m_image.bytesPerLine() * (12 + curSrcLineNum); // 12?

        memcpy(croppedpos, src + srcLineOffset + m_borderLRBytes + m_curHOffset, cdg::FRAME_DIM_CROPPED.width());
        croppedpos += cdg::FRAME_DIM_CROPPED.width();
    }

    for(int i=0; i<m_image.colorCount(); i++)
    {
        QRgb color = m_image.color(i);
        memcpy(croppedpos, &color, sizeof(uint));
        croppedpos += sizeof(uint);
    }
    return cropped;
}

bool CdgParser::readCdgSubcodePacket(const cdg::CDG_SubCode &subCode)
{
    if ((subCode.command & m_subcodeMask) != m_subcodeCommand)
        return false;

    bool updated{false};
    switch (subCode.instruction & m_subcodeMask)
    {
    case cdg::CmdMemoryPreset:
        updated = cmdMemoryPreset(cdg::CdgMemoryPresetData(subCode.data));
        break;
    case cdg::CmdBorderPreset:
        cmdBorderPreset(cdg::CdgBorderPresetData(subCode.data));
        updated = true;
        break;
    case cdg::CmdTileBlock:
        cmdTileBlock(cdg::CdgTileBlockData(subCode.data), cdg::TileBlockNormal);
        updated = true;
        break;
    case cdg::CmdScrollPreset:
        cmdScroll(subCode.data, cdg::ScrollPreset);
        updated = true;
        break;
    case cdg::CmdScrollCopy:
        cmdScroll(subCode.data, cdg::ScrollCopy);
        updated = true;
        break;
    case cdg::CmdDefineTrans:
        cmdDefineTransparent(subCode.data);
        break;
    case cdg::CmdColorsLow:
        updated = cmdColors(cdg::CdgColorsData(subCode.data), cdg::LowColors);
        break;
    case cdg::CmdColorsHigh:
        updated = cmdColors(cdg::CdgColorsData(subCode.data), cdg::HighColors);
        break;
    case cdg::CmdTileBlockXOR:
        cmdTileBlock(cdg::CdgTileBlockData(subCode.data), cdg::TileBlockXOR);
        updated = true;
        break;
    }
    m_lastCmdWasMempreset = (subCode.instruction == cdg::CmdMemoryPreset);
    return updated;
}

void CdgParser::cmdBorderPreset(const cdg::CdgBorderPresetData &borderPreset)
{
    // reject out of range value from corrupted CDG packets
    if (borderPreset.color >= 16)
        return false;
    // Is there a safer C++ way to do these memory copies?
    if (
    for (auto line=0; line < 216; line++)
    {
        if (line < 12 || line > 202)
            memset(m_image.scanLine(line), borderPreset.color, m_image.bytesPerLine());
        else
        {
            memset(m_image.scanLine(line), borderPreset.color, m_borderLRBytes);
            memset(m_image.scanLine(line) + m_borderRBytesOffset, borderPreset.color, m_borderLRBytes);
        }
    }
}

bool CdgParser::cmdColors(const cdg::CdgColorsData &data, const cdg::CdgColorTables &table)
{
    bool changed{false};
    int curColor = (table == cdg::HighColors) ? 8 : 0;
    std::for_each(data.colors.begin(), data.colors.end(), [&] (auto color) {
        if (m_image.colorTable().at(curColor) != color.rgb())
        {
            changed = true;
            m_image.setColor(curColor, color.rgb());
        }
        curColor++;
    });
    return changed;
}


bool CdgParser::cmdMemoryPreset(const cdg::CdgMemoryPresetData &memoryPreset)
{
    // reject out of range value from corrupted CDG packets
    if (memoryPreset.color >= 16)
        return false;
    if (m_lastCmdWasMempreset && memoryPreset.repeat)
    {
        return false;
    }
    m_image.fill(memoryPreset.color);
    return true;
}



void CdgParser::cmdTileBlock(const cdg::CdgTileBlockData &tileBlockPacket, const cdg::TileBlockType &type)
{
    
    // reject corrupted CDG packets w/ invalid row/column
    if (tileBlockPacket.row >= 18 || tileBlockPacket.column >= 50 || tileBlockPacket.color0 >= 16 || tileBlockPacket.color1 >= 16)
        return;
    
    // There's probably a better way to do this, needs research
    for (auto y = 0; y < 12; y++)
    {
        auto ptr = m_image.scanLine(y + tileBlockPacket.top);
        auto rowData = tileBlockPacket.tilePixels[y];
        switch (type) {
        case cdg::TileBlockXOR:
            *(ptr + (tileBlockPacket.left * m_bytesPerPixel))       ^= (rowData & m_masks[0]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 1) * m_bytesPerPixel)) ^= (rowData & m_masks[1]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 2) * m_bytesPerPixel)) ^= (rowData & m_masks[2]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 3) * m_bytesPerPixel)) ^= (rowData & m_masks[3]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 4) * m_bytesPerPixel)) ^= (rowData & m_masks[4]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 5) * m_bytesPerPixel)) ^= (rowData & m_masks[5]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            break;
        case cdg::TileBlockNormal:
            *(ptr + (tileBlockPacket.left * m_bytesPerPixel))       = (rowData & m_masks[0]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 1) * m_bytesPerPixel)) = (rowData & m_masks[1]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 2) * m_bytesPerPixel)) = (rowData & m_masks[2]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 3) * m_bytesPerPixel)) = (rowData & m_masks[3]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 4) * m_bytesPerPixel)) = (rowData & m_masks[4]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 5) * m_bytesPerPixel)) = (rowData & m_masks[5]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            break;
        }
    }
}



QString CdgParser::md5HashByTime(const unsigned int ms)
{
    size_t frameno = ms / 40;
    auto size = m_frameArrays.size();
    if (ms % 40 > 0) frameno++;
    if (frameno > size)
        frameno = size - 1;
    QByteArray arr = QByteArray::fromRawData((const char*)m_frameArrays.at(frameno).data(), m_frameArrays.at(frameno).size());
    return QString(QCryptographicHash::hash(arr, QCryptographicHash::Md5).toHex());
}

unsigned int CdgParser::duration()
{
    return m_cdgData.size() * 40;
}

bool CdgParser::isOpen()
{
    return m_isOpen;
}

unsigned int CdgParser::lastCDGUpdate()
{
    return m_lastCDGCommandMS;
}

int CdgParser::tempo()
{
    return m_tempo;
}

void CdgParser::setTempo(const int percent)
{
    m_tempo = percent;
}

std::size_t CdgParser::getFrameCount() {
    auto retSize = 0;
    auto count = m_frameLookupTable.size();
    if (m_tempo == 100)
        retSize = count;
    if (m_tempo < 100)
        retSize = (count / ((float)m_tempo / 100.0)) + 10;
    if (m_tempo > 100)
        retSize = (count / ((float)m_tempo / 100.0)) + 10;
    return retSize;
}

std::array<uchar, cdg::CDG_IMAGE_SIZE> CdgParser::videoFrameDataByTime(const unsigned int ms)
{
    return videoFrameDataByIndex((ms * ((float)m_tempo / 100.0)) / 40);
}

std::array<uchar, cdg::CDG_IMAGE_SIZE> CdgParser::videoFrameDataByIndex(const size_t frame)
{
    if (frame >= m_frameLookupTable.size())
        return blank;
    if (m_frameLookupTable.at(frame) >= m_frameArrays.size())
        return blank;
    return m_frameArrays.at(m_frameLookupTable.at(frame));
}

void CdgParser::cmdScroll(const cdg::CdgScrollCmdData &scrollCmdData, const cdg::ScrollType type)
{
    if (scrollCmdData.hSCmd == 2)
    {
        // scroll left 6px
        for (auto i=0; i < 216; i++)
        {
            auto bits = m_image.scanLine(i);
            unsigned char* tmpPixels[6];
            memcpy(tmpPixels, bits, 6);
            memcpy(bits, bits + (6 * m_bytesPerPixel), 294 * m_bytesPerPixel);
            if (type == cdg::ScrollCopy)
                memcpy(bits + m_borderRBytesOffset, tmpPixels, 6);
            else
                memset(bits + m_borderLRBytes, scrollCmdData.color, 6);
        }
    }
    if (scrollCmdData.hSCmd == 1)
    {
        // scroll right 6px
        for (auto i=0; i < 216; i++)
        {
            auto bits = m_image.scanLine(i);
            unsigned char* tmpPixels[6];
            memcpy(tmpPixels, bits + (m_bytesPerPixel * 294), 6);
            memcpy(bits + (6 * m_bytesPerPixel), bits , 294 * m_bytesPerPixel);
            if (type == cdg::ScrollCopy)
                memcpy(bits, tmpPixels, 6);
            else
                memset(bits, scrollCmdData.color, 6);
        }
    }
    if (scrollCmdData.vSCmd == 2)
    {
        // scroll up 12px
        auto bits = m_image.bits();
        unsigned char* tmpLines[3600]; // m_image.bytesPerLine() * 12
        memcpy(tmpLines, bits, m_image.bytesPerLine() * 12);
        memcpy(bits, bits + m_image.bytesPerLine() * 12, 204 * m_image.bytesPerLine());
        if (type == cdg::ScrollCopy)
            memcpy(bits + (204 * m_image.bytesPerLine()), tmpLines, m_image.bytesPerLine() * 12);
        else
            memset(bits + (204 * m_image.bytesPerLine()), scrollCmdData.color, m_image.bytesPerLine() * 12);
    }
    if (scrollCmdData.vSCmd == 1)
    {
        // scroll down 12px
        auto bits = m_image.bits();
        unsigned char* tmpLines[3600];
        memcpy(tmpLines, bits + (m_image.bytesPerLine() * 204), m_image.bytesPerLine() * 12);
        memcpy(bits + (m_image.bytesPerLine() * 12), bits, 204 * m_image.bytesPerLine());
        if (type == cdg::ScrollCopy)
            memcpy(bits, tmpLines, m_image.bytesPerLine() * 12);
        else
            memset(bits, scrollCmdData.color, m_image.bytesPerLine() * 12);
    }
    if (m_curVOffset != scrollCmdData.vSOffset)
    m_curHOffset = scrollCmdData.hSOffset;
    m_curVOffset = scrollCmdData.vSOffset;

}

void CdgParser::cmdDefineTransparent([[maybe_unused]] const std::array<char,16> &data)
{
    //qInfo() << "libCDG - unsupported DefineTransparent command called";
    // Unused CDG command from redbook spec
    // This is rarely if ever used
    // No idea what the data structure is, it's missing from CDG Revealed
}
