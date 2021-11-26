#define SQL(...) #__VA_ARGS__
#include "karaokefilepatternresolver.h"
#include <QSqlQuery>

KaraokeFilePatternResolver::KaraokeFilePatternResolver()
{
}

void KaraokeFilePatternResolver::InitializeData()
{
    QSqlQuery query(SQL(
        SELECT
            sourceDirs.path, sourceDirs.pattern,
            custompatterns.name,
            custompatterns.artistregex, custompatterns.artistcapturegrp,
            custompatterns.titleregex,  custompatterns.titlecapturegrp,
            custompatterns.discidregex, custompatterns.discidcapturegrp
        FROM sourceDirs
        LEFT JOIN custompatterns ON sourceDirs.custompattern == custompatterns.patternid
        ORDER BY sourceDirs.path
    ));

    while (query.next()) {
        auto pattern = static_cast<SourceDir::NamingPattern>(query.value(1).toInt());
        auto customPattern =
                pattern == SourceDir::CUSTOM && !query.isNull("name")
                    ? CustomPattern(
                          query.value("name").toString(),
                          query.value("artistregex").toString(), query.value("artistcapturegrp").toInt(),
                          query.value("titleregex").toString(), query.value("titlecapturegrp").toInt(),
                          query.value("discidregex").toString(), query.value("discidcapturegrp").toInt())
                    : CustomPattern();

        m_path_pattern_map[query.value(0).toString()] =
                KaraokeFilePatternResolver::KaraokeFilePattern {
                .pattern = pattern,
                .customPattern = customPattern
        };
    }
    m_initialized = true;
}

const KaraokeFilePatternResolver::KaraokeFilePattern& KaraokeFilePatternResolver::getPattern(const QString &filename)
{
    if (!m_initialized) {
        InitializeData();
    }

    // The map is ordered by paths.
    // Enumerate backwards so '/media/abc' is matched before '/media/a' in the case of filename '/media/abc/somefile.zip'

    auto it =  m_path_pattern_map.cend();
    while (it != m_path_pattern_map.cbegin()) {
        --it;
        if (filename.startsWith(it.key())) {
            return it.value();
        }
    }
    // default:
    return getDefaultPattern();
}

static const KaraokeFilePatternResolver::KaraokeFilePattern defaultPattern { KaraokeFilePatternResolver::KaraokeFilePattern { .pattern = SourceDir::SAT } };

const KaraokeFilePatternResolver::KaraokeFilePattern &KaraokeFilePatternResolver::getDefaultPattern()
{
    return defaultPattern;
}


