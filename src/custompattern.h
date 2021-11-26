#ifndef CUSTOMPATTERN_H
#define CUSTOMPATTERN_H

#include <QString>


class CustomPattern
{
private:
    bool m_isNull;

    QString m_name;

    QString m_artistRegex;
    int m_artistCaptureGrp {0};

    QString m_titleRegex;
    int m_titleCaptureGrp {0};

    QString m_songIdRegex;
    int m_songIdCaptureGrp {0};

public:
    QString getArtistRegex() const
    {
        return m_artistRegex;
    }

    QString getTitleRegex() const
    {
        return m_titleRegex;
    }

    QString getSongIdRegex() const
    {
        return m_songIdRegex;
    }

    int getArtistCaptureGrp() const
    {
        return m_artistCaptureGrp;
    }

    int getTitleCaptureGrp() const
    {
        return m_titleCaptureGrp;
    }

    int getSongIdCaptureGrp() const
    {
        return m_songIdCaptureGrp;
    }

    QString getName() const
    {
        return m_name;
    }

public:
    CustomPattern();

    explicit CustomPattern(
            QString name,
            QString artistPattern, int artistCaptureGroup,
            QString titlePattern, int titleCaptureGroup,
            QString diskIdPattern, int diskIdCaptureGroup);

    bool isNull() const { return m_isNull; }

};

#endif // CUSTOMPATTERN_H
