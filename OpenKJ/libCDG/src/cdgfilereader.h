#ifndef CDGFILEREADER_H
#define CDGFILEREADER_H

#include <QString>
#include "cdgimageframe.h"
#include <libCDG/include/libCDG.h>

class CdgFileReader
{
public:
    CdgFileReader(const QString &filename);

    uint getTotalDurationMS();

    bool moveToNextFrame();

    std::array<uchar, cdg::CDG_IMAGE_SIZE> currentFrame() { return m_current_image_data; }
    uint currentFrameDurationMS();
    uint currentFramePositionMS();

    bool seek(uint positionMS);

private:
    void rewind();
    bool readAndProcessNextPackage();

    inline static uint getDurationOfPackagesInMS(const uint numberOfPackages);

    QByteArray m_cdgData;
    unsigned int m_cdgDataPos;

    std::array<uchar, cdg::CDG_IMAGE_SIZE> m_current_image_data;
    unsigned int m_current_image_pgk_idx;

    CdgImageFrame m_next_image;
    unsigned int m_next_image_pgk_idx;

};

#endif // CDGFILEREADER_H
