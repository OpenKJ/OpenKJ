#include "cdgimageframe.h"


CdgImageFrame::CdgImageFrame()
{
    m_image = QImage(cdg::FRAME_DIM_FULL, QImage::Format_Indexed8);

    m_curHOffset = 0;
    m_curVOffset = 0;
    QVector<QRgb> palette;
    for (int i=0; i < 16; i++)
        palette.append(QColor(0,0,0).rgb());
    m_image = QImage(cdg::FRAME_DIM_FULL, QImage::Format_Indexed8);
    m_bytesPerPixel = m_image.pixelFormat().bitsPerPixel() / 8;
    m_borderLRBytes = m_bytesPerPixel * 6;
    m_borderRBytesOffset = 294 * m_bytesPerPixel;
    m_image.setColorTable(palette);
    m_image.fill(0);
}

bool CdgImageFrame::applySubCode(const cdg::CDG_SubCode &subCode)
{
    constexpr static char SUBCODE_MASK = 0x3F;
    constexpr static char SUBCODE_COMMAND = 0x09;

    if ((subCode.command & SUBCODE_MASK) != SUBCODE_COMMAND)
        return false;

    bool updated{false};
    switch (subCode.instruction & SUBCODE_MASK)
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

std::array<uchar, cdg::CDG_IMAGE_SIZE> CdgImageFrame::getCroppedImagedata()
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

void CdgImageFrame::cmdBorderPreset(const cdg::CdgBorderPresetData &borderPreset)
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
}

bool CdgImageFrame::cmdColors(const cdg::CdgColorsData &data, const cdg::CdgColorTables &table)
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


bool CdgImageFrame::cmdMemoryPreset(const cdg::CdgMemoryPresetData &memoryPreset)
{
    if (m_lastCmdWasMempreset && memoryPreset.repeat)
    {
        return false;
    }
    m_image.fill(memoryPreset.color);
    return true;
}


void CdgImageFrame::cmdTileBlock(const cdg::CdgTileBlockData &tileBlockPacket, const cdg::TileBlockType &type)
{
    constexpr static std::array<char,6> MASKS{0x20,0x10,0x08,0x04,0x02,0x01};

    // There's probably a better way to do this, needs research
    for (auto y = 0; y < 12; y++)
    {
        auto ptr = m_image.scanLine(y + tileBlockPacket.top);
        auto rowData = tileBlockPacket.tilePixels[y];
        switch (type) {
        case cdg::TileBlockXOR:
            *(ptr + (tileBlockPacket.left * m_bytesPerPixel))       ^= (rowData & MASKS[0]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 1) * m_bytesPerPixel)) ^= (rowData & MASKS[1]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 2) * m_bytesPerPixel)) ^= (rowData & MASKS[2]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 3) * m_bytesPerPixel)) ^= (rowData & MASKS[3]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 4) * m_bytesPerPixel)) ^= (rowData & MASKS[4]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 5) * m_bytesPerPixel)) ^= (rowData & MASKS[5]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            break;
        case cdg::TileBlockNormal:
            *(ptr + (tileBlockPacket.left * m_bytesPerPixel))       = (rowData & MASKS[0]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 1) * m_bytesPerPixel)) = (rowData & MASKS[1]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 2) * m_bytesPerPixel)) = (rowData & MASKS[2]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 3) * m_bytesPerPixel)) = (rowData & MASKS[3]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 4) * m_bytesPerPixel)) = (rowData & MASKS[4]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            *(ptr + ((tileBlockPacket.left + 5) * m_bytesPerPixel)) = (rowData & MASKS[5]) ? tileBlockPacket.color1 : tileBlockPacket.color0;
            break;
        }
    }
}

void CdgImageFrame::cmdScroll(const cdg::CdgScrollCmdData &scrollCmdData, const cdg::ScrollType type)
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


void CdgImageFrame::cmdDefineTransparent([[maybe_unused]] const std::array<char,16> &data)
{
    //qInfo() << "libCDG - unsupported DefineTransparent command called";
    // Unused CDG command from redbook spec
    // This is rarely if ever used
    // No idea what the data structure is, it's missing from CDG Revealed
}

