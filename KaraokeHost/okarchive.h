#ifndef KHARCHIVE_H
#define KHARCHIVE_H

#include <QObject>
#include <KZip>
#include <K7Zip>

class OkArchive : public QObject
{
    Q_OBJECT
public:
    explicit OkArchive(QString ArchiveFile, QObject *parent = 0);
    explicit OkArchive(QObject *parent = 0);

signals:

public slots:
};

#endif // KHARCHIVE_H
