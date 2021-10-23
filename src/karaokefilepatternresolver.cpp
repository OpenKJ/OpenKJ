#include "karaokefilepatternresolver.h"
#include <QSqlQuery>

KaraokeFilePatternResolver::KaraokeFilePatternResolver(QObject *parent) : QObject(parent)
{

}

void KaraokeFilePatternResolver::InitializeData()
{
    QSqlQuery query("SELECT path, pattern, custompattern FROM sourceDirs ORDER BY path");
    while (query.next()) {
        m_path_pattern_map[query.value(0).toString()] = static_cast<SourceDir::NamingPattern>(query.value(1).toInt());
    }
    m_initialized = true;
}

SourceDir::NamingPattern KaraokeFilePatternResolver::getPattern(const QString &filename)
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
    return SourceDir::SAT;
}

