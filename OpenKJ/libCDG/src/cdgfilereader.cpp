#include "cdgfilereader.h"
#include <QFile>

constexpr uint CDG_PACKAGES_PER_SECOND = 300;

CdgFileReader::CdgFileReader(const QString &filename)
{
    // read entire cdg to memory
    QFile file(filename);
    file.open(QFile::ReadOnly);
    m_cdgData = file.readAll();

    rewind();
}

uint CdgFileReader::getTotalDurationMS()
{
    return getDurationOfPackagesInMS(m_cdgData.length() / sizeof (cdg::CDG_SubCode));
}

std::tuple<std::array<uchar, cdg::CDG_IMAGE_SIZE>, uint> CdgFileReader::videoFrameDataByIndex(const unsigned int frameidx)
{
    if (m_current_image_idx > frameidx)
        rewind();

    readForward(frameidx);

    uint msUntilNextFrame = getDurationOfPackagesInMS(m_next_image_idx - m_current_image_idx);

    return std::tuple<std::array<uchar, cdg::CDG_IMAGE_SIZE>, uint>(m_current_image_data, msUntilNextFrame);
}

std::tuple<std::array<uchar, cdg::CDG_IMAGE_SIZE>, uint> CdgFileReader::videoFrameDataByTime(const uint ms)
{
    return videoFrameDataByIndex((ms * CDG_PACKAGES_PER_SECOND) / 1000);
}

void CdgFileReader::rewind()
{
    m_cdgDataPos = 0;
    m_current_image_data.fill(0); // all black frame
    m_current_image_idx = 0;
    m_next_image = CdgImageFrame();
    m_next_image_idx = 0;
}

void CdgFileReader::readForward(const uint target_frame_idx)
{
    // Read until the next changed frame after frameidx
    while (m_next_image_idx <= target_frame_idx)
    {
        while(true)
        {
            if (m_cdgDataPos + sizeof(cdg::CDG_SubCode) >= m_cdgData.length()) // todo: > or >= ?
            {
                // EOF
                return;
            }

            cdg::CDG_SubCode* subCode = (cdg::CDG_SubCode*)(m_cdgData.constData() + m_cdgDataPos);
            m_cdgDataPos += sizeof(cdg::CDG_SubCode);

            m_next_image_idx++;
            if (m_next_image.applySubCode(*subCode))
            {
                // image has changed!


                // todo: probably not correct...
                if (target_frame_idx >= m_next_image_idx)
                {
                    m_current_image_data = m_next_image.getCroppedImagedata(); // todo: perhaps optimize by creating a copy of qimage instead...
                    m_current_image_idx = m_next_image_idx;
                }
                break;
            }

        }
    }
}

uint CdgFileReader::getDurationOfPackagesInMS(const uint numberOfPackages)
{
    // cdg specs: 300 packages per second
    return (numberOfPackages * 1000) / CDG_PACKAGES_PER_SECOND;
}
