#ifndef FILENAMEPARSER_H
#define FILENAMEPARSER_H

#include <QObject>
#include "sourcedirtablemodel.h"
#include "tagreader.h"

class KaraokeFileInfo : public QObject
{
    Q_OBJECT
    QString artistPattern;
    int artistCaptureGroup{0};
    QString titlePattern;
    int titleCaptureGroup{0};
    QString songIdPattern;
    int songIdCaptureGroup{0};
    QString fileName;
    QString fileBaseName;
    bool useMetadata{false};
    bool tagsRead{false};
    void readTags();
    QString tagArtist;
    QString tagTitle;
    QString tagSongid;
    int duration{0};
    SourceDir::NamingPattern pattern{SourceDir::SAT};
    QString path;
    QString artist;
    QString title;
    QString songId;

public:
    void setArtistRegEx(QString pattern, int captureGroup = 0) {artistPattern = pattern; artistCaptureGroup = captureGroup;}
    void setTitleRegEx(QString pattern, int captureGroup = 0) {titlePattern = pattern; titleCaptureGroup = captureGroup;}
    void setSongIdRegEx(QString pattern, int captureGroup = 0) {songIdPattern = pattern; songIdCaptureGroup = captureGroup;}
    void setFileName(const QString &filename);
    void setPattern(SourceDir::NamingPattern pattern, QString path = "");
    const QString& getArtist();
    const QString& getTitle();
    const QString& getSongId();
    QString testPattern(QString regex, QString filename, int captureGroup = 0);
    const int& getDuration();
    void getMetadata();

signals:

public slots:
};

#endif // FILENAMEPARSER_H
