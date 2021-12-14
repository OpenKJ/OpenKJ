#ifndef KARAOKEFILEPATTERNRESOLVER_H
#define KARAOKEFILEPATTERNRESOLVER_H

#include "src/models/tablemodelkaraokesourcedirs.h"
#include "custompattern.h"

class KaraokeFilePatternResolver
{

public:

    struct KaraokeFilePattern
    {
        SourceDir::NamingPattern pattern;
        CustomPattern customPattern;
    };

    explicit KaraokeFilePatternResolver();

    const KaraokeFilePattern& getPattern(const QString &filename);

    static const KaraokeFilePattern& getDefaultPattern();

private:

    QMap<QString, KaraokeFilePattern> m_path_pattern_map;
    bool m_initialized {false};

    void InitializeData();

};

#endif // KARAOKEFILEPATTERNRESOLVER_H
