#include "karaokefileinfo.h"
#include <QRegularExpression>
#include <QTemporaryDir>
#include "tagreader.h"
#include "okarchive.h"
#include <QSqlQuery>

KaraokeFileInfo::KaraokeFileInfo(QObject *parent, std::shared_ptr<KaraokeFilePatternResolver> patternResolver) : QObject(parent), m_patternResolver(patternResolver) {
    m_logger = spdlog::get("logger");
}

KaraokeFileInfo::KaraokeFileInfo(QObject *parent) : KaraokeFileInfo(parent, std::make_shared<KaraokeFilePatternResolver>()) {
}

void KaraokeFileInfo::readTags()
{
    if (tagsRead)
        return;
    TagReader *tagReader = new TagReader(this);

    if (m_filename.endsWith(".cdg", Qt::CaseInsensitive))
    {
        QString baseFn = m_filename;
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
    else if (m_filename.endsWith(".zip", Qt::CaseInsensitive))
    {
        OkArchive archive;
        QTemporaryDir dir;
        archive.setArchiveFile(m_filename);
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
        m_logger->info("{} readTags called on non zip or cdg file '{}'.  Trying taglib.", m_loggingPrefix);
        tagReader->setMedia(m_filename);
        tagArtist = tagReader->getArtist();
        tagTitle = tagReader->getTitle();
        tagSongid = tagReader->getAlbum();
        duration = tagReader->getDuration();
    }
    tagsRead = true;
    delete tagReader;
}

void KaraokeFileInfo::setFile(const QString &filename)
{
    m_filename = filename;
    fileBaseName = QFileInfo(filename).completeBaseName();
    tagArtist = "";
    tagTitle = "";
    tagSongid = "";
    artist = "";
    title = "";
    songId = "";
    duration = 0;
    tagsRead = false;
    m_metadata_parsed = false;
    m_metadata_parsed_success = false;
}

void KaraokeFileInfo::ensureMetadataParsed() {
    if (!m_metadata_parsed) {
        auto &pattern = m_patternResolver->getPattern(m_filename);
        m_metadata_parsed_success = parseMetadata(pattern);

        if (!m_metadata_parsed_success && pattern.pattern != SourceDir::METADATA) {
            // Something went wrong, no metadata found. File is probably named wrong. If we didn't try media tags, give it a shot
            m_metadata_parsed_success = parseMetadata(m_patternResolver->getDefaultPattern());
        }
        m_metadata_parsed = true;
    }
}

const QString &KaraokeFileInfo::getArtist()
{
    ensureMetadataParsed();
    return artist;
}

const QString &KaraokeFileInfo::getTitle()
{
    ensureMetadataParsed();
    return title;
}

const QString &KaraokeFileInfo::getSongId()
{
    ensureMetadataParsed();
    return songId;
}

const int& KaraokeFileInfo::getDuration()
{
    if (duration > 0)
        return duration;
    if (m_filename.endsWith(".zip", Qt::CaseInsensitive))
    {
        OkArchive archive;
        archive.setArchiveFile(m_filename);
        duration = archive.getSongDuration();
    }
    else if (m_filename.endsWith(".cdg", Qt::CaseInsensitive))
    {
        duration = ((QFile(m_filename).size() / 96) / 75) * 1000;
    }
    else
    {
        //TODO: make sure tags are not read twice (here and if reading metadata tags)!
        TagReader reader;
        reader.setMedia(m_filename);
        try
        {
            duration = reader.getDuration();
        }
        catch (...)
        {
            m_logger->error("{} Unable to get duration for file {}", m_loggingPrefix, fileBaseName);
        }
    }
    return duration;
}

bool KaraokeFileInfo::parseMetadata(const KaraokeFilePatternResolver::KaraokeFilePattern& pattern)
{
    //TODO: This needs to be cleaned up, the logic is convoluted and is probably
    // slowing down db updates

    QString baseNameFiltered = fileBaseName;
    baseNameFiltered.replace("_", " ");
    QStringList parts = baseNameFiltered.split(" - ");
    switch (pattern.pattern)
    {
    case SourceDir::STA:
        if (!parts.empty())
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
        if (!parts.empty())
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
        if (!parts.empty())
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
        if (!parts.empty())
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
        if (!parts.empty())
        {
            artist = parts.at(0);
            parts.removeFirst();
        }
        title = parts.join(" - ");
        break;
    case SourceDir::TA:
        if (!parts.empty())
        {
            title = parts.at(0);
            parts.removeFirst();
        }
        artist = parts.join(" - ");
        break;
    case SourceDir::S_T_A:
        parts = fileBaseName.split("_");
        if (!parts.empty())
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
        readTags();
        artist = tagArtist;
        title = tagTitle;
        songId = tagSongid;
        break;
    case SourceDir::CUSTOM:
        if (pattern.customPattern.isNull())
            return false;

        QRegularExpression r;
        QRegularExpressionMatch match;

        r.setPattern(pattern.customPattern.getArtistRegex());
        match = r.match(fileBaseName);
        artist = match.captured(pattern.customPattern.getArtistCaptureGrp()).replace("_", " ");

        r.setPattern(pattern.customPattern.getTitleRegex());
        match = r.match(fileBaseName);
        title = match.captured(pattern.customPattern.getTitleCaptureGrp()).replace("_", " ");

        r.setPattern(pattern.customPattern.getSongIdRegex());
        match = r.match(fileBaseName);
        songId = match.captured(pattern.customPattern.getSongIdRegex()).replace("_", " ");

        break;
    }
    if ( !artist.isEmpty() || !title.isEmpty() || !songId.isEmpty())
        return true;
    else
        return false;
}

// Static:
QString KaraokeFileInfo::testPattern(const QString& regex, const QString& filename, int captureGroup)
{
    QRegularExpression r;
    QRegularExpressionMatch match;
    r.setPattern(regex);
    match = r.match(filename);
    return match.captured(captureGroup).replace("_", " ");
}
