#ifndef FILENAMEPARSER_H
#define FILENAMEPARSER_H

#include <QObject>
#include "src/models/tablemodelkaraokesourcedirs.h"
#include "tagreader.h"
#include "karaokefilepatternresolver.h"
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>

std::ostream& operator<<(std::ostream& os, const QString& s);

class KaraokeFileInfo : public QObject
{
    Q_OBJECT
    std::shared_ptr<KaraokeFilePatternResolver> m_patternResolver;
    QString artistPattern;
    int artistCaptureGroup{0};
    QString titlePattern;
    int titleCaptureGroup{0};
    QString songIdPattern;
    int songIdCaptureGroup{0};
    QString m_filename;
    QString fileBaseName;
    bool useMetadata{false};
    bool tagsRead{false};
    void readTags();
    QString tagArtist;
    QString tagTitle;
    QString tagSongid;
    int duration{0};
    //SourceDir::NamingPattern m_pattern {SourceDir::SAT};
    //QString m_path;
    QString artist;
    QString title;
    QString songId;
    bool m_metadata_parsed {false};
    bool m_metadata_parsed_success {false};

    std::string m_loggingPrefix{"[KaraokeFileInfo]"};
    std::shared_ptr<spdlog::logger> m_logger;

    void ensureMetadataParsed();
    bool parseMetadata(SourceDir::NamingPattern pattern);

public:
    explicit KaraokeFileInfo(QObject *parent = nullptr);
    explicit KaraokeFileInfo(QObject *parent, std::shared_ptr<KaraokeFilePatternResolver> patternResolver);
    void setArtistRegEx(QString pattern, int captureGroup = 0) {artistPattern = pattern; artistCaptureGroup = captureGroup;}
    void setTitleRegEx(QString pattern, int captureGroup = 0) {titlePattern = pattern; titleCaptureGroup = captureGroup;}
    void setSongIdRegEx(QString pattern, int captureGroup = 0) {songIdPattern = pattern; songIdCaptureGroup = captureGroup;}

    void setFile(const QString &filename);
    bool parsedSuccessfully() { return m_metadata_parsed_success; }

    const QString& getArtist();
    const QString& getTitle();
    const QString& getSongId();
    const int& getDuration();

    static QString testPattern(const QString& regex, const QString& filename, int captureGroup = 0);

signals:

public slots:
};

#endif // FILENAMEPARSER_H
