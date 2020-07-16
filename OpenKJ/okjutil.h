#ifndef OKJUTIL_H
#define OKJUTIL_H
#include <QString>
#include <QFileInfo>
#include <QDirIterator>


inline QString findMatchingAudioFile(const QString &cdgFilePath)
{
    QStringList audioExtensions;
    audioExtensions.append("mp3");
    audioExtensions.append("wav");
    audioExtensions.append("ogg");
    audioExtensions.append("mov");
    audioExtensions.append("flac");
    QFileInfo cdgInfo(cdgFilePath);
    QDir srcDir = cdgInfo.absoluteDir();
    QDirIterator it(srcDir);
    while (it.hasNext())
    {
        it.next();
        if (it.fileInfo().completeBaseName() != cdgInfo.completeBaseName())
            continue;
        if (it.fileInfo().suffix().toLower() == "cdg")
            continue;
        QString ext;
        foreach (ext, audioExtensions)
        {
            if (it.fileInfo().suffix().toLower() == ext)
            {
                return it.filePath();
            }
        }
    }
    return QString();
}






#endif // OKJUTIL_H
