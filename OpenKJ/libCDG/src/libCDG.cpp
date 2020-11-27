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
    if (m_memoryCompressionLevel > 0)
        m_frameArraysComp.reserve(byteArray.size() / 24);
    else
        m_frameArrays.reserve(byteArray.size() / 24);
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
    m_needupdate = true;
    m_lastCmdWasMempreset = false;
    m_lastCDGCommandMS = 0;
    m_position = 0;
    m_curHOffset = 0;
    m_curVOffset = 0;
    m_cdgData = QByteArray();
    QVector<QRgb> palette;
    for (int i=0; i < 16; i++)
        palette.append(QColor(0,0,0).rgb());
    m_image = QImage(QSize(300,216),QImage::Format_Indexed8);
    m_bytesPerPixel = m_image.pixelFormat().bitsPerPixel() / 8;
    m_borderLRBytes = m_bytesPerPixel * 6;
    m_borderRBytesOffset = 294 * m_bytesPerPixel;
    m_image.setColorTable(palette);
    m_image.fill(0);
    m_frameArraysComp.clear();
    m_frameArraysComp.shrink_to_fit();
    m_frameArrays.clear();
    m_frameArrays.shrink_to_fit();
    m_tempo = 100;

    // Uncomment the following to help test for memory leaks,
    //m_frames.shrink_to_fit();
    //m_skip.shrink_to_fit();
}

bool CdgParser::process()
{
    qInfo() << "libCDG - Beginning processing of CDG data";
    auto t1 = std::chrono::high_resolution_clock::now();
    m_needupdate = false;
    cdg::CDG_SubCode subCode;
    int frameno = 0;
    QBuffer ioDevice(&m_cdgData);
    if (!ioDevice.open(QIODevice::ReadOnly))
        return false;
    while (ioDevice.read((char *)&subCode, sizeof(subCode)) > 0)
    {
        m_needupdate = false;
        readCdgSubcodePacket(subCode);
        if (m_needupdate)
            m_lastCDGCommandMS = frameno * 40;
        m_position++;
        if (((position() % 40) == 0) && position() >= 40)
        {
            if (m_memoryCompressionLevel > 0)
                m_frameArraysComp.emplace_back(qCompress(getSafeArea().convertToFormat(QImage::Format_RGB16).bits(),110592,1));
            else
            {
                std::array<uchar, 110592> frameArr;
                memcpy(frameArr.data(),getSafeArea().convertToFormat(QImage::Format_RGB16).bits(),110592);
                m_frameArrays.emplace_back(frameArr);
            }
            frameno++;
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


void CdgParser::readCdgSubcodePacket(const cdg::CDG_SubCode &subCode)
{
    if ((subCode.command & m_subcodeMask) != m_subcodeCommand)
        return;
    switch (subCode.instruction & m_subcodeMask)
    {
    case cdg::CmdMemoryPreset:
        cmdMemoryPreset(cdg::CdgMemoryPresetData(subCode.data));
        m_lastCmdWasMempreset = true;
        break;
    case cdg::CmdBorderPreset:
        cmdBorderPreset(cdg::CdgBorderPresetData(subCode.data));
        break;
    case cdg::CmdTileBlock:
        cmdTileBlock(cdg::CdgTileBlockData(subCode.data), cdg::TileBlockNormal);
        break;
    case cdg::CmdScrollPreset:
        cmdScroll(subCode.data, cdg::ScrollPreset);
        break;
    case cdg::CmdScrollCopy:
        cmdScroll(subCode.data, cdg::ScrollCopy);
        break;
    case cdg::CmdDefineTrans:
        cmdDefineTransparent(subCode.data);
        break;
    case cdg::CmdColorsLow:
        cmdColors(cdg::CdgColorsData(subCode.data), cdg::LowColors);
        break;
    case cdg::CmdColorsHigh:
        cmdColors(cdg::CdgColorsData(subCode.data), cdg::HighColors);
        break;
    case cdg::CmdTileBlockXOR:
        cmdTileBlock(cdg::CdgTileBlockData(subCode.data), cdg::TileBlockXOR);
        break;
    }
    m_lastCmdWasMempreset = (subCode.instruction == cdg::CmdMemoryPreset);
}

void CdgParser::cmdBorderPreset(const cdg::CdgBorderPresetData &borderPreset)
{
    // Is there a safer C++ way to do these memory copies?

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
    m_needupdate = true;
}

void CdgParser::cmdColors(const cdg::CdgColorsData &data, const cdg::CdgColorTables &table)
{
    int curColor = (table == cdg::HighColors) ? 8 : 0;
    std::for_each(data.colors.begin(), data.colors.end(), [&] (auto color) {
        if (m_image.colorTable().at(curColor) != color.rgb())
        {
            m_image.setColor(curColor, color.rgb());
            m_needupdate = true;
        }
        curColor++;
    });
}

QImage CdgParser::getSafeArea()
{

    QImage image(QSize(288,192),QImage::Format_Indexed8);
    image.setColorTable(m_image.colorTable());
    for (auto i=0; i < 192; i++)
    {
        auto curSrcLine = i + m_curVOffset;
        auto srcLineOffset = m_image.bytesPerLine() * (12 + curSrcLine);
        auto dstLineOffset = image.bytesPerLine() * i;
        auto copiedLineSize = 288 * m_bytesPerPixel;
        auto srcBits = m_image.bits();
        auto dstBits = image.bits();
        memcpy(dstBits + dstLineOffset, srcBits + srcLineOffset + m_borderLRBytes + (m_curHOffset * m_bytesPerPixel), copiedLineSize);

    }
    return image;
}



void CdgParser::cmdMemoryPreset(const cdg::CdgMemoryPresetData &memoryPreset)
{
    if (m_lastCmdWasMempreset && memoryPreset.repeat)
    {
        return;
    }
    m_image.fill(memoryPreset.color);
    m_needupdate = true;
}



void CdgParser::cmdTileBlock(const cdg::CdgTileBlockData &tileBlockPacket, const cdg::TileBlockType &type)
{
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
    m_needupdate = true;
}



QString CdgParser::md5HashByTime(const unsigned int ms)
{
    size_t frameno = ms / 40;
    auto size = (m_memoryCompressionLevel > 0) ? m_frameArraysComp.size() : m_frameArrays.size();
    if (ms % 40 > 0) frameno++;
    if (frameno > size)
        frameno = size - 1;
    QByteArray arr;
    if (m_memoryCompressionLevel > 0)
        arr = QByteArray::fromRawData((const char*)qUncompress(m_frameArraysComp.at(frameno)).data(), m_frameArraysComp.at(frameno).size());
    else
        arr = QByteArray::fromRawData((const char*)m_frameArrays.at(frameno).data(), m_frameArrays.at(frameno).size());
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
    auto count = (m_memoryCompressionLevel > 0) ? m_frameArraysComp.size() : m_frameArrays.size();
    if (m_tempo == 100)
        retSize = count;
    if (m_tempo < 100)
        retSize = (count / ((float)m_tempo / 100.0)) + 10;
    if (m_tempo > 100)
        retSize = (count / ((float)m_tempo / 100.0)) + 10;
    return retSize;
}

std::array<uchar, 110592> CdgParser::videoFrameDataByTime(const unsigned int ms)
{
    return videoFrameDataByIndex((ms * ((float)m_tempo / 100.0)) / 40);
}

std::array<uchar, 110592> CdgParser::videoFrameDataByIndex(const size_t frame)
{
    if ((m_memoryCompressionLevel > 0 && frame >= m_frameArraysComp.size()) || (m_memoryCompressionLevel == 0 && frame >= m_frameArrays.size()))
    {
        return blank;
    }
    if (m_memoryCompressionLevel > 0)
    {
        std::array<uchar, 110592> frameArr;
        memcpy(frameArr.data(),qUncompress(m_frameArraysComp.at(frame)).data(), 110592);
        return frameArr;
    }
    return m_frameArrays.at(frame);
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
    m_needupdate = true;

}

void CdgParser::cmdDefineTransparent([[maybe_unused]] const std::array<char,16> &data)
{
    //qInfo() << "libCDG - unsupported DefineTransparent command called";
    // Unused CDG command from redbook spec
    // This is rarely if ever used
    // No idea what the data structure is, it's missing from CDG Revealed
}
