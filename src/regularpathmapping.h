#ifndef REGULARPATHMAPPING_H
#define REGULARPATHMAPPING_H

#include <QDir>
#include <QMap>
#include <QRegularExpression>

namespace RegularPaths {
inline QString normalized(QString path)
{
    return QDir::cleanPath(path.replace('\\', '/'));
}

inline bool absolute(const QString &path)
{
    return QDir::isAbsolutePath(path) ||
           QRegularExpression(QStringLiteral("^[A-Za-z]:/")).match(path).hasMatch();
}

inline bool contains(const QString &root, const QString &path)
{
    return path.startsWith(root.endsWith('/') ? root : root + '/');
}

// Use the longest directory match, with a component boundary (Music != Music2).
inline QString encode(const QString &path, const QMap<QString, QString> &roots)
{
    const QString clean = normalized(path);
    QString best;
    QString token;
    for (auto it = roots.begin(); it != roots.end(); ++it) {
        const QString root = normalized(it.value());
        if (root.size() > best.size() && contains(root, clean)) {
            best = root;
            token = '%' + it.key() + '%';
        }
    }
    if (best.isEmpty())
        return path;
    return token + '/' + clean.mid(best.size() + (best.endsWith('/') ? 0 : 1));
}

// Mapping keys are either %rootname% tokens or old absolute directory prefixes.
inline QString resolve(const QString &path, const QMap<QString, QString> &mappings)
{
    QString source = path;
    source.replace('\\', '/');
    QString best;
    for (auto it = mappings.begin(); it != mappings.end(); ++it) {
        if (it.key().size() > best.size() && contains(it.key(), source))
            best = it.key();
    }
    if (best.isEmpty())
        return source.startsWith('%') ? QString() : normalized(source);
    const QString root = normalized(mappings.value(best));
    if (!absolute(root))
        return {};
    const QString relative = source.mid(best.size() + (best.endsWith('/') ? 0 : 1));
    const QString cleanRelative = normalized(relative);
    if (absolute(cleanRelative) || cleanRelative == ".." || cleanRelative.startsWith("../"))
        return {};
    return normalized(QDir(root).filePath(cleanRelative));
}
}
#endif
