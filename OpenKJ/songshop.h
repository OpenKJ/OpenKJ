#ifndef SONGSHOP_H
#define SONGSHOP_H

#include <QObject>

class SongShop : public QObject
{
    Q_OBJECT
public:
    explicit SongShop(QObject *parent = nullptr);

signals:

public slots:
};

#endif // SONGSHOP_H