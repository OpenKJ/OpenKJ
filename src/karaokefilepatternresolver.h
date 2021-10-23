#ifndef KARAOKEFILEPATTERNRESOLVER_H
#define KARAOKEFILEPATTERNRESOLVER_H

#include <QObject>
#include "src/models/tablemodelkaraokesourcedirs.h"

class KaraokeFilePatternResolver : public QObject
{
    Q_OBJECT

private:
    QMap<QString, SourceDir::NamingPattern> m_path_pattern_map;
    bool m_initialized {false};

    void InitializeData();

public:
    explicit KaraokeFilePatternResolver(QObject *parent = nullptr);

    SourceDir::NamingPattern getPattern(const QString &filename);

signals:

};

#endif // KARAOKEFILEPATTERNRESOLVER_H
