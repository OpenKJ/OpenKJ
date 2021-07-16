#include "cdgfilereader.h"
#include <QFile>
#include <spdlog/spdlog.h>

constexpr int CDG_PACKAGES_PER_SECOND = 300;
constexpr int MAXFPS = 60;  // no need to go higher than 60 fps
constexpr int MIN_PACKAGES_BEFORE_NEW_FRAME = CDG_PACKAGES_PER_SECOND / MAXFPS;

CdgFileReader::CdgFileReader(const QString &filename)
{
    logger = spdlog::get("logger");
    // read entire cdg to memory
    QFile file(filename);
    file.open(QFile::ReadOnly);
    m_cdgData = file.readAll();
    rewind();
}

int CdgFileReader::getTotalDurationMS()
{
    return getDurationOfPackagesInMS(m_cdgData.length() / (int)sizeof (cdg::CDG_SubCode));
}

int CdgFileReader::positionOfFinalFrameMS()
{
    return isEOF() ? getDurationOfPackagesInMS(m_last_image_change_pgk_idx) : -1;
}

bool CdgFileReader::moveToNextFrame()
{
    if (m_current_image_pgk_idx == 0)
    {
        // Read until we have the very first frame
        while(!isEOF() && !readAndProcessNextPackage());
    }

    // shift m_next_image to current image
    m_next_image.copyCroppedImagedata(m_current_image_data.data());
    m_current_image_pgk_idx = m_next_image_pgk_idx;

    bool imageChanged = false;

    // process packages until a package actually change the image visibly (or eof).
    while(true)
    {
        // reached EOF?
        if (isEOF())
        {
            // Only return false ("this is the last image") when current image == next image
            return m_current_image_pgk_idx != m_next_image_pgk_idx;
        }

        // check if max FPS is met before returning
        if (imageChanged && (m_next_image_pgk_idx - m_current_image_pgk_idx) >= MIN_PACKAGES_BEFORE_NEW_FRAME)
        {
            return true;
        }

        if(readAndProcessNextPackage())
        {
            imageChanged = true;
            m_last_image_change_pgk_idx = m_next_image_pgk_idx;
        }
    }
}

int CdgFileReader::currentFrameDurationMS() const
{
    return getDurationOfPackagesInMS(m_next_image_pgk_idx - m_current_image_pgk_idx);
}

int CdgFileReader::currentFramePositionMS() const
{
    return getDurationOfPackagesInMS(m_current_image_pgk_idx);
}

bool CdgFileReader::seek(int positionMS)
{
    int pkgIdx = (positionMS * CDG_PACKAGES_PER_SECOND) / 1000;

    if (pkgIdx > m_cdgData.length() / (int)sizeof(cdg::CDG_SubCode))
    {
        logger->warn("CDG: Tried to seek past file size!", m_loggingPrefix);
        return false;
    }

    if (pkgIdx < m_current_image_pgk_idx)
    {
        logger->debug("CDG: Seek backwards - rewinding", m_loggingPrefix);
        rewind();
    }

    while (m_next_image_pgk_idx < pkgIdx)
    {
        readAndProcessNextPackage();
    }

    return true;
}

void CdgFileReader::rewind()
{
    m_cdgDataPos = 0;
    m_current_image_data.fill(0); // all black frame
    m_current_image_pgk_idx = 0;
    m_next_image = CdgImageFrame();
    m_next_image_pgk_idx = 0;
    m_last_image_change_pgk_idx = -1;
}

bool CdgFileReader::readAndProcessNextPackage()
{
    auto subCode = (cdg::CDG_SubCode*)(m_cdgData.constData() + m_cdgDataPos);
    m_cdgDataPos += sizeof(cdg::CDG_SubCode);
    m_next_image_pgk_idx++;
    return m_next_image.applySubCode(*subCode);
}

inline bool CdgFileReader::isEOF()
{
    return m_cdgDataPos + (int)sizeof(cdg::CDG_SubCode) > m_cdgData.length();
}

inline int CdgFileReader::getDurationOfPackagesInMS(const int numberOfPackages)
{
    return (numberOfPackages * 1000) / CDG_PACKAGES_PER_SECOND;
}

#ifdef QT_DEBUG

[[maybe_unused]] void CdgFileReader::saveNextImgToFile()
{
    auto fn = QString("/tmp/cdgimg/nxt-%1.png").arg(m_next_image_pgk_idx, 4, 10, QLatin1Char('0'));
    m_next_image.getImage().save(fn);
}

[[maybe_unused]] void CdgFileReader::saveCurrentImgToFile()
{
    auto fn = QString("/tmp/cdgimg/cur-%1-%2-%3.png")
            .arg(m_current_image_pgk_idx, 4, 10, QLatin1Char('0'))
            .arg(currentFramePositionMS(), 4, 10, QLatin1Char('0'))
            .arg(currentFrameDurationMS(), 4, 10, QLatin1Char('0'));
    QImage img = QImage(m_current_image_data.data(), cdg::FRAME_DIM_CROPPED.width(), cdg::FRAME_DIM_CROPPED.height(), QImage::Format_Indexed8);
    auto colors = QVector<QRgb>(1024 / sizeof(QRgb));
    memcpy(colors.data(), m_current_image_data.data() + cdg::CDG_IMAGE_SIZE - 1024, 1024);
    img.setColorTable(colors);
    img.save(fn);
}
#endif
