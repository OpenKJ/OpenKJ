#include "cdgfilereader.h"
#include <QFile>
#include <QDebug>

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

bool CdgFileReader::moveToNextFrame()
{
    // shift m_next_image to current image
    m_current_image_data = m_next_image.getCroppedImagedata();
    m_current_image_pgk_idx = m_next_image_pgk_idx;

    while(true)
    {
        // reached EOF?
        if (m_cdgDataPos + sizeof(cdg::CDG_SubCode) > m_cdgData.length())
        {
            return false;
        }

        if (readAndProcessNextPackage())
        {
            return true;
        }
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

    // todo: what happens if seek is done in the middle of a buffer-fill-loop?

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


uint CdgFileReader::getDurationOfPackagesInMS(const uint numberOfPackages)
{
    // cdg specs: 300 packages per second
    return (numberOfPackages * 1000) / CDG_PACKAGES_PER_SECOND;
}
