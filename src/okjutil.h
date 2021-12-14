#ifndef OKJUTIL_H
#define OKJUTIL_H
#include <QString>
#include <QFileInfo>
#include <QDirIterator>

// Given a cdg file path, tries to find a matching supported audio file
// Returns an empty QString if no match is found
// Optimized for finding most common file extensions first
inline QString findMatchingAudioFile(const QString &cdgFilePath) {
    std::array<QString, 41> audioExtensions{
        "mp3",
        "MP3",
        "wav",
        "WAV",
        "ogg",
        "OGG",
        "mov",
        "MOV",
        "flac",
        "FLAC",
        "Mp3","mP3",
        "Wav","wAv","waV","WAv","wAV","WaV",
        "Ogg","oGg","ogG","OGg","oGG","OgG",
        "Mov","mOv","moV","MOv","mOV","MoV",
        "Flac","fLac","flAc","flaC","FLac","FLAc",
        "flAC","fLAC","FlaC", "FLaC", "FlAC"
    };

    QFileInfo cdgInfo(cdgFilePath);
    for (const auto &ext : audioExtensions) {
        QString testPath = cdgInfo.absolutePath() + QDir::separator() + cdgInfo.completeBaseName() + '.' + ext;
        if (QFile::exists(testPath))
            return testPath;
    }
    return QString();
}






#endif // OKJUTIL_H
