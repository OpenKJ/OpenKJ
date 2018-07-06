#ifndef FILENAMEPARSER_H
#define FILENAMEPARSER_H

#include <QObject>
#include "sourcedirtablemodel.h"
#include "tagreader.h"

class KaraokeFileInfo : public QObject
{
    Q_OBJECT
    QString artistPattern;
    int artistCaptureGroup;
    QString titlePattern;
    int titleCaptureGroup;
    QString songIdPattern;
    int songIdCaptureGroup;
    QString fileName;
    QString fileBaseName;
    bool useMetadata;
    bool tagsRead;
    void readTags();
    QString tagArtist;
    QString tagTitle;
    QString tagSongid;
    int duration;

public:
    explicit KaraokeFileInfo(QObject *parent = 0);
    void setArtistRegEx(QString pattern, int captureGroup = 0) {artistPattern = pattern; artistCaptureGroup = captureGroup;}
    void setTitleRegEx(QString pattern, int captureGroup = 0) {titlePattern = pattern; titleCaptureGroup = captureGroup;}
    void setSongIdRegEx(QString pattern, int captureGroup = 0) {songIdPattern = pattern; songIdCaptureGroup = captureGroup;}
    void setFileName(QString filename);
    void setPattern(SourceDir::NamingPattern pattern, QString path = "");
    QString getArtist();
    QString getTitle();
    QString getSongId();
    int getDuration();

signals:

public slots:
};

#endif // FILENAMEPARSER_H
