#include "okarchive.h"
#include <QDebug>
#include <QFile>

OkArchive::OkArchive(QString ArchiveFile, QObject *parent) : QObject(parent)
{
    archiveFile = ArchiveFile;
}

OkArchive::OkArchive(QObject *parent) : QObject(parent)
{

}

int OkArchive::getSongDuration()
{
    KZip zip = KZip(archiveFile);
    zip.open(QIODevice::ReadOnly);
    for (int i=0; i < zip.directory()->entries().count(); i++)
    {
        QString filename = zip.directory()->entries().at(i);
      //  qCritical() << "Zip Contents: " << filename;
        if (filename.endsWith(".cdg", Qt::CaseInsensitive))
        {
            if (zip.directory()->entry(filename)->isFile())
            {
                qint64 size = zip.directory()->file(filename)->size();
                zip.close();
                return ((size / 96) / 75) * 1000;
            }
        }
    }
    zip.close();
    return -1;
}

QString OkArchive::getArchiveFile() const
{
    return archiveFile;
}

void OkArchive::setArchiveFile(const QString &value)
{
    archiveFile = value;
}
