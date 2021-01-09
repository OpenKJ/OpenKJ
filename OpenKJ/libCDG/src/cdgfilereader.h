#ifndef CDGFILEREADER_H
#define CDGFILEREADER_H

#include <QString>
#include "cdgimageframe.h"
#include <libCDG/include/libCDG.h>

class CdgFileReader
{
public:
    CdgFileReader(const QString &filename);
    //~CdgFileReader();

    uint getTotalDurationMS();

    // todo: replace theese with seek-functions
    std::tuple<std::array<uchar, cdg::CDG_IMAGE_SIZE>, uint> videoFrameDataByIndex(const unsigned int frameidx );
    std::tuple<std::array<uchar, cdg::CDG_IMAGE_SIZE>, uint> videoFrameDataByTime(const uint ms);



private:
    void rewind();
    void readForward(const uint target_frame_idx);
    //void getDuration();

    inline static uint getDurationOfPackagesInMS(const uint numberOfPackages);

    QByteArray m_cdgData;
    unsigned int m_cdgDataPos;

    std::array<uchar, cdg::CDG_IMAGE_SIZE> m_current_image_data;
    unsigned int m_current_image_idx;

    CdgImageFrame m_next_image;
    unsigned int m_next_image_idx;

};

#endif // CDGFILEREADER_H
