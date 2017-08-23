#include "filenameparser.h"
#include <QRegularExpression>
#include <QDebug>

FilenameParser::FilenameParser(QObject *parent) : QObject(parent)
{
    artistCaptureGroup = 0;
    titleCaptureGroup  = 0;
    discIdCaptureGroup = 0;
}

void FilenameParser::setFileName(QString filename)
{
    fileName = filename;
}

QString FilenameParser::getArtist()
{
    QRegularExpression r(artistPattern);
    QRegularExpressionMatch match = r.match(fileName);
    return match.captured(artistCaptureGroup);
}

QString FilenameParser::getTitle()
{
    QRegularExpression r(titlePattern);
    QRegularExpressionMatch match = r.match(fileName);
    return match.captured(titleCaptureGroup);
}

QString FilenameParser::getDiscId()
{
    QRegularExpression r(discIdPattern);
    QRegularExpressionMatch match = r.match(fileName);
    return match.captured(discIdCaptureGroup);
}
