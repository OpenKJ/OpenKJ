#include "karaokefileinfo.h"
#include <QRegularExpression>
#include <QDebug>
#include <QTemporaryDir>
#include "tagreader.h"
#include "okarchive.h"
#include <QSqlQuery>

void KaraokeFileInfo::readTags()
{
    if (tagsRead)
        return;
    TagReader *tagReader = new TagReader(this);

    if (fileName.endsWith(".cdg", Qt::CaseInsensitive))
    {
        QString baseFn = fileName;
        QString mediaFile;
        baseFn.chop(3);
        if (QFile::exists(baseFn + "mp3"))
            mediaFile = baseFn + "mp3";
        else if (QFile::exists(baseFn + "Mp3"))
            mediaFile = baseFn + "Mp3";
        else if (QFile::exists(baseFn + "MP3"))
            mediaFile = baseFn + "MP3";
        else if (QFile::exists(baseFn + "mP3"))
            mediaFile = baseFn + "mP3";
        tagReader->setMedia(mediaFile);
        tagArtist = tagReader->getArtist();
        tagTitle = tagReader->getTitle();
        tagSongid = tagReader->getAlbum();
        QString track = tagReader->getTrack();
        if (track != "")
        {
            tagSongid.append("-" + track);
        }
    }
    else if (fileName.endsWith(".zip", Qt::CaseInsensitive))
    {
        OkArchive archive;
        QTemporaryDir dir;
        archive.setArchiveFile(fileName);
        archive.checkAudio();
        QString audioFile = "temp" + archive.audioExtension();
        archive.extractAudio(dir.path(), audioFile);
        tagReader->setMedia(dir.path() + QDir::separator() + audioFile);
        tagArtist = tagReader->getArtist();
        tagTitle = tagReader->getTitle();
        tagSongid = tagReader->getAlbum();
        duration = archive.getSongDuration();
        QString track = tagReader->getTrack();
        if (track != "")
        {
            tagSongid.append("-" + track);
        }
    }
    else
    {
        qInfo() << "KaraokeFileInfo::readTags() called on non zip or cdg file (" << fileName << ").  Trying taglib.";
        tagReader->setMedia(fileName);
        tagArtist = tagReader->getArtist();
        tagTitle = tagReader->getTitle();
        tagSongid = tagReader->getAlbum();
        duration = tagReader->getDuration();
//        tagArtist = "Error";
//        tagTitle = "Error";
//        tagSongid = "Error";
//        duration = 0;
    }
    tagsRead = true;
    delete tagReader;
}

KaraokeFileInfo::KaraokeFileInfo(QObject *parent) : QObject(parent)
{
    duration = false;
    artistCaptureGroup = 0;
    titleCaptureGroup  = 0;
    songIdCaptureGroup = 0;
    useMetadata = false;
    tagsRead = false;
}

void KaraokeFileInfo::setFileName(const QString &filename)
{
    fileName = filename;
    fileBaseName = QFileInfo(filename).completeBaseName();
    tagArtist = "";
    tagTitle = "";
    tagSongid = "";
    artist = "";
    title = "";
    songId = "";
    duration = 0;
    tagsRead = false;
}

void KaraokeFileInfo::setPattern(SourceDir::NamingPattern pattern, QString path)
{
//    useMetadata = false;
//    int customPatternId = 0;
    artist = "";
    title = "";
    songId = "";
    this->path = path;
    this->pattern = pattern;
//    switch (pattern)
//    {
//    case SourceDir::STA:
////        setSongIdRegEx("^.+?(?=(\\s|_)-(\\s|_))");
////        setTitleRegEx("(?<=(\\s|_)-(\\s|_))(.*?)(?=(\\s|_)-(\\s|_))", 0);
////        setArtistRegEx("(?:^\\S+(?:\\s|_)-(?:\\s|_).+(?:\\s|_)-(?:\\s|_))(.+)",1);
//        break;
//    case SourceDir::SAT:
////        setSongIdRegEx("^.+?(?=(\\s|_)-(\\s|_))");
////        setTitleRegEx("(?:^\\S+(?:\\s|_)-(?:\\s|_).+(?:\\s|_)-(?:\\s|_))(.+)",1);
////        setArtistRegEx("(?<=(\\s|_)-(\\s|_))(.*?)(?=(\\s|_)-(\\s|_))", 0);
//        break;
//    case SourceDir::ATS:
////        setArtistRegEx(".+?(?=(\\s|_)-(\\s|_))",0);
////        setTitleRegEx("(?<=(\\s|_)-(\\s|_))(.*?)(?=(\\s|_)-(\\s|_))");
////        setSongIdRegEx("(?:^.+(?:\\s|_)-(?:\\s|_).+(?:\\s|_)-(?:\\s|))(.+)", 1);
//        break;
//    case SourceDir::TAS:
////        setTitleRegEx(".+?(?=(\\s|_)-(\\s|_))",0);
////        setArtistRegEx("(?<=(\\s|_)-(\\s|_))(.*?)(?=(\\s|_)-(\\s|_))");
////        setSongIdRegEx("(?:^.+(?:\\s|_)-(?:\\s|_).+(?:\\s|_)-(?:\\s|))(.+)", 1);
//        break;
//    case SourceDir::AT:
////        setArtistRegEx(".+?(?=(\\s|_)-(\\s|_))");
////        setTitleRegEx("(?<=(\\s|_)-(\\s|_))(.*)");
//        break;
//    case SourceDir::TA:
////        setTitleRegEx(".+?(?=(\\s|_)-(\\s|_))");
////        setArtistRegEx("(?<=(\\s|_)-(\\s|_))(.*)");
//        break;
//    case SourceDir::S_T_A:
////        setSongIdRegEx("^[^_]*_");
////        setTitleRegEx("((?<=_)[^_]*_)");
////        setArtistRegEx("([^_]+(?=_[^_]*(\\r?\\n|$)))");
//        break;
//    case SourceDir::METADATA:
//        useMetadata = true;
//        break;
//    case SourceDir::CUSTOM:
//        QSqlQuery query;
//        query.exec("SELECT custompattern FROM sourcedirs WHERE path == \"" + path + "\"" );
//        if (query.first())
//        {
//            customPatternId = query.value(0).toInt();
//        }
//        if (customPatternId < 1)
//        {
//            qCritical() << "Custom pattern set for path, but pattern ID is invalid!  Bailing out!";
//            return;
//        }
//        query.exec("SELECT * FROM custompatterns WHERE patternid == " + QString::number(customPatternId));
//        if (query.first())
//        {
//            setArtistRegEx(query.value("artistregex").toString(), query.value("artistcapturegrp").toInt());
//            setTitleRegEx(query.value("titleregex").toString(), query.value("titlecapturegrp").toInt());
//            setSongIdRegEx(query.value("discidregex").toString(), query.value("discidcapturegrp").toInt());
//        }
//        break;
//    }
}

const QString &KaraokeFileInfo::getArtist()
{
    getMetadata();
    return artist;
//    if (useMetadata)
//    {
//        readTags();
//        return tagArtist;
//    }
//    QRegularExpression r(artistPattern);
//    QRegularExpressionMatch match = r.match(fileBaseName);
//    QString result = match.captured(artistCaptureGroup);
//    result.replace("_", " ");
//    return result;
}

const QString &KaraokeFileInfo::getTitle()
{
    getMetadata();
    return title;
//    if (useMetadata)
//    {
//        readTags();
//        return tagTitle;
//    }
//    QRegularExpression r(titlePattern);
//    QRegularExpressionMatch match = r.match(fileBaseName);
//    QString result = match.captured(titleCaptureGroup);
//    result.replace("_", " ");
//    return result;
}

const QString &KaraokeFileInfo::getSongId()
{
    getMetadata();
    return songId;
//    if (useMetadata)
//    {
//        readTags();
//        return tagSongid;
//    }
//    QRegularExpression r(songIdPattern);
//    QRegularExpressionMatch match = r.match(fileBaseName);
//    QString result = match.captured(songIdCaptureGroup);
//    result.replace("_", " ");
    //    return result;
}

QString KaraokeFileInfo::testPattern(QString regex, QString filename, int captureGroup)
{
    QRegularExpression r;
    QRegularExpressionMatch match;
    r.setPattern(regex);
    match = r.match(filename);
    return match.captured(captureGroup).replace("_", " ");

}

const int& KaraokeFileInfo::getDuration()
{
    if (duration > 0)
        return duration;
    if (fileName.endsWith(".zip", Qt::CaseInsensitive))
    {
        OkArchive archive;
        archive.setArchiveFile(fileName);
        duration = archive.getSongDuration();
    }
    else if (fileName.endsWith(".cdg", Qt::CaseInsensitive))
    {
        duration = ((QFile(fileName).size() / 96) / 75) * 1000;
    }
    else
    {
        TagReader reader;
        reader.setMedia(fileName);
        try
        {
            duration = reader.getDuration();
        }
        catch (...)
        {
            qInfo() << "KaraokeFileInfo unable to get duration for file: " << fileBaseName;
        }
    }
    return duration;
}

void KaraokeFileInfo::getMetadata()
{
    if (artist != "" || title != "" || songId != "")
        return;
    int customPatternId = 0;
    QString baseNameFiltered = fileBaseName;
    baseNameFiltered.replace("_", " ");
    QStringList parts = baseNameFiltered.split(" - ");
    switch (pattern)
    {
    case SourceDir::STA:
        if (parts.size() >= 1)
            songId = parts.at(0);
        if (parts.size() >= 2)
            title = parts.at(1);
        if (!parts.isEmpty())
            parts.removeFirst();
        if (!parts.isEmpty())
            parts.removeFirst();
        artist = parts.join(" - ");
        break;
    case SourceDir::SAT:
        if (parts.size() >= 1)
            songId = parts.at(0);
        if (parts.size() >= 2)
            artist = parts.at(1);
        if (!parts.isEmpty())
            parts.removeFirst();
        if (!parts.isEmpty())
            parts.removeFirst();
        title = parts.join(" - ");
        break;
    case SourceDir::ATS:
        if (parts.size() >= 1)
            artist = parts.at(0);
        if (parts.size() >= 3)
        {
            songId = parts.at(parts.size() - 1);
            parts.removeLast();
        }
        if (!parts.isEmpty())
            parts.removeFirst();
        title = parts.join(" - ");
        break;
    case SourceDir::TAS:
        if (parts.size() >= 1)
            title = parts.at(0);
        if (parts.size() >= 3)
        {
            songId = parts.at(parts.size() - 1);
            if (!parts.isEmpty())
                parts.removeLast();
        }
        if (!parts.isEmpty())
            parts.removeFirst();
        artist = parts.join(" - ");
        break;
    case SourceDir::AT:
        if (parts.size() >= 1)
        {
            artist = parts.at(0);
            parts.removeFirst();
        }
        title = parts.join(" - ");
        break;
    case SourceDir::TA:
        if (parts.size() >= 1)
        {
            title = parts.at(0);
            parts.removeFirst();
        }
        artist = parts.join(" - ");
        break;
    case SourceDir::S_T_A:
        parts = fileBaseName.split("_");
        if (parts.size() >= 1)
            songId = parts.at(0);
        if (parts.size() >= 2)
            title = parts.at(1);
        if (!parts.isEmpty())
            parts.removeFirst();
        if (!parts.isEmpty())
            parts.removeFirst();
        artist = parts.join(" - ");
        break;
    case SourceDir::METADATA:
        qInfo() << "Using metadata";
        readTags();
        artist = tagArtist;
        title = tagTitle;
        songId = tagSongid;
        break;
    case SourceDir::CUSTOM:
        QSqlQuery query;
        query.exec("SELECT custompattern FROM sourcedirs WHERE path == \"" + path + "\"" );
        if (query.first())
        {
            customPatternId = query.value(0).toInt();
        }
        if (customPatternId < 1)
        {
            qCritical() << "Custom pattern set for path, but pattern ID is invalid!  Bailing out!";
            return;
        }
        query.exec("SELECT * FROM custompatterns WHERE patternid == " + QString::number(customPatternId));
        if (query.first())
        {
            setArtistRegEx(query.value("artistregex").toString(), query.value("artistcapturegrp").toInt());
            setTitleRegEx(query.value("titleregex").toString(), query.value("titlecapturegrp").toInt());
            setSongIdRegEx(query.value("discidregex").toString(), query.value("discidcapturegrp").toInt());
        }
        QRegularExpression r;
        QRegularExpressionMatch match;
        r.setPattern(artistPattern);
        match = r.match(fileBaseName);
        artist = match.captured(artistCaptureGroup).replace("_", " ");
        r.setPattern(titlePattern);
        match = r.match(fileBaseName);
        title = match.captured(titleCaptureGroup).replace("_", " ");
        r.setPattern(songIdPattern);
        match = r.match(fileBaseName);
        songId = match.captured(songIdCaptureGroup).replace("_", " ");
        break;
    }
}
