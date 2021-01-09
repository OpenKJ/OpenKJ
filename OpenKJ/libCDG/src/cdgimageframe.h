#ifndef CDGIMAGEFRAME_H
#define CDGIMAGEFRAME_H

#include <QImage>
#include <libCDG/include/libCDG.h>

class CdgImageFrame
{
public:
    CdgImageFrame();
    //~CdgImageFrame();

    // Modify image with subcode command. Return true if there are any visible changes to the image.
    bool applySubCode(const cdg::CDG_SubCode &subCode);

    std::array<uchar, cdg::CDG_IMAGE_SIZE> getCroppedImagedata();

private:
    QImage m_image;

    int m_bytesPerPixel;
    int m_borderLRBytes;
    int m_borderRBytesOffset;
    int m_curVOffset;
    int m_curHOffset;

    bool m_lastCmdWasMempreset {false};

    void cmdScroll(const cdg::CdgScrollCmdData &scrollCmdData, const cdg::ScrollType type);
    void cmdTileBlock(const cdg::CdgTileBlockData &tileBlockPacket, const cdg::TileBlockType &type);
    bool cmdMemoryPreset(const cdg::CdgMemoryPresetData &memoryPreset);
    void cmdBorderPreset(const cdg::CdgBorderPresetData &borderPreset);
    bool cmdColors(const cdg::CdgColorsData &data,const cdg::CdgColorTables &table);
    void cmdDefineTransparent(const std::array<char,16> &data);

};

#endif // CDGIMAGEFRAME_H
