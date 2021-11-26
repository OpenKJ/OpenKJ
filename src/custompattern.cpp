#include "custompattern.h"

CustomPattern::CustomPattern() : m_isNull(true)
{
}

CustomPattern::CustomPattern(
        QString name,
        QString artistPattern, int artistCaptureGroup,
        QString titlePattern, int titleCaptureGroup,
        QString songIdPattern, int songIdCaptureGroup)
    :
      m_isNull(false),
      m_name(name),
      m_artistRegex(artistPattern), m_artistCaptureGrp(artistCaptureGroup),
      m_titleRegex(titlePattern), m_titleCaptureGrp(titleCaptureGroup),
      m_songIdRegex(songIdPattern), m_songIdCaptureGrp(songIdCaptureGroup)
{
}
