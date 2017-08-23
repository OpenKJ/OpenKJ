#ifndef FILENAMEPARSER_H
#define FILENAMEPARSER_H

#include <QObject>

class FilenameParser : public QObject
{
    Q_OBJECT
    QString artistPattern;
    int artistCaptureGroup;
    QString titlePattern;
    int titleCaptureGroup;
    QString discIdPattern;
    int discIdCaptureGroup;
    QString fileName;

public:
    explicit FilenameParser(QObject *parent = 0);
    void setArtistRegEx(QString pattern, int captureGroup = 0) {artistPattern = pattern; artistCaptureGroup = captureGroup;}
    void setTitleRegEx(QString pattern, int captureGroup = 0) {titlePattern = pattern; titleCaptureGroup = captureGroup;}
    void setDiscIdRegEx(QString pattern, int captureGroup = 0) {discIdPattern = pattern; discIdCaptureGroup = captureGroup;}
    void setFileName(QString filename);
    QString getArtist();
    QString getTitle();
    QString getDiscId();

signals:

public slots:
};

#endif // FILENAMEPARSER_H
