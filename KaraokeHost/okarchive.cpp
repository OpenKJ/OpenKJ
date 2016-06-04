#include "okarchive.h"
#include <QDebug>
#include <QFile>
#include <QBuffer>


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

QByteArray OkArchive::getCDGData()
{
    KZip zip = KZip(archiveFile);
    QByteArray data;
    zip.open(QIODevice::ReadOnly);
    for (int i=0; i < zip.directory()->entries().count(); i++)
    {
        QString filename = zip.directory()->entries().at(i);
        if (filename.endsWith(".cdg", Qt::CaseInsensitive))
        {
            if (zip.directory()->entry(filename)->isFile())
            {
                return zip.directory()->file(filename)->data();
            }
            else
                qCritical() << "Error opening CDG IODevice!";
        }
    }
    return data;
}

QString OkArchive::getArchiveFile() const
{
    return archiveFile;
}

void OkArchive::setArchiveFile(const QString &value)
{
    archiveFile = value;
}
