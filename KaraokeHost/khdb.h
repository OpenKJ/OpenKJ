#ifndef KHDB_H
#define KHDB_H

#include <QObject>

class KhDb : public QObject
{
    Q_OBJECT
public:
    explicit KhDb(QObject *parent = 0);
    void beginTransaction();
    void endTransaction();
    bool singerSetRegular(int singerId, bool value);
    bool singerSetPosition(int singerId, int position);
    bool singerSetName(int singerId, QString name);
    bool singerSetRegIndex(int singerId, int regId);
    bool singerMove(int singerId, int newPosition);
    int  singerAdd(QString name, int position = -1, bool regular = false);
    bool singerDelete(int singerId);
    QString singerGetNextSong(int singerId);
    bool rotationClear();
    bool songSetDuration(int sondId, int duration);


signals:

public slots:

};

#endif // KHDB_H
