#include "okarchive.h"
#include <QDebug>

OkArchive::OkArchive(QString ArchiveFile, QObject *parent)
{
    KZip zip = KZip(ArchiveFile);
    zip.open(QIODevice::ReadOnly);
    qDebug() << "Zip contents:" << zip.directory()->entries();
}

OkArchive::OkArchive(QObject *parent) : QObject(parent)
{

}
