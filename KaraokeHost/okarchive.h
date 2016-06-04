#ifndef KHARCHIVE_H
#define KHARCHIVE_H

#include <QObject>

class KHARCHIVE : public QObject
{
    Q_OBJECT
public:
    explicit KHARCHIVE(QObject *parent = 0);

signals:

public slots:
};

#endif // KHARCHIVE_H