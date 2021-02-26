#include "cdgfilereader.h"
#include <QFile>
#include <QDebug>


constexpr uint CDG_PACKAGES_PER_SECOND = 300;
constexpr int MAXFPS = 60;  // no need to go higher than 60 fps
constexpr int MIN_PACKAGES_BEFORE_NEW_FRAME = CDG_PACKAGES_PER_SECOND / MAXFPS;

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

int CdgFileReader::positionOfFinalFrameMS()
{
    return isEOF() ? currentFramePositionMS() : -1;
}

bool CdgFileReader::moveToNextFrame()
{
    // shift m_next_image to current image
    m_current_image_data = m_next_image.getCroppedImagedata();
    m_current_image_pgk_idx = m_next_image_pgk_idx;

    bool imageChanged = false;

    // process packages until a package actually change the image visibly (or eof).
    while(true)
    {
        // reached EOF?
        if (isEOF())
        {
            return false;
        }

        // check if max FPS is met before returning
        if (imageChanged && (m_next_image_pgk_idx - m_current_image_pgk_idx) >= MIN_PACKAGES_BEFORE_NEW_FRAME)
        {
            return true;
        }
        imageChanged |= readAndProcessNextPackage();
    }
}

uint CdgFileReader::currentFrameDurationMS()
{
    return getDurationOfPackagesInMS(m_next_image_pgk_idx - m_current_image_pgk_idx);
}

uint CdgFileReader::currentFramePositionMS()
{
    return getDurationOfPackagesInMS(m_current_image_pgk_idx);
}

bool CdgFileReader::seek(uint positionMS)
{
    uint pkgIdx = (positionMS * CDG_PACKAGES_PER_SECOND) / 1000;

    if (pkgIdx > m_cdgData.length() / sizeof (cdg::CDG_SubCode))
    {
        qWarning() << "CDG: Tried to seek past file size!";
        return false;
    }

    if (pkgIdx < m_current_image_pgk_idx)
    {
        qDebug() << "CDG: Seek backwards - rewinding";
        rewind();
    }

    while (m_next_image_pgk_idx < pkgIdx)
    {
        readAndProcessNextPackage();
    }

    moveToNextFrame();
    return true;
}

void CdgFileReader::rewind()
{
    m_cdgDataPos = 0;
    m_current_image_data.fill(0); // all black frame
    m_current_image_pgk_idx = 0;
    m_next_image = CdgImageFrame();
    m_next_image_pgk_idx = 0;
    moveToNextFrame();
}

bool CdgFileReader::readAndProcessNextPackage()
{
    cdg::CDG_SubCode* subCode = (cdg::CDG_SubCode*)(m_cdgData.constData() + m_cdgDataPos);
    m_cdgDataPos += sizeof(cdg::CDG_SubCode);

    m_next_image_pgk_idx++;
    return m_next_image.applySubCode(*subCode);
}

inline bool CdgFileReader::isEOF()
{
    return m_cdgDataPos + sizeof(cdg::CDG_SubCode) > m_cdgData.length();
}

inline uint CdgFileReader::getDurationOfPackagesInMS(const uint numberOfPackages)
{
    return (numberOfPackages * 1000) / CDG_PACKAGES_PER_SECOND;
}
